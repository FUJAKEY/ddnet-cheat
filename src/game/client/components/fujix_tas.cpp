#include "fujix_tas.h"

#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/client.h>
#include <game/client/gameclient.h>

// +++ ИСПРАВЛЕНИЕ: Полный заголовок включается здесь, в .cpp файле +++
#include "characters.h"

const char *CFujixTas::ms_pFujixDir = "fujix";

CFujixTas::CFujixTas()
{
    m_Recording = false;
    m_Playing = false;
    m_StartTick = 0;
    m_PlayStartTick = 0;
    m_File = nullptr;
    m_PlayIndex = 0;
    m_aFilename[0] = '\0';
    m_StateSaved = false;
    // Инициализируем указатель как нулевой
    m_pSavedCharState = nullptr;
}

// +++ ИСПРАВЛЕНИЕ: Реализуем деструктор для освобождения памяти +++
CFujixTas::~CFujixTas()
{
    delete m_pSavedCharState;
}


void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjx", ms_pFujixDir, pMap);
}

void CFujixTas::RecordEntry(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || !m_File)
        return;
    SEntry e{Tick - m_StartTick, *pInput};
    io_write(m_File, &e, sizeof(e));
}

bool CFujixTas::FetchEntry(CNetObj_PlayerInput *pInput)
{
    if(!m_Playing || m_PlayIndex >= (int)m_vEntries.size())
        return false;
    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    if(m_PlayStartTick + m_vEntries[m_PlayIndex].m_Tick > PredTick)
        return false;

    *pInput = m_vEntries[m_PlayIndex].m_Input;
    m_PlayIndex++;
    if(m_PlayIndex >= (int)m_vEntries.size())
        m_Playing = false;
    return true;
}

void CFujixTas::SaveState()
{
    if(m_pClient->m_Snap.m_pLocalCharacter)
    {
        // Освобождаем старую память, если она была выделена
        delete m_pSavedCharState;
        // Выделяем новую память и копируем туда состояние
        m_pSavedCharState = new CCharacter();
        *m_pSavedCharState = m_pClient->m_PredictedChar;

        m_StateSaved = true;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "State saved.");
    }
    else
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "Cannot save state, not in game.");
    }
}

void CFujixTas::LoadState()
{
    if(m_StateSaved && m_pSavedCharState)
    {
        if(m_pClient->m_Snap.m_pLocalCharacter)
        {
            m_pClient->m_PredictedChar = *m_pSavedCharState;
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "State loaded.");
        }
        else
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "Cannot load state, not in game.");
        }
    }
    else
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "No state saved to load.");
    }
}

void CFujixTas::StartRecord()
{
    if(!m_StateSaved)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "You must save a state with 'fujix_save' before recording.");
        return;
    }
    if(m_Recording)
        return;

    GetPath(m_aFilename, sizeof(m_aFilename));
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        return;
    }
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Recording = true;
}

void CFujixTas::StopRecord()
{
    if(!m_Recording)
        return;
    if(m_File)
        io_close(m_File);
    m_File = nullptr;
    m_Recording = false;
}

void CFujixTas::StartPlay()
{
    if(m_Playing)
        StopPlay();

    if(!m_StateSaved)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "No state saved. Cannot play.");
        return;
    }
    LoadState();

    char aPath[IO_MAX_PATH_LENGTH];
    GetPath(aPath, sizeof(aPath));
    IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for playback");
        return;
    }

    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    m_PlayIndex = 0;
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_Playing = !m_vEntries.empty();
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    m_vEntries.clear();
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    return FetchEntry(pInput);
}

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    RecordEntry(pInput, Tick);
}

void CFujixTas::ConRecord(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Recording)
        pSelf->StopRecord();
    else
        pSelf->StartRecord();
}

void CFujixTas::ConPlay(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Playing)
        pSelf->StopPlay();
    else
        pSelf->StartPlay();
}

void CFujixTas::ConSaveState(IConsole::IResult *pResult, void *pUserData)
{
    static_cast<CFujixTas *>(pUserData)->SaveState();
}

void CFujixTas::ConLoadState(IConsole::IResult *pResult, void *pUserData)
{
    static_cast<CFujixTas *>(pUserData)->LoadState();
}

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
    Console()->Register("fujix_save", "", CFGFLAG_CLIENT, ConSaveState, this, "Save current player state for TAS");
    Console()->Register("fujix_load", "", CFGFLAG_CLIENT, ConLoadState, this, "Load saved player state");
}

void CFujixTas::OnMapLoad()
{
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    
    // Освобождаем память при смене карты
    delete m_pSavedCharState;
    m_pSavedCharState = nullptr;
    m_StateSaved = false;
}

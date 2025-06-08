#include "fujix_tas.h"

#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/client.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/animstate.h>
#include <game/gamecore.h>
#include <base/system.h>

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
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_PhantomActive = false;
    m_PhantomTick = 0;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
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
    if(!m_Playing)
        return false;

    UpdatePlaybackInput();
    *pInput = m_CurrentInput;
    return true;
}

void CFujixTas::UpdatePlaybackInput()
{
    if(!m_Playing)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    while(m_PlayIndex < (int)m_vEntries.size() &&
          m_PlayStartTick + m_vEntries[m_PlayIndex].m_Tick <= PredTick)
    {
        m_CurrentInput = m_vEntries[m_PlayIndex].m_Input;
        m_PlayIndex++;
    }

    if(m_PlayIndex >= (int)m_vEntries.size() &&
       PredTick >= m_PlayStartTick + m_vEntries.back().m_Tick)
    {
        m_Playing = false;
    }
}

void CFujixTas::StartRecord()
{
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
    // start recording on the next predicted tick to align with
    // the upcoming OnSnapInput call
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Recording = true;

    // init phantom
    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomActive = true;
}

void CFujixTas::StopRecord()
{
    if(!m_Recording)
        return;
    if(m_File)
        io_close(m_File);
    m_File = nullptr;
    m_Recording = false;
    m_PhantomActive = false;
}

void CFujixTas::StartPlay()
{
    if(m_Playing)
        StopPlay();

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
    // similar to recording, start playback on the next tick so the
    // first stored input is applied exactly when OnSnapInput runs
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Playing = !m_vEntries.empty();
    if(m_Playing)
        m_CurrentInput = m_vEntries[0].m_Input;
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    m_vEntries.clear();
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    return FetchEntry(pInput);
}

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(m_Recording)
        UpdatePhantomInput(pInput);
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

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
}

void CFujixTas::OnMapLoad()
{
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
}

void CFujixTas::UpdatePhantomInput(const CNetObj_PlayerInput *pInput)
{
    if(m_PhantomActive)
        m_PhantomInput = *pInput;
}

void CFujixTas::TickPhantom()
{
    if(!m_PhantomActive)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    while(m_PhantomTick < PredTick)
    {
        m_PhantomCore.m_Input = m_PhantomInput;
        m_PhantomCore.Tick(true);
        m_PhantomCore.Move();
        m_PhantomCore.Quantize();
        ++m_PhantomTick;
    }
}

void CFujixTas::OnUpdate()
{
    if(g_Config.m_ClFujixTasRecord && !m_Recording)
        StartRecord();
    else if(!g_Config.m_ClFujixTasRecord && m_Recording)
        StopRecord();

    if(g_Config.m_ClFujixTasPlay && !m_Playing)
        StartPlay();
    else if(!g_Config.m_ClFujixTasPlay && m_Playing)
        StopPlay();

    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(!m_PhantomActive)
        return;

    const CAnimState *pIdle = CAnimState::GetIdle();
    RenderTools()->RenderTee(pIdle, &m_PhantomRenderInfo, EMOTE_NORMAL, vec2(1,0), m_PhantomCore.m_Pos, 0.5f);
}


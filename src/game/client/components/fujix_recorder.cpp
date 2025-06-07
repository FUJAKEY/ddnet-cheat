#include "fujix_recorder.h"

#include <base/math.h>
#include <engine/storage.h>
#include <game/client/gameclient.h>

void CFujixRecorder::ConRecord(IConsole::IResult *pResult, void *pUserData)
{
    static_cast<CFujixRecorder *>(pUserData)->ToggleRecord();
}

void CFujixRecorder::ConPlay(IConsole::IResult *pResult, void *pUserData)
{
    static_cast<CFujixRecorder *>(pUserData)->StartPlay();
}

void CFujixRecorder::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Toggle Fujix recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play Fujix recording");
}

void CFujixRecorder::OnMapLoad()
{
    m_Recording = false;
    m_Playing = false;
    m_NumTicks = 0;
    m_LastRecordedTick = -1;
    if(m_pFile)
    {
        io_close(m_pFile);
        m_pFile = nullptr;
    }
    g_Config.m_ClFujixRecord = 0;
}

void CFujixRecorder::RecordInput(int GameTick, const CNetObj_PlayerInput &Input)
{
    if(!m_Recording)
        return;

    if(m_LastRecordedTick < 0)
        m_LastRecordedTick = GameTick - 1;

    while(m_LastRecordedTick < GameTick - 1)
    {
        CKjmTick Repeat{};
        Repeat.m_Input = m_vInputs.empty() ? Input : m_vInputs.back().m_Input;
        Repeat.m_Action = 0;
        io_write(m_pFile, &Repeat, sizeof(Repeat));

        m_vInputs.push_back({Repeat.m_Input});
        m_NumTicks++;
        m_LastRecordedTick++;
    }

    CKjmTick Record{};
    Record.m_Input = Input;
    Record.m_Action = (Input.m_Direction || Input.m_Jump || Input.m_Fire || Input.m_Hook ||
                       Input.m_WantedWeapon || Input.m_NextWeapon || Input.m_PrevWeapon) ? 1 : 0;

    io_write(m_pFile, &Record, sizeof(Record));

    CInputFrame Frame{};
    Frame.m_Input = Input;
    m_vInputs.push_back(Frame);
    m_NumTicks++;
    m_LastRecordedTick = GameTick;
}

void CFujixRecorder::OnUpdate()
{
    // recording handled during input snapshot

    if(!m_Playing)
        return;

    if(m_PlayIndex >= (int)m_vInputs.size())
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback finished");
    }
}

void CFujixRecorder::ToggleRecord()
{
    char aPath[IO_MAX_PATH_LENGTH];
    str_format(aPath, sizeof(aPath), "demos/fujix/%s.kjm", Client()->GetCurrentMap());

    if(m_Recording)
    {
        // finalize header
        io_seek(m_pFile, offsetof(CKjmHeader, m_NumTicks), IOSEEK_START);
        io_write(m_pFile, &m_NumTicks, sizeof(m_NumTicks));
        io_close(m_pFile);
        m_pFile = nullptr;
        m_Recording = false;
        g_Config.m_ClFujixRecord = 0;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "record saved");
    }
    else
    {
        Storage()->CreateFolder("demos/fujix", IStorage::TYPE_SAVE);
        m_pFile = Storage()->OpenFile(aPath, IOFLAG_WRITE, IStorage::TYPE_SAVE);
        if(!m_pFile)
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file");
            return;
        }
        CKjmHeader Header{};
        Header.m_aMarker[0] = 'K';
        Header.m_aMarker[1] = 'J';
        Header.m_aMarker[2] = 'M';
        Header.m_aMarker[3] = '1';
        Header.m_NumTicks = 0;
        io_write(m_pFile, &Header, sizeof(Header));

        m_NumTicks = 0;
        m_LastRecordedTick = -1;
        m_vInputs.clear();
        m_Recording = true;
        g_Config.m_ClFujixRecord = 1;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "recording started");
    }
}

bool CFujixRecorder::LoadRecording(const char *pFilename)
{
    IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "could not open recording");
        return false;
    }

    CKjmHeader Header;
    if(io_read(File, &Header, sizeof(Header)) != sizeof(Header) ||
       Header.m_aMarker[0] != 'K' || Header.m_aMarker[1] != 'J' ||
       Header.m_aMarker[2] != 'M' || Header.m_aMarker[3] != '1')
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "invalid recording file");
        io_close(File);
        return false;
    }

    m_vInputs.clear();
    for(int i = 0; i < Header.m_NumTicks; i++)
    {
        CKjmTick Tick;
        if(io_read(File, &Tick, sizeof(Tick)) != sizeof(Tick))
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "error reading recording");
            io_close(File);
            return false;
        }
        CInputFrame Frame{};
        Frame.m_Input = Tick.m_Input;
        m_vInputs.push_back(Frame);
    }
    io_close(File);
    return true;
}

void CFujixRecorder::StartPlay()
{
    char aPath[IO_MAX_PATH_LENGTH];
    str_format(aPath, sizeof(aPath), "demos/fujix/%s.kjm", Client()->GetCurrentMap());

    if(m_Playing)
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback stopped");
        return;
    }

    if(m_Recording)
        ToggleRecord();

    if(!LoadRecording(aPath))
        return;

    m_PlayIndex = 0;
    m_Playing = true;
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback started");
}

int CFujixRecorder::SnapInput(int *pData)
{
    if(!m_Playing)
        return 0;

    if(m_PlayIndex >= (int)m_vInputs.size())
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback finished");
        return 0;
    }

    const CNetObj_PlayerInput &Input = m_vInputs[m_PlayIndex].m_Input;
    GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy] = Input;
    GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy] = Input;
    mem_copy(pData, &Input, sizeof(Input));
    m_PlayIndex++;
    return sizeof(Input);
}


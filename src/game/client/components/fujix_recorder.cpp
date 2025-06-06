#include "fujix_recorder.h"
#include <game/client/gameclient.h>
#include <engine/demo.h>
#include <engine/storage.h>
#include <engine/shared/protocol.h>
#include <engine/shared/protocol_ex.h>
#include <base/math.h>
#include <memory>

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
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Toggle Fujix demo recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play Fujix demo");
}

void CFujixRecorder::OnMapLoad()
{
    m_Recording = false;
    m_Playing = false;
    g_Config.m_ClFujixRecord = 0;
}

void CFujixRecorder::OnUpdate()
{
    if(!m_Playing)
        return;

    if(m_PlayIndex >= (int)m_vInputs.size())
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback finished");
        return;
    }

    GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy] = m_vInputs[m_PlayIndex].m_Input;
    GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy] = m_vInputs[m_PlayIndex].m_Input;
    m_PlayIndex++;
}

void CFujixRecorder::ToggleRecord()
{
    char aPath[IO_MAX_PATH_LENGTH];
    str_format(aPath, sizeof(aPath), "fujix/%s", Client()->GetCurrentMap());

    if(m_Recording)
    {
        Client()->DemoRecorder(RECORDER_MANUAL)->Stop(IDemoRecorder::EStopMode::KEEP_FILE);
        m_Recording = false;
        g_Config.m_ClFujixRecord = 0;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "demo saved");
    }
    else
    {
        Storage()->CreateFolder("demos/fujix", IStorage::TYPE_SAVE);
        Client()->DemoRecorder_Start(aPath, false, RECORDER_MANUAL, true);
        m_Recording = true;
        g_Config.m_ClFujixRecord = 1;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "demo recording started");
    }
}

void CFujixRecorder::StartPlay()
{
    char aPath[IO_MAX_PATH_LENGTH];
    str_format(aPath, sizeof(aPath), "demos/fujix/%s.demo", Client()->GetCurrentMap());

    if(m_Playing)
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback stopped");
        return;
    }

    if(m_Recording)
        ToggleRecord();

    if(!LoadInputs(aPath))
        return;

    m_PlayIndex = 0;
    m_Playing = true;
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback started");
}

bool CFujixRecorder::LoadInputs(const char *pFilename)
{
    m_vInputs.clear();

    std::unique_ptr<CSnapshotDelta> pDelta = std::make_unique<CSnapshotDelta>();
    CDemoPlayer Player(pDelta.get(), false);

    if(Player.Load(Storage(), Console(), pFilename, IStorage::TYPE_SAVE))
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", Player.ErrorMessage());
        return false;
    }

    class CInputListener : public CDemoPlayer::IListener
    {
    public:
        std::vector<CInputFrame> *m_pFrames;
        void OnDemoPlayerSnapshot(void *, int) override {}
        void OnDemoPlayerMessage(void *pData, int Size) override
        {
            CUnpacker Unpacker;
            Unpacker.Reset(pData, Size);
            CMsgPacker Packer(NETMSG_EX, true);
            int Msg; bool Sys; CUuid Uuid;
            int Result = UnpackMessageId(&Msg, &Sys, &Uuid, &Unpacker, &Packer);
            if(Result != UNPACKMESSAGE_OK || Sys)
                return;
            if(Msg == NETMSG_INPUT)
            {
                Unpacker.GetInt(); // AckTick
                Unpacker.GetInt(); // Tick
                int DataSize = Unpacker.GetInt();
                CInputFrame Frame{};
                int Ints = minimum(DataSize / 4, (int)(sizeof(CNetObj_PlayerInput) / sizeof(int)));
                int *pDest = (int *)&Frame.m_Input;
                for(int i = 0; i < Ints; i++)
                    pDest[i] = Unpacker.GetInt();
                if(!Unpacker.Error())
                    m_pFrames->push_back(Frame);
            }
        }
    } Listener;

    Listener.m_pFrames = &m_vInputs;
    Player.SetListener(&Listener);
    Player.Play();
    while(Player.IsPlaying())
        Player.Update(false);
    Player.Stop();

    if(m_vInputs.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "no inputs found");
        return false;
    }
    return true;
}

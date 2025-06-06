#include "fujix_recorder.h"
#include <game/client/gameclient.h>
#include <engine/demo.h>
#include <engine/storage.h>

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
    if(m_Playing && !GameClient()->DemoPlayer()->IsPlaying())
        m_Playing = false;
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
        Client()->Disconnect();
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback stopped");
        return;
    }

    if(m_Recording)
        ToggleRecord();

    const char *pError = Client()->DemoPlayer_Play(aPath, IStorage::TYPE_SAVE);
    if(pError)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", pError);
        return;
    }
    m_Playing = true;
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback started");
}

#include "fujix_tas.h"

#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/client.h>
#include <game/client/gameclient.h>

const char *CFujixTas::ms_pFujixDir = "fujix";

CFujixTas::CFujixTas()
{
    m_Recording = false;
    m_aFilename[0] = '\0';
}

void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.demo", ms_pFujixDir, pMap);
}

void CFujixTas::StartRecord()
{
    if(m_Recording)
        return;
    GetPath(m_aFilename, sizeof(m_aFilename));
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    Client()->RaceRecord_Start(m_aFilename);
    m_Recording = true;
}

void CFujixTas::StopRecord()
{
    if(!m_Recording)
        return;
    Client()->RaceRecord_Stop();
    m_Recording = false;
}

void CFujixTas::Play()
{
    if(m_Recording)
        StopRecord();
    char aPath[IO_MAX_PATH_LENGTH];
    GetPath(aPath, sizeof(aPath));
    Client()->DemoPlayer_Play(aPath, IStorage::TYPE_SAVE);
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
    pSelf->Play();
}

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS demo for current map");
}

void CFujixTas::OnMapLoad()
{
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
}

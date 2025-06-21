#include "fujix_tas.h"

#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/client.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/animstate.h>
#include <base/math.h>
#include <game/gamecore.h>
#include <game/client/components/players.h>
#include <base/system.h>
#include <cmath>

const char *CFujixTas::ms_pFujixDir = "fujix";

CFujixTas::CFujixTas()
{
    m_Recording = false;
    m_Playing = false;
    m_Testing = false;
    m_StartTick = 0;
    m_TestStartTick = 0;
    m_PlayStartTick = 0;
    m_File = nullptr;
    m_HookFile = nullptr;
    m_EventFile = nullptr;
    m_PlayIndex = 0;
    m_PlayHookIndex = 0;
    m_PlayActionIndex = 0;
    m_LastRecordTick = -1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_aFilename[0] = '\0';
    m_aHookFilename[0] = '\0';
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_LastHookState = HOOK_IDLE;
    m_HookRecording = false;
    m_PhantomHookIndex = 0;
    m_PhantomActionIndex = 0;
    m_StopPending = false;
    m_StopTick = -1;
    m_PhantomActive = false;
    m_PhantomTick = 0;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomFreezeTime = 0;
    m_PhantomStep = 1;
    m_LastPredTick = 0;
    m_PhantomHistory.clear();
    m_PendingInputs.clear();
    m_OldShowOthers = g_Config.m_ClShowOthersAlpha;
    m_FreezeActive = false;
    m_FreezeLevel = 0.0f;
    m_FreezeHookTicks = 0;
    m_FreezeHookCooldown = 0;
}

void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjx", ms_pFujixDir, pMap);
}

void CFujixTas::GetHookPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjh", ms_pFujixDir, pMap);
}

void CFujixTas::GetEventPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fja", ms_pFujixDir, pMap);
}

void CFujixTas::RecordActionEvent(EActionType Type, vec2 Pos, int Tick)
{
    if(!m_Recording || !m_EventFile)
        return;
    SActionEvent Ev;
    Ev.m_Tick = Tick - m_StartTick;
    Ev.m_Pos = Pos;
    Ev.m_Type = Type;
    io_write(m_EventFile, &Ev, sizeof(Ev));
    m_vActionEvents.push_back(Ev);
}

void CFujixTas::RecordEntry(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || !m_File)
        return;
    bool Active = mem_comp(pInput, &m_LastInput, sizeof(*pInput)) != 0;
    vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
    SEntry e{Tick - m_StartTick, *pInput, Pos, Active};
    io_write(m_File, &e, sizeof(e));
    m_vEntries.push_back(e);
    m_LastInput = *pInput;
}


bool CFujixTas::FetchEntry(CNetObj_PlayerInput *pInput)
{
    if(!m_Playing)
        return false;

    UpdatePlaybackInput();
    *pInput = m_CurrentInput;

    // also update the local control state so prediction uses the TAS input
    GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy] = m_CurrentInput;
    GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy] = m_CurrentInput;

    return true;
}

void CFujixTas::UpdatePlaybackInput()
{
    if(!m_Playing)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_CurrentInput.m_Jump = 0;
    while(m_PlayIndex < (int)m_vEntries.size() &&
          m_PlayStartTick + m_vEntries[m_PlayIndex].m_Tick <= PredTick)
    {
        m_CurrentInput = m_vEntries[m_PlayIndex].m_Input;
        m_PlayIndex++;
    }

    // default hook state from entries is ignored
    m_CurrentInput.m_Hook = 0;

    while(m_PlayHookIndex < (int)m_vHookEvents.size() &&
          m_PlayStartTick + m_vHookEvents[m_PlayHookIndex].m_EndTick <= PredTick)
        m_PlayHookIndex++;

    while(m_PlayActionIndex < (int)m_vActionEvents.size() &&
          m_PlayStartTick + m_vActionEvents[m_PlayActionIndex].m_Tick <= PredTick)
    {
        const SActionEvent &Ev = m_vActionEvents[m_PlayActionIndex];
        switch(Ev.m_Type)
        {
        case ACTION_LEFT_PRESS: m_CurrentInput.m_Direction = -1; break;
        case ACTION_LEFT_RELEASE: if(m_CurrentInput.m_Direction < 0) m_CurrentInput.m_Direction = 0; break;
        case ACTION_RIGHT_PRESS: m_CurrentInput.m_Direction = 1; break;
        case ACTION_RIGHT_RELEASE: if(m_CurrentInput.m_Direction > 0) m_CurrentInput.m_Direction = 0; break;
        case ACTION_HOOK_PRESS: m_CurrentInput.m_Hook = 1; break;
        case ACTION_HOOK_RELEASE: m_CurrentInput.m_Hook = 0; break;
        case ACTION_JUMP: m_CurrentInput.m_Jump = 1; break;
        }
        m_PlayActionIndex++;
    }

    if(m_PlayHookIndex < (int)m_vHookEvents.size())
    {
        const SHookEvent &Ev = m_vHookEvents[m_PlayHookIndex];
        int Start = m_PlayStartTick + Ev.m_StartTick;
        int End = m_PlayStartTick + Ev.m_EndTick;
        if(PredTick >= Start && PredTick < End)
        {
            m_CurrentInput.m_Hook = 1;
            vec2 Target((Ev.m_TileX + 0.5f) * 32.0f, (Ev.m_TileY + 0.5f) * 32.0f);
            vec2 Dir = Target - GameClient()->m_PredictedChar.m_Pos;
            if(length(Dir) > 0.0f)
            {
                Dir = normalize(Dir);
                m_CurrentInput.m_TargetX = (int)(Dir.x * 256);
                m_CurrentInput.m_TargetY = (int)(Dir.y * 256);
            }
        }
    }


    if(m_PlayIndex >= (int)m_vEntries.size() &&
       m_PlayHookIndex >= (int)m_vHookEvents.size() &&
       m_PlayActionIndex >= (int)m_vActionEvents.size() &&
       PredTick >= m_PlayStartTick + m_vEntries.back().m_Tick)
    {
        m_Playing = false;
        g_Config.m_ClFujixTasPlay = 0;
    }
}

void CFujixTas::StartRecord()
{
    if(m_Recording)
        return;
    GetPath(m_aFilename, sizeof(m_aFilename));
    GetHookPath(m_aHookFilename, sizeof(m_aHookFilename));
    char aEventPath[IO_MAX_PATH_LENGTH];
    GetEventPath(aEventPath, sizeof(aEventPath));
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        if(m_File)
            io_close(m_File);
        m_File = nullptr;
        m_HookFile = nullptr;
        return;
    }
    m_HookFile = Storage()->OpenFile(m_aHookFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    m_EventFile = Storage()->OpenFile(aEventPath, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_HookFile || !m_EventFile)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open hook file for recording");
        if(m_File)
            io_close(m_File);
        m_File = nullptr;
        if(m_HookFile)
            io_close(m_HookFile);
        if(m_EventFile)
            io_close(m_EventFile);
        m_HookFile = nullptr;
        m_EventFile = nullptr;
        return;
    }
    // start recording on the next predicted tick to align with
    // the upcoming OnSnapInput call
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_LastHookState = GameClient()->m_PredictedChar.m_HookState;
    m_HookRecording = false;
    m_Recording = true;
    g_Config.m_ClFujixTasRecord = 1;
    m_vEntries.clear();
    m_vHookEvents.clear();
    m_vActionEvents.clear();
    m_PlayHookIndex = 0;
    m_PlayActionIndex = 0;
    m_PhantomHookIndex = 0;
    m_PhantomHistory.clear();
    m_PendingInputs.clear();

    // init phantom
    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    // Convert the configured tick rate into a simulation step. Higher values
    // yield more precise phantom movement. With the new default of 50 TPS this
    // results in a step of 1 game tick.
    m_PhantomStep = maximum(1, Client()->GameTickSpeed() / g_Config.m_ClFujixTasPhantomTps);
    m_LastPredTick = m_PhantomTick;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomFreezeTime = 0;
    m_PhantomActive = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_CollisionDisabled = true;
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_PhantomInput, m_PhantomFreezeTime});

    m_OldShowOthers = g_Config.m_ClShowOthersAlpha;
    if(!g_Config.m_ClFujixTasShowPlayers)
        g_Config.m_ClShowOthersAlpha = 0;
}

void CFujixTas::FinishRecord()
{
    if(!m_Recording)
        return;
    if(m_HookRecording)
    {
        m_CurHookEvent.m_EndTick = Client()->PredGameTick(g_Config.m_ClDummy) - m_StartTick;
        if(m_HookFile)
            io_write(m_HookFile, &m_CurHookEvent, sizeof(m_CurHookEvent));
        m_vHookEvents.push_back(m_CurHookEvent);
        m_HookRecording = false;
    }
    if(m_File)
        io_close(m_File);
    if(m_HookFile)
        io_close(m_HookFile);
    if(m_EventFile)
        io_close(m_EventFile);
    m_File = nullptr;
    m_HookFile = nullptr;
    m_EventFile = nullptr;
    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_PhantomActive = false;
    m_PendingInputs.clear();
    m_LastRecordTick = -1;
    m_StopPending = false;
    m_StopTick = -1;
    g_Config.m_ClShowOthersAlpha = m_OldShowOthers;
}

void CFujixTas::StopRecord()
{
    if(!m_Recording || m_StopPending)
        return;
    m_StopPending = true;
    m_StopTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
}

void CFujixTas::MaybeFinishRecord()
{
    if(m_StopPending && Client()->PredGameTick(g_Config.m_ClDummy) >= m_StopTick)
        FinishRecord();
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

    char aHookPath[IO_MAX_PATH_LENGTH];
    GetHookPath(aHookPath, sizeof(aHookPath));
    IOHANDLE HookFile = Storage()->OpenFile(aHookPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    char aEventPath[IO_MAX_PATH_LENGTH];
    GetEventPath(aEventPath, sizeof(aEventPath));
    IOHANDLE EventFile = Storage()->OpenFile(aEventPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    m_vHookEvents.clear();
    m_vActionEvents.clear();
    SHookEvent he;
    if(HookFile)
    {
        while(io_read(HookFile, &he, sizeof(he)) == sizeof(he))
            m_vHookEvents.push_back(he);
        io_close(HookFile);
    }
    if(EventFile)
    {
        SActionEvent ev;
        while(io_read(EventFile, &ev, sizeof(ev)) == sizeof(ev))
            m_vActionEvents.push_back(ev);
        io_close(EventFile);
    }
    m_PlayHookIndex = 0;
    m_PlayActionIndex = 0;
    m_PhantomHookIndex = 0;

    m_PlayIndex = 0;
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Playing = !m_vEntries.empty();
    if(m_Playing)
    {
        g_Config.m_ClFujixTasPlay = 1;
        m_CurrentInput = m_vEntries[0].m_Input;
    }
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    g_Config.m_ClFujixTasPlay = 0;
    m_vEntries.clear();
    m_vHookEvents.clear();
    m_vActionEvents.clear();
    m_PlayIndex = 0;
    m_PlayHookIndex = 0;
    m_PlayActionIndex = 0;
    m_PlayStartTick = 0;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
}

void CFujixTas::StartTest()
{
    if(m_Testing)
        StopTest();

    char aPath[IO_MAX_PATH_LENGTH];
    GetPath(aPath, sizeof(aPath));
    IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for test");
        return;
    }

    char aHookPath[IO_MAX_PATH_LENGTH];
    GetHookPath(aHookPath, sizeof(aHookPath));
    IOHANDLE HookFile = Storage()->OpenFile(aHookPath, IOFLAG_READ, IStorage::TYPE_SAVE);

    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    m_vHookEvents.clear();
    SHookEvent he;
    if(HookFile)
    {
        while(io_read(HookFile, &he, sizeof(he)) == sizeof(he))
            m_vHookEvents.push_back(he);
        io_close(HookFile);
    }
    if(EventFile)
    {
        SActionEvent ev;
        while(io_read(EventFile, &ev, sizeof(ev)) == sizeof(ev))
            m_vActionEvents.push_back(ev);
        io_close(EventFile);
    }
    m_PhantomHookIndex = 0;
    m_PhantomActionIndex = 0;

    // init phantom at current position
    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomStep = maximum(1, Client()->GameTickSpeed() / g_Config.m_ClFujixTasPhantomTps);
    m_LastPredTick = m_PhantomTick;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomFreezeTime = 0;
    m_PhantomActive = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_CollisionDisabled = true;
    m_PhantomHistory.clear();
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_PhantomInput, m_PhantomFreezeTime});

    m_PendingInputs.clear();
    m_TestStartTick = m_PhantomTick + 1;
    for(const auto &Entry : m_vEntries)
        m_PendingInputs.push_back({m_TestStartTick + Entry.m_Tick, Entry.m_Input});

    m_Testing = !m_vEntries.empty();
    if(m_Testing)
        g_Config.m_ClFujixTasTest = 1;
}

void CFujixTas::StopTest()
{
    m_Testing = false;
    g_Config.m_ClFujixTasTest = 0;
    m_PhantomActive = false;
    m_PendingInputs.clear();
    m_PhantomHistory.clear();
    m_vEntries.clear();
    m_vHookEvents.clear();
    m_vActionEvents.clear();
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    return FetchEntry(pInput);
}

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(Tick == m_LastRecordTick)
        return;
    m_LastRecordTick = Tick;

    vec2 Pos = GameClient()->m_PredictedChar.m_Pos;

    RecordEntry(pInput, Tick);

    if(m_Recording)
    {
        m_PendingInputs.push_back({Tick, *pInput});
        TickPhantomUpTo(Tick);

        if(pInput->m_Direction < 0 && m_LastInput.m_Direction >= 0)
            RecordActionEvent(ACTION_LEFT_PRESS, Pos, Tick);
        if(pInput->m_Direction >= 0 && m_LastInput.m_Direction < 0)
            RecordActionEvent(ACTION_LEFT_RELEASE, Pos, Tick);
        if(pInput->m_Direction > 0 && m_LastInput.m_Direction <= 0)
            RecordActionEvent(ACTION_RIGHT_PRESS, Pos, Tick);
        if(pInput->m_Direction <= 0 && m_LastInput.m_Direction > 0)
            RecordActionEvent(ACTION_RIGHT_RELEASE, Pos, Tick);
        if(pInput->m_Hook && !m_LastInput.m_Hook)
            RecordActionEvent(ACTION_HOOK_PRESS, Pos, Tick);
        if(!pInput->m_Hook && m_LastInput.m_Hook)
            RecordActionEvent(ACTION_HOOK_RELEASE, Pos, Tick);
        if(pInput->m_Jump && !m_LastInput.m_Jump)
            RecordActionEvent(ACTION_JUMP, Pos, Tick);

        if(!m_HookRecording && pInput->m_Hook)
        {
            m_CurHookEvent.m_StartTick = Tick - m_StartTick;
            m_CurHookEvent.m_TileX = -1;
            m_CurHookEvent.m_TileY = -1;
            m_HookRecording = true;
        }
        if(m_HookRecording)
        {
            if(m_CurHookEvent.m_TileX == -1 && GameClient()->m_PredictedChar.m_HookState == HOOK_GRABBED && GameClient()->m_PredictedChar.HookedPlayer() == -1)
            {
                vec2 HPos = GameClient()->m_PredictedChar.m_HookPos;
                m_CurHookEvent.m_TileX = (int)floor(HPos.x / 32.0f);
                m_CurHookEvent.m_TileY = (int)floor(HPos.y / 32.0f);
            }
            if(!pInput->m_Hook)
            {
                m_CurHookEvent.m_EndTick = Tick - m_StartTick;
                if(m_HookFile)
                    io_write(m_HookFile, &m_CurHookEvent, sizeof(m_CurHookEvent));
                m_vHookEvents.push_back(m_CurHookEvent);
                m_HookRecording = false;
            }
        }
    }
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

void CFujixTas::ConTest(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Testing)
        pSelf->StopTest();
    else
        pSelf->StartTest();
}

void CFujixTas::ConFreezeUp(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    g_Config.m_ClFujixFreezeLevel -= 32;
}

void CFujixTas::ConFreezeDown(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    g_Config.m_ClFujixFreezeLevel += 32;
}

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
    Console()->Register("fujix_test", "", CFGFLAG_CLIENT, ConTest, this, "Play FUJIX TAS as phantom");
    Console()->Register("fujix_freeze_up", "", CFGFLAG_CLIENT, ConFreezeUp, this, "Raise freeze level by one block");
    Console()->Register("fujix_freeze_down", "", CFGFLAG_CLIENT, ConFreezeDown, this, "Lower freeze level by one block");
}

void CFujixTas::OnMapLoad()
{
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
}

void CFujixTas::RewriteFile()
{
    if(!m_File)
        return;
    io_close(m_File);
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    for(const auto &e : m_vEntries)
        io_write(m_File, &e, sizeof(e));
}

void CFujixTas::RollbackPhantom(int Ticks)
{
    if(Ticks <= 0 || m_PhantomHistory.empty())
        return;
    int Target = m_PhantomTick - Ticks;
    if(Target < m_StartTick)
        Target = m_StartTick;
    SPhantomState State = m_PhantomHistory.front();
    for(const auto &s : m_PhantomHistory)
    {
        if(s.m_Tick <= Target)
            State = s;
        else
            break;
    }
    while(!m_PhantomHistory.empty() && m_PhantomHistory.back().m_Tick > Target)
        m_PhantomHistory.pop_back();
    while(!m_vEntries.empty() && m_StartTick + m_vEntries.back().m_Tick > Target)
        m_vEntries.pop_back();
    while(!m_PendingInputs.empty() && m_PendingInputs.back().m_Tick > Target)
        m_PendingInputs.pop_back();
    m_PhantomCore = State.m_Core;
    m_PhantomPrevCore = State.m_PrevCore;
    m_PhantomInput = State.m_Input;
    m_PhantomFreezeTime = State.m_FreezeTime;
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_PhantomInput, m_PhantomFreezeTime});
    RewriteFile();
}

void CFujixTas::PhantomFreeze(int Seconds)
{
    if(Seconds <= 0)
        Seconds = g_Config.m_SvFreezeDelay;
    int Time = Seconds * Client()->GameTickSpeed();
    if(m_PhantomFreezeTime >= Time)
        return;
    m_PhantomFreezeTime = Time;
    m_PhantomCore.m_FreezeStart = m_PhantomTick;
    m_PhantomCore.m_FreezeEnd = m_PhantomCore.m_DeepFrozen ? -1 : m_PhantomTick + m_PhantomFreezeTime;
}

void CFujixTas::PhantomUnfreeze()
{
    if(m_PhantomFreezeTime > 0)
    {
        m_PhantomFreezeTime = 0;
        m_PhantomCore.m_FreezeStart = 0;
        m_PhantomCore.m_FreezeEnd = m_PhantomCore.m_DeepFrozen ? -1 : 0;
    }
}

bool CFujixTas::HandlePhantomTiles(int MapIndex)
{
    bool Rewound = false;

    if(MapIndex < 0)
        return false;

    int Tile = Collision()->GetTileIndex(MapIndex);
    int FTile = Collision()->GetFrontTileIndex(MapIndex);
    int SwitchType = Collision()->GetSwitchType(MapIndex);

    int Tele = Collision()->IsTeleport(MapIndex);
    if(Tele && !Collision()->TeleOuts(Tele - 1).empty())
    {
        if(m_Recording && g_Config.m_ClFujixTasRewind)
        {
            RollbackPhantom(g_Config.m_ClFujixTasRewindTicks);
            Rewound = true;
        }
        else
            m_PhantomCore.m_Pos = Collision()->TeleOuts(Tele - 1)[0];
    }

    int EvilTele = Collision()->IsEvilTeleport(MapIndex);
    if(EvilTele && !Collision()->TeleOuts(EvilTele - 1).empty())
    {
        if(m_Recording && g_Config.m_ClFujixTasRewind)
        {
            RollbackPhantom(g_Config.m_ClFujixTasRewindTicks);
            Rewound = true;
        }
        else
            m_PhantomCore.m_Pos = Collision()->TeleOuts(EvilTele - 1)[0];
    }

    if(Tile == TILE_FREEZE || FTile == TILE_FREEZE || SwitchType == TILE_FREEZE)
    {
        if(m_Recording && g_Config.m_ClFujixTasRewind)
        {
            RollbackPhantom(g_Config.m_ClFujixTasRewindTicks);
            Rewound = true;
        }
        else
            PhantomFreeze(Collision()->GetSwitchDelay(MapIndex));
    }
    else if(Tile == TILE_UNFREEZE || FTile == TILE_UNFREEZE || SwitchType == TILE_DUNFREEZE)
    {
        PhantomUnfreeze();
    }

    if(Tile == TILE_DFREEZE || FTile == TILE_DFREEZE || SwitchType == TILE_DFREEZE)
    {
        if(m_Recording && g_Config.m_ClFujixTasRewind)
        {
            RollbackPhantom(g_Config.m_ClFujixTasRewindTicks);
            Rewound = true;
        }
        else
            m_PhantomCore.m_DeepFrozen = true;
    }
    else if(Tile == TILE_DUNFREEZE || FTile == TILE_DUNFREEZE || SwitchType == TILE_DUNFREEZE)
    {
        m_PhantomCore.m_DeepFrozen = false;
    }

    if(Tile == TILE_LFREEZE || FTile == TILE_LFREEZE || SwitchType == TILE_LFREEZE)
    {
        if(m_Recording && g_Config.m_ClFujixTasRewind)
        {
            RollbackPhantom(g_Config.m_ClFujixTasRewindTicks);
            Rewound = true;
        }
        else
            m_PhantomCore.m_LiveFrozen = true;
    }
    else if(Tile == TILE_LUNFREEZE || FTile == TILE_LUNFREEZE || SwitchType == TILE_LUNFREEZE)
    {
        m_PhantomCore.m_LiveFrozen = false;
    }

    if(SwitchType == TILE_JUMP)
    {
        int NewJumps = Collision()->GetSwitchDelay(MapIndex);
        if(NewJumps == 255)
            NewJumps = -1;
        if(NewJumps != m_PhantomCore.m_Jumps)
            m_PhantomCore.m_Jumps = NewJumps;
    }

    return Rewound;
}

void CFujixTas::TickPhantomUpTo(int TargetTick)
{
    if(!m_PhantomActive)
        return;

    while(m_PhantomTick + m_PhantomStep <= TargetTick)
    {
        m_PhantomPrevCore = m_PhantomCore;
        while(!m_PendingInputs.empty() && m_PendingInputs.front().m_Tick <= m_PhantomTick + m_PhantomStep)
        {
            m_PhantomInput = m_PendingInputs.front().m_Input;
            m_PendingInputs.pop_front();
        }
        CNetObj_PlayerInput Input = m_PhantomInput;

        int BaseTick = m_Recording ? m_StartTick : m_TestStartTick;
        while(m_PhantomHookIndex < (int)m_vHookEvents.size() && BaseTick + m_vHookEvents[m_PhantomHookIndex].m_EndTick <= m_PhantomTick)
            m_PhantomHookIndex++;
        while(m_PhantomActionIndex < (int)m_vActionEvents.size() && BaseTick + m_vActionEvents[m_PhantomActionIndex].m_Tick <= m_PhantomTick)
        {
            const SActionEvent &Ev = m_vActionEvents[m_PhantomActionIndex];
            switch(Ev.m_Type)
            {
            case ACTION_LEFT_PRESS: Input.m_Direction = -1; break;
            case ACTION_LEFT_RELEASE: if(Input.m_Direction < 0) Input.m_Direction = 0; break;
            case ACTION_RIGHT_PRESS: Input.m_Direction = 1; break;
            case ACTION_RIGHT_RELEASE: if(Input.m_Direction > 0) Input.m_Direction = 0; break;
            case ACTION_HOOK_PRESS: Input.m_Hook = 1; break;
            case ACTION_HOOK_RELEASE: Input.m_Hook = 0; break;
            case ACTION_JUMP: Input.m_Jump = 1; break;
            }
            m_PhantomActionIndex++;
        }
        if(m_PhantomHookIndex < (int)m_vHookEvents.size())
        {
            const SHookEvent &Ev = m_vHookEvents[m_PhantomHookIndex];
            int Start = BaseTick + Ev.m_StartTick;
            int End = BaseTick + Ev.m_EndTick;
            if(m_PhantomTick >= Start && m_PhantomTick < End)
            {
                Input.m_Hook = 1;
                vec2 Target((Ev.m_TileX + 0.5f) * 32.0f, (Ev.m_TileY + 0.5f) * 32.0f);
                vec2 Dir = Target - m_PhantomCore.m_Pos;
                if(length(Dir) > 0.0f)
                {
                    Dir = normalize(Dir);
                    Input.m_TargetX = (int)(Dir.x * 256);
                    Input.m_TargetY = (int)(Dir.y * 256);
                }
            }
            else
                Input.m_Hook = 0;
        }

        if(m_PhantomFreezeTime > 0)
        {
            Input.m_Direction = 0;
            Input.m_Jump = 0;
            Input.m_Hook = 0;
            m_PhantomFreezeTime--;
            if(m_PhantomFreezeTime == 0 && !m_PhantomCore.m_DeepFrozen)
                m_PhantomCore.m_FreezeEnd = 0;
        }
        m_PhantomCore.m_Input = Input;
        m_PhantomCore.Tick(true);
        m_PhantomCore.Move();
        int MapIndex = Collision()->GetMapIndex(m_PhantomCore.m_Pos);
        if(HandlePhantomTiles(MapIndex))
            return;
        m_PhantomCore.Quantize();
        m_PhantomTick += m_PhantomStep;
        m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_PhantomInput, m_PhantomFreezeTime});
        if(m_PhantomHistory.size() > 60)
            m_PhantomHistory.pop_front();
    }
}

void CFujixTas::TickPhantom()
{
    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    TickPhantomUpTo(PredTick);
}

void CFujixTas::CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick)
{
    CNetObj_CharacterCore CCore;
    Core.Write(&CCore);
    mem_zero(pChar, sizeof(*pChar));
    pChar->m_X = CCore.m_X;
    pChar->m_Y = CCore.m_Y;
    pChar->m_VelX = CCore.m_VelX;
    pChar->m_VelY = CCore.m_VelY;
    pChar->m_Angle = CCore.m_Angle;
    pChar->m_Direction = CCore.m_Direction;
    pChar->m_Weapon = Core.m_ActiveWeapon;
    pChar->m_HookState = CCore.m_HookState;
    pChar->m_HookTick = CCore.m_HookTick;
    pChar->m_HookX = CCore.m_HookX;
    pChar->m_HookY = CCore.m_HookY;
    pChar->m_HookDx = CCore.m_HookDx;
    pChar->m_HookDy = CCore.m_HookDy;
    pChar->m_HookedPlayer = CCore.m_HookedPlayer;
    pChar->m_Jumped = CCore.m_Jumped;
    pChar->m_Tick = Tick;
    pChar->m_AttackTick = Core.m_HookTick + (Client()->GameTick(g_Config.m_ClDummy) - Tick);
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

    if(g_Config.m_ClFujixTasTest && !m_Testing)
        StartTest();
    else if(!g_Config.m_ClFujixTasTest && m_Testing)
        StopTest();

    if(g_Config.m_ClFujixFreeze && !m_FreezeActive)
        StartFreeze();
    else if(!g_Config.m_ClFujixFreeze && m_FreezeActive)
        StopFreeze();

    if(m_FreezeActive)
        m_FreezeLevel = g_Config.m_ClFujixFreezeLevel;

    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(!m_PhantomActive && !m_FreezeActive)
        return;

    if(m_PhantomActive)
    {
    CNetObj_Character Prev, Curr;
    CoreToCharacter(m_PhantomPrevCore, &Prev, m_PhantomTick - m_PhantomStep);
    CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

    GameClient()->m_Players.RenderHook(&Prev, &Curr, &m_PhantomRenderInfo, -2);
    GameClient()->m_Players.RenderHookCollLine(&Prev, &Curr, -2);
    GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &m_PhantomRenderInfo, -2);

    RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    RenderRecommendedRoute(g_Config.m_ClFujixTasRouteTicks);
    }

    RenderFreezeIndicator();
    RenderAimLines();
}

void CFujixTas::RenderFuturePath(int TicksAhead)
{
    if(TicksAhead <= 0 || !m_PhantomActive)
        return;

    CFujixTas Tmp = *this;
    std::vector<vec2> Points;
    Points.reserve(TicksAhead + 1);
    Points.push_back(Tmp.m_PhantomCore.m_Pos);

    int TargetTick = m_PhantomTick + TicksAhead;
    while(Tmp.m_PhantomTick < TargetTick)
    {
        int StepTarget = minimum(TargetTick, Tmp.m_PhantomTick + Tmp.m_PhantomStep);
        Tmp.TickPhantomUpTo(StepTarget);
        Points.push_back(Tmp.m_PhantomCore.m_Pos);
    }

    if(Points.size() <= 1)
        return;

    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    for(size_t i = 1; i < Points.size(); i++)
    {
        IGraphics::CLineItem Line(Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y);
        Graphics()->LinesDraw(&Line, 1);
    }
    Graphics()->LinesEnd();
}

void CFujixTas::RenderRecommendedRoute(int TicksAhead)
{
    if(TicksAhead <= 0 || !m_PhantomActive)
        return;

    CFujixTas Tmp = *this;
    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);

    std::vector<vec2> HookPos;

    vec2 Prev = Tmp.m_PhantomCore.m_Pos;
    int TargetTick = m_PhantomTick + TicksAhead;
    while(Tmp.m_PhantomTick < TargetTick)
    {
        int StepTarget = minimum(TargetTick, Tmp.m_PhantomTick + Tmp.m_PhantomStep);
        Tmp.TickPhantomUpTo(StepTarget);
        vec2 Pos = Tmp.m_PhantomCore.m_Pos;
        IGraphics::CLineItem Line(Prev.x, Prev.y, Pos.x, Pos.y);
        Graphics()->LinesDraw(&Line, 1);

        if(Tmp.m_PhantomInput.m_Hook)
            HookPos.push_back(Pos);

        Prev = Pos;
    }

    Graphics()->LinesEnd();

    if(!HookPos.empty())
    {
        Graphics()->QuadsBegin();
        for(const vec2 &Pos : HookPos)
        {
            IGraphics::CQuadItem Quad(Pos.x - 2.0f, Pos.y - 2.0f, 4.0f, 4.0f);
            Graphics()->QuadsDraw(&Quad, 1);
        }
        Graphics()->QuadsEnd();
    }

    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CFujixTas::StartFreeze()
{
    if(m_FreezeActive)
        return;
    m_FreezeActive = true;
    m_FreezeLevel = GameClient()->m_PredictedChar.m_Pos.y;
    g_Config.m_ClFujixFreezeLevel = (int)m_FreezeLevel;
    m_FreezeHookTicks = 0;
    m_FreezeHookCooldown = 0;
}

void CFujixTas::StopFreeze()
{
    m_FreezeActive = false;
    m_FreezeHookTicks = 0;
    m_FreezeHookCooldown = 0;
}

void CFujixTas::UpdateFreezeInput(CNetObj_PlayerInput *pInput)
{
    if(!m_FreezeActive || !pInput)
        return;

    const CTuningParams *pTuning = GameClient()->GetTuning(0);

    if(m_FreezeHookTicks > 0)
    {
        pInput->m_Hook = 1;
        pInput->m_TargetX = 0;
        pInput->m_TargetY = -256;
        m_FreezeHookTicks--;
        return;
    }

    if(m_FreezeHookCooldown > 0)
    {
        pInput->m_Hook = 0;
        m_FreezeHookCooldown--;
        return;
    }

    const int PredictTicks = 20;
    float PosY = GameClient()->m_PredictedChar.m_Pos.y;
    float VelY = GameClient()->m_PredictedChar.m_Vel.y;
    float FutureY = PosY + VelY * PredictTicks + pTuning->m_Gravity * PredictTicks * PredictTicks / 2.0f;

    if(FutureY > m_FreezeLevel + 1.0f)
    {
        int Hold = 1;
        float SimPos = PosY;
        float SimVel = VelY;
        for(int t = 0; t < PredictTicks; t++)
        {
            SimVel += pTuning->m_Gravity - pTuning->m_HookDragAccel;
            SimPos += SimVel;
            if(SimPos <= m_FreezeLevel)
            {
                Hold = t + 1;
                break;
            }
        }
        pInput->m_Hook = 1;
        pInput->m_TargetX = 0;
        pInput->m_TargetY = -256;
        m_FreezeHookTicks = Hold;
        m_FreezeHookCooldown = 2;
    }
    else
    {
        pInput->m_Hook = 0;
    }
}

void CFujixTas::RenderFreezeIndicator()
{
    if(!m_FreezeActive)
        return;

    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.5f);
    float y = m_FreezeLevel;
    IGraphics::CLineItem Line(-100000.0f, y, 100000.0f, y);
    Graphics()->LinesDraw(&Line, 1);
    Graphics()->LinesEnd();
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CFujixTas::RenderAimLines()
{
    if(!g_Config.m_ClFujixAimbot || !g_Config.m_ClFujixAimLines)
        return;

    vec2 Pos = GameClient()->m_LocalCharacterPos;
    vec2 Mouse = GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy];
    vec2 Dir = normalize(Mouse);
    if(length(Mouse) == 0)
        Dir = vec2(1, 0);

    float BaseAng = angle(Dir);
    float Off = g_Config.m_ClFujixAimAngle * pi / 180.0f;

    vec2 Up = direction(BaseAng + Off);
    vec2 Down = direction(BaseAng - Off);

    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    IGraphics::CLineItem L1(Pos.x, Pos.y, Pos.x + Up.x * 500.0f, Pos.y + Up.y * 500.0f);
    IGraphics::CLineItem L2(Pos.x, Pos.y, Pos.x + Down.x * 500.0f, Pos.y + Down.y * 500.0f);
    Graphics()->LinesDraw(&L1, 1);
    Graphics()->LinesDraw(&L2, 1);
    Graphics()->LinesEnd();
}


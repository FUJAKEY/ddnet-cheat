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
    m_PlayIndex = 0;
    m_LastRecordTick = -1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_LastHookState = HOOK_IDLE;
    m_aFilename[0] = '\0';
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
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
    m_EventIndex = 0;
    m_EventFile = nullptr;
    m_vEvents.clear();
}

void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjx", ms_pFujixDir, pMap);
}

void CFujixTas::GetEventPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s_events.fjx", ms_pFujixDir, pMap);
}

void CFujixTas::RecordEntry(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || !m_File)
        return;
    bool Active = mem_comp(pInput, &m_LastInput, sizeof(*pInput)) != 0;
    SEntry e{Tick - m_StartTick, *pInput, Active};
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
    while(m_PlayIndex < (int)m_vEntries.size() &&
          m_PlayStartTick + m_vEntries[m_PlayIndex].m_Tick <= PredTick)
    {
        m_CurrentInput = m_vEntries[m_PlayIndex].m_Input;
        m_PlayIndex++;
    }

    while(m_EventIndex < (int)m_vEvents.size() &&
          m_PlayStartTick + m_vEvents[m_EventIndex].m_Tick <= PredTick)
    {
        const SEvent &Ev = m_vEvents[m_EventIndex];
        switch(Ev.m_Type)
        {
        case EVENT_HOOK:
            m_CurrentInput.m_Hook = Ev.m_Pressed ? 1 : 0;
            break;
        case EVENT_HOOK_ATTACH:
            m_CurrentInput.m_Hook = 1;
            break;
        case EVENT_HOOK_DETACH:
            m_CurrentInput.m_Hook = 0;
            break;
        case EVENT_HOOK_TARGET:
        {
            if(Ev.m_Pressed)
            {
                vec2 PlayerPos = GameClient()->m_PredictedChar.m_Pos;
                vec2 Dir = Ev.m_Pos - PlayerPos;
                if(length(Dir) > 0.0f)
                {
                    Dir = normalize(Dir);
                    m_CurrentInput.m_TargetX = (int)(Dir.x * 256.0f);
                    m_CurrentInput.m_TargetY = (int)(Dir.y * 256.0f);
                }
            }
            break;
        }
        case EVENT_LEFT:
            if(Ev.m_Pressed)
                m_CurrentInput.m_Direction = -1;
            else if(m_CurrentInput.m_Direction == -1)
                m_CurrentInput.m_Direction = 0;
            break;
        case EVENT_RIGHT:
            if(Ev.m_Pressed)
                m_CurrentInput.m_Direction = 1;
            else if(m_CurrentInput.m_Direction == 1)
                m_CurrentInput.m_Direction = 0;
            break;
        case EVENT_JUMP:
            if(Ev.m_Pressed)
                m_CurrentInput.m_Jump |= 1;
            else
                m_CurrentInput.m_Jump &= ~1;
            break;
        }
        m_EventIndex++;
    }

    if(m_PlayIndex >= (int)m_vEntries.size() &&
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
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    char aEventPath[IO_MAX_PATH_LENGTH];
    GetEventPath(aEventPath, sizeof(aEventPath));
    m_EventFile = Storage()->OpenFile(aEventPath, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_File || !m_EventFile)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        if(m_File)
            io_close(m_File);
        if(m_EventFile)
            io_close(m_EventFile);
        m_File = nullptr;
        m_EventFile = nullptr;
        return;
    }
    // start recording on the next predicted tick to align with
    // the upcoming OnSnapInput call
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_LastHookState = GameClient()->m_PredictedChar.m_HookState;
    m_Recording = true;
    g_Config.m_ClFujixTasRecord = 1;
    m_vEntries.clear();
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
    if(m_File)
        io_close(m_File);
    if(m_EventFile)
        io_close(m_EventFile);
    m_File = nullptr;
    m_EventFile = nullptr;
    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_PhantomActive = false;
    m_PendingInputs.clear();
    m_LastRecordTick = -1;
    m_StopPending = false;
    m_StopTick = -1;
    m_EventIndex = 0;
    m_vEvents.clear();
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

    char aEventPath[IO_MAX_PATH_LENGTH];
    GetEventPath(aEventPath, sizeof(aEventPath));
    IOHANDLE EventFile = Storage()->OpenFile(aEventPath, IOFLAG_READ, IStorage::TYPE_SAVE);

    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    m_vEvents.clear();
    if(EventFile)
    {
        SEvent Ev;
        while(io_read(EventFile, &Ev, sizeof(Ev)) == sizeof(Ev))
            m_vEvents.push_back(Ev);
        io_close(EventFile);
    }
    m_EventIndex = 0;

    m_PlayIndex = 0;
    // similar to recording, start playback on the next tick so the
    // first stored input is applied exactly when OnSnapInput runs
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
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_vEvents.clear();
    m_EventIndex = 0;
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

    char aEventPath[IO_MAX_PATH_LENGTH];
    GetEventPath(aEventPath, sizeof(aEventPath));
    IOHANDLE EventFile = Storage()->OpenFile(aEventPath, IOFLAG_READ, IStorage::TYPE_SAVE);

    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    m_vEvents.clear();
    if(EventFile)
    {
        SEvent Ev;
        while(io_read(EventFile, &Ev, sizeof(Ev)) == sizeof(Ev))
            m_vEvents.push_back(Ev);
        io_close(EventFile);
    }
    m_EventIndex = 0;

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
    m_vEvents.clear();
    m_EventIndex = 0;
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

    if(m_LastInput.m_Direction != pInput->m_Direction)
    {
        if(m_LastInput.m_Direction != 0)
            RecordEvent(Tick, Pos, m_LastInput.m_Direction < 0 ? EVENT_LEFT : EVENT_RIGHT, false);
        if(pInput->m_Direction != 0)
            RecordEvent(Tick, Pos, pInput->m_Direction < 0 ? EVENT_LEFT : EVENT_RIGHT, true);
    }

    if((pInput->m_Jump & 1) && !(m_LastInput.m_Jump & 1))
        RecordEvent(Tick, Pos, EVENT_JUMP, true);

    if(pInput->m_Hook != m_LastInput.m_Hook)
    {
        RecordEvent(Tick, Pos, EVENT_HOOK, pInput->m_Hook != 0);
        if(pInput->m_Hook)
        {
            vec2 HookDir = vec2(pInput->m_TargetX, pInput->m_TargetY);
            if(length(HookDir) > 0.0f)
            {
                HookDir = normalize(HookDir);
                float HookLength = GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookLength;
                vec2 TargetPos = Pos + HookDir * HookLength;
                RecordEvent(Tick, TargetPos, EVENT_HOOK_TARGET, true);
            }
        }
    }

    int HookState = GameClient()->m_PredictedChar.m_HookState;
    vec2 HookPos = GameClient()->m_PredictedChar.m_HookPos;
    if(HookState == HOOK_GRABBED && m_LastHookState != HOOK_GRABBED)
        RecordEvent(Tick, HookPos, EVENT_HOOK_ATTACH, true);
    else if(HookState != HOOK_GRABBED && m_LastHookState == HOOK_GRABBED)
        RecordEvent(Tick, HookPos, EVENT_HOOK_DETACH, true);
    m_LastHookState = HookState;

    RecordEntry(pInput, Tick);

    if(m_Recording)
    {
        m_PendingInputs.push_back({Tick, *pInput});
        TickPhantomUpTo(Tick);
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

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
    Console()->Register("fujix_test", "", CFGFLAG_CLIENT, ConTest, this, "Play FUJIX TAS as phantom");
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

void CFujixTas::RecordEvent(int Tick, vec2 Pos, EEventType Type, bool Pressed)
{
    if(!m_EventFile)
        return;
    if(!isfinite(Pos.x) || !isfinite(Pos.y))
        return;

    int RelTick = Tick - m_StartTick;
    if(RelTick < 0)
        return;

    SEvent Ev{RelTick, Pos, Type, Pressed};
    io_write(m_EventFile, &Ev, sizeof(Ev));
    m_vEvents.push_back(Ev);

    if(g_Config.m_ClFujixTasDebug)
    {
        char aBuf[128];
        str_format(aBuf, sizeof(aBuf), "Hook event: tick=%d type=%d pos=(%.1f,%.1f)", RelTick, Type, Pos.x, Pos.y);
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_debug", aBuf);
    }
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
        if(m_Testing)
        {
            while(m_EventIndex < (int)m_vEvents.size())
            {
                const SEvent &Ev = m_vEvents[m_EventIndex];
                int EventTick = m_TestStartTick + Ev.m_Tick;
                if(EventTick > m_PhantomTick + m_PhantomStep)
                    break;
                switch(Ev.m_Type)
                {
                case EVENT_HOOK:
                    m_PhantomInput.m_Hook = Ev.m_Pressed ? 1 : 0;
                    break;
                case EVENT_HOOK_ATTACH:
                    m_PhantomInput.m_Hook = 1;
                    m_PhantomCore.m_HookState = HOOK_GRABBED;
                    m_PhantomCore.m_HookPos = Ev.m_Pos;
                    m_PhantomCore.SetHookedPlayer(-1);
                    m_PhantomCore.m_HookTick = 0;
                    break;
                case EVENT_HOOK_DETACH:
                    m_PhantomInput.m_Hook = 0;
                    m_PhantomCore.m_HookState = HOOK_RETRACTED;
                    m_PhantomCore.m_HookPos = m_PhantomCore.m_Pos;
                    m_PhantomCore.SetHookedPlayer(-1);
                    m_PhantomCore.m_HookTick = 0;
                    break;
                case EVENT_HOOK_TARGET:
                    if(Ev.m_Pressed)
                    {
                        vec2 Dir = Ev.m_Pos - m_PhantomCore.m_Pos;
                        if(length(Dir) > 0.0f)
                        {
                            Dir = normalize(Dir);
                            m_PhantomInput.m_TargetX = (int)(Dir.x * 256.0f);
                            m_PhantomInput.m_TargetY = (int)(Dir.y * 256.0f);
                        }
                    }
                    break;
                case EVENT_LEFT:
                    if(Ev.m_Pressed)
                        m_PhantomInput.m_Direction = -1;
                    else if(m_PhantomInput.m_Direction == -1)
                        m_PhantomInput.m_Direction = 0;
                    break;
                case EVENT_RIGHT:
                    if(Ev.m_Pressed)
                        m_PhantomInput.m_Direction = 1;
                    else if(m_PhantomInput.m_Direction == 1)
                        m_PhantomInput.m_Direction = 0;
                    break;
                case EVENT_JUMP:
                    if(Ev.m_Pressed)
                        m_PhantomInput.m_Jump |= 1;
                    else
                        m_PhantomInput.m_Jump &= ~1;
                    break;
                }
                m_EventIndex++;
            }
        }
        CNetObj_PlayerInput Input = m_PhantomInput;
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

    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(!m_PhantomActive)
        return;

    CNetObj_Character Prev, Curr;
    CoreToCharacter(m_PhantomPrevCore, &Prev, m_PhantomTick - m_PhantomStep);
    CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

    GameClient()->m_Players.RenderHook(&Prev, &Curr, &m_PhantomRenderInfo, -2);
    GameClient()->m_Players.RenderHookCollLine(&Prev, &Curr, -2);
    GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &m_PhantomRenderInfo, -2);

    RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    RenderRecommendedRoute(g_Config.m_ClFujixTasRouteTicks);
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


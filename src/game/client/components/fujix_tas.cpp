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

const char *CFujixTas::ms_pFujixDir = "fujix";

CFujixTas::CFujixTas()
{
    m_Recording = false;
    m_Playing = false;
    m_StartTick = 0;
    m_PlayStartTick = 0;
    m_File = nullptr;
    m_PlayIndex = 0;
    m_LastRecordTick = -1;
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
    m_vEntries.push_back(e);
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
    if(!m_File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        return;
    }
    // start recording on the next predicted tick to align with
    // the upcoming OnSnapInput call
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
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
    m_PhantomStep = maximum(1, Client()->GameTickSpeed() / g_Config.m_ClFujixTasPhantomTps);
    m_LastPredTick = m_PhantomTick;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomFreezeTime = 0;
    m_PhantomActive = true;
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_PhantomInput, m_PhantomFreezeTime});
}

void CFujixTas::FinishRecord()
{
    if(!m_Recording)
        return;
    if(m_File)
        io_close(m_File);
    m_File = nullptr;
    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_PhantomActive = false;
    m_PendingInputs.clear();
    m_LastRecordTick = -1;
    m_StopPending = false;
    m_StopTick = -1;
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

    if(m_Recording)
    {
        if((Tick - m_StartTick) % m_PhantomStep == 0)
        {
            m_PendingInputs.push_back({Tick, *pInput});
            RecordEntry(pInput, Tick);
        }
    }
    else
    {
        RecordEntry(pInput, Tick);
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

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
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

void CFujixTas::TickPhantom()
{
    if(!m_PhantomActive)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    while(m_PhantomTick + m_PhantomStep <= PredTick)
    {
        m_PhantomPrevCore = m_PhantomCore;
        while(!m_PendingInputs.empty() && m_PendingInputs.front().m_Tick <= m_PhantomTick + m_PhantomStep)
        {
            m_PhantomInput = m_PendingInputs.front().m_Input;
            m_PendingInputs.pop_front();
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
}


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
    m_File = nullptr;
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    m_LastRecordTick = -1;
    mem_zero(&m_LastRecordedInput, sizeof(m_LastRecordedInput));
    m_aFilename[0] = '\0';
    mem_zero(&m_CurrentPlaybackInput, sizeof(m_CurrentPlaybackInput));
    
    m_StopPending = false;
    m_StopTick = -1;

    m_PhantomActive = false;
    m_PhantomTick = 0;
    m_PhantomStep = 1;
    m_LastPredTick = 0;
    m_PhantomFreezeTime = 0;
    m_PhantomHistory.clear();
    m_PhantomInputs.clear();
    
    m_OldShowOthers = g_Config.m_ClShowOthersAlpha;
}

void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.tas", ms_pFujixDir, pMap);
}

void CFujixTas::RecordEntry(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || !m_File)
        return;
    
    if(mem_comp(pInput, &m_LastRecordedInput, sizeof(*pInput)) == 0)
        return;

    STasEntry Entry = {Tick - m_StartTick, *pInput};
    io_write(m_File, &Entry, sizeof(Entry));
    m_vEntries.push_back(Entry);
    m_LastRecordedInput = *pInput;
}

void CFujixTas::UpdatePlaybackInput()
{
    if(!m_Playing)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);

    while(m_PlayIndex < (int)m_vEntries.size() &&
          m_PlayStartTick + m_vEntries[m_PlayIndex].m_Tick <= PredTick)
    {
        m_CurrentPlaybackInput = m_vEntries[m_PlayIndex].m_Input;
        m_PlayIndex++;
    }

    if(m_PlayIndex >= (int)m_vEntries.size()) 
    {
        const STasEntry& LastEntry = m_vEntries.back();
        if (PredTick >= m_PlayStartTick + LastEntry.m_Tick)
        {
             StopPlay();
             return;
        }
    }
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    if(!m_Playing)
        return false;

    UpdatePlaybackInput();
    *pInput = m_CurrentPlaybackInput;

    GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy] = m_CurrentPlaybackInput;
    GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy] = m_CurrentPlaybackInput;

    return true;
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
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "Failed to open file for recording.");
        return;
    }

    m_Recording = true;
    g_Config.m_ClFujixTasRecord = 1;
    m_vEntries.clear();
    
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
    mem_zero(&m_LastRecordedInput, sizeof(m_LastRecordedInput));
    
    m_PhantomInputs.clear();
    m_PhantomHistory.clear();

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
    m_PhantomActive = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_CollisionDisabled = true;
    m_PhantomFreezeTime = 0;
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, m_LastRecordedInput, m_PhantomFreezeTime});

    m_OldShowOthers = g_Config.m_ClShowOthersAlpha;
    if(!g_Config.m_ClFujixTasShowPlayers)
        g_Config.m_ClShowOthersAlpha = 0;
}

void CFujixTas::FinishRecord()
{
    if(!m_Recording)
        return;

    if(m_File)
    {
        io_close(m_File);
        m_File = nullptr;
    }

    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_PhantomActive = false;
    m_PhantomInputs.clear();
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
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "Failed to open file for playback.");
        return;
    }

    m_vEntries.clear();
    STasEntry Entry;
    while(io_read(File, &Entry, sizeof(Entry)) == sizeof(Entry))
        m_vEntries.push_back(Entry);
    io_close(File);
    
    if(m_vEntries.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "TAS file is empty.");
        return;
    }

    m_PlayIndex = 0;
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Playing = true;
    g_Config.m_ClFujixTasPlay = 1;
    mem_zero(&m_CurrentPlaybackInput, sizeof(m_CurrentPlaybackInput));
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    g_Config.m_ClFujixTasPlay = 0;
    m_vEntries.clear();
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    mem_zero(&m_CurrentPlaybackInput, sizeof(m_CurrentPlaybackInput));
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
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "Failed to open file for testing.");
        return;
    }

    m_vEntries.clear();
    STasEntry Entry;
    while(io_read(File, &Entry, sizeof(Entry)) == sizeof(Entry))
        m_vEntries.push_back(Entry);
    io_close(File);
    
    if(m_vEntries.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix_tas", "TAS file is empty for testing.");
        return;
    }
    
    m_Testing = true;
    g_Config.m_ClFujixTasTest = 1;
    
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
    m_PhantomActive = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_CollisionDisabled = true;
    m_PhantomHistory.clear();
    m_PhantomInputs.clear();
    m_PhantomFreezeTime = 0;
    
    CNetObj_PlayerInput StartInput;
    mem_zero(&StartInput, sizeof(StartInput));
    m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, StartInput, m_PhantomFreezeTime});
    
    int TestStartTick = m_PhantomTick + 1;
    for(const auto &entry : m_vEntries)
        m_PhantomInputs.push_back({TestStartTick + entry.m_Tick, entry.m_Input});
}

void CFujixTas::StopTest()
{
    m_Testing = false;
    g_Config.m_ClFujixTasTest = 0;
    m_PhantomActive = false;
    m_vEntries.clear();
    m_PhantomInputs.clear();
    m_PhantomHistory.clear();
}

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || Tick == m_LastRecordTick)
        return;

    m_LastRecordTick = Tick;
    
    RecordEntry(pInput, Tick);

    m_PhantomInputs.push_back({Tick, *pInput});
    TickPhantomUpTo(Tick);
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
    CFujixTas *pSelf = static__cast<CFujixTas *>(pUserData);
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
    if(MapIndex < 0)
        return false;

    int Tile = Collision()->GetTileIndex(MapIndex);
    int FTile = Collision()->GetFrontTileIndex(MapIndex);
    int SwitchType = Collision()->GetSwitchType(MapIndex);

    int Tele = Collision()->IsTeleport(MapIndex);
    if(Tele && !Collision()->TeleOuts(Tele - 1).empty())
    {
        m_PhantomCore.m_Pos = Collision()->TeleOuts(Tele - 1)[0];
    }

    int EvilTele = Collision()->IsEvilTeleport(MapIndex);
    if(EvilTele && !Collision()->TeleOuts(EvilTele - 1).empty())
    {
        m_PhantomCore.m_Pos = Collision()->TeleOuts(EvilTele - 1)[0];
    }

    if(Tile == TILE_FREEZE || FTile == TILE_FREEZE || SwitchType == TILE_FREEZE)
    {
        PhantomFreeze(Collision()->GetSwitchDelay(MapIndex));
    }
    else if(Tile == TILE_UNFREEZE || FTile == TILE_UNFREEZE || SwitchType == TILE_DUNFREEZE)
    {
        PhantomUnfreeze();
    }
    return false;
}

void CFujixTas::TickPhantomUpTo(int TargetTick)
{
    if(!m_PhantomActive)
        return;

    CNetObj_PlayerInput PhantomInput = m_PhantomHistory.back().m_Input;

    while(m_PhantomTick + m_PhantomStep <= TargetTick)
    {
        m_PhantomPrevCore = m_PhantomCore;
        
        while(!m_PhantomInputs.empty() && m_PhantomInputs.front().m_Tick <= m_PhantomTick + m_PhantomStep)
        {
            PhantomInput = m_PhantomInputs.front().m_Input;
            m_PhantomInputs.pop_front();
        }
        
        if(m_PhantomFreezeTime > 0)
        {
            PhantomInput.m_Direction = 0;
            PhantomInput.m_Jump = 0;
            PhantomInput.m_Hook = 0;
            m_PhantomFreezeTime--;
            if(m_PhantomFreezeTime == 0 && !m_PhantomCore.m_DeepFrozen)
                m_PhantomCore.m_FreezeEnd = 0;
        }

        m_PhantomCore.m_Input = PhantomInput;
        m_PhantomCore.Tick(true);
        m_PhantomCore.Move();
        
        int MapIndex = Collision()->GetMapIndex(m_PhantomCore.m_Pos);
        HandlePhantomTiles(MapIndex);
        
        m_PhantomCore.Quantize();
        
        m_PhantomTick += m_PhantomStep;

        m_PhantomHistory.push_back({m_PhantomTick, m_PhantomCore, m_PhantomPrevCore, PhantomInput, m_PhantomFreezeTime});
        if(m_PhantomHistory.size() > (size_t)(Client()->GameTickSpeed() * 5)) // Keep 5 seconds of history for rendering
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
    pChar->m_AttackTick = Core.m_AttackTick;
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
    
    MaybeFinishRecord();
    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(m_PhantomActive)
    {
        CNetObj_Character Prev, Curr;
        
        float IntraTick = Client()->PredIntraGameTick(g_Config.m_ClDummy);
        float RenderTick = m_PhantomTick + IntraTick;
        
        SPhantomState PrevState = m_PhantomHistory.front();
        for(const auto &s : m_PhantomHistory)
        {
            if(s.m_Tick <= RenderTick)
                PrevState = s;
            else
                break;
        }

        CoreToCharacter(PrevState.m_Core, &Prev, PrevState.m_Tick);
        CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

        m_PhantomRenderInfo.m_ColorBody.a = g_Config.m_ClFujixTasPhantomAlpha / 100.0f;
        m_PhantomRenderInfo.m_ColorFeet.a = g_Config.m_ClFujixTasPhantomAlpha / 100.0f;

        GameClient()->m_Players.RenderHook(&Prev, &Curr, &m_PhantomRenderInfo, -2);
        GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &m_PhantomRenderInfo, -2);
        
        RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    }
}

void CFujixTas::RenderFuturePath(int TicksAhead)
{
    if(TicksAhead <= 0 || !m_PhantomActive)
        return;

    CFujixTas TempTas = *this;
    TempTas.m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());

    std::vector<vec2> Points;
    Points.reserve(TicksAhead + 1);
    Points.push_back(TempTas.m_PhantomCore.m_Pos);

    int TargetTick = TempTas.m_PhantomTick + TicksAhead;
    
    while(TempTas.m_PhantomTick < TargetTick)
    {
        int StepTarget = minimum(TargetTick, TempTas.m_PhantomTick + TempTas.m_PhantomStep);
        TempTas.TickPhantomUpTo(StepTarget);
        Points.push_back(TempTas.m_PhantomCore.m_Pos);
    }

    if (Points.size() > 1)
    {
        Graphics()->TextureClear();
        Graphics()->LinesBegin();
        Graphics()->SetColor(0.0f, 1.0f, 0.0f, 0.5f);

        for(size_t i = 1; i < Points.size(); ++i)
        {
            IGraphics::CLineItem Line(Points[i-1].x, Points[i-1].y,
                                      Points[i].x, Points[i].y);
            Graphics()->LinesDraw(&Line, 1);
        }

        Graphics()->LinesEnd();
        Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
}
    pChar->m_Tick = Tick;
    pChar->m_AttackTick = Core.m_AttackTick;
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
    
    MaybeFinishRecord();
    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(m_PhantomActive)
    {
        CNetObj_Character Prev, Curr;
        // Find the correct history state to interpolate from
        SPhantomState PrevState = m_PhantomHistory.front();
        for(const auto &s : m_PhantomHistory)
        {
            if(s.m_Tick <= Client()->PredIntraGameTick(g_Config.m_ClDummy))
                PrevState = s;
            else
                break;
        }

        CoreToCharacter(PrevState.m_Core, &Prev, PrevState.m_Tick);
        CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

        // Adjust alpha for phantom
        m_PhantomRenderInfo.m_ColorBody.a = g_Config.m_ClFujixTasPhantomAlpha / 100.0f;
        m_PhantomRenderInfo.m_ColorFeet.a = g_Config.m_ClFujixTasPhantomAlpha / 100.0f;

        GameClient()->m_Players.RenderHook(&Prev, &Curr, &m_PhantomRenderInfo, -2);
        GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &m_PhantomRenderInfo, -2);
        
        RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    }
}

void CFujixTas::RenderFuturePath(int TicksAhead)
{
    if(TicksAhead <= 0 || !m_PhantomActive)
        return;

    // Create a temporary copy for simulation
    CFujixTas TempTas = *this;
    
    // We can't just copy the core, we need to re-initialize its world
    TempTas.m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());


    std::vector<vec2> Points;
    Points.reserve(TicksAhead + 1);
    Points.push_back(TempTas.m_PhantomCore.m_Pos);

    int TargetTick = TempTas.m_PhantomTick + TicksAhead * TempTas.m_PhantomStep;
    
    TempTas.TickPhantomUpTo(TargetTick);
    
    // The previous implementation simulated here, but it's better to just
    // draw the history of the main phantom if it's already computed
    // For now, let's just draw a line from current to the end of its already-processed input queue
    
    if (m_PhantomHistory.size() > 1)
    {
        Graphics()->TextureClear();
        Graphics()->LinesBegin();
        Graphics()->SetColor(0.0f, 1.0f, 0.0f, 0.5f);

        for(size_t i = 1; i < m_PhantomHistory.size(); ++i)
        {
            IGraphics::CLineItem Line(m_PhantomHistory[i-1].m_Core.m_Pos.x, m_PhantomHistory[i-1].m_Core.m_Pos.y,
                                      m_PhantomHistory[i].m_Core.m_Pos.x, m_PhantomHistory[i].m_Core.m_Pos.y);
            Graphics()->LinesDraw(&Line, 1);
        }

        Graphics()->LinesEnd();
        Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

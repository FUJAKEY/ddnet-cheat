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
#include <game/client/prediction/entities/character.h>
#include <base/system.h>
#include <cstdlib>
#include <ctime>
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
    m_aFilename[0] = '\0';
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_StopPending = false;
    m_StopTick = -1;
    m_PhantomActive = false;
    m_PhantomTick = 0;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomPlayIndex = 0;
    m_HookFile = nullptr;
    m_HookPlayIndex = 0;

    m_RageActive = false;
    m_LastHookState = HOOK_RETRACTED;
    m_LastHookedPlayer = -1;
    m_RageActive = false;
    m_RageTarget = vec2(0.f, 0.f);
    m_RagePrevEnabled = false;
}

int CFujixTas::Sizeof() const
{
    return sizeof(*this);
}

void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjx", ms_pFujixDir, pMap);
}

void CFujixTas::GetHookPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.hook", ms_pFujixDir, pMap);
}

void CFujixTas::UpdatePlaybackInput()
{
    if(!m_Playing && !m_Testing)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    int BaseTick = m_Playing ? m_PlayStartTick : m_TestStartTick;
    int *pPlayIndex = m_Playing ? &m_PlayIndex : &m_PhantomPlayIndex;

    while(*pPlayIndex < (int)m_vEntries.size() && BaseTick + m_vEntries[*pPlayIndex].m_Tick <= PredTick)
    {
        if (m_Playing)
            m_CurrentInput = m_vEntries[*pPlayIndex].m_Input;
        else // m_Testing
            m_PhantomInput = m_vEntries[*pPlayIndex].m_Input;

        (*pPlayIndex)++;
    }

    if (m_Playing)
    {
        if(m_PlayIndex >= (int)m_vEntries.size() && (m_vEntries.empty() || PredTick >= BaseTick + m_vEntries.back().m_Tick))
        {
            StopPlay();
        }
        ApplyHookEvents(PredTick, false);
    }
    else // m_Testing
    {
        if(m_PhantomPlayIndex >= (int)m_vEntries.size() && (m_vEntries.empty() || PredTick >= BaseTick + m_vEntries.back().m_Tick))
        {
            StopTest();
        }
        ApplyHookEvents(PredTick, true);
    }
}

void CFujixTas::RecordHookState(int Tick)
{
    if(!m_Recording)
        return;

    const CCharacterCore &Core = GameClient()->m_PredictedChar;
    if(Core.m_HookState != m_LastHookState || Core.HookedPlayer() != m_LastHookedPlayer)
    {
        SHookEvent Ev;
        Ev.m_Tick = Tick - m_StartTick;
        Ev.m_State = Core.m_HookState;
        Ev.m_HookedPlayer = Core.HookedPlayer();
        Ev.m_HookX = round_to_int(Core.m_HookPos.x);
        Ev.m_HookY = round_to_int(Core.m_HookPos.y);
        Ev.m_HookTick = Core.m_HookTick;
        m_vHookEvents.push_back(Ev);
        if(m_HookFile)
            io_write(m_HookFile, &Ev, sizeof(Ev));
        m_LastHookState = Core.m_HookState;
        m_LastHookedPlayer = Core.HookedPlayer();
    }
}

void CFujixTas::ApplyHookEvents(int PredTick, bool ToPhantom)
{
    int BaseTick = m_Playing ? m_PlayStartTick : m_TestStartTick;
    while(m_HookPlayIndex < (int)m_vHookEvents.size() && BaseTick + m_vHookEvents[m_HookPlayIndex].m_Tick <= PredTick)
    {
        const SHookEvent &Ev = m_vHookEvents[m_HookPlayIndex];
        CCharacterCore *pCore = ToPhantom ? &m_PhantomCore : &GameClient()->m_PredictedChar;
        pCore->m_HookState = Ev.m_State;
        pCore->m_HookTick = Ev.m_HookTick;
        pCore->m_HookPos = vec2(Ev.m_HookX, Ev.m_HookY);
        pCore->SetHookedPlayer(Ev.m_HookedPlayer);
        m_HookPlayIndex++;
    }
}

void CFujixTas::ApplyRageInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeRage || !GameClient()->m_Snap.m_pLocalCharacter || !m_RageActive)
        return;

    vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
    vec2 Diff = m_RageTarget - Pos;

    if(length(Diff) < 2.0f)
    {
        pInput->m_Direction = 0;
        pInput->m_Hook = 0;
        pInput->m_Jump = 0;
        m_RageActive = false;
        return;
    }

    if(Diff.x > 2.0f)
        pInput->m_Direction = 1;
    else if(Diff.x < -2.0f)
        pInput->m_Direction = -1;
    else
        pInput->m_Direction = 0;

    if(Diff.y < -32.0f)
        pInput->m_Jump = 1;

    if(length(Diff) > 96.0f)
    {
        pInput->m_Hook = 1;
        pInput->m_TargetX = (int)(Diff.x * 256.0f);
        pInput->m_TargetY = (int)(Diff.y * 256.0f);
    }
    else
    {
        pInput->m_Hook = 0;
    }
}

void CFujixTas::UpdateRageTarget()
{
    if(g_Config.m_ClFujixBlockFreezeRage != m_RagePrevEnabled)
    {
        m_RagePrevEnabled = g_Config.m_ClFujixBlockFreezeRage;
        if(!m_RagePrevEnabled)
            m_RageActive = false;
        else
        {
            m_RageTarget = vec2(Ui()->MouseWorldX(), Ui()->MouseWorldY());
            m_RageActive = true;
        }
    }

    if(!g_Config.m_ClFujixBlockFreezeRage)
        return;

    if(Input()->KeyPress(KEY_MOUSE_1))
    {
        m_RageTarget = vec2(Ui()->MouseWorldX(), Ui()->MouseWorldY());
        m_RageActive = true;
    }
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
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

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || Tick < m_StartTick)
        return;

    if(Tick == m_LastRecordTick)
        return;

    if(mem_comp(pInput, &m_LastInput, sizeof(*pInput)) != 0)
    {
        SEntry e = {Tick - m_StartTick, *pInput};
        if(m_File)
            io_write(m_File, &e, sizeof(e));
        m_vEntries.push_back(e);
        m_LastInput = *pInput;
    }
    m_LastRecordTick = Tick;

    if((m_Testing || m_Recording) && m_PhantomActive)
    {
        m_PhantomInput = *pInput;
        m_PhantomPlayIndex = (int)m_vEntries.size();
    }
}


void CFujixTas::StartRecord()
{
    if(m_Recording)
        return;

    GetPath(m_aFilename, sizeof(m_aFilename));
    GetHookPath(m_aHookFilename, sizeof(m_aHookFilename));
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        return;
    }
    m_HookFile = Storage()->OpenFile(m_aHookFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);

    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_Recording = true;
    g_Config.m_ClFujixTasRecord = 1;
    m_vEntries.clear();
    m_vHookEvents.clear();
    m_HookPlayIndex = 0;
    m_LastHookState = GameClient()->m_PredictedChar.m_HookState;
    m_LastHookedPlayer = GameClient()->m_PredictedChar.HookedPlayer();

    m_RageActive = false;

    // initialize phantom to visualize recording
    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomStep = 1;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomPlayIndex = 0;
    // ignore other players but keep map collisions
    m_PhantomCore.m_CollisionDisabled = false;
    m_PhantomCore.m_Solo = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_HammerHitDisabled = true;
    m_PhantomCore.m_GrenadeHitDisabled = true;
    m_PhantomCore.m_ShotgunHitDisabled = true;
    m_PhantomCore.m_LaserHitDisabled = true;
    m_TestStartTick = m_PhantomTick;
    m_PhantomActive = true;
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
    if(m_HookFile)
    {
        io_close(m_HookFile);
        m_HookFile = nullptr;
    }

    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_LastRecordTick = -1;
    m_StopPending = false;
    m_StopTick = -1;

    m_PhantomActive = false;
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

void CFujixTas::BlockFreezeInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeLegit || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    auto PredictFreeze = [&](const CNetObj_PlayerInput &Input, int HookMode) {
        CCharacterCore Core = GameClient()->m_PredictedChar;
        Core.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        const int Steps = 12;
        for(int i = 0; i < Steps; i++)
        {
            CNetObj_PlayerInput Step = Input;
            if(HookMode == 0)
                Step.m_Hook = 0;
            else if(HookMode == 1)
                Step.m_Hook = 1;
            else if(HookMode == 2)
                Step.m_Hook = i == 0 ? 1 : 0;
            Core.m_Input = Step;
            Core.Tick(true);
            Core.Move();
            Core.Quantize();
            int Index = Collision()->GetPureMapIndex(Core.m_Pos.x, Core.m_Pos.y);
            int Tile = Collision()->GetTileIndex(Index);
            int Front = Collision()->GetFrontTileIndex(Index);
            bool Freeze = Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE ||
                          Front == TILE_FREEZE || Front == TILE_DFREEZE || Front == TILE_LFREEZE;
            if(Freeze)
                return i + 1;
        }
        return 0;
    };

    int FreezeCurrent = PredictFreeze(*pInput, -1);
    if(!FreezeCurrent)
        return;

    CNetObj_PlayerInput Adjusted = *pInput;

    int FreezeNoHook = PredictFreeze(Adjusted, 0);
    int FreezeFullHook = PredictFreeze(Adjusted, 1);
    int FreezeShortHook = PredictFreeze(Adjusted, 2);

    if(GameClient()->m_PredictedChar.m_Vel.y < 0 && FreezeFullHook &&
       (!FreezeNoHook || FreezeFullHook <= FreezeNoHook))
    {
        if(!FreezeShortHook || FreezeShortHook >= FreezeFullHook)
            Adjusted.m_Hook = 0;
    }

    if(FreezeFullHook && (!FreezeNoHook || FreezeFullHook < FreezeNoHook))
    {
        if(!(FreezeShortHook && (!FreezeNoHook || FreezeShortHook < FreezeNoHook)))
            Adjusted.m_Hook = 0;
    }
    else if(FreezeNoHook && !FreezeFullHook)
    {
        Adjusted.m_Hook = 1;
        if(GameClient()->m_PredictedChar.m_Vel.y < 0)
            Adjusted.m_Jump = 1;
    }

    CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
    bool OnGround = pLocalChar && pLocalChar->IsGrounded();

    if(!OnGround)
    {
        float VelX = GameClient()->m_PredictedChar.m_Vel.x;
        if(VelX > 0.5f)
            Adjusted.m_Direction = -1;
        else if(VelX < -0.5f)
            Adjusted.m_Direction = 1;
        else
            Adjusted.m_Direction = 0;

        if(FreezeCurrent <= 3)
        {
            if(VelX > 0.1f)
                Adjusted.m_Direction = -1;
            else if(VelX < -0.1f)
                Adjusted.m_Direction = 1;
        }
    }
    else
        Adjusted.m_Direction = 0;

    *pInput = Adjusted;
}

void CFujixTas::UpdateFreezeInput(CNetObj_PlayerInput *pInput)
{
    BlockFreezeInput(pInput);
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

    GetHookPath(aPath, sizeof(aPath));
    IOHANDLE HookFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    m_vHookEvents.clear();
    if(HookFile)
    {
        SHookEvent Ev;
        while(io_read(HookFile, &Ev, sizeof(Ev)) == sizeof(Ev))
            m_vHookEvents.push_back(Ev);
        io_close(HookFile);
    }
    m_HookPlayIndex = 0;

    if(m_vEntries.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "tas file is empty");
        return;
	}

    m_PlayIndex = 0;
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Playing = true;
    g_Config.m_ClFujixTasPlay = 1;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    g_Config.m_ClFujixTasPlay = 0;
    m_vEntries.clear();
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    m_vHookEvents.clear();
    m_HookPlayIndex = 0;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_RageActive = false;
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

    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    if(m_vEntries.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "tas file is empty");
        return;
	}

    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomStep = 1;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomPlayIndex = 0;

    // ignore other players while keeping map collisions
    m_PhantomCore.m_CollisionDisabled = false;
    m_PhantomCore.m_Solo = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_HammerHitDisabled = true;
    m_PhantomCore.m_GrenadeHitDisabled = true;
    m_PhantomCore.m_ShotgunHitDisabled = true;
    m_PhantomCore.m_LaserHitDisabled = true;

    m_TestStartTick = m_PhantomTick;
    m_Testing = true;
    m_PhantomActive = true;
    g_Config.m_ClFujixTasTest = 1;
}

void CFujixTas::StopTest()
{
    m_Testing = false;
    g_Config.m_ClFujixTasTest = 0;
    m_PhantomActive = false;
    m_vEntries.clear();
    m_vHookEvents.clear();
    m_HookPlayIndex = 0;
    m_RageActive = false;
}

void CFujixTas::TickPhantomUpTo(int TargetTick)
{
    if(!m_PhantomActive)
        return;

    while(m_PhantomTick < TargetTick)
    {
        // keep world pointers fresh in case prediction updated
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomPrevCore = m_PhantomCore;

        if(m_Testing || m_Playing)
            UpdatePlaybackInput();
        if(m_Testing || m_Playing)
            ApplyHookEvents(m_PhantomTick, true);

        m_PhantomCore.m_Input = m_PhantomInput;
        m_PhantomCore.Tick(true);
        m_PhantomCore.Move();
        m_PhantomCore.Quantize();

        m_PhantomTick++;
    }
}

void CFujixTas::TickPhantom()
{
    if(!m_PhantomActive)
        return;
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

    MaybeFinishRecord();
    UpdateRageTarget();
    RecordHookState(Client()->PredGameTick(g_Config.m_ClDummy));
    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(m_PhantomActive)
    {
        CNetObj_Character Prev, Curr;
        CoreToCharacter(m_PhantomPrevCore, &Prev, m_PhantomTick - 1);
        CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

        CTeeRenderInfo PhantomRenderInfo = m_PhantomRenderInfo;
		PhantomRenderInfo.m_ColorBody = ColorRGBA(0.7f, 0.7f, 1.0f, 0.6f);
		PhantomRenderInfo.m_ColorFeet = ColorRGBA(0.7f, 0.7f, 1.0f, 0.6f);

        GameClient()->m_Players.RenderHook(&Prev, &Curr, &PhantomRenderInfo, -2);
        GameClient()->m_Players.RenderHookCollLine(&Prev, &Curr, -2);
        GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &PhantomRenderInfo, -2);

        RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    }

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
        Tmp.TickPhantomUpTo(Tmp.m_PhantomTick + 1);
        Points.push_back(Tmp.m_PhantomCore.m_Pos);
    }

    if(Points.size() <= 1)
        return;

    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    Graphics()->SetColor(0.2f, 1.0f, 0.2f, 0.5f);
    for(size_t i = 1; i < Points.size(); i++)
    {
        IGraphics::CLineItem Line(Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y);
        Graphics()->LinesDraw(&Line, 1);
    }
    Graphics()->LinesEnd();
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
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
    StopPlay();
    if(m_Recording)
        FinishRecord();
    StopTest();
    m_RageActive = false;

}

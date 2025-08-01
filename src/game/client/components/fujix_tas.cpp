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
#include <array>
#include <vector>
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

    m_LastHookState = HOOK_RETRACTED;
    m_LastHookedPlayer = -1;
    m_RageHookTicks = 0;
    m_RageMoveDir = 0;
    m_RageMoveTicks = 0;

    m_RageHysteresisTicks = 0;
    m_RageSafeGraceTicks = 0;
    m_RageSoftReleaseTicks = 0;
    m_LastFreezeDetectedAt = 0;
    m_LastSafeTick = 0;
    m_LastInterventionTick = 0;
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
        else
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
    else
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


bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    if(!m_Playing)
        return false;

    UpdatePlaybackInput();
    *pInput = m_CurrentInput;
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

bool CFujixTas::IsFreezeIndex(int Idx) const
{
    int Tile = Collision()->GetTileIndex(Idx);
    int Front = Collision()->GetFrontTileIndex(Idx);
    return Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE ||
           Front == TILE_FREEZE || Front == TILE_DFREEZE || Front == TILE_LFREEZE;
}

bool CFujixTas::NearFreezePos(vec2 Pos, float Margin) const
{
    const float Half = CCharacterCore::PhysicalSize() / 2.f;
    // sample center + edges + corners
    constexpr int N = 9;
    const vec2 Offs[N] = {
        vec2(0,0),
        vec2( Half+Margin, 0),
        vec2(-Half-Margin, 0),
        vec2(0,  Half+Margin),
        vec2(0, -Half-Margin),
        vec2( Half+Margin,  Half+Margin),
        vec2(-Half-Margin,  Half+Margin),
        vec2( Half+Margin, -Half-Margin),
        vec2(-Half-Margin, -Half-Margin),
    };
    for(int i=0;i<N;i++)
    {
        int Idx = Collision()->GetPureMapIndex(Pos.x + Offs[i].x, Pos.y + Offs[i].y);
        if(IsFreezeIndex(Idx))
            return true;
    }
    return false;
}

bool CFujixTas::PathNearFreeze(vec2 From, vec2 To, float Step, float Margin, bool CapsuleSides) const
{
    float Dist = distance(From, To);
    int StepsLine = maximum(1, (int)ceilf(Dist / Step));
    // sample center path
    for(int i = 0; i <= StepsLine; i++)
    {
        float a = i / (float)StepsLine;
        vec2 Pos = mix(From, To, a);
        if(NearFreezePos(Pos, Margin))
            return true;
    }
    if(!CapsuleSides)
        return false;
    // sample side-offset paths to catch skinny slits
    vec2 Dir = normalize(To - From);
    vec2 Nrm = vec2(-Dir.y, Dir.x);
    const float side = RAGE_CAPSULE_SIDE_OFFSET;
    for(int s = -1; s <= 1; s += 2)
    {
        vec2 Off = (float)s * side * Nrm;
        for(int i = 0; i <= StepsLine; i++)
        {
            float a = i / (float)StepsLine;
            vec2 Pos = mix(From + Off, To + Off, a);
            if(NearFreezePos(Pos, Margin))
                return true;
        }
    }
    return false;
}

int CFujixTas::PredictFreezeGeneric(const CNetObj_PlayerInput &Base, int Steps, float Margin, bool CapsuleSides, int HookMode) const
{
    CCharacterCore Core = GameClient()->m_PredictedChar;
    Core.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
    CNetObj_PlayerInput In = Base;
    for(int i = 0; i < Steps; i++)
    {
        if(HookMode == 0) In.m_Hook = 0;
        else if(HookMode == 1) In.m_Hook = 1;
        else if(HookMode == 2) In.m_Hook = (i == 0) ? 1 : 0;

        Core.m_Input = In;
        vec2 Prev = Core.m_Pos;
        Core.Tick(true);
        Core.Move();
        Core.Quantize();
        // check path between Prev and Core.m_Pos for freeze
        if(PathNearFreeze(Prev, Core.m_Pos, RAGE_PATH_STEP, Margin, CapsuleSides))
            return i + 1;
    }
    return 0;
}

void CFujixTas::BlockFreezeInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeLegit || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    auto PredictFreeze = [&](const CNetObj_PlayerInput &Input, int HookMode)
    {
        // a bit stronger margin and capsule check for legit too
        return PredictFreezeGeneric(Input, 40, RAGE_NEAR_MARGIN, true, HookMode);
    };

    int FreezeCurrent = PredictFreeze(*pInput, -1);
    if(!FreezeCurrent)
        return;

    CNetObj_PlayerInput Adjusted = *pInput;
    int FreezeNoHook = PredictFreeze(Adjusted, 0);
    int FreezeFullHook = PredictFreeze(Adjusted, 1);
    int FreezeShortHook = PredictFreeze(Adjusted, 2);

    // prefer less intrusive changes
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
    // soft ground/air direction handling
    CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
    bool OnGround = pLocalChar && pLocalChar->IsGrounded();

    float VelX = GameClient()->m_PredictedChar.m_Vel.x;
    if(!OnGround)
    {
        // only slight counter-steer and with speed check
        if(fabsf(VelX) > 0.2f)
        {
            if(VelX > 0.0f) Adjusted.m_Direction = -1;
            else Adjusted.m_Direction = 1;
        }
    }
    else
    {
        // do not lock completely; allow small user drift
        if(fabsf(VelX) > 0.4f)
        {
            if(VelX > 0.0f) Adjusted.m_Direction = -1;
            else Adjusted.m_Direction = 1;
        }
        else
            Adjusted.m_Direction = 0;
    }

    // Safe release: if predicted distances improved, release earlier
    int After = PredictFreeze(Adjusted, -1);
    if(After && After > FreezeCurrent + 3)
    {
        // still danger but further, keep minor changes only
    }
    else if(!After)
    {
        // safe - do minimal intervention
    }

    *pInput = Adjusted;
}

void CFujixTas::BlockFreezeRageInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeRage || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    int NowTick = Client()->PredGameTick(g_Config.m_ClDummy);

    // handle ongoing enforced hold
    if(m_RageHookTicks > 0)
    {
        pInput->m_Hook = 1;
        pInput->m_Direction = m_RageMoveDir;
        m_RageHookTicks--;
        m_LastInterventionTick = NowTick;
        if(GameClient()->m_PredictedChar.m_HookState != HOOK_FLYING)
            m_RageHookTicks = 0;
        if(m_RageHookTicks == 0 && m_RageMoveTicks == 0)
            m_RageMoveDir = 0;
        return;
    }
    if(m_RageMoveTicks > 0)
    {
        pInput->m_Direction = m_RageMoveDir;
        m_RageMoveTicks--;
        m_LastInterventionTick = NowTick;
        if(m_RageMoveTicks == 0)
            m_RageMoveDir = 0;
    }

    CNetObj_PlayerInput Base = *pInput;

    // dynamic steps: if speed larger, predict further
    float Speed = length(GameClient()->m_PredictedChar.m_Vel);
    int Steps = RAGE_PREDICT_STEPS + (int)clamp((Speed - 5.0f) * 2.0f, 0.0f, 30.0f);

    auto PredictFreezeKeep = [&](const CNetObj_PlayerInput &In)
    {
        return PredictFreezeGeneric(In, Steps, RAGE_NEAR_MARGIN, true, -1);
    };

    int FreezeCurrent = PredictFreezeKeep(Base);

    if(!FreezeCurrent)
    {
        // safe area: decay all states softly
        if(m_RageHysteresisTicks > 0) m_RageHysteresisTicks--;
        if(m_RageSafeGraceTicks > 0) m_RageSafeGraceTicks--;
        if(m_RageSoftReleaseTicks > 0) m_RageSoftReleaseTicks--;
        m_LastFreezeDetectedAt = 0;
        m_LastSafeTick = NowTick;

        // short micro-window to not grab control aggressively
        if(NowTick - m_LastInterventionTick > RAGE_INTERVENTION_SOFT_LIMIT/2)
        {
            m_RageMoveDir = 0;
            m_RageMoveTicks = 0;
        }
        return;
    }

    m_LastFreezeDetectedAt = FreezeCurrent;

    // target directions
    std::vector<vec2> vDirs;
    vDirs.reserve(RAGE_DIR_TOTAL + 6);
    for(int i = 0; i < RAGE_DIR_TOTAL; i++)
    {
        float a = 2.f * pi * i / RAGE_DIR_TOTAL;
        vDirs.push_back(vec2(cosf(a), sinf(a)));
    }
    int WantedDir = clamp(Base.m_Direction, -1, 1);
    if(WantedDir)
    {
        vDirs.push_back(normalize(vec2(WantedDir * 0.25f, -1.f)));
        vDirs.push_back(normalize(vec2(WantedDir * 0.5f, -1.f)));
        vDirs.push_back(normalize(vec2(WantedDir * 0.75f, -1.f)));
    }

    const float HookLen = GameClient()->GetTuning(g_Config.m_ClDummy)->m_HookLength;
    const float AimLen = HookLen;
    float HookSpeed = GameClient()->GetTuning(g_Config.m_ClDummy)->m_HookFireSpeed;

    auto PredictFreezeSeq = [&](const vec2 &Dir, int Move, int Hold) {
        CCharacterCore Core = GameClient()->m_PredictedChar;
        Core.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        int StepLimit = Hold + RAGE_EXTRA_AFTER_HOLD + RAGE_RELEASE_SAFE;
        if(StepLimit < Steps)
            StepLimit = Steps;
        for(int i = 0; i < StepLimit; i++)
        {
            CNetObj_PlayerInput Step = Base;
            Step.m_Direction = Move;
            if(i == 0)
            {
                Step.m_TargetX = (int)(Dir.x * AimLen);
                Step.m_TargetY = (int)(Dir.y * AimLen);
            }
            Step.m_Hook = i < Hold ? 1 : 0;
            vec2 Prev = Core.m_Pos;
            Core.m_Input = Step;
            Core.Tick(true);
            Core.Move();
            Core.Quantize();
            if(PathNearFreeze(Prev, Core.m_Pos, RAGE_PATH_STEP, RAGE_NEAR_MARGIN, true))
                return i + 1;
        }
        return 0;
    };

    CNetObj_PlayerInput Best = Base;
    int BestFreeze = FreezeCurrent;
    int BestDir = 0;
    int BestHold = 0;

    // Evaluate hooking options
    for(const vec2 &DirRaw : vDirs)
    {
        vec2 Dir = DirRaw;
        if(Dir.y == 0.f && Dir.x != 0.f) Dir.y = -0.25f;
        Dir = normalize(Dir);
        vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
        vec2 To = Pos + Dir * HookLen;
        vec2 Col;
        int Hit = Collision()->IntersectLineTeleHook(Pos, To, &Col, nullptr);
        int ColIndex = Collision()->GetPureMapIndex(Col.x, Col.y);
        bool ColFreeze = IsFreezeIndex(ColIndex);

        if(Hit && Hit != TILE_NOHOOK && !ColFreeze && !PathNearFreeze(Pos, Col, RAGE_PATH_STEP, RAGE_NEAR_MARGIN, true))
        {
            int aMove[3];
            if(WantedDir)
            {
                aMove[0] = WantedDir;
                aMove[1] = 0;
                aMove[2] = -WantedDir;
            }
            else
            {
                aMove[0] = 0;
                aMove[1] = 1;
                aMove[2] = -1;
            }

            for(int Move : aMove)
            {
                float Dist = distance(Pos, Col);
                int Hold = (DirRaw.y > 0.f) ? RAGE_HOOK_DOWN_HOLD : (int)ceilf(Dist / HookSpeed) + 1;
                Hold = clamp(Hold, RAGE_HOOK_HOLD_MIN, RAGE_HOOK_HOLD_MAX);

                int Freeze = PredictFreezeSeq(Dir, Move, Hold);
                if(Freeze && Freeze <= Hold && Hold > 1)
                {
                    Hold = Freeze - 1;
                    Hold = maximum(Hold, 1);
                    Freeze = PredictFreezeSeq(Dir, Move, Hold);
                }
                // skip if release zone still dangerous
                if(Freeze && Freeze <= Hold) continue;
                if(Freeze && Freeze <= Hold + RAGE_RELEASE_SAFE) continue;

                // choose less intrusive if equal
                bool Better = false;
                if(!Freeze)
                {
                    if(BestFreeze != Steps) Better = true;
                }
                else
                {
                    if(!BestFreeze || Freeze > BestFreeze) Better = true;
                }

                if(Better)
                {
                    Best = Base;
                    Best.m_Hook = 1;
                    Best.m_TargetX = (int)(Dir.x * AimLen);
                    Best.m_TargetY = (int)(Dir.y * AimLen);
                    Best.m_Direction = Move;
                    BestFreeze = Freeze ? Freeze : Steps;
                    BestDir = Move;
                    BestHold = Hold;
                    if(!Freeze)
                        break;
                }
            }
            if(BestFreeze == Steps)
                break;
        }
    }

    // Try non-hook alternatives: small steering only (to avoid "not letting move")
    if(BestFreeze <= FreezeCurrent)
    {
        // just try tiny steering left/right without hook to delay freeze
        for(int Move : { WantedDir, 0, -WantedDir })
        {
            if(Move == 0 && WantedDir == 0) continue;
            CNetObj_PlayerInput Try = Base;
            Try.m_Direction = clamp(Move, -1, 1);
            int F = PredictFreezeKeep(Try);
            if(!F || F > BestFreeze)
            {
                Best = Try;
                BestFreeze = F ? F : Steps;
                BestDir = Try.m_Direction;
                BestHold = 0;
                if(!F) break;
            }
        }
    }

    if(BestFreeze > FreezeCurrent)
    {
        // adopt Best softly
        *pInput = Best;
        // clamp direction change by current velocity to not be too strong
        float vx = GameClient()->m_PredictedChar.m_Vel.x;
        int Dir = BestDir;
        if(Dir != 0 && fabsf(vx) > RAGE_MAX_REVERSAL_SPEED)
        {
            // if moving fast right and Best wants left, reduce to 0; similar reverse
            if((vx > 0 && Dir < 0) || (vx < 0 && Dir > 0))
                Dir = 0;
        }
        pInput->m_Direction = Dir;
        m_RageMoveDir = Dir;

        // hold hook+move but with soft limit
        m_RageHookTicks = BestHold;
        m_RageMoveTicks = BestHold + RAGE_MOVE_EXTRA;
        m_RageHysteresisTicks = RAGE_HYSTERESIS_TICKS;
        m_RageSafeGraceTicks = RAGE_SAFE_GRACE_TICKS;
        m_RageSoftReleaseTicks = RAGE_SOFT_RELEASE_TICKS;
        m_LastInterventionTick = NowTick;
    }
    else
    {
        // If not improved, but danger is close, do minimal: release hook if on and steer slightly
        if(pInput->m_Hook)
        {
            CNetObj_PlayerInput Tmp = Base;
            Tmp.m_Hook = 0;
            int F = PredictFreezeKeep(Tmp);
            if(F >= FreezeCurrent) // not worse
                pInput->m_Hook = 0;
        }
        // micro steer
        if(WantedDir != 0)
        {
            CNetObj_PlayerInput Tmp = Base;
            Tmp.m_Direction = -WantedDir;
            int F = PredictFreezeKeep(Tmp);
            if(F > FreezeCurrent) pInput->m_Direction = -WantedDir;
        }
    }

    // Early safe release if after all changes it’s safe
    int After = PredictFreezeKeep(*pInput);
    if(!After && m_RageSoftReleaseTicks > 0)
    {
        m_RageHookTicks = 0;
        m_RageMoveTicks = 0;
        m_RageMoveDir = 0;
        m_RageSoftReleaseTicks--;
    }
}

void CFujixTas::UpdateFreezeInput(CNetObj_PlayerInput *pInput)
{
    if(g_Config.m_ClFujixBlockFreezeRage)
        BlockFreezeRageInput(pInput);
    else
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
}

void CFujixTas::TickPhantomUpTo(int TargetTick)
{
    if(!m_PhantomActive)
        return;

    while(m_PhantomTick < TargetTick)
    {
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
}

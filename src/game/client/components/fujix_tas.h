#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <game/client/component.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <game/generated/protocol.h>
#include <game/gamecore.h>
#include <game/client/render.h>
#include <vector>
#include <deque>

class CFujixTas : public CComponent
{
public:
       static const char *ms_pFujixDir;

private:
struct SEntry
{
       int m_Tick;
       CNetObj_PlayerInput m_Input;
};

struct SHookEvent
{
       int m_Tick;
       int m_State;
       int m_HookedPlayer;
       int m_HookX;
       int m_HookY;
       int m_HookTick;
};

bool m_Recording;
bool m_Playing;
bool m_Testing;
int m_StartTick;
int m_TestStartTick;
int m_PlayStartTick;
char m_aFilename[IO_MAX_PATH_LENGTH];
IOHANDLE m_File;
std::vector<SEntry> m_vEntries;
int m_PlayIndex;
int m_LastRecordTick;
CNetObj_PlayerInput m_LastInput;
CNetObj_PlayerInput m_CurrentInput;
bool m_StopPending;
int m_StopTick;

 char m_aHookFilename[IO_MAX_PATH_LENGTH];
 IOHANDLE m_HookFile;
 std::vector<SHookEvent> m_vHookEvents;
int m_HookPlayIndex;
int m_LastHookState;
int m_LastHookedPlayer;


// Phantom
bool m_PhantomActive;
int m_PhantomTick;
CCharacterCore m_PhantomCore;
CCharacterCore m_PhantomPrevCore;
CTeeRenderInfo m_PhantomRenderInfo;
int m_PhantomStep;
CNetObj_PlayerInput m_PhantomInput;
int m_PhantomPlayIndex;

// Rage control state
int m_RageHookTicks;
int m_RageMoveDir;
int m_RageMoveTicks;

// New safety/hysteresis state
int m_RageHysteresisTicks;
int m_RageSafeGraceTicks;
int m_RageSoftReleaseTicks;
int m_LastFreezeDetectedAt; // 0 means no current danger, else ticks to freeze predicted
int m_LastSafeTick;
int m_LastInterventionTick;

// Tunables
static constexpr int RAGE_HOOK_HOLD_MIN = 6;
static constexpr int RAGE_HOOK_HOLD_MAX = 24;
static constexpr int RAGE_MOVE_EXTRA = 5;
static constexpr int RAGE_PREDICT_STEPS = 80;
static constexpr int RAGE_HOOK_DOWN_HOLD = 3;
static constexpr float RAGE_NEAR_MARGIN = 7.0f;
static constexpr int RAGE_EXTRA_AFTER_HOLD = 12;
static constexpr int RAGE_RELEASE_SAFE = 8;
static constexpr int RAGE_DIR_TOTAL = 32;
static constexpr float RAGE_PATH_STEP = 3.0f;
static constexpr int RAGE_HYSTERESIS_TICKS = 5;
static constexpr int RAGE_SAFE_GRACE_TICKS = 6;
static constexpr int RAGE_SOFT_RELEASE_TICKS = 8;
static constexpr int RAGE_INTERVENTION_SOFT_LIMIT = 22; // max hard enforcement window
static constexpr float RAGE_MAX_REVERSAL_SPEED = 0.9f; // if |vx| < this, allow reversal, else clamp softer
static constexpr float RAGE_CAPSULE_SIDE_OFFSET = 2.0f; // for path sampling with lateral offsets

void GetPath(char *pBuf, int Size) const;
void GetHookPath(char *pBuf, int Size) const;
void UpdatePlaybackInput();
void TickPhantom();
void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
void FinishRecord();
void RenderFuturePath(int TicksAhead);
void TickPhantomUpTo(int TargetTick);
void RecordHookState(int Tick);
void ApplyHookEvents(int PredTick, bool ToPhantom);

// helpers shared by both blockers
bool IsFreezeIndex(int Idx) const;
bool NearFreezePos(vec2 Pos, float Margin) const;
bool PathNearFreeze(vec2 From, vec2 To, float Step, float Margin, bool CapsuleSides) const;
int PredictFreezeGeneric(const CNetObj_PlayerInput &Base, int Steps, float Margin, bool CapsuleSides, int HookMode /*-1 keep, 0 off, 1 on, 2 short*/) const;

public:

static void ConRecord(IConsole::IResult *pResult, void *pUserData);
static void ConPlay(IConsole::IResult *pResult, void *pUserData);
static void ConTest(IConsole::IResult *pResult, void *pUserData);

CFujixTas();
       virtual int Sizeof() const override;

virtual void OnConsoleInit() override;
virtual void OnMapLoad() override;
virtual void OnUpdate() override;
virtual void OnRender() override;

void StartRecord();
void StopRecord();
void StartPlay();
void StopPlay();
void StartTest();
void StopTest();
bool IsRecording() const { return m_Recording; }
bool IsPlaying() const { return m_Playing; }
bool IsTesting() const { return m_Testing; }
bool IsPhantomActive() const { return m_PhantomActive; }
vec2 PhantomPos() const { return m_PhantomCore.m_Pos; }
bool FetchPlaybackInput(CNetObj_PlayerInput *pInput);
void RecordInput(const CNetObj_PlayerInput *pInput, int Tick);
void MaybeFinishRecord();
void BlockFreezeInput(CNetObj_PlayerInput *pInput);
void BlockFreezeRageInput(CNetObj_PlayerInput *pInput);
void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#ifndef GAME_CLIENT_FUJIX_GEROS_BOT_H
#define GAME_CLIENT_FUJIX_GEROS_BOT_H

#include <base/vmath.h>
#include <game/client/component.h>

// Forward declarations
class CCharacterCore;

struct SGerosBotPrediction
{
    vec2 m_Pos;
    vec2 m_Vel;
    bool m_InDanger;
    float m_DangerLevel;
    int m_TicksUntilDeath;
    bool m_CanUseHook;
    bool m_ShouldJump;
    vec2 m_HookTarget;
    vec2 m_DesiredDir;
};

class CFujixGerosBot : public CComponent
{
private:
    // Core prediction and simulation
    void PredictMovement(SGerosBotPrediction *pPredictions, int NumTicks);
    void SimulateCharacterCore(CCharacterCore *pCore, int Ticks);
    bool IsPositionDangerous(vec2 Pos, vec2 Vel);
    float CalculateDangerLevel(vec2 Pos, vec2 Vel);

    // Rescue algorithms
    vec2 FindBestHookTarget(vec2 Pos, vec2 Vel);
    vec2 CalculateEscapeDirection(vec2 Pos, vec2 Vel, float DangerLevel);
    bool CanReachSafetyWithHook(vec2 From, vec2 HookTarget);
    bool ShouldUseJump(vec2 Pos, vec2 Vel, vec2 DesiredDir);

    // Anti-suicide detection
    bool DetectSuicideAttempt(vec2 Pos, vec2 Vel, int InputDirection);
    bool IsPlayerTryingToKillThemselves();

    // State tracking
    SGerosBotPrediction m_aPredictions[16];
    int m_LastPredictionTick;
    int m_RescueAttempts;
    int m_LastRescueTick;
    float m_PlayerTrustLevel;
    bool m_EmergencyMode;
    bool m_FullPredictionMode;
    int m_LastFullPredictionTick;
    
    // 🕷️ WALL/CEILING RIDING STATE
    bool m_IsWallRiding;
    bool m_IsCeilingRiding;
    int m_RidingStartTick;
    int m_LastHookReleaseTick;
    vec2 m_CurrentRidingTarget;
    int m_RidingSide; // -1 = left, 1 = right, 0 = none
public:
    CFujixGerosBot();

    // Base component overrides
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnRender() override;
    virtual void OnMessage(int MsgType, void *pRawMsg) override;

    // Main bot functions
    void Update();
    bool IsActive() const;
    bool ShouldOverrideInput();
    void GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
    
    // 🧪 ТЕСТОВЫЕ ФУНКЦИИ
    void ForceEmergencyMode() { m_EmergencyMode = true; }
    bool IsInEmergencyMode() const { return m_EmergencyMode; }
    
    // 🧠 ЖЕСТКАЯ ЛОГИКА КРЮКА - НОВЫЕ ФУНКЦИИ
    bool IsFreezeInDirection(vec2 Pos, vec2 Dir, int TileDistance);
    vec2 FindCeilingRidingTarget(vec2 Pos, vec2 Vel);
    vec2 FindWallRidingTarget(vec2 Pos, vec2 Vel);
    vec2 FindHookableInDirection(vec2 Pos, vec2 Direction, float MaxRange);
    float CalculateHookScore(vec2 Pos, vec2 Target, vec2 Vel);
    bool CanDoCeilingRiding(vec2 Pos, vec2 CeilingTarget);
    bool CanDoWallRiding(vec2 Pos, vec2 WallTarget, int Side);
    bool IsHookTrajectorysSafe(vec2 From, vec2 To);
    bool IsGoodForRiding(vec2 Pos, vec2 Target);
    
    // 🕷️ WALL/CEILING RIDING FUNCTIONS
    void StartRiding(vec2 Target, int CurrentTick);
    void ExecuteRidingLogic(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX, int CurrentTick);
    bool ShouldHookForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease);
    bool ShouldHookForCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease);
    bool ShouldJumpForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    int CalculateCeilingRidingDirection(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    bool ShouldStopWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    bool ShouldStopCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    void StopRiding();

    // Emergency rescue system
    void ExecuteEmergencyRescue();
    bool IsInEmergencyState();

    // Configuration
    int GetAggressiveness() const;
    int GetPredictionTicks() const;
    bool IsAntiSuicideEnabled() const;
};

#endif // GAME_CLIENT_FUJIX_GEROS_BOT_H
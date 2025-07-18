#ifndef GAME_CLIENT_FUJIX_GEROS_BOT_H
#define GAME_CLIENT_FUJIX_GEROS_BOT_H

#include <base/vmath.h>
#include <game/client/component.h>

// Forward declarations
class CCharacterCore;

// 🚀 РЕВОЛЮЦИОННЫЕ СТРУКТУРЫ ДАННЫХ

// Расширенное предсказание с AI анализом
struct SAdvancedPrediction
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
    
    // 🧠 AI АНАЛИЗ
    float m_DeathProbability;          // Вероятность смерти (0.0 - 1.0)
    int m_SafetyScore;                 // Оценка безопасности (-100 до 100)
    bool m_IsFloorTrap;                // Ловушка пола
    bool m_IsCeilingTrap;              // Ловушка потолка
    bool m_IsWallTrap;                 // Ловушка стены
    vec2 m_EscapeVector;               // Вектор экстренного побега
    bool m_RequiresFullOverride;       // Требует полного отключения игрока
};

// Анализ безопасности движения
struct SMovementSafety
{
    bool m_IsInputSafe[3];            // Left/None/Right безопасность
    bool m_IsJumpSafe;                // Безопасность прыжка
    bool m_IsHookSafe;                // Безопасность крюка
    vec2 m_SafestDirection;           // Самое безопасное направление
    int m_DangerousInputs;            // Количество опасных вводов
    bool m_ShouldBlockAllInput;       // Заблокировать весь ввод
};

// Глубокий анализ крюка
struct SHookAnalysis
{
    vec2 m_BestTarget;                // Лучшая цель
    float m_SuccessRate;              // Процент успеха (0.0 - 1.0)
    bool m_WillHitFloor;              // Попадет ли в пол
    bool m_WillHitWall;               // Попадет ли в стену
    bool m_WillHitCeiling;            // Попадет ли в потолок
    vec2 m_TrajectoryPoints[32];      // Точки траектории
    int m_TrajectoryLength;           // Длина траектории
    bool m_IsEmergencyOnly;           // Только для экстренных случаев
};

class CFujixGerosBot : public CComponent
{
private:
    // 🚀 РЕВОЛЮЦИОННАЯ СИСТЕМА ПРЕДСКАЗАНИЯ
    SAdvancedPrediction m_aDeepPredictions[64];  // 64 тика глубокого анализа
    SMovementSafety m_CurrentSafety;
    SHookAnalysis m_HookAnalysis;
    
    // 🧠 AI СИСТЕМА ПРИНЯТИЯ РЕШЕНИЙ
    float m_aDeathMatrix[5][5];       // Матрица смерти для каждого ввода
    bool m_aInputBlocked[5];          // Заблокированные вводы [Left, Right, Jump, Hook, Fire]
    int m_OverrideLevel;              // Уровень переопределения (0-5)
    bool m_FullAutonomyMode;          // Полная автономия
    
    // 🛡️ ПРЕВЕНТИВНАЯ ЗАЩИТА
    int m_DangerDetectionRange;       // Дальность обнаружения опасности
    int m_PreventiveActivationTicks;  // Тики до превентивной активации
    bool m_BlockPlayerInput;          // Блокировка ввода игрока
    int m_LastBlockTick;              // Последний заблокированный тик
    
    // 🕷️ СУПЕР RIDING СИСТЕМА
    bool m_IsAdvancedRiding;          // Продвинутый riding
    int m_RidingStrategy;             // Стратегия riding (0-5)
    vec2 m_SecondaryTarget;           // Запасная цель
    bool m_HasEscapePlan;             // Есть план побега
    
    // 📊 СТАТИСТИКА И ОБУЧЕНИЕ
    int m_SuccessfulRescues;          // Успешные спасения
    int m_FailedAttempts;             // Неудачные попытки
    float m_AdaptiveThreshold;        // Адаптивный порог
    
    // 🚀 РЕВОЛЮЦИОННЫЕ МЕТОДЫ АНАЛИЗА
    void DeepPredict(SAdvancedPrediction *pPredictions, int NumTicks);
    void AnalyzeDangerMatrix();
    bool WillInputCauseDeath(int InputType, int Direction);
    void CalculateMovementSafety();
    void AnalyzeHookSafety();
    bool IsFloorHookDangerous(vec2 HookTarget);
    
    // 🧠 AI ПРИНЯТИЕ РЕШЕНИЙ
    void UpdateDeathMatrix();
    bool ShouldBlockInput(int InputType);
    void DetermineOverrideLevel();
    void ExecuteAutonomousControl(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
    
    // 🛡️ ПРЕВЕНТИВНАЯ СИСТЕМА
    void PreventiveDangerScan();
    bool DetectIncomingDanger(int TicksAhead);
    void BlockDangerousInputs();
    void ForceEmergencyEscape();
    
    // 🕷️ СУПЕР КРЮК СИСТЕМА
    vec2 FindUltraSafeHookTarget(vec2 Pos, vec2 Vel);
    bool IsHookTargetFloor(vec2 Target);
    float CalculateUltraSafeHookScore(vec2 Pos, vec2 Target, vec2 Vel);
    bool IsHookTrajectorysSafe(vec2 From, vec2 To);
    vec2 FindHookableInDirection(vec2 Pos, vec2 Direction, float MaxRange);
    float CalculateHookScore(vec2 Pos, vec2 Target, vec2 Vel);
    bool CanDoCeilingRiding(vec2 Pos, vec2 Target);
    bool CanDoWallRiding(vec2 Pos, vec2 Target, int Side);
    // 🎯 ПРОДВИНУТЫЕ АЛГОРИТМЫ
    vec2 FindEscapeRoute(vec2 Pos, vec2 Vel, int MaxTicks);
    bool CanSurviveMovement(vec2 StartPos, vec2 Movement, int Ticks);
    void OptimizeRescueStrategy();
    
    // Старые методы (улучшенные)
    void PredictMovement(SAdvancedPrediction *pPredictions, int NumTicks);
    void SimulateCharacterCore(CCharacterCore *pCore, int Ticks);
    bool IsPositionDangerous(vec2 Pos, vec2 Vel);
    float CalculateDangerLevel(vec2 Pos, vec2 Vel);
    
    // Улучшенные алгоритмы спасения
    vec2 FindBestHookTarget(vec2 Pos, vec2 Vel);
    vec2 CalculateEscapeDirection(vec2 Pos, vec2 Vel, float DangerLevel);
    bool CanReachSafetyWithHook(vec2 From, vec2 HookTarget);
    bool ShouldUseJump(vec2 Pos, vec2 Vel, vec2 DesiredDir);
    
    // Anti-suicide system
    bool DetectSuicideAttempt(vec2 Pos, vec2 Vel, int InputDirection);
    bool IsPlayerTryingToKillThemselves();
    
    // Состояние
    int m_LastPredictionTick;
    int m_RescueAttempts;
    int m_LastRescueTick;
    float m_PlayerTrustLevel;
    bool m_EmergencyMode;
    
    // 🕷️ WALL/CEILING RIDING SYSTEM
    bool m_IsWallRiding;
    bool m_IsCeilingRiding;
    int m_RidingSide;
    vec2 m_CurrentRidingTarget;
    int m_RidingStartTick;
    int m_LastHookReleaseTick;
    
    // Riding methods
    void StartRiding(vec2 Target, int CurrentTick);
    void ExecuteRidingLogic(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX, int CurrentTick);
    bool ShouldHookForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease);
    bool ShouldHookForCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease);
    bool ShouldJumpForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    int CalculateCeilingRidingDirection(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    bool ShouldStopWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    bool ShouldStopCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration);
    void StopRiding();
    
    // Analysis methods
    bool AnalyzeHookTrajectory(vec2 From, vec2 To);
    void AvoidFloorHooks();
    
public:
    CFujixGerosBot();

    // Base component overrides
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnRender() override;
    virtual void OnMessage(int MsgType, void *pRawMsg) override;

    // 🚀 РЕВОЛЮЦИОННЫЙ ИНТЕРФЕЙС
    void Update();
    bool IsActive() const;
    bool ShouldOverrideInput();
    void GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
    
    // 🛡️ СИСТЕМА БЛОКИРОВКИ ВВОДА
    bool ShouldBlockMovement(int Direction);
    bool ShouldBlockJump();
    bool ShouldBlockHook(vec2 TargetPos);
    void OverridePlayerInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
    
    // 🧠 AI ИНТЕРФЕЙС
    void ActivateFullAutonomy();
    void DeactivateFullAutonomy();
    bool IsInFullAutonomy() const { return m_FullAutonomyMode; }
    float GetCurrentDangerLevel();
    bool IsSafeToMove(int Direction);
    
    // 📊 СТАТИСТИКА
    int GetSuccessfulRescues() const { return m_SuccessfulRescues; }
    int GetFailedAttempts() const { return m_FailedAttempts; }
    float GetSuccessRate() const;
    
    // 🧪 ОТЛАДКА И ТЕСТИРОВАНИЕ
    void ForceEmergencyMode() { m_EmergencyMode = true; }
    bool IsInEmergencyMode() const { return m_EmergencyMode; }
    void SetDangerDetectionRange(int Range) { m_DangerDetectionRange = Range; }
    
    // Улучшенные существующие функции
    bool IsFreezeInDirection(vec2 Pos, vec2 Dir, int TileDistance);
    vec2 FindCeilingRidingTarget(vec2 Pos, vec2 Vel);
    vec2 FindWallRidingTarget(vec2 Pos, vec2 Vel);
    bool IsGoodForRiding(vec2 Pos, vec2 Target);
    
    // Emergency rescue system
    void ExecuteEmergencyRescue();
    bool IsInEmergencyState();

    // Configuration
    int GetAggressiveness() const;
    int GetPredictionTicks() const;
    bool IsAntiSuicideEnabled() const;
};

#endif // GAME_CLIENT_FUJIX_GEROS_BOT_H

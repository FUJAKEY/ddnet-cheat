#include "fujix_geros_bot.h"

#include <base/math.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/collision.h>
#include <engine/client.h>

// Game constants from game/collision.h 
#define TILE_DEATH 1
#define TILE_FREEZE 2  
#define TILE_DFREEZE 3
#define TILE_LFREEZE 4

// 🚀 РЕВОЛЮЦИОННЫЕ КОНСТАНТЫ
#define MAX_PREDICTION_TICKS 64
#define DANGER_DETECTION_RANGE 16
#define PREVENTIVE_ACTIVATION_TICKS 12
#define DEATH_PROBABILITY_THRESHOLD 0.15f
#define SAFETY_SCORE_THRESHOLD 20
#define FLOOR_HOOK_ANGLE_THRESHOLD 45.0f

CFujixGerosBot::CFujixGerosBot()
{
    // Основные переменные
    m_LastPredictionTick = 0;
    m_RescueAttempts = 0;
    m_LastRescueTick = 0;
    m_PlayerTrustLevel = 1.0f;
    m_EmergencyMode = false;
    
    // 🚀 РЕВОЛЮЦИОННАЯ ИНИЦИАЛИЗАЦИЯ
    m_OverrideLevel = 0;
    m_FullAutonomyMode = false;
    m_DangerDetectionRange = DANGER_DETECTION_RANGE;
    m_PreventiveActivationTicks = PREVENTIVE_ACTIVATION_TICKS;
    m_BlockPlayerInput = false;
    m_LastBlockTick = 0;
    
    // 🕷️ СУПЕР RIDING
    m_IsAdvancedRiding = false;
    m_RidingStrategy = 0;
    m_SecondaryTarget = vec2{0, 0};
    m_HasEscapePlan = false;
    
    // 📊 СТАТИСТИКА
    m_SuccessfulRescues = 0;
    m_FailedAttempts = 0;
    m_AdaptiveThreshold = DEATH_PROBABILITY_THRESHOLD;
    
    // Инициализация матрицы смерти
    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < 5; j++)
        {
            m_aDeathMatrix[i][j] = 0.0f;
        }
        m_aInputBlocked[i] = false;
    }
    
    // Инициализация предсказаний
    for(int i = 0; i < MAX_PREDICTION_TICKS; i++)
    {
        m_aDeepPredictions[i].m_Pos = vec2{0, 0};
        m_aDeepPredictions[i].m_Vel = vec2{0, 0};
        m_aDeepPredictions[i].m_InDanger = false;
        m_aDeepPredictions[i].m_DangerLevel = 0.0f;
        m_aDeepPredictions[i].m_TicksUntilDeath = -1;
        m_aDeepPredictions[i].m_CanUseHook = false;
        m_aDeepPredictions[i].m_ShouldJump = false;
        m_aDeepPredictions[i].m_HookTarget = vec2{0, 0};
        m_aDeepPredictions[i].m_DesiredDir = vec2{0, 0};
        
        // AI анализ
        m_aDeepPredictions[i].m_DeathProbability = 0.0f;
        m_aDeepPredictions[i].m_SafetyScore = 100;
        m_aDeepPredictions[i].m_IsFloorTrap = false;
        m_aDeepPredictions[i].m_IsCeilingTrap = false;
        m_aDeepPredictions[i].m_IsWallTrap = false;
        m_aDeepPredictions[i].m_EscapeVector = vec2{0, 0};
        m_aDeepPredictions[i].m_RequiresFullOverride = false;
    }
    
    // Инициализация анализа безопасности
    m_CurrentSafety.m_IsInputSafe[0] = true; // Left
    m_CurrentSafety.m_IsInputSafe[1] = true; // None
    m_CurrentSafety.m_IsInputSafe[2] = true; // Right
    m_CurrentSafety.m_IsJumpSafe = true;
    m_CurrentSafety.m_IsHookSafe = true;
    m_CurrentSafety.m_SafestDirection = vec2{0, 0};
    m_CurrentSafety.m_DangerousInputs = 0;
    m_CurrentSafety.m_ShouldBlockAllInput = false;
    
    // Инициализация анализа крюка
    m_HookAnalysis.m_BestTarget = vec2{0, 0};
    m_HookAnalysis.m_SuccessRate = 0.0f;
    m_HookAnalysis.m_WillHitFloor = false;
    m_HookAnalysis.m_WillHitWall = false;
    m_HookAnalysis.m_WillHitCeiling = false;
    m_HookAnalysis.m_TrajectoryLength = 0;
    m_HookAnalysis.m_IsEmergencyOnly = false;
}

void CFujixGerosBot::OnInit()
{
    // Инициализация системы GEROS BOT
    m_EmergencyMode = false;
    m_FullAutonomyMode = false;
    m_BlockPlayerInput = false;
    m_OverrideLevel = 0;
    
    // Сброс статистики
    m_SuccessfulRescues = 0;
    m_FailedAttempts = 0;
    m_PlayerTrustLevel = 1.0f;
}
void CFujixGerosBot::OnRender()
{
	if(!IsActive())
		return;
		
	// 🎯 ОТЛАДОЧНАЯ ВИЗУАЛИЗАЦИЯ В РЕАЛЬНОМ ВРЕМЕНИ
	CGameClient *pGameClient = GameClient();
	if(pGameClient && g_Config.m_Debug)
	{
		// Рендер предсказанных позиций и опасных зон
		for(int i = 0; i < GetPredictionTicks(); i++)
		{
            if(m_aDeepPredictions[i].m_InDanger)
            {
                // Красные точки = опасность
                Graphics()->TextureClear();
                Graphics()->QuadsBegin();
                Graphics()->SetColor(1.0f, 0.2f, 0.2f, 0.8f);
                IGraphics::CQuadItem QuadItem(m_aDeepPredictions[i].m_Pos.x - 8, m_aDeepPredictions[i].m_Pos.y - 8, 16, 16);
                Graphics()->QuadsDrawTL(&QuadItem, 1);
                Graphics()->QuadsEnd();
            }
        }
    }
}

void CFujixGerosBot::Update()
{
    if(!IsActive())
        return;
    
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return;
    
    // 🚀 РЕВОЛЮЦИОННАЯ СИСТЕМА ОБНОВЛЕНИЯ
    
    // 1. ГЛУБОКОЕ ПРЕДСКАЗАНИЕ (64 тика вперед)
    DeepPredict(m_aDeepPredictions, MAX_PREDICTION_TICKS);
    
    // 2. АНАЛИЗ МАТРИЦЫ СМЕРТИ
    AnalyzeDangerMatrix();
    
    // 3. РАСЧЕТ БЕЗОПАСНОСТИ ДВИЖЕНИЙ
    CalculateMovementSafety();
    
    // 4. АНАЛИЗ БЕЗОПАСНОСТИ КРЮКА
    AnalyzeHookSafety();
    
    // 5. ПРЕВЕНТИВНОЕ СКАНИРОВАНИЕ ОПАСНОСТИ
    PreventiveDangerScan();
    
    // 6. ОПРЕДЕЛЕНИЕ УРОВНЯ ПЕРЕОПРЕДЕЛЕНИЯ
    DetermineOverrideLevel();
    
    // 7. БЛОКИРОВКА ОПАСНЫХ ВВОДОВ
    BlockDangerousInputs();
    
    // 8. ПРОВЕРКА ЭКСТРЕННЫХ СИТУАЦИЙ
    if(IsInEmergencyState())
    {
        ExecuteEmergencyRescue();
        ActivateFullAutonomy(); // Полная автономия в экстренных случаях
    }
    
    // 9. АДАПТИВНОЕ ОБУЧЕНИЕ
    if(IsAntiSuicideEnabled() && WillInputCauseDeath(0, 0))
    {
        m_PlayerTrustLevel = maximum(0.05f, m_PlayerTrustLevel - 0.1f);
    }
    else
    {
        m_PlayerTrustLevel = minimum(1.0f, m_PlayerTrustLevel + 0.02f);
    }
    
    m_LastPredictionTick = pGameClient->Client()->GameTick(0);
}

// 🚀 РЕВОЛЮЦИОННОЕ ГЛУБОКОЕ ПРЕДСКАЗАНИЕ
void CFujixGerosBot::DeepPredict(SAdvancedPrediction *pPredictions, int NumTicks)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return;
    
    CCharacterCore Core = pGameClient->m_PredictedChar;
    CNetObj_PlayerInput *pCurrentInput = &pGameClient->m_Controls.m_aInputData[g_Config.m_ClDummy];
    
    for(int i = 0; i < NumTicks && i < MAX_PREDICTION_TICKS; i++)
    {
        // Симуляция с РЕАЛЬНЫМ вводом игрока
        Core.m_Input.m_Direction = pCurrentInput->m_Direction;
        Core.m_Input.m_Jump = pCurrentInput->m_Jump;
        Core.m_Input.m_Hook = pCurrentInput->m_Hook;
        Core.m_Input.m_TargetX = pCurrentInput->m_TargetX;
        Core.m_Input.m_TargetY = pCurrentInput->m_TargetY;
        
        // 🧠 AI МОДИФИКАЦИЯ ВВОДА для безопасности
        if(m_FullAutonomyMode)
        {
            // В режиме полной автономии - используем AI ввод
            vec2 AIDirection = FindEscapeRoute(Core.m_Pos, Core.m_Vel, i);
            Core.m_Input.m_Direction = (AIDirection.x > 0.1f) ? 1 : ((AIDirection.x < -0.1f) ? -1 : 0);
            Core.m_Input.m_Jump = (AIDirection.y < -0.5f) ? 1 : 0;
            
            vec2 SafeHookTarget = FindUltraSafeHookTarget(Core.m_Pos, Core.m_Vel);
            if(SafeHookTarget.x != 0 || SafeHookTarget.y != 0)
            {
                Core.m_Input.m_Hook = 1;
                Core.m_Input.m_TargetX = SafeHookTarget.x;
                Core.m_Input.m_TargetY = SafeHookTarget.y;
            }
            else
            {
                Core.m_Input.m_Hook = 0;
            }
        }
        
        // Симуляция одного тика
        SimulateCharacterCore(&Core, 1);
        
        // Заполнение базовых данных
        pPredictions[i].m_Pos = Core.m_Pos;
        pPredictions[i].m_Vel = Core.m_Vel;
        pPredictions[i].m_InDanger = IsPositionDangerous(Core.m_Pos, Core.m_Vel);
        pPredictions[i].m_DangerLevel = CalculateDangerLevel(Core.m_Pos, Core.m_Vel);
        
        // 🧠 РЕВОЛЮЦИОННЫЙ AI АНАЛИЗ
        
        // 1. Расчет вероятности смерти
        // 🧠 РЕВОЛЮЦИОННЫЙ AI АНАЛИЗ
        
        // 1. Расчет вероятности смерти
        float DeathProb = 0.0f;
        if(pPredictions[i].m_InDanger)
            DeathProb = minimum(1.0f, pPredictions[i].m_DangerLevel / 10.0f);
        pPredictions[i].m_DeathProbability = DeathProb;
        
        // 2. Оценка безопасности
        int SafetyScore = 100 - (int)(pPredictions[i].m_DangerLevel * 10.0f);
        pPredictions[i].m_SafetyScore = maximum(-100, SafetyScore);
        
        // 3. Анализ ловушек (упрощенная версия)
        pPredictions[i].m_IsFloorTrap = (Core.m_Vel.y > 8.0f && pPredictions[i].m_InDanger);
        pPredictions[i].m_IsCeilingTrap = (Core.m_Vel.y < -8.0f && pPredictions[i].m_InDanger);
        pPredictions[i].m_IsWallTrap = (abs(Core.m_Vel.x) > 8.0f && pPredictions[i].m_InDanger);
        
        // 4. Вектор экстренного побега (упрощенная версия)
        vec2 EscapeVec = vec2{0, -1}; // По умолчанию вверх
        if(pPredictions[i].m_InDanger)
            EscapeVec = CalculateEscapeDirection(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DangerLevel);
        pPredictions[i].m_EscapeVector = EscapeVec;
        if(pPredictions[i].m_InDanger)
        {
            pPredictions[i].m_TicksUntilDeath = i + 1;
        }
        else
        {
            pPredictions[i].m_TicksUntilDeath = -1;
        }
        
        // Расчет превентивных действий
        pPredictions[i].m_HookTarget = FindUltraSafeHookTarget(Core.m_Pos, Core.m_Vel);
        pPredictions[i].m_DesiredDir = pPredictions[i].m_EscapeVector;
        pPredictions[i].m_CanUseHook = (pPredictions[i].m_HookTarget.x != 0 || pPredictions[i].m_HookTarget.y != 0);
        pPredictions[i].m_ShouldJump = ShouldUseJump(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DesiredDir);
    }
    }

void CFujixGerosBot::OnMessage(int MsgType, void *pRawMsg)
{
    // Обработка сообщений для GEROS BOT
}

// 🚀 РЕВОЛЮЦИОННЫЕ МЕТОДЫ АНАЛИЗА ОПАСНОСТИ
void CFujixGerosBot::AnalyzeDangerMatrix()
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return;
    
    // Анализируем каждый тип ввода на опасность
    for(int inputType = 0; inputType < 5; inputType++) // Left, Right, Jump, Hook, Fire
    {
        for(int direction = -1; direction <= 1; direction++)
        {
            float dangerLevel = 0.0f;
            
            // Проверяем каждый тип ввода
            switch(inputType)
            {
                case 0: // Left movement
                    dangerLevel = WillInputCauseDeath(0, -1) ? 1.0f : 0.0f;
                    break;
                case 1: // Right movement  
                    dangerLevel = WillInputCauseDeath(0, 1) ? 1.0f : 0.0f;
                    break;
                case 2: // Jump
                    dangerLevel = WillInputCauseDeath(1, 0) ? 1.0f : 0.0f;
                    break;
                case 3: // Hook
                    dangerLevel = WillInputCauseDeath(2, 0) ? 1.0f : 0.0f;
                    break;
                case 4: // Fire (пока не используется)
                    dangerLevel = 0.0f;
                    break;
            }
            
            if(direction >= 0 && direction < 5)
                m_aDeathMatrix[inputType][direction] = dangerLevel;
        }
    }
}

bool CFujixGerosBot::WillInputCauseDeath(int InputType, int Direction)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return false;
    
    // Симулируем ввод на несколько тиков вперед
    CCharacterCore TestCore = pGameClient->m_PredictedChar;
    
    // Применяем тестовый ввод
    switch(InputType)
    {
        case 0: // Movement
            TestCore.m_Input.m_Direction = Direction;
            break;
        case 1: // Jump
            TestCore.m_Input.m_Jump = 1;
            break;
        case 2: // Hook
            TestCore.m_Input.m_Hook = 1;
            break;
    }
    
    // Симулируем на 10 тиков вперед
    // Симулируем на 10 тиков вперед
    for(int i = 0; i < 10; i++)
    {
        SimulateCharacterCore(&TestCore, 1);
        
        // Проверяем безопасность каждый тик
        if(IsPositionDangerous(TestCore.m_Pos, TestCore.m_Vel))
        {
            return true; // Опасное действие
        }
    }
    
    return false; // Безопасное действие
}

void CFujixGerosBot::CalculateMovementSafety()
{
    // Проверяем безопасность каждого типа движения
    m_CurrentSafety.m_IsInputSafe[0] = !WillInputCauseDeath(0, -1); // Left
    m_CurrentSafety.m_IsInputSafe[1] = !WillInputCauseDeath(0, 0);  // None
    m_CurrentSafety.m_IsInputSafe[2] = !WillInputCauseDeath(0, 1);  // Right
    m_CurrentSafety.m_IsJumpSafe = !WillInputCauseDeath(1, 0);
    m_CurrentSafety.m_IsHookSafe = !WillInputCauseDeath(2, 0);
    
    // Подсчет опасных вводов
    m_CurrentSafety.m_DangerousInputs = 0;
    for(int i = 0; i < 3; i++)
    {
        if(!m_CurrentSafety.m_IsInputSafe[i])
            m_CurrentSafety.m_DangerousInputs++;
    }
    if(!m_CurrentSafety.m_IsJumpSafe)
        m_CurrentSafety.m_DangerousInputs++;
    if(!m_CurrentSafety.m_IsHookSafe)
        m_CurrentSafety.m_DangerousInputs++;
    
    // Определяем самое безопасное направление
    if(m_CurrentSafety.m_IsInputSafe[1]) // None движение безопасно
        m_CurrentSafety.m_SafestDirection = vec2{0, 0};
    else if(m_CurrentSafety.m_IsInputSafe[0]) // Left безопасно
        m_CurrentSafety.m_SafestDirection = vec2{-1, 0};
    else if(m_CurrentSafety.m_IsInputSafe[2]) // Right безопасно
        m_CurrentSafety.m_SafestDirection = vec2{1, 0};
    else if(m_CurrentSafety.m_IsJumpSafe) // Прыжок безопасен
        m_CurrentSafety.m_SafestDirection = vec2{0, -1};
    else // Все опасно - нужна полная блокировка
        m_CurrentSafety.m_SafestDirection = vec2{0, 0};
    
    // Решение о полной блокировке ввода
    m_CurrentSafety.m_ShouldBlockAllInput = (m_CurrentSafety.m_DangerousInputs >= 4);
}

void CFujixGerosBot::AnalyzeHookSafety()
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return;
    
    // Находим лучшую цель для крюка
    vec2 PlayerPos = pGameClient->m_PredictedChar.m_Pos;
    vec2 PlayerVel = pGameClient->m_PredictedChar.m_Vel;
    
    m_HookAnalysis.m_BestTarget = FindUltraSafeHookTarget(PlayerPos, PlayerVel);
    
    // Анализ типа цели
    if(m_HookAnalysis.m_BestTarget.x != 0 || m_HookAnalysis.m_BestTarget.y != 0)
    {
        m_HookAnalysis.m_WillHitFloor = IsHookTargetFloor(m_HookAnalysis.m_BestTarget);
        m_HookAnalysis.m_WillHitWall = !m_HookAnalysis.m_WillHitFloor && abs(m_HookAnalysis.m_BestTarget.x - PlayerPos.x) > abs(m_HookAnalysis.m_BestTarget.y - PlayerPos.y);
        m_HookAnalysis.m_WillHitCeiling = !m_HookAnalysis.m_WillHitFloor && !m_HookAnalysis.m_WillHitWall;
        
        // Расчет процента успеха
        if(m_HookAnalysis.m_WillHitFloor)
            m_HookAnalysis.m_SuccessRate = 0.1f; // Крюк в пол очень опасен!
        else if(m_HookAnalysis.m_WillHitCeiling)
            m_HookAnalysis.m_SuccessRate = 0.9f; // Потолок безопасен
        else
            m_HookAnalysis.m_SuccessRate = 0.7f; // Стена средне безопасна
        
        m_HookAnalysis.m_IsEmergencyOnly = (m_HookAnalysis.m_SuccessRate < 0.5f);
    }
    else
    {
        // Нет доступных целей
        m_HookAnalysis.m_SuccessRate = 0.0f;
        m_HookAnalysis.m_WillHitFloor = false;
        m_HookAnalysis.m_WillHitWall = false;
        m_HookAnalysis.m_WillHitCeiling = false;
        m_HookAnalysis.m_IsEmergencyOnly = false;
    }
}

// 🛡️ СИСТЕМА ПРЕВЕНТИВНОЙ ЗАЩИТЫ И БЛОКИРОВКИ ВВОДА
void CFujixGerosBot::PreventiveDangerScan()
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return;
    
    // Сканируем опасность на m_DangerDetectionRange тиков вперед
    bool DangerDetected = false;
    
    for(int i = 1; i <= m_DangerDetectionRange; i++)
    {
        if(DetectIncomingDanger(i))
        {
            DangerDetected = true;
            
            // Если опасность близко - активируем превентивные меры
            if(i <= m_PreventiveActivationTicks)
            {
                m_EmergencyMode = true;
                if(i <= 5) // Очень близкая опасность
                    ActivateFullAutonomy();
            }
            break;
        }
    }
    
    // Адаптивная настройка дальности обнаружения
    if(!DangerDetected && m_DangerDetectionRange < 20)
        m_DangerDetectionRange++;
    else if(DangerDetected && m_DangerDetectionRange > 8)
        m_DangerDetectionRange--;
}

bool CFujixGerosBot::DetectIncomingDanger(int TicksAhead)
{
    if(TicksAhead < MAX_PREDICTION_TICKS)
    {
        return m_aDeepPredictions[TicksAhead].m_InDanger || 
               m_aDeepPredictions[TicksAhead].m_DeathProbability > m_AdaptiveThreshold;
    }
    return false;
}

void CFujixGerosBot::BlockDangerousInputs()
{
    // Блокируем конкретные типы ввода
    m_aInputBlocked[0] = !m_CurrentSafety.m_IsInputSafe[0]; // Left
    m_aInputBlocked[1] = !m_CurrentSafety.m_IsInputSafe[2]; // Right
    m_aInputBlocked[2] = !m_CurrentSafety.m_IsJumpSafe;     // Jump
    m_aInputBlocked[3] = !m_CurrentSafety.m_IsHookSafe;     // Hook
    m_aInputBlocked[4] = false; // Fire (пока не блокируем)
    
    // Общая блокировка при критической опасности
    if(m_CurrentSafety.m_ShouldBlockAllInput)
    {
        m_BlockPlayerInput = true;
        m_LastBlockTick = GameClient()->Client()->GameTick(0);
    }
    else
    {
        // Снимаем блокировку если прошло достаточно времени
        int CurrentTick = GameClient()->Client()->GameTick(0);
        if(CurrentTick - m_LastBlockTick > 30) // 0.5 секунды
            m_BlockPlayerInput = false;
    }
}

void CFujixGerosBot::DetermineOverrideLevel()
{
    // Определяем уровень переопределения (0-5)
    int NewLevel = 0;
    
    // Уровень 1: Легкая опасность
    if(m_aDeepPredictions[0].m_InDanger)
        NewLevel = 1;
    
    // Уровень 2: Средняя опасность
    if(m_aDeepPredictions[0].m_DangerLevel > 3.0f)
        NewLevel = 2;
    
    // Уровень 3: Высокая опасность
    if(m_aDeepPredictions[0].m_DeathProbability > 0.3f)
        NewLevel = 3;
    
    // Уровень 4: Критическая опасность
    if(m_aDeepPredictions[0].m_RequiresFullOverride)
        NewLevel = 4;
    
    // Уровень 5: Неминуемая смерть
    if(m_aDeepPredictions[0].m_DeathProbability > 0.8f)
        NewLevel = 5;
    
    m_OverrideLevel = NewLevel;
    
    // Автоматическая активация полной автономии на высоких уровнях
    if(m_OverrideLevel >= 4)
        ActivateFullAutonomy();
    else if(m_OverrideLevel <= 1)
        DeactivateFullAutonomy();
}

// 🕷️ РЕВОЛЮЦИОННАЯ СУПЕР-КРЮК СИСТЕМА (НЕ ЦЕПЛЯЕТСЯ ЗА ПОЛ!)
vec2 CFujixGerosBot::FindUltraSafeHookTarget(vec2 Pos, vec2 Vel)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient || !pGameClient->Collision())
        return vec2{0, 0};
    
    vec2 BestTarget = vec2{0, 0};
    float BestScore = -999.0f;
    float HookRange = 380.0f;
    
    // 🚨 ПРИОРИТЕТ 1: НИКОГДА НЕ ЦЕПЛЯЕМСЯ ЗА ПОЛ!
    // Проверяем только направления ВВЕРХ и В СТОРОНЫ
    
    for(int angle = -170; angle <= -10; angle += 8) // Только вверх и диагонали!
    {
        float rad = angle * pi / 180.0f;
        vec2 Direction = vec2{cos(rad), sin(rad)};
        
        for(float dist = 80.0f; dist <= HookRange; dist += 20.0f)
        {
            vec2 TestPos = Pos + Direction * dist;
            
            // КРИТИЧЕСКАЯ ПРОВЕРКА: это НЕ пол?
            if(IsHookTargetFloor(TestPos))
                continue; // ПРОПУСКАЕМ ПОЛЫ!
            
            if(pGameClient->Collision()->CheckPoint(TestPos.x, TestPos.y))
            {
                int Tile = pGameClient->Collision()->GetCollisionAt(TestPos.x, TestPos.y);
                if(Tile != TILE_FREEZE && Tile != TILE_DFREEZE && Tile != TILE_LFREEZE)
                {
                    float Score = CalculateUltraSafeHookScore(Pos, TestPos, Vel);
                    
                    // БОНУС за направления вверх
                    if(TestPos.y < Pos.y - 32.0f)
                        Score += 50.0f; // Огромный бонус за потолок!
                    
                    // ШТРАФ за близость к полу
                    if(abs(TestPos.y - Pos.y) < 64.0f)
                        Score -= 30.0f;
                    
                    if(Score > BestScore && Score > 10.0f) // Минимальный порог
                    {
                        BestScore = Score;
                        BestTarget = TestPos;
                    }
                }
            }
        }
    }
    
    return BestTarget;
}

bool CFujixGerosBot::IsHookTargetFloor(vec2 Target)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return false;
    
    vec2 PlayerPos = pGameClient->m_PredictedChar.m_Pos;
    
    // Проверяем угол к цели
    vec2 Direction = Target - PlayerPos;
    float angle = atan2(Direction.y, Direction.x) * 180.0f / pi;
    
    // Если угол направлен вниз или горизонтально - это может быть пол
    if(angle > -45.0f && angle < 45.0f) // Горизонтально
        return true;
    if(angle > 45.0f && angle < 135.0f) // Вниз
        return true;
    
    // Дополнительная проверка: есть ли пол прямо под целью?
    vec2 FloorCheck = Target + vec2{0, 32.0f};
    if(pGameClient->Collision()->CheckPoint(FloorCheck.x, FloorCheck.y))
        return true;
    
    return false; // Безопасная цель (потолок/стена)
}

float CFujixGerosBot::CalculateUltraSafeHookScore(vec2 Pos, vec2 Target, vec2 Vel)
{
    float Score = 0.0f;
    float Distance = distance(Pos, Target);
    
    // Базовая оценка по дистанции
    Score += (400.0f - Distance) / 10.0f;
    
    // ОГРОМНЫЙ БОНУС за цели выше игрока
    if(Target.y < Pos.y)
        Score += 100.0f * abs(Target.y - Pos.y) / 64.0f;
    
    // ШТРАФ за цели ниже игрока (потенциальный пол)
    if(Target.y > Pos.y)
        Score -= 200.0f * abs(Target.y - Pos.y) / 64.0f;
    
    // Анализ безопасности траектории
    if(IsHookTrajectorysSafe(Pos, Target))
        Score += 50.0f;
    else
        Score -= 100.0f;
    
    // Проверка что это хорошо для riding
    if(IsGoodForRiding(Pos, Target))
        Score += 75.0f;
    
    return Score;
}

// 🤖 СИСТЕМА ПОЛНОЙ АВТОНОМИИ И БЛОКИРОВКИ ИГРОКА
void CFujixGerosBot::ActivateFullAutonomy()
{
    m_FullAutonomyMode = true;
    m_BlockPlayerInput = true;
    m_EmergencyMode = true;
    m_SuccessfulRescues++; // Считаем попытку спасения
}

void CFujixGerosBot::DeactivateFullAutonomy()
{
    m_FullAutonomyMode = false;
    m_BlockPlayerInput = false;
    if(!IsInEmergencyState())
        m_EmergencyMode = false;
}

bool CFujixGerosBot::ShouldBlockMovement(int Direction)
{
    if(m_FullAutonomyMode)
        return true; // В автономном режиме блокируем ВСЕ
    
    if(Direction == -1)
        return m_aInputBlocked[0]; // Left
    else if(Direction == 1)
        return m_aInputBlocked[1]; // Right
    
    return false; // Нет движения - не блокируем
}

bool CFujixGerosBot::ShouldBlockJump()
{
    return m_FullAutonomyMode || m_aInputBlocked[2];
}

bool CFujixGerosBot::ShouldBlockHook(vec2 TargetPos)
{
    if(m_FullAutonomyMode)
        return true; // В автономии используем только AI крюк
    
    // КРИТИЧЕСКАЯ ПРОВЕРКА: блокируем крюки в пол!
    if(IsHookTargetFloor(TargetPos))
        return true; // НЕ ДАЕМ цепляться за пол!
    
    return m_aInputBlocked[3];
}

void CFujixGerosBot::ExecuteAutonomousControl(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX)
{
    if(!m_FullAutonomyMode)
        return;
    
    // 🤖 AI ПОЛНОСТЬЮ УПРАВЛЯЕТ ПЕРСОНАЖЕМ
    
    // 1. Определяем лучшее направление движения
    vec2 SafestDir = m_CurrentSafety.m_SafestDirection;
    
    if(SafestDir.x > 0.1f)
        *pInputDirection = 1; // Right
    else if(SafestDir.x < -0.1f)
        *pInputDirection = -1; // Left
    else
        *pInputDirection = 0; // Stop
    
    // 2. Прыжок только если это безопасно
    *pJump = (SafestDir.y < -0.5f && m_CurrentSafety.m_IsJumpSafe) ? 1 : 0;
    
    // 3. Крюк только в безопасные цели
    if(m_HookAnalysis.m_SuccessRate > 0.6f && !m_HookAnalysis.m_WillHitFloor)
    {
        *pHook = 1;
        *pTargetX = m_HookAnalysis.m_BestTarget;
    }
    else
    {
        *pHook = 0;
    }
}

void CFujixGerosBot::OverridePlayerInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX)
{
    // 🛡️ БЛОКИРОВКА ОПАСНЫХ ДЕЙСТВИЙ ИГРОКА
    
    // 1. Блокировка движения
    if(ShouldBlockMovement(*pInputDirection))
    {
        *pInputDirection = 0; // Останавливаем опасное движение!
    }
    
    // 2. Блокировка прыжка
    if(ShouldBlockJump())
    {
        *pJump = 0; // Блокируем опасный прыжок!
    }
    
    // 3. Блокировка крюка в пол
    if(*pHook && ShouldBlockHook(*pTargetX))
    {
        *pHook = 0; // НЕ ДАЕМ цепляться за пол!
        
        // Предлагаем безопасную альтернативу
        vec2 SafeTarget = m_HookAnalysis.m_BestTarget;
        if(SafeTarget.x != 0 || SafeTarget.y != 0)
        {
            *pHook = 1;
            *pTargetX = SafeTarget;
        }
    }
}

// 🎯 УЛУЧШЕННЫЙ ГЛАВНЫЙ МЕТОД GetBotInput
void CFujixGerosBot::GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX)
{
    if(!IsActive())
        return;
    
    // 🚀 РЕЖИМ ПОЛНОЙ АВТОНОМИИ
    if(m_FullAutonomyMode)
    {
        ExecuteAutonomousControl(pInputDirection, pJump, pHook, pTargetX);
        return; // AI полностью управляет!
    }
    
    // 🛡️ РЕЖИМ БЛОКИРОВКИ ОПАСНЫХ ДЕЙСТВИЙ
    if(m_BlockPlayerInput || m_OverrideLevel >= 2)
    {
        OverridePlayerInput(pInputDirection, pJump, pHook, pTargetX);
    }
    
    // 📊 СТАТИСТИКА
    if(*pInputDirection != 0 || *pJump || *pHook)
    {
        // Проверяем успешность наших решений
        if(!WillInputCauseDeath(0, *pInputDirection) && !WillInputCauseDeath(1, *pJump ? 1 : 0))
            m_SuccessfulRescues++;
        else
            m_FailedAttempts++;
    }
}

// 🎯 ДОПОЛНИТЕЛЬНЫЕ РЕВОЛЮЦИОННЫЕ ФУНКЦИИ

vec2 CFujixGerosBot::FindEscapeRoute(vec2 Pos, vec2 Vel, int MaxTicks)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return vec2{0, -1}; // По умолчанию вверх
    
    vec2 BestDirection = vec2{0, -1};
    float BestScore = -999.0f;
    
    // Проверяем все возможные направления
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            if(x == 0 && y == 0) continue;
            
            vec2 TestDirection = vec2{x, y};
            float Score = 0.0f;
            
            // Симулируем движение в этом направлении
            CCharacterCore TestCore = pGameClient->m_PredictedChar;
            TestCore.m_Input.m_Direction = x;
            if(y < 0) TestCore.m_Input.m_Jump = 1;
            
            bool IsSafe = true;
            for(int i = 0; i < MaxTicks; i++)
            {
                SimulateCharacterCore(&TestCore, 1);
                if(IsPositionDangerous(TestCore.m_Pos, TestCore.m_Vel))
                {
                    IsSafe = false;
                    break;
                }
                Score += 1.0f; // Бонус за каждый безопасный тик
            }
            
            if(IsSafe)
                Score += 50.0f; // Большой бонус за полную безопасность
            
            // Бонус за движение от опасности
            if(y < 0) Score += 30.0f; // Вверх
            if(abs(x) > 0) Score += 10.0f; // В стороны
            
            if(Score > BestScore)
            {
                BestScore = Score;
                BestDirection = TestDirection;
            }
        }
    }
    
    return normalize(BestDirection);
}

bool CFujixGerosBot::CanSurviveMovement(vec2 StartPos, vec2 Movement, int Ticks)
{
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return false;
    
    // Проверяем выживание при данном движении
    vec2 TestPos = StartPos;
    for(int i = 0; i < Ticks; i++)
    {
        TestPos += Movement;
        if(IsPositionDangerous(TestPos, Movement))
            return false;
    }
    
    return true;
}

float CFujixGerosBot::GetSuccessRate() const
{
    int Total = m_SuccessfulRescues + m_FailedAttempts;
    if(Total == 0) return 1.0f;
    return (float)m_SuccessfulRescues / (float)Total;
}

float CFujixGerosBot::GetCurrentDangerLevel()
{
    if(MAX_PREDICTION_TICKS > 0)
        return m_aDeepPredictions[0].m_DangerLevel;
    return 0.0f;
}

bool CFujixGerosBot::IsSafeToMove(int Direction)
{
    if(Direction == -1)
        return m_CurrentSafety.m_IsInputSafe[0];
    else if(Direction == 1)
        return m_CurrentSafety.m_IsInputSafe[2];
    else
        return m_CurrentSafety.m_IsInputSafe[1];
}

// 🚨 УЛУЧШЕННАЯ ПРОВЕРКА АКТИВНОСТИ
bool CFujixGerosBot::IsActive() const
{
    if(!g_Config.m_FujixGerosBot)
        return false;
    
    CGameClient *pGameClient = GameClient();
    if(!pGameClient)
        return false;
    
    // Активен если есть опасность ИЛИ включен принудительно
    bool HasDanger = false;
    for(int i = 0; i < minimum(5, MAX_PREDICTION_TICKS); i++)
    {
        if(m_aDeepPredictions[i].m_InDanger || m_aDeepPredictions[i].m_DeathProbability > 0.05f)
        {
            HasDanger = true;
            break;
        }
    }
    
    return HasDanger || m_EmergencyMode || m_FullAutonomyMode;
}

// 🛡️ УЛУЧШЕННАЯ ПРОВЕРКА ПЕРЕОПРЕДЕЛЕНИЯ ВВОДА  
bool CFujixGerosBot::ShouldOverrideInput()
{
    return IsActive() && (m_OverrideLevel >= 1 || m_BlockPlayerInput || m_FullAutonomyMode);
}

// 🚨 ЭКСТРЕННОЕ СПАСЕНИЕ
void CFujixGerosBot::ExecuteEmergencyRescue()
{
    // Немедленно активируем полную автономию
    ActivateFullAutonomy();
    
    // Попытка найти экстренный крюк
    CGameClient *pGameClient = GameClient();
    if(pGameClient)
    {
        vec2 PlayerPos = pGameClient->m_PredictedChar.m_Pos;
        vec2 PlayerVel = pGameClient->m_PredictedChar.m_Vel;
        
        vec2 EmergencyTarget = FindUltraSafeHookTarget(PlayerPos, PlayerVel);
        if(EmergencyTarget.x != 0 || EmergencyTarget.y != 0)
        {
            // Принудительно используем экстренный крюк
            m_HookAnalysis.m_BestTarget = EmergencyTarget;
            m_HookAnalysis.m_SuccessRate = 1.0f; // Принудительно считаем успешным
        }
    }
}

bool CFujixGerosBot::IsInEmergencyState()
{
    // Экстренное состояние если высокая опасность сейчас или в ближайшем будущем
    for(int i = 0; i < minimum(8, MAX_PREDICTION_TICKS); i++)
    {
        if(m_aDeepPredictions[i].m_DeathProbability > 0.7f || 
           m_aDeepPredictions[i].m_RequiresFullOverride)
        {
            return true;
        }
    }
    
    return m_EmergencyMode;
}

int CFujixGerosBot::GetAggressiveness() const
{
	return g_Config.m_FujixGerosAggressiveness;
}

int CFujixGerosBot::GetPredictionTicks() const
{
	return g_Config.m_FujixGerosPredictionTicks;
}

bool CFujixGerosBot::IsAntiSuicideEnabled() const
{
	return g_Config.m_FujixGerosAntiSuicide;
}


// GameClient() наследуется от CComponent
void CFujixGerosBot::PredictMovement(SAdvancedPrediction *pPredictions, int NumTicks)
{
    // Просто вызываем более продвинутую версию
    DeepPredict(pPredictions, NumTicks);
}

void CFujixGerosBot::SimulateCharacterCore(CCharacterCore *pCore, int Ticks)
{
	CGameClient *pGameClient = GameClient();
	if(!pCore || !pGameClient->Collision())
		return;
		
	for(int i = 0; i < Ticks; i++)
	{
		// Simulate physics step
		pCore->Tick(false, false);
		pCore->Move();
		pCore->Quantize();
	}
}

bool CFujixGerosBot::IsPositionDangerous(vec2 Pos, vec2 Vel)
{
	CGameClient *pGameClient = GameClient();
	if(!pGameClient || !pGameClient->Collision())
		return false;
		
	// Check for collision with death tiles
	int TileIndex = pGameClient->Collision()->GetCollisionAt(Pos.x, Pos.y);
	if(TileIndex == TILE_DEATH)
		return true;
		
	// Check for freeze tiles (can be dangerous in certain situations)
	if(TileIndex == TILE_FREEZE || TileIndex == TILE_DFREEZE || TileIndex == TILE_LFREEZE)
		return true;
		
	// Check if falling into void
	if(Pos.y > pGameClient->Collision()->GetHeight() * 32.0f)
		return true;
		
	// Predict if current velocity will lead to death
	vec2 PredictedPos = Pos + Vel * 2.0f; // 2 ticks ahead
	int PredictedTile = pGameClient->Collision()->GetCollisionAt(PredictedPos.x, PredictedPos.y);
	if(PredictedTile == TILE_DEATH)
		return true;
		
	// Check if predicted velocity will lead to freeze tiles
	if(PredictedTile == TILE_FREEZE || PredictedTile == TILE_DFREEZE || PredictedTile == TILE_LFREEZE)
		return true;
		
	return false;
}

float CFujixGerosBot::CalculateDangerLevel(vec2 Pos, vec2 Vel)
{
	float DangerLevel = 0.0f;
	CGameClient *pGameClient = GameClient();
	
	
	// Base danger from current tile
	int TileIndex = pGameClient->Collision()->GetCollisionAt(Pos.x, Pos.y);
	float Speed = length(Vel);
	
	// Death tiles are extremely dangerous
	if(TileIndex == TILE_DEATH)
		DangerLevel += 10.0f;
		
	// Freeze tiles are also dangerous
	if(TileIndex == TILE_FREEZE || TileIndex == TILE_DFREEZE || TileIndex == TILE_LFREEZE)
		DangerLevel += 7.0f;
		
	// Velocity-based danger (high speed = higher danger)
	DangerLevel += Speed * 0.01f;

	// Distance to nearest safe ground
	float DistanceToSafety = 999.0f;
	for(int x = -5; x <= 5; x++)
	{
		for(int y = -5; y <= 5; y++)
		{
			vec2 TestPos = Pos + vec2{x * 32.0f, y * 32.0f};
			int TestTile = pGameClient->Collision()->GetCollisionAt(TestPos.x, TestPos.y);
			if(TestTile != TILE_DEATH && TestTile != TILE_FREEZE && TestTile != TILE_DFREEZE && TestTile != TILE_LFREEZE && pGameClient->Collision()->CheckPoint(TestPos.x, TestPos.y + 16))
			{
				float Distance = distance(Pos, TestPos);
				DistanceToSafety = minimum(DistanceToSafety, Distance);
			}
		}
	}
	
	if(DistanceToSafety > 500.0f)
		DangerLevel += 5.0f;
	else if(DistanceToSafety > 200.0f)
		DangerLevel += 2.0f;
	return DangerLevel;
}

vec2 CFujixGerosBot::FindBestHookTarget(vec2 Pos, vec2 Vel)
{
	
	
	vec2 BestTarget = vec2{0, 0};
	float BestScore = -1.0f;
	float HookRange = 380.0f; // Максимальная дальность крюка
	
	// 🧠 ЖЕСТКАЯ ЛОГИКА: анализ ситуации и выбор стратегии
	bool FreezeAbove = IsFreezeInDirection(Pos, vec2{0, -1}, 4); // Потолок
	bool FreezeBelow = IsFreezeInDirection(Pos, vec2{0, 1}, 4);  // Пол
	// Переменные FreezeLeft и FreezeRight не используются в этой функции
	
	// 🎯 СТРАТЕГИЯ 1: CEILING RIDING (если сверху и снизу freeze)
	if(FreezeAbove && FreezeBelow)
	{
		// Ищем потолок для ceiling riding
		vec2 CeilingTarget = FindCeilingRidingTarget(Pos, Vel);
		if(CeilingTarget.x != 0 || CeilingTarget.y != 0)
		{
			return CeilingTarget;
		}
	}
	
	// 🎯 СТРАТЕГИЯ 2: WALL RIDING (если потолок далеко)
	if(FreezeBelow && !FreezeAbove)
	{
		// Пытаемся найти стену для wall riding
		vec2 WallTarget = FindWallRidingTarget(Pos, Vel);
		if(WallTarget.x != 0 || WallTarget.y != 0)
		{
			return WallTarget;
		}
	}
	
	// 🎯 СТРАТЕГИЯ 3: УМНОЕ ИЗБЕГАНИЕ ПОЛА
	if(FreezeBelow)
	{
		// НЕ цепляемся за пол! Ищем только вверх и в стороны
		for(int angle = -150; angle <= -30; angle += 10) // Только вверх и диагонали
		{
			float rad = angle * 3.14159265f / 180.0f;
			vec2 Direction = vec2{cosf(rad), sinf(rad)};
			
			vec2 Target = FindHookableInDirection(Pos, Direction, HookRange);
			if(Target.x != 0 || Target.y != 0)
			{
				float Score = CalculateHookScore(Pos, Target, Vel);
				if(Score > BestScore)
				{
					BestScore = Score;
					BestTarget = Target;
				}
			}
		}
	}
	else
	{
		// 🎯 СТРАТЕГИЯ 4: ПОЛНЫЙ ПОИСК (если нет freeze пола)
		for(int angle = 0; angle < 360; angle += 12)
		{
			float rad = angle * 3.14159265f / 180.0f;
			vec2 Direction = vec2{cosf(rad), sinf(rad)};
			
			vec2 Target = FindHookableInDirection(Pos, Direction, HookRange);
			if(Target.x != 0 || Target.y != 0)
			{
				float Score = CalculateHookScore(Pos, Target, Vel);
				if(Score > BestScore)
				{
					BestScore = Score;
					BestTarget = Target;
				}
			}
		}
	}
	
	
	return BestTarget;
}

// 🧠 ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ЖЕСТКОЙ ЛОГИКИ КРЮКА

bool CFujixGerosBot::IsFreezeInDirection(vec2 Pos, vec2 Dir, int TileDistance)
{
	CGameClient *pGameClient = GameClient();
	if(!pGameClient || !pGameClient->Collision())
		return false;
	
	for(int i = 1; i <= TileDistance; i++)
	{
		vec2 CheckPos = Pos + Dir * (i * 32.0f);
		int Tile = pGameClient->Collision()->GetCollisionAt(CheckPos.x, CheckPos.y);
		if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE)
			return true;
	}
	return false;
}

vec2 CFujixGerosBot::FindCeilingRidingTarget(vec2 Pos, vec2 Vel)
{
	CGameClient *pGameClient = GameClient();
	float BestDistance = 999.0f;
	vec2 BestTarget = vec2{0, 0};
	
	// Ищем потолок в диапазоне -45° до -135° (вверх)
	for(int angle = -45; angle >= -135; angle -= 15)
	{
		float rad = angle * 3.14159265f / 180.0f;
		vec2 Direction = vec2{cosf(rad), sinf(rad)};
		
		for(float dist = 64.0f; dist <= 380.0f; dist += 16.0f)
		{
			vec2 TestPos = Pos + Direction * dist;
			
			if(pGameClient->Collision()->CheckPoint(TestPos.x, TestPos.y))
			{
				// Проверяем что это действительно потолок (не freeze)
				int Tile = pGameClient->Collision()->GetCollisionAt(TestPos.x, TestPos.y);
				if(Tile != TILE_FREEZE && Tile != TILE_DFREEZE && Tile != TILE_LFREEZE)
				{
					// Проверяем возможность ceiling riding
					if(CanDoCeilingRiding(Pos, TestPos))
					{
						if(dist < BestDistance)
						{
							BestDistance = dist;
							BestTarget = TestPos;
						}
					}
				}
				break;
			}
		}
	}
	
	return BestTarget;
}

vec2 CFujixGerosBot::FindWallRidingTarget(vec2 Pos, vec2 Vel)
{
	CGameClient *pGameClient = GameClient();
	vec2 BestTarget = vec2{0, 0};
	float BestScore = -1.0f;
	
	// Ищем стены слева и справа на 3-4 тайла выше
	for(int side = -1; side <= 1; side += 2) // -1 = лево, 1 = право
	{
		for(int height = 2; height <= 5; height++) // 2-5 тайлов выше
		{
			vec2 WallPos = Pos + vec2{side * 96.0f, -height * 32.0f}; // 3 тайла в сторону, height вверх
			
			if(pGameClient->Collision()->CheckPoint(WallPos.x, WallPos.y))
			{
				// Проверяем что это не freeze
				int Tile = pGameClient->Collision()->GetCollisionAt(WallPos.x, WallPos.y);
				if(Tile != TILE_FREEZE && Tile != TILE_DFREEZE && Tile != TILE_LFREEZE)
				{
					// Проверяем возможность wall riding
					if(CanDoWallRiding(Pos, WallPos, side))
					{
						float Score = 10.0f - height; // Выше = лучше
						if(distance(Pos, WallPos) <= 380.0f) // В пределах крюка
						{
							if(Score > BestScore)
							{
								BestScore = Score;
								BestTarget = WallPos;
							}
						}
					}
				}
			}
		}
	}
	
	return BestTarget;
}

vec2 CFujixGerosBot::FindHookableInDirection(vec2 Pos, vec2 Direction, float MaxRange)
{
	CGameClient *pGameClient = GameClient();
	
	for(float dist = 32.0f; dist <= MaxRange; dist += 16.0f)
	{
		vec2 TestPos = Pos + Direction * dist;
		
		if(pGameClient->Collision()->CheckPoint(TestPos.x, TestPos.y))
		{
			// Не цепляемся за freeze tiles!
			int Tile = pGameClient->Collision()->GetCollisionAt(TestPos.x, TestPos.y);
			if(Tile != TILE_FREEZE && Tile != TILE_DFREEZE && Tile != TILE_LFREEZE)
			{
				return TestPos;
			}
		}
	}
	
	return vec2{0, 0};
}

float CFujixGerosBot::CalculateHookScore(vec2 Pos, vec2 Target, vec2 Vel)
{
	float Score = 0.0f;
	float Distance = distance(Pos, Target);
	
	// Выше = лучше (избегаем пола)
	if(Target.y < Pos.y)
		Score += 8.0f;
	
	// Ближе = лучше (до определенной точки)
	if(Distance < 200.0f)
		Score += (200.0f - Distance) * 0.02f;
	
	// Проверяем безопасность траектории
	if(IsHookTrajectorysSafe(Pos, Target))
		Score += 10.0f;
	
	// Бонус за wall/ceiling riding позиции
	if(IsGoodForRiding(Pos, Target))
		Score += 15.0f;
	
	return Score;
}

bool CFujixGerosBot::CanDoCeilingRiding(vec2 Pos, vec2 CeilingTarget)
{
	// Проверяем что можем делать ceiling riding
	float Distance = distance(Pos, CeilingTarget);
	
	// Достаточно близко для riding
	if(Distance > 380.0f || Distance < 64.0f)
		return false;
	
	// Проверяем что ceiling выше нас
	if(CeilingTarget.y >= Pos.y)
		return false;
	
	// Проверяем траекторию на безопасность
	return IsHookTrajectorysSafe(Pos, CeilingTarget);
}

bool CFujixGerosBot::CanDoWallRiding(vec2 Pos, vec2 WallTarget, int Side)
{
	float Distance = distance(Pos, WallTarget);
	
	// Проверяем дистанцию
	if(Distance > 380.0f || Distance < 64.0f)
		return false;
	
	// Проверяем что стена сбоку и выше
	if(WallTarget.y >= Pos.y)
		return false;
	
	// Проверяем что стена в правильной стороне
	if((Side > 0 && WallTarget.x <= Pos.x) || (Side < 0 && WallTarget.x >= Pos.x))
		return false;
	
	return IsHookTrajectorysSafe(Pos, WallTarget);
}

bool CFujixGerosBot::IsHookTrajectorysSafe(vec2 From, vec2 To)
{
	CGameClient *pGameClient = GameClient();
	vec2 Direction = normalize(To - From);
	float Distance = distance(From, To);
	
	// Проверяем траекторию на freeze tiles
	for(float step = 16.0f; step < Distance; step += 16.0f)
	{
		vec2 CheckPos = From + Direction * step;
		int Tile = pGameClient->Collision()->GetCollisionAt(CheckPos.x, CheckPos.y);
		if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE)
		{
			return false;
		}
	}
	
	return true;
}

// 🧠 УЛУЧШЕННАЯ ЛОГИКА ESCAPE DIRECTION
vec2 CFujixGerosBot::CalculateEscapeDirection(vec2 Pos, vec2 Vel, float DangerLevel)
{
	CGameClient *pGameClient = GameClient();
	
	vec2 BestDirection = vec2{0, 0};
	float BestScore = -999.0f;

	// Test different movement directions
	for(int x = -1; x <= 1; x++)
	{
		for(int y = -1; y <= 1; y++)
		{
			if(x == 0 && y == 0)
				continue;
				
			vec2 TestDirection = vec2{(float)x, (float)y};
			vec2 TestPos = Pos + TestDirection * 64.0f; // Test position 64 units away
			
			float Score = 0.0f;
			
			// 🎯 ПРЕВЕНТИВНАЯ ПРОВЕРКА: анализируем путь к TestPos на наличие freeze
			bool PathHasFreeze = false;
			for(float step = 16.0f; step <= 64.0f; step += 16.0f)
			{
				vec2 PathPos = Pos + TestDirection * step;
				int PathTile = pGameClient->Collision()->GetCollisionAt(PathPos.x, PathPos.y);
				if(PathTile == TILE_FREEZE || PathTile == TILE_DFREEZE || PathTile == TILE_LFREEZE)
				{
					PathHasFreeze = true;
					break;
				}
			}
			
			// Higher score for directions that lead away from danger
			if(!IsPositionDangerous(TestPos, vec2{0, 0}) && !PathHasFreeze)
				Score += 15.0f; // Увеличили бонус за безопасный путь
			else if(PathHasFreeze)
				Score -= 10.0f; // Штраф за freeze на пути
			else
				Score -= 5.0f;
			
			// 🧊 СПЕЦИАЛЬНАЯ ЛОГИКА ДЛЯ ТЕКУЩИХ FREEZE TILES
			int CurrentTile = pGameClient->Collision()->GetCollisionAt(Pos.x, Pos.y);
			if(CurrentTile == TILE_FREEZE || CurrentTile == TILE_DFREEZE || CurrentTile == TILE_LFREEZE)
			{
				// При заморозке приоритет - движение вверх и в стороны
				if(y < 0) Score += 8.0f; // Вверх - высший приоритет
				if(x != 0) Score += 5.0f; // В стороны - средний приоритет
			}
				
			// Prefer upward movement when in danger
			if(DangerLevel > 3.0f && y < 0)
				Score += 3.0f;
			// Check if this direction has walkable ground
			vec2 GroundCheck = TestPos + vec2{0, 16};
			if(pGameClient->Collision()->CheckPoint(GroundCheck.x, GroundCheck.y))
				Score += 2.0f;
				
			if(Score > BestScore)
			{
				BestScore = Score;
				BestDirection = TestDirection;
			}
		}
	}
	
	return normalize(BestDirection);
}
bool CFujixGerosBot::IsGoodForRiding(vec2 Pos, vec2 Target)
{
	// Хорошие позиции для riding:
	// 1. Стена на 3-4 тайла выше и в стороне
	// 2. Потолок выше нас
	
	vec2 Diff = Target - Pos;
	
	// Стена для wall riding
	if(abs(Diff.x) >= 64.0f && abs(Diff.x) <= 128.0f && Diff.y < -64.0f && Diff.y > -160.0f)
		return true;
	
	// Потолок для ceiling riding
	if(Diff.y < -32.0f && abs(Diff.x) <= 160.0f)
		return true;
	
	return false;
}


bool CFujixGerosBot::CanReachSafetyWithHook(vec2 From, vec2 HookTarget)
{
	CGameClient *pGameClient = GameClient();
	// Simple simulation: check if hooking to target would allow reaching safe ground
	vec2 HookDirection = normalize(HookTarget - From);
	vec2 SimulatedPos = From;
	
	// Simulate hook swing
	for(int i = 0; i < 50; i++)
	{
		SimulatedPos += HookDirection * 8.0f;
		
		// Check if we've reached a safe position
		if(!IsPositionDangerous(SimulatedPos, vec2{0, 0}))
		{
			vec2 GroundCheck = SimulatedPos + vec2{0, 16};
			if(pGameClient->Collision()->CheckPoint(GroundCheck.x, GroundCheck.y))
				return true;
		}
		
		// Stop if we hit a wall
		if(pGameClient->Collision()->CheckPoint(SimulatedPos.x, SimulatedPos.y))
			break;
	}
	
	return false;
}

bool CFujixGerosBot::ShouldUseJump(vec2 Pos, vec2 Vel, vec2 DesiredDir)
{
	CGameClient *pGameClient = GameClient();
	
	// 🧊 ПРИОРИТЕТ ПРЫЖКА ДЛЯ FREEZE TILES
	int CurrentTile = pGameClient->Collision()->GetCollisionAt(Pos.x, Pos.y);
	if(CurrentTile == TILE_FREEZE || CurrentTile == TILE_DFREEZE || CurrentTile == TILE_LFREEZE)
	{
		// В freeze tile всегда пытаемся прыгнуть для побега
		return true;
	}
	
	// Jump if we need to go upward
	if(DesiredDir.y < -0.5f)
		return true;
		
	// Jump if there's an obstacle in front of us
	vec2 FrontCheck = Pos + vec2{DesiredDir.x * 32.0f, 0};
	if(pGameClient->Collision()->CheckPoint(FrontCheck.x, FrontCheck.y))
		return true;
		
	// Jump if we're moving fast downward and in danger
	if(Vel.y > 10.0f && IsPositionDangerous(Pos, Vel))
		return true;
		
	return false;
}

bool CFujixGerosBot::DetectSuicideAttempt(vec2 Pos, vec2 Vel, int InputDirection)
{
	if(!IsAntiSuicideEnabled())
		return false;
		
	// Check if player is deliberately moving toward death tiles
	vec2 InputDir = vec2{(float)InputDirection, 0};
	vec2 FuturePos = Pos + InputDir * 64.0f;
	
	if(IsPositionDangerous(FuturePos, Vel))
	{
		// Check if there are safer alternatives
		for(int dir = -1; dir <= 1; dir += 2)
		{
			if(dir == InputDirection)
				continue;
				
			vec2 SafePos = Pos + vec2{(float)dir * 64.0f, 0};
			if(!IsPositionDangerous(SafePos, Vel))
				return true; // Player chose dangerous direction when safer ones exist
		}
	}
	
	return false;
}

bool CFujixGerosBot::IsPlayerTryingToKillThemselves()
{
	// Analyze recent player behavior patterns
	// This is a simplified version - real implementation would track behavior history
	
	// Check if player is consistently moving toward danger
	if(m_PlayerTrustLevel < 0.3f)
		return true;
		
	return false;
}

bool CFujixGerosBot::IsInEmergencyState()
{
    // Используем новую систему глубокого предсказания
    for(int i = 0; i < minimum(8, MAX_PREDICTION_TICKS); i++)
    {
        if(m_aDeepPredictions[i].m_InDanger || 
           m_aDeepPredictions[i].m_DeathProbability > 0.5f ||
           m_aDeepPredictions[i].m_RequiresFullOverride)
        {
            return true;
        }
    }
    
    return m_EmergencyMode;
}
void CFujixGerosBot::ExecuteEmergencyRescue()
{
	CGameClient *pGameClient = GameClient();
	if(pGameClient->Client()->GameTick(0) - m_LastRescueTick < 5) // Prevent spam rescues
		return;

	m_EmergencyMode = true;
	m_RescueAttempts++;
	m_LastRescueTick = pGameClient->Client()->GameTick(0);
	
	// Проверяем, вышли ли мы из опасности
	if(!IsInEmergencyState())
	{
		m_EmergencyMode = false;
	}
	
	// Force override player input to execute rescue
	// This will be used by the input system
}
bool CFujixGerosBot::ShouldOverrideInput()
{
	if(!IsActive())
		return false;
	
	// Check if we're in emergency mode and should override input
	if(m_EmergencyMode && IsInEmergencyState())
		return true;
	
	// Check if player is trying to suicide and anti-suicide is enabled
	if(IsAntiSuicideEnabled() && IsPlayerTryingToKillThemselves())
		return true;
		
	return false;
}

void CFujixGerosBot::GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX)
{
	if(!ShouldOverrideInput())
		return;
	
	CGameClient *pGameClient = GameClient();
	int CurrentTick = pGameClient->Client()->GameTick(0);
	
	// 🕷️ WALL/CEILING RIDING ЛОГИКА С ТАЙМИНГАМИ
	if(m_IsWallRiding || m_IsCeilingRiding)
	{
		ExecuteRidingLogic(pInputDirection, pJump, pHook, pTargetX, CurrentTick);
		return;
	}
	
	// Use the first prediction to determine immediate action
    SAdvancedPrediction *pPred = &m_aDeepPredictions[0];
	
	// 🎯 ПРОВЕРЯЕМ НУЖНО ЛИ НАЧАТЬ RIDING
	if(pPred->m_CanUseHook && IsGoodForRiding(pGameClient->m_PredictedChar.m_Pos, pPred->m_HookTarget))
	{
		StartRiding(pPred->m_HookTarget, CurrentTick);
		ExecuteRidingLogic(pInputDirection, pJump, pHook, pTargetX, CurrentTick);
		return;
	}
	
	// 🎯 ОБЫЧНАЯ ЛОГИКА УПРАВЛЕНИЯ (если не riding)
	// Use the first prediction to determine immediate action
	
	// Set movement direction
	if(pPred->m_DesiredDir.x > 0.1f)
		*pInputDirection = 1;
	else if(pPred->m_DesiredDir.x < -0.1f)
		*pInputDirection = -1;
	else
		*pInputDirection = 0;
		
	// Set jump
	*pJump = pPred->m_ShouldJump ? 1 : 0;

	// Set hook
	*pHook = pPred->m_CanUseHook ? 1 : 0;
	*pTargetX = pPred->m_HookTarget;
}

// 🕷️ МАКСИМАЛЬНО ЖЕСТКАЯ WALL/CEILING RIDING ЛОГИКА

void CFujixGerosBot::StartRiding(vec2 Target, int CurrentTick)
{
	CGameClient *pGameClient = GameClient();
	vec2 PlayerPos = pGameClient->m_PredictedChar.m_Pos;
	vec2 Diff = Target - PlayerPos;
	
	m_CurrentRidingTarget = Target;
	m_RidingStartTick = CurrentTick;
	
	// Определяем тип riding
	if(abs(Diff.x) > abs(Diff.y) && Diff.y < -32.0f)
	{
		// WALL RIDING: стена сбоку и выше
		m_IsWallRiding = true;
		m_IsCeilingRiding = false;
		m_RidingSide = (Diff.x > 0) ? 1 : -1;
	}
	else if(Diff.y < -32.0f)
	{
		// CEILING RIDING: потолок выше
		m_IsCeilingRiding = true;
		m_IsWallRiding = false;
		m_RidingSide = 0;
	}
}

void CFujixGerosBot::ExecuteRidingLogic(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX, int CurrentTick)
{
	CGameClient *pGameClient = GameClient();
	vec2 PlayerPos = pGameClient->m_PredictedChar.m_Pos;
	vec2 PlayerVel = pGameClient->m_PredictedChar.m_Vel;
	
	int RidingDuration = CurrentTick - m_RidingStartTick;
	int TimeSinceRelease = CurrentTick - m_LastHookReleaseTick;
	
	// 🕷️ WALL RIDING LOGIC
	if(m_IsWallRiding)
	{
		// Движемся в противоположную сторону от стены
		*pInputDirection = -m_RidingSide;
		
		// 🎯 ТАЙМИНИНГ ОТПУСКАНИЯ/ПЕРЕХВАТА КРЮКА
		bool ShouldHook = ShouldHookForWallRiding(PlayerPos, PlayerVel, RidingDuration, TimeSinceRelease);
		*pHook = ShouldHook ? 1 : 0;
		
		if(ShouldHook)
		{
			*pTargetX = m_CurrentRidingTarget;
		}
		else if(*pHook == 0 && TimeSinceRelease == 0)
		{
			m_LastHookReleaseTick = CurrentTick;
		}
		
		// Прыжок при необходимости
		*pJump = ShouldJumpForWallRiding(PlayerPos, PlayerVel, RidingDuration) ? 1 : 0;
		
		// Проверяем нужно ли завершить wall riding
		if(ShouldStopWallRiding(PlayerPos, PlayerVel, RidingDuration))
		{
			StopRiding();
		}
	}
	// 🔄 CEILING RIDING LOGIC  
	else if(m_IsCeilingRiding)
	{
		// Тонкая настройка направления для ceiling riding
		*pInputDirection = CalculateCeilingRidingDirection(PlayerPos, PlayerVel, RidingDuration);
		
		// 🎯 ТАЙМИНИНГ ДЛЯ CEILING RIDING
		bool ShouldHook = ShouldHookForCeilingRiding(PlayerPos, PlayerVel, RidingDuration, TimeSinceRelease);
		*pHook = ShouldHook ? 1 : 0;
		
		if(ShouldHook)
		{
			*pTargetX = m_CurrentRidingTarget;
		}
		else if(*pHook == 0 && TimeSinceRelease == 0)
		{
			m_LastHookReleaseTick = CurrentTick;
		}
		
		// Прыжок редко используется в ceiling riding
		*pJump = 0;
		
		// Проверяем нужно ли завершить ceiling riding
		if(ShouldStopCeilingRiding(PlayerPos, PlayerVel, RidingDuration))
		{
			StopRiding();
		}
	}
}

bool CFujixGerosBot::ShouldHookForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease)
{
	// 🕷️ WALL RIDING ПАТТЕРН: хук 3-4 тика, отпуск 2-3 тика, повтор
	
	// Если недавно отпустили, ждем
	if(TimeSinceRelease > 0 && TimeSinceRelease < 3)
		return false;
	
	// Если слишком далеко от стены, хукаемся
	float DistanceToWall = distance(PlayerPos, m_CurrentRidingTarget);
	if(DistanceToWall > 350.0f)
		return true;
	
	// Если падаем слишком быстро, хукаемся
	if(PlayerVel.y > 8.0f)
		return true;
	
	// Если висим на крюке слишком долго, отпускаем
	if(RidingDuration % 7 < 4) // 4 тика хук, 3 тика без
		return true;
	
	return false;
}

bool CFujixGerosBot::ShouldHookForCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration, int TimeSinceRelease)
{
	// 🔄 CEILING RIDING ПАТТЕРН: более частые перехваты для удержания высоты
	
	// Если недавно отпустили, ждем меньше
	if(TimeSinceRelease > 0 && TimeSinceRelease < 2)
		return false;
	
	// Если падаем, сразу хукаемся
	if(PlayerVel.y > 5.0f)
		return true;
	
	// Если слишком далеко от потолка
	float DistanceToCeiling = distance(PlayerPos, m_CurrentRidingTarget);
	if(DistanceToCeiling > 320.0f)
		return true;
	
	// Частые перехваты: 3 тика хук, 2 тика без
	if(RidingDuration % 5 < 3)
		return true;
	
	return false;
}

bool CFujixGerosBot::ShouldJumpForWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration)
{
	// Прыгаем если падаем слишком быстро или в начале riding
	return (PlayerVel.y > 10.0f) || (RidingDuration < 5);
}

int CFujixGerosBot::CalculateCeilingRidingDirection(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration)
{
	// Проверяем что впереди нет freeze
	// Проверяем что впереди нет freeze
	bool FreezeLeft = IsFreezeInDirection(PlayerPos, vec2{-1, 0}, 3);
	bool FreezeRight = IsFreezeInDirection(PlayerPos, vec2{1, 0}, 3);
	
	if(FreezeLeft && !FreezeRight)
		return 1; // Движемся направо
	if(FreezeRight && !FreezeLeft)
		return -1; // Движемся налево
	
	// Меняем направление каждые 30 тиков для разнообразия
	return ((RidingDuration / 30) % 2 == 0) ? 1 : -1;
}

bool CFujixGerosBot::ShouldStopWallRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration)
{
	// Останавливаем если нет опасности или riding слишком долго
	if(!IsInEmergencyState() && RidingDuration > 150) // 2.5 секунды
		return true;
	
	// Останавливаем если достигли безопасной зоны
	if(!IsPositionDangerous(PlayerPos, PlayerVel))
		return true;
	
	return false;
}

bool CFujixGerosBot::ShouldStopCeilingRiding(vec2 PlayerPos, vec2 PlayerVel, int RidingDuration)
{
	// Аналогично wall riding
	return ShouldStopWallRiding(PlayerPos, PlayerVel, RidingDuration);
}

void CFujixGerosBot::StopRiding()
{
	m_IsWallRiding = false;
	m_IsCeilingRiding = false;
	m_RidingSide = 0;
	m_CurrentRidingTarget = vec2{0, 0};
}

// 🔧 НЕДОСТАЮЩИЕ РЕАЛИЗАЦИИ МЕТОДОВ

bool CFujixGerosBot::IsFloorHookDangerous(vec2 HookTarget)
{
    // Проверяем что крюк направлен в пол
    return IsHookTargetFloor(HookTarget);
}

void CFujixGerosBot::UpdateDeathMatrix()
{
    // Обновляем матрицу смерти - вызываем существующий метод
    AnalyzeDangerMatrix();
}

bool CFujixGerosBot::ShouldBlockInput(int InputType)
{
    // Определяем нужно ли блокировать ввод
    switch(InputType)
    {
        case 0: return !m_CurrentSafety.m_IsInputSafe[0]; // Left
        case 1: return !m_CurrentSafety.m_IsInputSafe[2]; // Right  
        case 2: return !m_CurrentSafety.m_IsJumpSafe;     // Jump
        case 3: return !m_CurrentSafety.m_IsHookSafe;     // Hook
        default: return false;
    }
}

void CFujixGerosBot::ForceEmergencyEscape()
{
    // Принудительное экстренное спасение
    ExecuteEmergencyRescue();
}

bool CFujixGerosBot::AnalyzeHookTrajectory(vec2 From, vec2 To)
{
    // Анализ безопасности траектории крюка
    return IsHookTrajectorysSafe(From, To);
}

void CFujixGerosBot::AvoidFloorHooks()
{
    // Избегание крюков в пол - уже реализовано в FindUltraSafeHookTarget
    // Этот метод просто блокирует такие крюки
    m_HookAnalysis.m_WillHitFloor = true;
    m_HookAnalysis.m_SuccessRate = 0.0f;
}

void CFujixGerosBot::OptimizeRescueStrategy()
{
    // Оптимизация стратегии спасения
    // Адаптивная настройка порогов
    float CurrentSuccessRate = GetSuccessRate();
    
    if(CurrentSuccessRate < 0.5f)
    {
        // Понижаем пороги для более агрессивного вмешательства
        m_AdaptiveThreshold = maximum(0.05f, m_AdaptiveThreshold - 0.05f);
        m_DangerDetectionRange = minimum(24, m_DangerDetectionRange + 2);
    }
    else if(CurrentSuccessRate > 0.8f)
    {
        // Повышаем пороги для менее агрессивного вмешательства  
        m_AdaptiveThreshold = minimum(0.3f, m_AdaptiveThreshold + 0.02f);
        m_DangerDetectionRange = maximum(8, m_DangerDetectionRange - 1);
    }
}

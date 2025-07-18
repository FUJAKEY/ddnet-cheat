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
CFujixGerosBot::CFujixGerosBot()
{
	m_LastPredictionTick = 0;
	m_RescueAttempts = 0;
	m_LastRescueTick = 0;
	m_PlayerTrustLevel = 1.0f;
	m_EmergencyMode = false;
	m_FullPredictionMode = true;
	m_LastFullPredictionTick = 0;
	
	// 🕷️ WALL/CEILING RIDING INITIALIZATION
	m_IsWallRiding = false;
	m_IsCeilingRiding = false;
	m_RidingStartTick = 0;
	m_LastHookReleaseTick = 0;
	m_CurrentRidingTarget = vec2{0, 0};
	m_RidingSide = 0;
	for(int i = 0; i < 16; i++)
	{
		m_aPredictions[i].m_Pos = vec2{0, 0};
		m_aPredictions[i].m_Vel = vec2{0, 0};
		m_aPredictions[i].m_InDanger = false;
		m_aPredictions[i].m_DangerLevel = 0.0f;
		m_aPredictions[i].m_TicksUntilDeath = -1;
		m_aPredictions[i].m_CanUseHook = false;
		m_aPredictions[i].m_ShouldJump = false;
		m_aPredictions[i].m_HookTarget = vec2{0, 0};
		m_aPredictions[i].m_DesiredDir = vec2{0, 0};
	}
}

void CFujixGerosBot::OnInit()
{
	// Initialize GEROS BOT system
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
			if(m_aPredictions[i].m_InDanger)
			{
				// Красные точки = опасность
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 0.2f, 0.2f, 0.8f);
				IGraphics::CQuadItem QuadItem(m_aPredictions[i].m_Pos.x - 8, m_aPredictions[i].m_Pos.y - 8, 16, 16);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			
			// Линии к целям крюка
			if(m_aPredictions[i].m_CanUseHook)
			{
				Graphics()->LinesBegin();
				Graphics()->SetColor(0.2f, 1.0f, 0.2f, 0.6f);
				IGraphics::CLineItem LineItem(m_aPredictions[i].m_Pos.x, m_aPredictions[i].m_Pos.y, 
											  m_aPredictions[i].m_HookTarget.x, m_aPredictions[i].m_HookTarget.y);
				Graphics()->LinesDraw(&LineItem, 1);
				Graphics()->LinesEnd();
			}
		}
		
		// 🕷️ ВИЗУАЛИЗАЦИЯ WALL/CEILING RIDING
		if(m_IsWallRiding || m_IsCeilingRiding)
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 0.0f, 0.5f); // Желтый = riding mode
			IGraphics::CQuadItem RidingQuad(m_CurrentRidingTarget.x - 12, m_CurrentRidingTarget.y - 12, 24, 24);
			Graphics()->QuadsDrawTL(&RidingQuad, 1);
			Graphics()->QuadsEnd();
		}
	}
}

void CFujixGerosBot::OnMessage(int MsgType, void *pRawMsg)
{
	// Handle game messages if needed
}

bool CFujixGerosBot::IsActive() const
{
	return g_Config.m_FujixGerosBot;
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

void CFujixGerosBot::Update()
{
	if(!IsActive())
		return;
	// Update prediction system
	PredictMovement(m_aPredictions, GetPredictionTicks());
	
	// Check for emergency situations
	if(IsInEmergencyState())
	{
		ExecuteEmergencyRescue();
	}
	
	// Update trust level based on player behavior
	if(IsAntiSuicideEnabled() && IsPlayerTryingToKillThemselves())
	{
		m_PlayerTrustLevel = maximum(0.1f, m_PlayerTrustLevel - 0.05f);
	}
	else
	{
		m_PlayerTrustLevel = minimum(1.0f, m_PlayerTrustLevel + 0.01f);
	}
	
	CGameClient *pGameClient = GameClient();
	m_LastPredictionTick = pGameClient->Client()->GameTick(0);
}

// GameClient() наследуется от CComponent
void CFujixGerosBot::PredictMovement(SGerosBotPrediction *pPredictions, int NumTicks)
{
	CGameClient *pGameClient = GameClient();
	CCharacterCore Core = GameClient()->m_PredictedChar;
	
	// 🎯 ПОЛУЧАЕМ ТЕКУЩИЙ ВВОД ИГРОКА для реалистичной симуляции
	CNetObj_PlayerInput *pCurrentInput = &pGameClient->m_Controls.m_aInputData[g_Config.m_ClDummy];
	
	for(int i = 0; i < NumTicks && i < 16; i++)
	{
		// 🔮 СИМУЛИРУЕМ РЕАЛЬНОЕ ДВИЖЕНИЕ с текущим вводом игрока
		Core.m_Input.m_Direction = pCurrentInput->m_Direction;
		Core.m_Input.m_Jump = pCurrentInput->m_Jump;
		Core.m_Input.m_Hook = pCurrentInput->m_Hook;
		Core.m_Input.m_TargetX = pCurrentInput->m_TargetX;
		Core.m_Input.m_TargetY = pCurrentInput->m_TargetY;
		
		// Simulate one tick ahead с РЕАЛЬНЫМ вводом
		SimulateCharacterCore(&Core, 1);
		
		pPredictions[i].m_Pos = Core.m_Pos;
		pPredictions[i].m_Vel = Core.m_Vel;
		
		// 🚨 ПРОВЕРЯЕМ: попадет ли игрок в freeze через i+1 тиков?
		pPredictions[i].m_InDanger = IsPositionDangerous(Core.m_Pos, Core.m_Vel);
		pPredictions[i].m_DangerLevel = CalculateDangerLevel(Core.m_Pos, Core.m_Vel);
		
		// ⏰ КРИТИЧЕСКИ ВАЖНО: сколько тиков до freeze?
		if(pPredictions[i].m_InDanger)
		{
			pPredictions[i].m_TicksUntilDeath = i + 1;
		}
		else
		{
			pPredictions[i].m_TicksUntilDeath = -1;
		}
		
		// 🛡️ РАССЧИТЫВАЕМ ПРЕВЕНТИВНЫЕ ДЕЙСТВИЯ для этого тика
		pPredictions[i].m_HookTarget = FindBestHookTarget(Core.m_Pos, Core.m_Vel);
		pPredictions[i].m_DesiredDir = CalculateEscapeDirection(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DangerLevel);
		pPredictions[i].m_CanUseHook = pPredictions[i].m_HookTarget.x != 0 || pPredictions[i].m_HookTarget.y != 0;
		pPredictions[i].m_ShouldJump = ShouldUseJump(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DesiredDir);
	}
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
	CGameClient *pGameClient = GameClient();
	
	vec2 BestTarget = vec2{0, 0};
	float BestScore = -1.0f;
	float HookRange = 380.0f; // Максимальная дальность крюка
	
	// 🧠 ЖЕСТКАЯ ЛОГИКА: анализ ситуации и выбор стратегии
	bool FreezeAbove = IsFreezeInDirection(Pos, vec2{0, -1}, 4); // Потолок
	bool FreezeBelow = IsFreezeInDirection(Pos, vec2{0, 1}, 4);  // Пол
	bool FreezeLeft = IsFreezeInDirection(Pos, vec2{-1, 0}, 4);  // Левая стена
	bool FreezeRight = IsFreezeInDirection(Pos, vec2{1, 0}, 4);  // Правая стена
	
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
	CGameClient *pGameClient = GameClient();
	
	// ⚡ ПРЕВЕНТИВНАЯ ЛОГИКА: проверяем БУДУЩИЕ позиции на 8 тиков вперед
	for(int i = 0; i < GetPredictionTicks(); i++)
	{
		// 🎯 КРИТЕРИЙ АКТИВАЦИИ: freeze через 2-8 тиков = EMERGENCY!
		if(m_aPredictions[i].m_InDanger && m_aPredictions[i].m_TicksUntilDeath >= 2 && m_aPredictions[i].m_TicksUntilDeath <= 8)
		{
			return true; // Активируем bot ЗАРАНЕЕ!
		}
		
		// 🧊 СПЕЦИАЛЬНАЯ ПРОВЕРКА: freeze tiles на пути движения
		if(pGameClient && pGameClient->Collision())
		{
			vec2 FuturePos = m_aPredictions[i].m_Pos;
			int FutureTile = pGameClient->Collision()->GetCollisionAt(FuturePos.x, FuturePos.y);
			if(FutureTile == TILE_FREEZE || FutureTile == TILE_DFREEZE || FutureTile == TILE_LFREEZE)
			{
				return true; // Видим freeze впереди - активируем bot!
			}
		}
	}
	
	// 🚨 РЕЗЕРВНАЯ ПРОВЕРКА: уже в freeze (последний шанс)
	if(pGameClient && pGameClient->m_Snap.m_pLocalCharacter)
	{
		vec2 PlayerPos = vec2(pGameClient->m_Snap.m_pLocalCharacter->m_X, pGameClient->m_Snap.m_pLocalCharacter->m_Y);
		int CurrentTile = pGameClient->Collision()->GetCollisionAt(PlayerPos.x, PlayerPos.y);
		if(CurrentTile == TILE_FREEZE || CurrentTile == TILE_DFREEZE || CurrentTile == TILE_LFREEZE)
		{
			return true; // Freeze tile = немедленная emergency
		}
	}
	
	// ⚠️ ДОПОЛНИТЕЛЬНО: критически высокий уровень опасности
	if(m_aPredictions[0].m_DangerLevel > 8.0f)
		return true;
		
	return false;
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
	SGerosBotPrediction *pPred = &m_aPredictions[0];
	
	// 🎯 ПРОВЕРЯЕМ НУЖНО ЛИ НАЧАТЬ RIDING
	if(pPred->m_CanUseHook && IsGoodForRiding(pGameClient->m_PredictedChar.m_Pos, pPred->m_HookTarget))
	{
		StartRiding(pPred->m_HookTarget, CurrentTick);
		ExecuteRidingLogic(pInputDirection, pJump, pHook, pTargetX, CurrentTick);
		return;
	}
	
	// 🎯 ОБЫЧНАЯ ЛОГИКА УПРАВЛЕНИЯ (если не riding)
	// Use the first prediction to determine immediate action
	SGerosBotPrediction *pPred = &m_aPredictions[0];
	
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
	CGameClient *pGameClient = GameClient();
	
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

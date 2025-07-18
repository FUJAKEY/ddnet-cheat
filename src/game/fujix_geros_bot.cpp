#include "fujix_geros_bot.h"
#include <cmath>

// Math functions for vec2
vec2 operator+(const vec2& a, const vec2& b) { return {a.x + b.x, a.y + b.y}; }
vec2 operator-(const vec2& a, const vec2& b) { return {a.x - b.x, a.y - b.y}; }
vec2 operator*(const vec2& a, float f) { return {a.x * f, a.y * f}; }
vec2& operator+=(vec2& a, const vec2& b) { a.x += b.x; a.y += b.y; return a; }

float length(const vec2& v) { return sqrtf(v.x*v.x + v.y*v.y); }
float distance(const vec2& a, const vec2& b) { vec2 d = a - b; return length(d); }
vec2 normalize(const vec2& v) { float l = length(v); return l > 0 ? v * (1.0f/l) : vec2{0,0}; }
float minimum(float a, float b) { return a < b ? a : b; }
float maximum(float a, float b) { return a > b ? a : b; }

// Game constants
#define TILE_DEATH 1
#define TILE_FREEZE 2
#define TILE_DFREEZE 3  
#define TILE_LFREEZE 4

// Mock structures for standalone compilation  
struct CCharacterCore { vec2 m_Pos, m_Vel; void Tick(bool, bool) {} void Move() {} void Quantize() {} };
struct CCollision { int GetCollisionAt(float, float) { return 0; } bool CheckPoint(float, float) { return true; } float GetHeight() { return 1000.0f; } };
struct CGameWorld { CCollision *m_pCollision = new CCollision(); };  
struct CClient { int GameTick(int) { return 0; } };
class CGameClient { public: CGameWorld m_GameWorld; CCharacterCore m_PredictedChar; CClient* Client() { return &m_Client; } private: CClient m_Client; };

// External configuration (will be defined in real project)
bool g_Config_m_FujixGerosBot = true;
bool g_Config_m_Debug = false;
int g_Config_m_FujixGerosAggressiveness = 5;
int g_Config_m_FujixGerosPredictionTicks = 8;
bool g_Config_m_FujixGerosAntiSuicide = true;
CFujixGerosBot::CFujixGerosBot()
{
	m_LastPredictionTick = 0;
	m_RescueAttempts = 0;
	m_LastRescueTick = 0;
	m_PlayerTrustLevel = 1.0f;
	m_EmergencyMode = false;
	m_FullPredictionMode = true;
	m_LastFullPredictionTick = 0;
	
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
		
	// Optional: Render prediction debug information
	if(g_Config_m_Debug)
	{
		// Render predicted positions and danger zones
		for(int i = 0; i < GetPredictionTicks(); i++)
		{
			if(m_aPredictions[i].m_InDanger)
			{
				// Render danger indicators
			}
		}
	}
}

void CFujixGerosBot::OnMessage(int MsgType, void *pRawMsg)
{
	// Handle game messages if needed
}

bool CFujixGerosBot::IsActive() const
{
	return g_Config_m_FujixGerosBot;
}

int CFujixGerosBot::GetAggressiveness() const
{
	return g_Config_m_FujixGerosAggressiveness;
}

int CFujixGerosBot::GetPredictionTicks() const
{
	return g_Config_m_FujixGerosPredictionTicks;
}

bool CFujixGerosBot::IsAntiSuicideEnabled() const
{
	return g_Config_m_FujixGerosAntiSuicide;
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
	
	CGameClient *pGameClient = GetGameClient();
	m_LastPredictionTick = pGameClient->Client()->GameTick(0);
}

CGameClient *CFujixGerosBot::GetGameClient()
{
	return GameClient();
}


void CFujixGerosBot::PredictMovement(SGerosBotPrediction *pPredictions, int NumTicks)
{
	CGameClient *pGameClient = GetGameClient();
	CCharacterCore Core = pGameClient->m_PredictedChar;
	
	for(int i = 0; i < NumTicks && i < 16; i++)
	{
		// Simulate one tick ahead
		SimulateCharacterCore(&Core, 1);
		
		pPredictions[i].m_Pos = Core.m_Pos;
		pPredictions[i].m_Vel = Core.m_Vel;
		pPredictions[i].m_InDanger = IsPositionDangerous(Core.m_Pos, Core.m_Vel);
		pPredictions[i].m_DangerLevel = CalculateDangerLevel(Core.m_Pos, Core.m_Vel);
		
		// Calculate time until potential death
		if(pPredictions[i].m_InDanger)
		{
			pPredictions[i].m_TicksUntilDeath = i + 1;
		}
		else
		{
			pPredictions[i].m_TicksUntilDeath = -1;
		}
		
		// Calculate rescue options
		pPredictions[i].m_HookTarget = FindBestHookTarget(Core.m_Pos, Core.m_Vel);
		pPredictions[i].m_DesiredDir = CalculateEscapeDirection(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DangerLevel);
		pPredictions[i].m_CanUseHook = pPredictions[i].m_HookTarget.x != 0 || pPredictions[i].m_HookTarget.y != 0;
		pPredictions[i].m_ShouldJump = ShouldUseJump(Core.m_Pos, Core.m_Vel, pPredictions[i].m_DesiredDir);
	}
}

void CFujixGerosBot::SimulateCharacterCore(CCharacterCore *pCore, int Ticks)
{
	CGameClient *pGameClient = GetGameClient();
	if(!pCore || !pGameClient->m_GameWorld.m_pCollision)
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
	CGameClient *pGameClient = GetGameClient();
	if(!pGameClient || !pGameClient->m_GameWorld.m_pCollision)
		return false;
		
	// Check for collision with death tiles
	int TileIndex = pGameClient->m_GameWorld.m_pCollision->GetCollisionAt(Pos.x, Pos.y);
	if(TileIndex == TILE_DEATH)
		return true;
		
	// Check for freeze tiles (can be dangerous in certain situations)
	if(TileIndex == TILE_FREEZE || TileIndex == TILE_DFREEZE || TileIndex == TILE_LFREEZE)
		return true;
		
	// Check if falling into void
	if(Pos.y > pGameClient->m_GameWorld.m_pCollision->GetHeight() * 32.0f)
		return true;
		
	// Predict if current velocity will lead to death
	vec2 PredictedPos = Pos + Vel * 2.0f; // 2 ticks ahead
	int PredictedTile = pGameClient->m_GameWorld.m_pCollision->GetCollisionAt(PredictedPos.x, PredictedPos.y);
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
	CGameClient *pGameClient = GetGameClient();
	
	if(!pGameClient->m_GameWorld.m_pCollision)
		return DangerLevel;
	// Base danger from current tile
	int TileIndex = pGameClient->m_GameWorld.m_pCollision->GetCollisionAt(Pos.x, Pos.y);
	if(TileIndex == TILE_DEATH)
		DangerLevel += 10.0f;
		
	// Freeze tiles are also dangerous
	if(TileIndex == TILE_FREEZE || TileIndex == TILE_DFREEZE || TileIndex == TILE_LFREEZE)
		DangerLevel += 7.0f;
		
	// Velocity-based danger (high speed = higher danger)
	float Speed = length(Vel);
	DangerLevel += Speed * 0.01f;

	// Distance to nearest safe ground
	float DistanceToSafety = 999.0f;
	for(int x = -5; x <= 5; x++)
	{
		for(int y = -5; y <= 5; y++)
		{
			vec2 TestPos = Pos + vec2{x * 32.0f, y * 32.0f};
			int TestTile = pGameClient->m_GameWorld.m_pCollision->GetCollisionAt(TestPos.x, TestPos.y);
			if(TestTile != TILE_DEATH && TestTile != TILE_FREEZE && TestTile != TILE_DFREEZE && TestTile != TILE_LFREEZE && pGameClient->m_GameWorld.m_pCollision->CheckPoint(TestPos.x, TestPos.y + 16))
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
	CGameClient *pGameClient = GetGameClient();
	if(!pGameClient->m_GameWorld.m_pCollision)
		return vec2{0, 0};
		
	vec2 BestTarget = vec2{0, 0};
	float BestScore = -1.0f;
	float HookRange = 320.0f; // Maximum hook range
	
	// Search in multiple directions for hookable surfaces
	for(int angle = 0; angle < 360; angle += 15)
	{
		float rad = angle * 3.14159265f / 180.0f;
		vec2 Direction = vec2{cosf(rad), sinf(rad)};
		
		for(float distance = 32.0f; distance <= HookRange; distance += 16.0f)
		{
			vec2 TestPos = Pos + Direction * distance;
			
			// Check if this position is hookable
			if(pGameClient->m_GameWorld.m_pCollision->CheckPoint(TestPos.x, TestPos.y))
			{
				// Calculate score based on safety and reachability
				float Score = 0.0f;
				
				// Higher score for positions that get us further from danger
				if(!IsPositionDangerous(TestPos, vec2{0, 0}))
					Score += 5.0f;
					
				// Higher score for positions above us (easier to reach safety)
				if(TestPos.y < Pos.y)
					Score += 2.0f;
					
				// Lower score for very distant targets
				Score -= distance * 0.01f;
				
				// Check if we can actually reach safety from this hook point
				if(CanReachSafetyWithHook(Pos, TestPos))
					Score += 3.0f;
					
				if(Score > BestScore)
				{
					BestScore = Score;
					BestTarget = TestPos;
				}
				break; // Found a hookable surface in this direction
			}
		}
	}
	
	return BestTarget;
}

vec2 CFujixGerosBot::CalculateEscapeDirection(vec2 Pos, vec2 Vel, float DangerLevel)
{
	CGameClient *pGameClient = GetGameClient();
	if(!pGameClient->m_GameWorld.m_pCollision)
		return vec2{0, 0};
		
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
			
			// Higher score for directions that lead away from danger
			if(!IsPositionDangerous(TestPos, vec2{0, 0}))
				Score += 10.0f;
			else
				Score -= 5.0f;
				
			// Prefer upward movement when in danger
			if(DangerLevel > 3.0f && y < 0)
				Score += 3.0f;
			// Check if this direction has walkable ground
			vec2 GroundCheck = TestPos + vec2{0, 16};
			if(pGameClient->m_GameWorld.m_pCollision->CheckPoint(GroundCheck.x, GroundCheck.y))
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

bool CFujixGerosBot::CanReachSafetyWithHook(vec2 From, vec2 HookTarget)
{
	CGameClient *pGameClient = GetGameClient();
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
			if(pGameClient->m_GameWorld.m_pCollision->CheckPoint(GroundCheck.x, GroundCheck.y))
				return true;
		}
		
		// Stop if we hit a wall
		if(pGameClient->m_GameWorld.m_pCollision->CheckPoint(SimulatedPos.x, SimulatedPos.y))
			break;
	}
	
	return false;
}

bool CFujixGerosBot::ShouldUseJump(vec2 Pos, vec2 Vel, vec2 DesiredDir)
{
	CGameClient *pGameClient = GetGameClient();
	if(!pGameClient->m_GameWorld.m_pCollision)
		return false;
	// Jump if we need to go upward
	if(DesiredDir.y < -0.5f)
		return true;
		
	// Jump if there's an obstacle in front of us
	vec2 FrontCheck = Pos + vec2{DesiredDir.x * 32.0f, 0};
	if(pGameClient->m_GameWorld.m_pCollision->CheckPoint(FrontCheck.x, FrontCheck.y))
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
	// Check if any of the near-future predictions show imminent death
	for(int i = 0; i < minimum(4, GetPredictionTicks()); i++)
	{
		if(m_aPredictions[i].m_InDanger && m_aPredictions[i].m_TicksUntilDeath <= 3)
			return true;
	}
	
	// Check if danger level is critically high
	if(m_aPredictions[0].m_DangerLevel > 8.0f)
		return true;
		
	return false;
}

void CFujixGerosBot::ExecuteEmergencyRescue()
{
	CGameClient *pGameClient = GetGameClient();
	if(pGameClient->Client()->GameTick(0) - m_LastRescueTick < 5) // Prevent spam rescues
		return;

	m_EmergencyMode = true;
	m_RescueAttempts++;
	m_LastRescueTick = pGameClient->Client()->GameTick(0);
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

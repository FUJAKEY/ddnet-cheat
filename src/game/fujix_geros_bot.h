#ifndef GAME_CLIENT_FUJIX_GEROS_BOT_H
#define GAME_CLIENT_FUJIX_GEROS_BOT_H

// Simple vector2 structure with operators
struct vec2
{
	float x, y;
	vec2() : x(0), y(0) {}
	vec2(float x_, float y_) : x(x_), y(y_) {}
	
	vec2 operator+(const vec2& other) const { return vec2(x + other.x, y + other.y); }
	vec2 operator-(const vec2& other) const { return vec2(x - other.x, y - other.y); }
	vec2 operator*(float f) const { return vec2(x * f, y * f); }
	vec2& operator+=(const vec2& other) { x += other.x; y += other.y; return *this; }
};

// Mock structures
struct CCharacterCore { vec2 m_Pos, m_Vel; void Tick(bool, bool) {} void Move() {} void Quantize() {} };
struct CCollision { 
	int GetCollisionAt(float, float) { return 0; } 
	bool CheckPoint(float, float) { return true; }
	float GetHeight() { return 1000.0f; }
};
struct CGameWorld { CCollision *m_pCollision = new CCollision(); };
struct CClient { int GameTick(int) { return 0; } };
class CGameClient { 
public:
	CGameWorld m_GameWorld; 
	CCharacterCore m_PredictedChar;
	CClient* Client() { return &m_Client; }
private:
	CClient m_Client;
};

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

class CFujixGerosBot
{
private:
	CGameClient *GetGameClient();
	void PredictMovement(SGerosBotPrediction *pPredictions, int NumTicks);
	void SimulateCharacterCore(CCharacterCore *pCore, int Ticks);
	bool IsPositionDangerous(vec2 Pos, vec2 Vel);
	float CalculateDangerLevel(vec2 Pos, vec2 Vel);
	
	vec2 FindBestHookTarget(vec2 Pos, vec2 Vel);
	vec2 CalculateEscapeDirection(vec2 Pos, vec2 Vel, float DangerLevel);
	bool CanReachSafetyWithHook(vec2 From, vec2 HookTarget);
	bool ShouldUseJump(vec2 Pos, vec2 Vel, vec2 DesiredDir);
	
	bool DetectSuicideAttempt(vec2 Pos, vec2 Vel, int InputDirection);
	bool IsPlayerTryingToKillThemselves();
	
	SGerosBotPrediction m_aPredictions[16];
	int m_LastPredictionTick;
	int m_RescueAttempts;
	int m_LastRescueTick;
	float m_PlayerTrustLevel;
	bool m_EmergencyMode;
	bool m_FullPredictionMode;
	int m_LastFullPredictionTick;

public:
	CFujixGerosBot();
	
	void OnInit();
	void OnRender();
	void OnMessage(int MsgType, void *pRawMsg);
	
	void Update();
	bool IsActive() const;
	bool ShouldOverrideInput();
	void GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
	
	void ExecuteEmergencyRescue();
	bool IsInEmergencyState();
	
	int GetAggressiveness() const;
	int GetPredictionTicks() const;
	bool IsAntiSuicideEnabled() const;
};

#endif

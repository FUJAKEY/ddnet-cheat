#ifndef GAME_CLIENT_FUJIX_GEROS_BOT_H
#define GAME_CLIENT_FUJIX_GEROS_BOT_H

// vec2 structure  
struct vec2 { 
	float x, y; 
	vec2() : x(0), y(0) {}
	vec2(float x_, float y_) : x(x_), y(y_) {}
};

// Forward declarations
class CCharacterCore;
class CGameClient;

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
	// Helper method
	CGameClient *GetGameClient();
	
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

public:
	CFujixGerosBot();
	
	int Sizeof() const { return sizeof(*this); }
	void OnInit();
	void OnRender();
	void OnMessage(int MsgType, void *pRawMsg);
	
	// Main bot functions
	void Update();
	bool IsActive() const;
	bool ShouldOverrideInput();
	void GetBotInput(int *pInputDirection, int *pJump, int *pHook, vec2 *pTargetX);
	
	// Emergency rescue system
	void ExecuteEmergencyRescue();
	bool IsInEmergencyState();
	
	// Configuration
	int GetAggressiveness() const;
	int GetPredictionTicks() const;
	bool IsAntiSuicideEnabled() const;
};

#endif

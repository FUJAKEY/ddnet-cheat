/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_FUJIX_GORES_BOT_H
#define GAME_CLIENT_FUJIX_GORES_BOT_H

#include <game/client/prediction/entities/character.h>
#include <engine/shared/config.h>

class CFujixGoresBot
{
private:
       class CGameClient *m_pGameClient;
       class CCollision *m_pCollision;

       // AI prediction parameters
       static const int PREDICTION_TICKS = 9;
       static const int SAFETY_DISTANCE_TICKS = 2;

       // internal state for delayed blocking
       int m_BlockDirection;
       int m_TicksUntilBlock;
	
public:
	CFujixGoresBot();
	void Init(class CGameClient *pGameClient, class CCollision *pCollision);
	
	// Main AI function - modifies input to prevent freeze tile collisions
       void ProcessPlayerInput(CNetObj_PlayerInput *pInput, class CCharacter *pCharacter);
	
private:
	// Prediction functions
       bool WillHitFreezeTile(class CCharacter *pCharacter, vec2 velocity, int ticks);
       int PredictFreezeTick(vec2 pos, vec2 vel, int direction, int maxTicks);
       vec2 PredictPosition(vec2 currentPos, vec2 velocity, int direction, int ticks);
       bool IsFreezeTile(vec2 position);
};

#endif
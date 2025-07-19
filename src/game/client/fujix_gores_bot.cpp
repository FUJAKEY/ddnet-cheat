/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "fujix_gores_bot.h"
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/mapitems.h>
#include <base/math.h>

CFujixGoresBot::CFujixGoresBot()
{
	m_pGameClient = nullptr;
	m_pCollision = nullptr;
}

void CFujixGoresBot::Init(CGameClient *pGameClient, CCollision *pCollision)
{
	m_pGameClient = pGameClient;
	m_pCollision = pCollision;
}

void CFujixGoresBot::ProcessPlayerInput(CNetObj_PlayerInput *pInput, CCharacter *pCharacter)
{
	if(!g_Config.m_FujixGoresBot || !pInput || !pCharacter || !m_pCollision)
		return;
	
	// Get current character state
       vec2 currentPos = pCharacter->Core()->m_Pos;
       vec2 currentVel = pCharacter->Core()->m_Vel;
	
	// Calculate intended velocity based on current input
	vec2 intendedVel = currentVel;
	if(pInput->m_Direction != 0)
	{
		// Simulate how direction input affects velocity
		float acceleration = 2.0f; // Approximate ground acceleration
		intendedVel.x += pInput->m_Direction * acceleration;
	}
	
       int FreezeTick = 0;
       // Check if player will hit freeze tile in the next PREDICTION_TICKS
       if(WillHitFreezeTile(pCharacter, intendedVel, pInput->m_Direction, PREDICTION_TICKS, &FreezeTick))
       {
               int SafeTick = FreezeTick - SAFETY_DISTANCE_TICKS;
               if(SafeTick > 0)
               {
                       vec2 safePos = PredictPosition(currentPos, intendedVel, SafeTick);
                       if(!IsFreezeTile(safePos))
                       {
                               pInput->m_Direction = 0;
                       }
               }
       }
}

bool CFujixGoresBot::WillHitFreezeTile(CCharacter *pCharacter, vec2 velocity, int direction, int ticks, int *pFreezeTick)
{
       vec2 currentPos = pCharacter->Core()->m_Pos;
       for(int i = 1; i <= ticks; i++)
       {
               velocity.x += direction * 2.0f;
               currentPos += velocity * (1.0f / 50.0f);
               velocity.x *= 0.5f;
               velocity.y += 0.75f;
               if(IsFreezeTile(currentPos))
               {
                       if(pFreezeTick)
                               *pFreezeTick = i;
                       return true;
               }
       }
       return false;
}

int CFujixGoresBot::PredictFreezeTick(vec2 currentPos, vec2 velocity, int direction, int ticks)
{
       for(int i = 1; i <= ticks; i++)
       {
               velocity.x += direction * 2.0f;
               currentPos += velocity * (1.0f / 50.0f);
               velocity.x *= 0.5f;
               velocity.y += 0.75f;
               if(IsFreezeTile(currentPos))
                       return i;
       }
       return -1;
}

vec2 CFujixGoresBot::PredictPosition(vec2 currentPos, vec2 velocity, int ticks)
{
	// Simple prediction: position = current + velocity * time
	// This is a simplified version - real DDNet physics are more complex
	vec2 predictedPos = currentPos;
	
	for(int i = 0; i < ticks; i++)
	{
		predictedPos += velocity * (1.0f / 50.0f); // 50 ticks per second
		
		// Apply simple physics (friction, gravity)
		velocity.x *= 0.5f; // Simple ground friction
		velocity.y += 0.75f; // Gravity
	}
	
	return predictedPos;
}

bool CFujixGoresBot::IsFreezeTile(vec2 position)
{
	if(!m_pCollision)
		return false;
	
	int mapIndex = m_pCollision->GetPureMapIndex(position);
	if(mapIndex < 0)
		return false;
	
	int tileIndex = m_pCollision->GetTileIndex(mapIndex);
	int frontTileIndex = m_pCollision->GetFrontTileIndex(mapIndex);
	
	// Check for freeze tiles
	return (tileIndex == TILE_FREEZE || 
	        tileIndex == TILE_DFREEZE || 
	        tileIndex == TILE_LFREEZE ||
	        frontTileIndex == TILE_FREEZE || 
	        frontTileIndex == TILE_DFREEZE || 
	        frontTileIndex == TILE_LFREEZE);
}

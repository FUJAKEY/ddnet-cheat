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
	
	// Check if player will hit freeze tile in the next PREDICTION_TICKS
	if(WillHitFreezeTile(pCharacter, intendedVel, PREDICTION_TICKS))
	{
		// Check if stopping SAFETY_DISTANCE_TICKS before would be safe
		int safeStopTick = PREDICTION_TICKS - SAFETY_DISTANCE_TICKS;
		if(safeStopTick > 0)
		{
			vec2 safePos = PredictPosition(currentPos, intendedVel, safeStopTick);
			
			// If we can safely stop before the freeze tile, block the direction input
			if(!IsFreezeTile(safePos))
			{
				// Block movement in the dangerous direction
				int dangerousDirection = GetDirectionToAvoid(pCharacter, intendedVel);
				if(pInput->m_Direction == dangerousDirection)
				{
					pInput->m_Direction = 0; // Stop moving in that direction
				}
			}
		}
	}
}

bool CFujixGoresBot::WillHitFreezeTile(CCharacter *pCharacter, vec2 velocity, int ticks)
{
       vec2 currentPos = pCharacter->Core()->m_Pos;
	
	// Simulate movement for each tick
	for(int i = 1; i <= ticks; i++)
	{
		vec2 predictedPos = PredictPosition(currentPos, velocity, i);
		if(IsFreezeTile(predictedPos))
		{
			return true;
		}
	}
	
	return false;
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

int CFujixGoresBot::GetDirectionToAvoid(CCharacter *pCharacter, vec2 velocity)
{
	// Return the direction that would lead to danger
	if(velocity.x > 0)
		return 1;  // Moving right is dangerous
	else if(velocity.x < 0)
		return -1; // Moving left is dangerous
	
	return 0; // No horizontal movement
}
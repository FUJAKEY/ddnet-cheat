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
       m_BlockDirection = 0;
       m_TicksUntilBlock = 0;
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

       // handle delayed block from previous prediction
       if(m_TicksUntilBlock > 0)
       {
               m_TicksUntilBlock--;
               if(m_TicksUntilBlock == 0 && pInput->m_Direction == m_BlockDirection)
               {
                       pInput->m_Direction = 0;
                       m_BlockDirection = 0;
               }
       }

       vec2 pos = pCharacter->Core()->m_Pos;
       vec2 vel = pCharacter->Core()->m_Vel;
       int direction = pInput->m_Direction;

       int freezeTick = PredictFreezeTick(pos, vel, direction, PREDICTION_TICKS);
       if(freezeTick > 0)
       {
               int stopTick = freezeTick - SAFETY_DISTANCE_TICKS;
               if(stopTick <= 0)
               {
                       if(direction != 0)
                       {
                               pInput->m_Direction = 0;
                               m_BlockDirection = 0;
                               m_TicksUntilBlock = 0;
                       }
               }
               else if(m_TicksUntilBlock == 0)
               {
                       m_TicksUntilBlock = stopTick;
                       m_BlockDirection = direction;
               }
       }
}

bool CFujixGoresBot::WillHitFreezeTile(CCharacter *pCharacter, vec2 velocity, int ticks)
{
       return PredictFreezeTick(pCharacter->Core()->m_Pos, velocity, 0, ticks) > 0;
}

int CFujixGoresBot::PredictFreezeTick(vec2 pos, vec2 vel, int direction, int maxTicks)
{
       for(int i = 1; i <= maxTicks; i++)
       {
               if(direction != 0)
               {
                       float acceleration = 2.0f;
                       vel.x += direction * acceleration;
               }

               pos += vel * (1.0f / 50.0f);

               vel.x *= 0.5f;
               vel.y += 0.75f;

               if(IsFreezeTile(pos))
                       return i;
       }

       return -1;
}

vec2 CFujixGoresBot::PredictPosition(vec2 currentPos, vec2 velocity, int direction, int ticks)
{
	// Simple prediction: position = current + velocity * time
	// This is a simplified version - real DDNet physics are more complex
	vec2 predictedPos = currentPos;
	
       for(int i = 0; i < ticks; i++)
       {
               if(direction != 0)
               {
                       float acceleration = 2.0f;
                       velocity.x += direction * acceleration;
               }

               predictedPos += velocity * (1.0f / 50.0f);

               velocity.x *= 0.5f;
               velocity.y += 0.75f;
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

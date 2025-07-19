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
       m_BlockActive = false;
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

       // countdown to block if scheduled
       if(m_TicksUntilBlock > 0)
       {
               m_TicksUntilBlock--;
               if(m_TicksUntilBlock == 0)
               {
                       m_BlockActive = true;
               }
       }

       // if actively blocking, ensure we keep the player safe
       if(m_BlockActive)
       {
               int freeze = PredictFreezeTick(pCharacter, m_BlockDirection, PREDICTION_TICKS);
               if(freeze > 0 && freeze <= SAFETY_DISTANCE_TICKS)
               {
                       if(pInput->m_Direction == m_BlockDirection)
                               pInput->m_Direction = 0;
               }
               else
               {
                       m_BlockActive = false;
                       m_BlockDirection = 0;
               }
       }

       // evaluate new prediction if not already blocking or waiting
       if(!m_BlockActive && m_TicksUntilBlock == 0)
       {
               int direction = pInput->m_Direction;
               int freezeTick = PredictFreezeTick(pCharacter, direction, PREDICTION_TICKS);
               if(freezeTick > 0)
               {
                       int stopTick = freezeTick - SAFETY_DISTANCE_TICKS;
                       if(stopTick <= 0)
                       {
                               if(direction != 0)
                               {
                                       pInput->m_Direction = 0;
                                       m_BlockDirection = direction;
                                       m_BlockActive = true;
                               }
                       }
                       else
                       {
                               m_TicksUntilBlock = stopTick;
                               m_BlockDirection = direction;
                       }
               }
       }
}

bool CFujixGoresBot::WillHitFreezeTile(CCharacter *pCharacter, vec2 velocity, int ticks)
{
       if(!pCharacter)
               return false;

       CCharacterCore TmpCore = *pCharacter->Core();
       TmpCore.SetCoreWorld(nullptr, m_pCollision, nullptr);
       TmpCore.m_Vel = velocity;
       for(int i = 0; i < ticks; i++)
       {
               TmpCore.m_Input.m_Direction = 0;
               TmpCore.Tick(true);
               TmpCore.Move();
               TmpCore.Quantize();
               if(IsFreezeTile(TmpCore.m_Pos))
                       return true;
       }
       return false;
}

int CFujixGoresBot::PredictFreezeTick(CCharacter *pCharacter, int direction, int maxTicks)
{
       if(!pCharacter)
               return -1;

       CCharacterCore Core = *pCharacter->Core();
       Core.SetCoreWorld(nullptr, m_pCollision, nullptr);

       for(int i = 1; i <= maxTicks; i++)
       {
               Core.m_Input.m_Direction = direction;
               Core.Tick(true);
               Core.Move();
               Core.Quantize();

               if(IsFreezeTile(Core.m_Pos))
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

#include "goresbot.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/collision.h>

void CGoresBot::PredictMovement(CCharacterCore *pChar, int Ticks)
{
	CCharacterCore TempCore = *pChar;
	for(int i = 0; i < Ticks; ++i)
	{
		TempCore.Tick(false);
		TempCore.Move();
	}
	*pChar = TempCore;
}

void CGoresBot::OnRender()
{
	if(!g_Config.m_ClGoresBot)
		return;

	if(m_pClient->m_Snap.m_pLocalCharacter)
	{
		CCharacterCore OriginalChar = m_pClient->m_PredictedChar;
		CCharacterCore PredictedChar = OriginalChar;
		PredictMovement(&PredictedChar, 9);

		int Tile = m_pClient->Collision()->GetTile(PredictedChar.m_Pos.x, PredictedChar.m_Pos.y);
		if(m_pClient->Collision()->IsTileSolid(PredictedChar.m_Pos.x, PredictedChar.m_Pos.y) && m_pClient->Collision()->GetCollisionMap()[Tile].m_Index == TILE_FREEZE)
		{
			// Analyze surroundings for safe spots
			vec2 BestHookPos = vec2(0,0);
			float BestHookRisk = 10000.0f;

			for(int y = -10; y < 10; ++y)
			{
				for(int x = -10; x < 10; ++x)
				{
					vec2 TilePos = vec2(OriginalChar.m_Pos.x + x * 32, OriginalChar.m_Pos.y + y * 32);
					if(m_pClient->Collision()->IsTileSolid(TilePos.x, TilePos.y) && m_pClient->Collision()->GetCollisionMap()[m_pClient->Collision()->GetTile(TilePos.x, TilePos.y)].m_Index != TILE_FREEZE)
					{
						CCharacterCore HookChar = OriginalChar;
						HookChar.m_Input.m_Hook = 1;
						HookChar.m_Input.m_TargetX = TilePos.x - OriginalChar.m_Pos.x;
						HookChar.m_Input.m_TargetY = TilePos.y - OriginalChar.m_Pos.y;

						float Risk = 0.0f;
						for(int Tick = 0; Tick < 100; ++Tick)
						{
							PredictMovement(&HookChar, 1);
							int HookTile = m_pClient->Collision()->GetTile(HookChar.m_Pos.x, HookChar.m_Pos.y);
							if(m_pClient->Collision()->IsTileSolid(HookChar.m_Pos.x, HookChar.m_Pos.y) && m_pClient->Collision()->GetCollisionMap()[HookTile].m_Index == TILE_FREEZE)
							{
								Risk += 1.0f;
							}
						}

						if(Risk < BestHookRisk)
						{
							BestHookRisk = Risk;
							BestHookPos = TilePos;
						}
					}
				}
			}

			if(BestHookRisk < 1.0f)
			{
				m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Hook = 1;
				m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX = BestHookPos.x - OriginalChar.m_Pos.x;
				m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY = BestHookPos.y - OriginalChar.m_Pos.y;
			}
			else
			{
				m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction = 0;
			}
		}
	}
}

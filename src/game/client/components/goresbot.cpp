#include "goresbot.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>

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
		CCharacterCore PredictedChar = m_pClient->m_PredictedChar;
		PredictMovement(&PredictedChar, 9);

		int Tile = m_pClient->Collision()->GetTile(PredictedChar.m_Pos.x, PredictedChar.m_Pos.y);
		if(m_pClient->Collision()->IsTileSolid(PredictedChar.m_Pos.x, PredictedChar.m_Pos.y) && m_pClient->Collision()->GetCollisionMap()[Tile].m_Index == TILE_FREEZE)
		{
			// Block input to stop the player
			m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction = 0;

			// Check if we need to hook
			CCharacterCore HookChar = m_pClient->m_PredictedChar;
			HookChar.m_Input.m_Hook = 1;
			PredictMovement(&HookChar, 9);
			int HookTile = m_pClient->Collision()->GetTile(HookChar.m_Pos.x, HookChar.m_Pos.y);
			if(!m_pClient->Collision()->IsTileSolid(HookChar.m_Pos.x, HookChar.m_Pos.y) || m_pClient->Collision()->GetCollisionMap()[HookTile].m_Index != TILE_FREEZE)
			{
				m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Hook = 1;
			}
		}
	}
}

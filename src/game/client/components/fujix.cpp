#include "fujix.h"

#include <base/math.h>
#include <cmath>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/projectile.h>
#include <game/collision.h>
#include <game/mapitems.h>

float CFujix::FreezeMargin(vec2 Start, vec2 End) const
{
	const float Step = 16.0f;
	float Margin = 1e9f;
	vec2 Dir = End - Start;
	float Len = length(Dir);
	if(Len <= 0.0f)
		return -1.0f;
	Dir /= Len;
	for(float d = 0.0f; d <= Len; d += Step)
	{
		vec2 Pos = Start + Dir * d;
		int Idx = Collision()->GetMapIndex(Pos);
		int Tile = Collision()->GetTileIndex(Idx);
		if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE)
			return -1.0f;

		for(int ox = -1; ox <= 1; ++ox)
			for(int oy = -1; oy <= 1; ++oy)
			{
				vec2 Off = Pos + vec2(ox * 32.0f, oy * 32.0f);
				int NIdx = Collision()->GetMapIndex(Off);
				int NTile = Collision()->GetTileIndex(NIdx);
				if(NTile == TILE_FREEZE || NTile == TILE_DFREEZE || NTile == TILE_LFREEZE)
				{
					float Dist = distance(Pos, Collision()->GetPos(NIdx));
					if(Dist < Margin)
						Margin = Dist;
				}
			}
	}
	return Margin;
}

bool CFujix::PathBlocked(vec2 Start, vec2 End) const
{
	vec2 Dummy;
	if(GameClient()->IntersectCharacter(Start, End, Dummy, GameClient()->m_Snap.m_LocalClientId) != -1)
		return true;

	for(CProjectile *pProj = (CProjectile *)GameClient()->m_PredictedWorld.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj; pProj = (CProjectile *)pProj->TypeNext())
	{
		float T = 1.0f / (float)SERVER_TICK_SPEED;
		vec2 P0 = pProj->m_Pos;
		vec2 P1 = pProj->GetPos(T);
		vec2 Inter;
		if(closest_point_on_line(P0, P1, Start, Inter) && distance(Inter, Start) < distance(P1, P0))
		{
			if(distance(Inter, Start) <= distance(Start, End) && distance(Inter, End) <= distance(Start, End))
				return true;
		}
	}
	return false;
}

void CFujix::OnUpdate()
{
	if(!g_Config.m_ClAvoidanceFreeze)
		return;

	if(!GameClient()->m_Snap.m_pLocalCharacter)
		return;

       int DummyID = g_Config.m_ClDummy;

       if(m_RehookDelay > 0)
       {
               GameClient()->m_Controls.m_aInputData[DummyID].m_Hook = 0;
               m_RehookDelay--;
               if(m_RehookDelay == 0)
               {
                       vec2 Mouse = m_NextHookPos - GameClient()->m_PredictedChar.m_Pos;
                       GameClient()->m_Controls.m_aMousePos[DummyID] = Mouse;
                       GameClient()->m_Controls.ClampMousePos();
                       GameClient()->m_Controls.m_aInputData[DummyID].m_TargetX = (int)GameClient()->m_Controls.m_aMousePos[DummyID].x;
			GameClient()->m_Controls.m_aInputData[DummyID].m_TargetY = (int)GameClient()->m_Controls.m_aMousePos[DummyID].y;
			GameClient()->m_Controls.m_aInputData[DummyID].m_Hook = 1;
			m_HookActive = true;
               }
               return;
       }

	CCharacterCore Char = GameClient()->m_PredictedChar;
	int PredictTicks = clamp(g_Config.m_ClAvoidanceFreezePredict, 2, 20);
	for(int i = 0; i < PredictTicks; ++i)
	{
		Char.Tick(false);
		Char.Move();
		Char.Quantize();
	}

	vec2 NextPos = Char.m_Pos;
	int Index = Collision()->GetMapIndex(NextPos);
	int Tile = Collision()->GetTileIndex(Index);
	if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE)
	{
	               GameClient()->m_Controls.m_aInputData[DummyID].m_Hook = 0;
	
	               const float HookLength = GameClient()->m_aTuning[DummyID].m_HookLength * 0.95f;
			const int Steps = 64;
			float BestMargin = -1.0f;
			vec2 BestColl = Char.m_Pos;
			for(int i = 0; i < Steps; ++i)
			{
				float Angle = (2.0f * pi * i) / Steps;
				vec2 Dir = vec2(cosf(Angle), sinf(Angle));
				vec2 Target = Char.m_Pos + Dir * HookLength;
				vec2 Coll, Before;
				if(!Collision()->IntersectLine(Char.m_Pos, Target, &Coll, &Before))
					continue;
	
				int HitIdx = Collision()->GetMapIndex(Coll);
				int HitTile = Collision()->GetTileIndex(HitIdx);
				if(HitTile == TILE_FREEZE || HitTile == TILE_DFREEZE || HitTile == TILE_LFREEZE)
					continue;
	
				if(PathBlocked(Char.m_Pos, Coll))
					continue;
	
				float Margin = FreezeMargin(Char.m_Pos, Coll);
				if(Margin <= 0.0f)
					continue;
	
				if(Margin > BestMargin)
				{
					BestMargin = Margin;
					BestColl = Coll;
				}
			}
	
	       if(BestMargin > 0.0f)
	       {
	               m_NextHookPos = BestColl;
	               m_RehookDelay = 2;
	               m_HookActive = true;
	       }
	}
	else if(m_HookActive)
	{
	CCharacterCore Sim = GameClient()->m_PredictedChar;
	Sim.m_Input = GameClient()->m_PredictedChar.m_Input;
	Sim.m_Input.m_Hook = 0;
	for(int i = 0; i < PredictTicks; ++i)
	{
	Sim.Tick(true);
	Sim.Move();
	Sim.Quantize();
	}
	int SafeIdx = Collision()->GetMapIndex(Sim.m_Pos);
	int SafeTile = Collision()->GetTileIndex(SafeIdx);
	if(SafeTile != TILE_FREEZE && SafeTile != TILE_DFREEZE && SafeTile != TILE_LFREEZE)
	{
	GameClient()->m_Controls.m_aInputData[DummyID].m_Hook = 0;
	m_HookActive = false;
	}
	}
	}

#include "fujix.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/mapitems.h>

void CFujix::OnUpdate()
{
    if(!g_Config.m_ClFujixFreeze)
    {
        m_HookTicks = 0;
        return;
    }

    CCharacter *pChar = m_pClient->m_PredictedWorld.GetCharacterById(m_pClient->m_Snap.m_LocalClientId);
    if(!pChar)
        return;

    CCharacterCore Core = *pChar->Core();
    bool PredFreeze = false;
    for(int i = 0; i < 9 && !PredFreeze; ++i)
    {
        Core.Tick(true, false);
        Core.Move();
        int Tile = Collision()->GetCollisionAt(Core.m_Pos.x, Core.m_Pos.y);
        PredFreeze = Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE;
    }

    if(PredFreeze)
    {
        // search a hookable direction, prefer upwards
        static const vec2 aDirs[] = {
            vec2(0.f, -1.f),
            vec2(-1.f, -1.f),
            vec2(1.f, -1.f),
            vec2(-1.f, 0.f),
            vec2(1.f, 0.f)
        };

        vec2 HookDir = vec2(0.f, -1.f);
        vec2 ColPos, Before;
        for(auto Dir : aDirs)
        {
            if(Collision()->IntersectLineTeleHook(Core.m_Pos, Core.m_Pos + Dir * 380.f, &ColPos, &Before) != 0)
            {
                int Tile = Collision()->GetCollisionAt(round_to_int(ColPos.x), round_to_int(ColPos.y));
                if(Tile != TILE_NOHOOK && Tile != TILE_FREEZE && Tile != TILE_DFREEZE && Tile != TILE_LFREEZE)
                {
                    HookDir = Dir;
                    break;
                }
            }
        }

        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)(HookDir.x * 100);
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)(HookDir.y * 100);
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Hook = 1;
        m_HookTicks++;
        if(m_HookTicks > 5)
        {
            m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Hook = 0;
            m_HookTicks = 0;
        }
    }
    else
    {
        m_HookTicks = 0;
    }
}


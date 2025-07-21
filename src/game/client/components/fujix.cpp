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
    for(int i = 0; i < 9; ++i)
    {
        Core.Tick(true, false);
        Core.Move();
    }

    int Tile = Collision()->GetCollisionAt(Core.m_Pos.x, Core.m_Pos.y);
    bool PredFreeze = Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE;

    if(PredFreeze)
    {
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Hook = 1;
        m_HookTicks++;
        if(m_HookTicks > 3)
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


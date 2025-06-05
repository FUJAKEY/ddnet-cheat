#include "fujix_autoplay.h"
#include <game/client/gameclient.h>
#include <game/client/components/controls.h>
#include <game/client/components/camera.h>
#include <game/collision.h>
#include <game/mapitems.h>
#include <queue>
#include <algorithm>

std::vector<int> CFujixAutoplay::FindPath(int StartIndex, int FinishIndex)
{
    int Width = Collision()->GetWidth();
    int Height = Collision()->GetHeight();
    std::vector<int> Came(Width * Height, -1);
    std::queue<int> Q;
    Q.push(StartIndex);
    Came[StartIndex] = StartIndex;
    static const int DIRS[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    while(!Q.empty())
    {
        int Cur = Q.front();
        Q.pop();
        if(Cur == FinishIndex)
            break;
        int X = Cur % Width;
        int Y = Cur / Width;
        for(const auto &Dir : DIRS)
        {
            int Nx = X + Dir[0];
            int Ny = Y + Dir[1];
            if(Nx < 0 || Ny < 0 || Nx >= Width || Ny >= Height)
                continue;
            int N = Ny * Width + Nx;
            if(Came[N] != -1)
                continue;
            if(Collision()->GetCollisionAt(Nx * 32 + 16, Ny * 32 + 16))
                continue;
            Came[N] = Cur;
            Q.push(N);
        }
    }
    std::vector<int> Path;
    if(Came[FinishIndex] == -1)
        return Path;
    for(int At = FinishIndex; At != StartIndex; At = Came[At])
        Path.push_back(At);
    Path.push_back(StartIndex);
    std::reverse(Path.begin(), Path.end());
    return Path;
}

void CFujixAutoplay::OnMapLoad()
{
    m_Path.clear();
    m_Current = 0;
    int Width = Collision()->GetWidth();
    int Height = Collision()->GetHeight();
    int FinishIndex = -1;
    for(int i = 0; i < Width * Height; i++)
    {
        if(Collision()->GetTileIndex(i) == TILE_FINISH)
        {
            FinishIndex = i;
            break;
        }
    }
    if(FinishIndex >= 0)
        m_FinishPos = Collision()->GetPos(FinishIndex);
    else
        m_FinishPos = vec2(0,0);
}

void CFujixAutoplay::OnUpdate()
{
    if(!g_Config.m_ClFujixAutoplay || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    vec2 Pos = GameClient()->m_LocalCharacterPos;
    int StartIndex = Collision()->GetPureMapIndex(Pos);
    int FinishIndex = Collision()->GetPureMapIndex(m_FinishPos);
    if(m_Path.empty())
    {
        std::vector<int> PathIdx = FindPath(StartIndex, FinishIndex);
        for(int idx : PathIdx)
            m_Path.push_back(Collision()->GetPos(idx));
        m_Current = 0;
    }

    if(m_Path.empty() || m_Current >= (int)m_Path.size())
        return;

    vec2 Target = m_Path[m_Current];
    if(distance(Pos, Target) < 16.0f)
    {
        m_Current++;
        if(m_Current >= (int)m_Path.size())
            return;
        Target = m_Path[m_Current];
    }

    CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
    Input.m_Direction = 0;
    if(Target.x > Pos.x + 2.0f)
        Input.m_Direction = 1;
    else if(Target.x < Pos.x - 2.0f)
        Input.m_Direction = -1;

    if(Target.y + 2.0f < Pos.y)
        Input.m_Jump = 1;
    else
        Input.m_Jump = 0;

    Input.m_TargetX = (int)(Target.x - Pos.x);
    Input.m_TargetY = (int)(Target.y - Pos.y);
}

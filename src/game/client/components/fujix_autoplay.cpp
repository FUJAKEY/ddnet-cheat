#include "fujix_autoplay.h"
#include <game/client/gameclient.h>
#include <game/client/components/controls.h>
#include <game/client/components/camera.h>
#include <base/math.h>
#include <game/collision.h>
#include <game/mapitems.h>
#include <queue>
#include <algorithm>
#include <limits>

int CFujixAutoplay::Heuristic(int Index, int FinishIndex) const
{
    int Width = Collision()->GetWidth();
    int X = Index % Width;
    int Y = Index / Width;
    int FX = FinishIndex % Width;
    int FY = FinishIndex / Width;
    return absolute(X - FX) + absolute(Y - FY);
}

std::vector<int> CFujixAutoplay::FindPath(int StartIndex, int FinishIndex)
{
    int Width = Collision()->GetWidth();
    int Height = Collision()->GetHeight();

    std::vector<int> Came(Width * Height, -1);
    std::vector<int> GScore(Width * Height, std::numeric_limits<int>::max());

    struct Node
    {
        int m_Index;
        int m_FScore;
        bool operator<(const Node &Other) const { return m_FScore > Other.m_FScore; }
    };

    std::priority_queue<Node> Open;
    GScore[StartIndex] = 0;
    Open.push({StartIndex, Heuristic(StartIndex, FinishIndex)});
    Came[StartIndex] = StartIndex;

    static const int DIRS[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    const int HOOK_RANGE_TILES = 11;
    const int MAX_ITER = 5000;
    int Iter = 0;

    while(!Open.empty() && Iter < MAX_ITER)
    {
        Iter++;
        int Cur = Open.top().m_Index;
        if(Cur == FinishIndex)
            break;
        Open.pop();

        int X = Cur % Width;
        int Y = Cur / Width;

        // walk neighbors
        for(const auto &Dir : DIRS)
        {
            int Nx = X + Dir[0];
            int Ny = Y + Dir[1];
            if(Nx < 0 || Ny < 0 || Nx >= Width || Ny >= Height)
                continue;
            int N = Ny * Width + Nx;
            if(Collision()->GetCollisionAt(Nx * 32 + 16, Ny * 32 + 16))
                continue;
            int Tentative = GScore[Cur] + 1;
            if(Tentative < GScore[N])
            {
                Came[N] = Cur;
                GScore[N] = Tentative;
                Open.push({N, Tentative + Heuristic(N, FinishIndex)});
            }
        }

        // hook neighbors (limited directions)
        vec2 CurPos = Collision()->GetPos(Cur);
        static const int DIR_COUNT = 8;
        static const int HDIRS[DIR_COUNT][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for(const auto &HDir : HDIRS)
        {
            int Nx = X + HDir[0] * HOOK_RANGE_TILES;
            int Ny = Y + HDir[1] * HOOK_RANGE_TILES;
            if(Nx < 0 || Ny < 0 || Nx >= Width || Ny >= Height)
                continue;
            int N = Ny * Width + Nx;
            if(Collision()->GetCollisionAt(Nx * 32 + 16, Ny * 32 + 16))
                continue;
            vec2 Pos = Collision()->GetPos(N);
            if(distance(Pos, CurPos) > 380.0f)
                continue;
            if(Collision()->IntersectLineTeleHook(CurPos, Pos, nullptr, nullptr) != 0)
                continue;
            int Tentative = GScore[Cur] + 2; // hooking costs more
            if(Tentative < GScore[N])
            {
                Came[N] = Cur;
                GScore[N] = Tentative;
                Open.push({N, Tentative + Heuristic(N, FinishIndex)});
            }
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
    m_Hooking = false;
    m_HookTicks = 0;
    m_LastPathTick = 0;
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
    {
        m_FinishPos = Collision()->GetPos(FinishIndex);
        m_HaveFinish = true;
    }
    else
    {
        m_FinishPos = vec2(0,0);
        m_HaveFinish = false;
    }
}

void CFujixAutoplay::OnUpdate()
{
    if(!g_Config.m_ClFujixAutoplay || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    if(!m_HaveFinish)
        return;

    vec2 Pos = GameClient()->m_LocalCharacterPos;
    int StartIndex = Collision()->GetPureMapIndex(Pos);
    int FinishIndex = Collision()->GetPureMapIndex(m_FinishPos);

    int Now = Client()->GameTick(g_Config.m_ClDummy);
    if((m_Path.empty() || m_Current >= (int)m_Path.size()) && Now - m_LastPathTick > 30)
    {
        std::vector<int> PathIdx = FindPath(StartIndex, FinishIndex);
        m_Path.clear();
        for(int idx : PathIdx)
            m_Path.push_back(Collision()->GetPos(idx));
        m_Current = 0;
        m_LastPathTick = Now;
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
    else if(distance(Pos, Target) > 160.0f && Now - m_LastPathTick > 30)
    {
        // path likely invalid, recompute
        std::vector<int> PathIdx = FindPath(StartIndex, FinishIndex);
        m_Path.clear();
        for(int idx : PathIdx)
            m_Path.push_back(Collision()->GetPos(idx));
        m_Current = 0;
        m_LastPathTick = Now;
        if(m_Path.empty())
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

    // basic hook assistance
    bool Obstructed = Collision()->IntersectLine(Pos, Target, nullptr, nullptr) != 0;
    bool Clear = Collision()->IntersectLineTeleHook(Pos, Target, nullptr, nullptr) == 0;
    if(!m_Hooking && Clear && (Obstructed || distance(Pos, Target) > 64.0f))
    {
        m_Hooking = true;
        m_HookTicks = 0;
    }

    if(m_Hooking)
    {
        Input.m_Hook = 1;
        m_HookTicks++;
        if(m_HookTicks > 60 || distance(Pos, Target) < 24.0f)
            m_Hooking = false;
    }
    else
        Input.m_Hook = 0;

    Input.m_TargetX = (int)(Target.x - Pos.x);
    Input.m_TargetY = (int)(Target.y - Pos.y);
}

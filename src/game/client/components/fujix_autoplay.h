#ifndef GAME_CLIENT_COMPONENTS_FUJIX_AUTOPLAY_H
#define GAME_CLIENT_COMPONENTS_FUJIX_AUTOPLAY_H

#include <game/client/component.h>
#include <vector>

class CFujixAutoplay : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnUpdate() override;
    virtual void OnMapLoad() override;

private:
    std::vector<vec2> m_Path;
    int m_Current;
    vec2 m_FinishPos;
    bool m_Hooking;
    int m_HookTicks;
    bool m_HaveFinish;
    int m_LastPathTick;

    std::vector<int> FindPath(int StartIndex, int FinishIndex);
    int Heuristic(int Index, int FinishIndex) const;
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_AUTOPLAY_H

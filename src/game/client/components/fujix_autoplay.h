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

    std::vector<int> FindPath(int StartIndex, int FinishIndex);
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_AUTOPLAY_H

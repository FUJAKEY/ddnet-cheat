#ifndef GAME_CLIENT_COMPONENTS_FUJIX_H
#define GAME_CLIENT_COMPONENTS_FUJIX_H

#include <game/client/component.h>

class CFujix : public CComponent
{
	int m_RehookDelay{0};
	vec2 m_NextHookPos{0.f, 0.f};
	bool m_HookActive{false};

public:
       virtual int Sizeof() const override { return sizeof(*this); }
       virtual void OnUpdate() override;

private:
       float FreezeMargin(vec2 Start, vec2 End) const;
       bool PathBlocked(vec2 Start, vec2 End) const;
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_H

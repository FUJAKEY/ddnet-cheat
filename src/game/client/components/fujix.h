#ifndef GAME_CLIENT_COMPONENTS_FUJIX_H
#define GAME_CLIENT_COMPONENTS_FUJIX_H

#include <game/client/component.h>

class CFujix : public CComponent
{
    int m_HookTicks = 0;

public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnReset() override { m_HookTicks = 0; }
    virtual void OnUpdate() override;
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_H

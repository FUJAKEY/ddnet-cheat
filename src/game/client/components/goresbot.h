#ifndef GAME_CLIENT_COMPONENTS_GORESBOT_H
#define GAME_CLIENT_COMPONENTS_GORESBOT_H

#include <game/client/component.h>
#include <game/gamecore.h>

class CGoresBot : public CComponent
{
	void PredictMovement(CCharacterCore *pChar, int Ticks);
public:
	virtual void OnRender() override;
};

#endif

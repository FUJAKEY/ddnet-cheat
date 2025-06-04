/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/client.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/components/camera.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/animstate.h>
#include <game/collision.h>
#include <game/mapitems.h>

#include <base/vmath.h>

#include "controls.h"

CControls::CControls()
{
        mem_zero(&m_aLastData, sizeof(m_aLastData));
        mem_zero(m_aMousePos, sizeof(m_aMousePos));
       mem_zero(m_aMousePosOnAction, sizeof(m_aMousePosOnAction));
       mem_zero(m_aTargetPos, sizeof(m_aTargetPos));

       m_FujixTicksLeft = 0;
       m_FujixTarget = vec2(0, 0);
       m_FujixLockControls = 0;
       m_FujixFallbackTicksLeft = 0;
       m_FujixUsingFallback = false;
       m_PhantomRecording = false;
       m_PhantomFile = 0;
       m_PhantomLastTick = 0;
}

void CControls::OnReset()
{
	ResetInput(0);
	ResetInput(1);

	for(int &AmmoCount : m_aAmmoCount)
		AmmoCount = 0;

       m_LastSendTime = 0;

       m_FujixTicksLeft = 0;
       m_FujixLockControls = 0;
       m_FujixFallbackTicksLeft = 0;
       m_FujixUsingFallback = false;
       if(m_PhantomFile)
               io_close(m_PhantomFile);
       m_PhantomRecording = false;
       m_PhantomFile = 0;
}

void CControls::ResetInput(int Dummy)
{
	m_aLastData[Dummy].m_Direction = 0;
	// simulate releasing the fire button
	if((m_aLastData[Dummy].m_Fire & 1) != 0)
		m_aLastData[Dummy].m_Fire++;
	m_aLastData[Dummy].m_Fire &= INPUT_STATE_MASK;
	m_aLastData[Dummy].m_Jump = 0;
	m_aInputData[Dummy] = m_aLastData[Dummy];

       m_aInputDirectionLeft[Dummy] = 0;
       m_aInputDirectionRight[Dummy] = 0;

       m_FujixTicksLeft = 0;
       m_FujixLockControls = 0;
       m_FujixFallbackTicksLeft = 0;
       m_FujixUsingFallback = false;
       if(m_PhantomFile)
               io_close(m_PhantomFile);
       m_PhantomRecording = false;
       m_PhantomFile = 0;
}

void CControls::OnPlayerDeath()
{
        for(int &AmmoCount : m_aAmmoCount)
                AmmoCount = 0;

       m_FujixTicksLeft = 0;
       m_FujixLockControls = 0;
       m_FujixFallbackTicksLeft = 0;
       m_FujixUsingFallback = false;
       if(m_PhantomFile)
               io_close(m_PhantomFile);
       m_PhantomRecording = false;
       m_PhantomFile = 0;
}

struct CInputState
{
	CControls *m_pControls;
	int *m_apVariables[NUM_DUMMIES];
};

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	CInputState *pState = (CInputState *)pUserData;

	if(pState->m_pControls->GameClient()->m_GameInfo.m_BugDDRaceInput && pState->m_pControls->GameClient()->m_Snap.m_SpecInfo.m_Active)
		return;

	*pState->m_apVariables[g_Config.m_ClDummy] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	CInputState *pState = (CInputState *)pUserData;

	if((pState->m_pControls->GameClient()->m_GameInfo.m_BugDDRaceInput && pState->m_pControls->GameClient()->m_Snap.m_SpecInfo.m_Active) || pState->m_pControls->GameClient()->m_Spectator.IsActive())
		return;

	int *pVariable = pState->m_apVariables[g_Config.m_ClDummy];
	if(((*pVariable) & 1) != pResult->GetInteger(0))
		(*pVariable)++;
	*pVariable &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_apVariables[NUM_DUMMIES];
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
	{
		*pSet->m_apVariables[g_Config.m_ClDummy] = pSet->m_Value;
	}
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet);
	pSet->m_pControls->m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	{
		static CInputState s_State = {this, {&m_aInputDirectionLeft[0], &m_aInputDirectionLeft[1]}};
		Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Move left");
	}
	{
		static CInputState s_State = {this, {&m_aInputDirectionRight[0], &m_aInputDirectionRight[1]}};
		Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Move right");
	}
	{
		static CInputState s_State = {this, {&m_aInputData[0].m_Jump, &m_aInputData[1].m_Jump}};
		Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Jump");
	}
	{
		static CInputState s_State = {this, {&m_aInputData[0].m_Hook, &m_aInputData[1].m_Hook}};
		Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Hook");
	}
	{
		static CInputState s_State = {this, {&m_aInputData[0].m_Fire, &m_aInputData[1].m_Fire}};
		Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, &s_State, "Fire");
	}
	{
		static CInputState s_State = {this, {&m_aShowHookColl[0], &m_aShowHookColl[1]}};
		Console()->Register("+showhookcoll", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Show Hook Collision");
	}

	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_WantedWeapon, &m_aInputData[1].m_WantedWeapon}, 1};
		Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, &s_Set, "Switch to hammer");
	}
	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_WantedWeapon, &m_aInputData[1].m_WantedWeapon}, 2};
		Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, &s_Set, "Switch to gun");
	}
	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_WantedWeapon, &m_aInputData[1].m_WantedWeapon}, 3};
		Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, &s_Set, "Switch to shotgun");
	}
	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_WantedWeapon, &m_aInputData[1].m_WantedWeapon}, 4};
		Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, &s_Set, "Switch to grenade");
	}
	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_WantedWeapon, &m_aInputData[1].m_WantedWeapon}, 5};
		Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, &s_Set, "Switch to laser");
	}

	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_NextWeapon, &m_aInputData[1].m_NextWeapon}, 0};
		Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, &s_Set, "Switch to next weapon");
	}
	{
		static CInputSet s_Set = {this, {&m_aInputData[0].m_PrevWeapon, &m_aInputData[1].m_PrevWeapon}, 0};
		Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, &s_Set, "Switch to previous weapon");
	}
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
		if(g_Config.m_ClAutoswitchWeapons)
			m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = pMsg->m_Weapon + 1;
		// We don't really know ammo count, until we'll switch to that weapon, but any non-zero count will suffice here
		m_aAmmoCount[maximum(0, pMsg->m_Weapon % NUM_WEAPONS)] = 10;
	}
}

int CControls::SnapInput(int *pData)
{
	// update player state
	if(m_pClient->m_Chat.IsActive())
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags = PLAYERFLAG_CHATTING;
	else if(m_pClient->m_Menus.IsActive())
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags = PLAYERFLAG_IN_MENU;
	else
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags = PLAYERFLAG_PLAYING;

	if(m_pClient->m_Scoreboard.IsActive())
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags |= PLAYERFLAG_SCOREBOARD;

	if(Client()->ServerCapAnyPlayerFlag() && m_pClient->m_Controls.m_aShowHookColl[g_Config.m_ClDummy])
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags |= PLAYERFLAG_AIM;

	if(Client()->ServerCapAnyPlayerFlag() && m_pClient->m_Camera.CamType() == CCamera::CAMTYPE_SPEC)
		m_aInputData[g_Config.m_ClDummy].m_PlayerFlags |= PLAYERFLAG_SPEC_CAM;

	bool Send = m_aLastData[g_Config.m_ClDummy].m_PlayerFlags != m_aInputData[g_Config.m_ClDummy].m_PlayerFlags;

       m_aLastData[g_Config.m_ClDummy].m_PlayerFlags = m_aInputData[g_Config.m_ClDummy].m_PlayerFlags;

       if(g_Config.m_ClPhantomRecorder && !m_PhantomRecording)
               StartPhantomRecord();
       else if(!g_Config.m_ClPhantomRecorder && m_PhantomRecording)
               StopPhantomRecord();

	// we freeze the input if chat or menu is activated
	if(!(m_aInputData[g_Config.m_ClDummy].m_PlayerFlags & PLAYERFLAG_PLAYING))
	{
		if(!GameClient()->m_GameInfo.m_BugDDRaceInput)
			ResetInput(g_Config.m_ClDummy);

		mem_copy(pData, &m_aInputData[g_Config.m_ClDummy], sizeof(m_aInputData[0]));

		// set the target anyway though so that we can keep seeing our surroundings,
		// even if chat or menu are activated
		m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)m_aMousePos[g_Config.m_ClDummy].x;
		m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)m_aMousePos[g_Config.m_ClDummy].y;

		// send once a second just to be sure
		Send = Send || time_get() > m_LastSendTime + time_freq();
	}
	else
	{
		m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)m_aMousePos[g_Config.m_ClDummy].x;
		m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)m_aMousePos[g_Config.m_ClDummy].y;

		if(g_Config.m_ClSubTickAiming && m_aMousePosOnAction[g_Config.m_ClDummy] != vec2(0.0f, 0.0f))
		{
			m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)m_aMousePosOnAction[g_Config.m_ClDummy].x;
			m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)m_aMousePosOnAction[g_Config.m_ClDummy].y;
			m_aMousePosOnAction[g_Config.m_ClDummy] = vec2(0.0f, 0.0f);
		}

		if(!m_aInputData[g_Config.m_ClDummy].m_TargetX && !m_aInputData[g_Config.m_ClDummy].m_TargetY)
		{
			m_aInputData[g_Config.m_ClDummy].m_TargetX = 1;
			m_aMousePos[g_Config.m_ClDummy].x = 1;
		}

		// set direction
		m_aInputData[g_Config.m_ClDummy].m_Direction = 0;
               if(m_aInputDirectionLeft[g_Config.m_ClDummy] && !m_aInputDirectionRight[g_Config.m_ClDummy])
                       m_aInputData[g_Config.m_ClDummy].m_Direction = -1;
               if(!m_aInputDirectionLeft[g_Config.m_ClDummy] && m_aInputDirectionRight[g_Config.m_ClDummy])
                       m_aInputData[g_Config.m_ClDummy].m_Direction = 1;

               if(g_Config.m_ClFujixEnable && m_pClient->m_Snap.m_pLocalCharacter && !m_pClient->m_Snap.m_SpecInfo.m_Active)
               {
                       if(m_pClient->m_PredictedChar.m_IsInFreeze)
                       {
                               m_FujixTicksLeft = 0;
                               m_FujixLockControls = 0;
                       }

                       bool Freeze = false;
                       vec2 SafePos = m_pClient->m_PredictedChar.m_Pos;
                       CCharacterCore Pred = m_pClient->m_PredictedChar;
                       CNetObj_PlayerInput PredInput = m_aInputData[g_Config.m_ClDummy];
                       if(m_FujixTicksLeft > 0)
                       {
                               vec2 LPos = m_pClient->m_LocalCharacterPos;
                               vec2 Dir = normalize(m_FujixTarget - LPos);
                               PredInput.m_TargetX = (int)(Dir.x * GetMaxMouseDistance());
                               PredInput.m_TargetY = (int)(Dir.y * GetMaxMouseDistance());
                               PredInput.m_Hook = 1;
                       }
                       else
                               PredInput.m_Hook = 0;
                       Pred.m_Input = PredInput;

                       for(int i = 0; i < g_Config.m_ClFujixTicks; i++)
                       {
                               Pred.Tick(true);
                               Pred.Move();
                               Pred.Quantize();

                               int MapIndex = Collision()->GetPureMapIndex(Pred.m_Pos);
                               int Tiles[3] = {Collision()->GetTileIndex(MapIndex), Collision()->GetFrontTileIndex(MapIndex), Collision()->GetSwitchType(MapIndex)};
                               for(int t : Tiles)
                               {
                                       if(t == TILE_FREEZE || t == TILE_DFREEZE || t == TILE_LFREEZE || t == TILE_DEATH)
                                       {
                                               Freeze = true;
                                               break;
                                       }
                               }
                               if(Freeze)
                                       break;

                               SafePos = Pred.m_Pos;
                       }

                       if(Freeze)
                       {
                               const int NumDir = 64;
                               float HookLen = m_pClient->m_aTuning[g_Config.m_ClDummy].m_HookLength;
                               bool Found = false;
                               vec2 BestTarget = vec2(0, 0);
                               float BestDist = 1e9f;
                               bool FallbackFound = false;
                               vec2 FallbackTarget = vec2(0, 0);
                               float FallbackDist = 1e9f;
                               const auto IsCandidateSafe = [&](const vec2 &Target) {
                                       CCharacterCore Test = m_pClient->m_PredictedChar;
                                       CNetObj_PlayerInput TestInput = m_aInputData[g_Config.m_ClDummy];

                                       TestInput.m_Hook = 1;
                                       int HookSteps = clamp(g_Config.m_ClFujixTicks, 1, 20);
                                       for(int i = 0; i < HookSteps; i++)
                                       {
                                               vec2 DirT = normalize(Target - Test.m_Pos);
                                               TestInput.m_TargetX = (int)(DirT.x * GetMaxMouseDistance());
                                               TestInput.m_TargetY = (int)(DirT.y * GetMaxMouseDistance());
                                               Test.m_Input = TestInput;
                                               Test.Tick(true);
                                               Test.Move();
                                               Test.Quantize();

                                               int MapIdx = Collision()->GetPureMapIndex(Test.m_Pos);
                                               int T[3] = {Collision()->GetTileIndex(MapIdx), Collision()->GetFrontTileIndex(MapIdx), Collision()->GetSwitchType(MapIdx)};
                                               for(int t : T)
                                               {
                                                       if(t == TILE_FREEZE || t == TILE_DFREEZE || t == TILE_LFREEZE || t == TILE_DEATH)
                                                               return false;
                                               }
                                       }

                                       TestInput.m_Hook = 0;
                                       for(int i = 0; i < 20; i++)
                                       {
                                               vec2 DirT = normalize(Target - Test.m_Pos);
                                               TestInput.m_TargetX = (int)(DirT.x * GetMaxMouseDistance());
                                               TestInput.m_TargetY = (int)(DirT.y * GetMaxMouseDistance());
                                               Test.m_Input = TestInput;
                                               Test.Tick(true);
                                               Test.Move();
                                               Test.Quantize();

                                               int MapIdx = Collision()->GetPureMapIndex(Test.m_Pos);
                                               int T[3] = {Collision()->GetTileIndex(MapIdx), Collision()->GetFrontTileIndex(MapIdx), Collision()->GetSwitchType(MapIdx)};
                                               for(int t : T)
                                               {
                                                       if(t == TILE_FREEZE || t == TILE_DFREEZE || t == TILE_LFREEZE || t == TILE_DEATH)
                                                               return false;
                                               }
                                       }

                                       return true;
                               };
                               for(int k = 0; k < NumDir; k++)
                               {
                                       float a = (2.0f * pi * k) / NumDir;
                                       vec2 Dir = vec2(cosf(a), sinf(a));
                                       vec2 To = SafePos + normalize(Dir) * HookLen;

                                       vec2 Col, Before;
                                       int Hit = Collision()->IntersectLine(SafePos, To, &Col, &Before);
                                       if(Hit && (Hit == TILE_NOHOOK || Hit == TILE_FREEZE || Hit == TILE_DFREEZE || Hit == TILE_LFREEZE || Hit == TILE_DEATH))
                                               Hit = 0;
                                       if(!Hit)
                                               continue;

                                       bool ThroughFreeze = false;
                                       for(int s = 0; s < 10 && !ThroughFreeze; s++)
                                       {
                                               float aa = (s + 1) / 10.0f;
                                               vec2 Pos = mix(SafePos, Col, aa);
                                               int MapIdx = Collision()->GetPureMapIndex(Pos);
                                               int T[3] = {Collision()->GetTileIndex(MapIdx), Collision()->GetFrontTileIndex(MapIdx), Collision()->GetSwitchType(MapIdx)};
                                               for(int t : T)
                                               {
                                                       if(t == TILE_FREEZE || t == TILE_DFREEZE || t == TILE_LFREEZE || t == TILE_DEATH)
                                                       {
                                                               ThroughFreeze = true;
                                                               break;
                                                       }
                                               }
                                       }

                                       if(ThroughFreeze)
                                               continue;

                                       float Dist = distance(SafePos, Col);
                                       if(IsCandidateSafe(Col))
                                       {
                                               if(Dist < BestDist)
                                               {
                                                       BestDist = Dist;
                                                       BestTarget = Col;
                                                       Found = true;
                                               }
                                       }
                                       else
                                       {
                                               if(Dist < FallbackDist)
                                               {
                                                       FallbackDist = Dist;
                                                       FallbackTarget = Col;
                                                       FallbackFound = true;
                                               }
                                       }
                               }

                               if(Found)
                               {
                                       m_FujixTicksLeft = 5;
                                       m_FujixTarget = BestTarget;
                                       m_FujixLockControls = 1;
                                       m_FujixFallbackTicksLeft = 0;
                                       m_FujixUsingFallback = false;
                               }
                               else if(FallbackFound)
                               {
                                       m_FujixTicksLeft = 5;
                                       m_FujixTarget = FallbackTarget;
                                       m_FujixLockControls = 1;
                                       m_FujixFallbackTicksLeft = 15;
                                       m_FujixUsingFallback = true;
                               }
                       }

                       if(!Freeze)
                       {
                               if(m_FujixTicksLeft > 0)
                                       m_FujixTicksLeft--;
                               if(m_FujixFallbackTicksLeft > 0)
                                       m_FujixFallbackTicksLeft--;
                               if(m_FujixFallbackTicksLeft == 0 && m_FujixUsingFallback)
                               {
                                       m_FujixTicksLeft = 0;
                                       m_FujixLockControls = 0;
                                       m_FujixUsingFallback = false;
                               }
                               if(m_FujixTicksLeft == 0 && !m_FujixUsingFallback)
                                       m_FujixLockControls = 0;
                       }

                       if(m_FujixTicksLeft > 0 && !m_pClient->m_PredictedChar.m_IsInFreeze)
                       {
                               vec2 LocalPos = m_pClient->m_LocalCharacterPos;
                               vec2 Dir = normalize(m_FujixTarget - LocalPos);
                               m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)(Dir.x * GetMaxMouseDistance());
                               m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)(Dir.y * GetMaxMouseDistance());
                               m_aInputData[g_Config.m_ClDummy].m_Hook = 1;
                               m_aInputData[g_Config.m_ClDummy].m_Direction = 0;
                               m_aInputData[g_Config.m_ClDummy].m_Jump = 0;
                       }
               }

		// dummy copy moves
		if(g_Config.m_ClDummyCopyMoves)
		{
			CNetObj_PlayerInput *pDummyInput = &m_pClient->m_DummyInput;
			pDummyInput->m_Direction = m_aInputData[g_Config.m_ClDummy].m_Direction;
			pDummyInput->m_Hook = m_aInputData[g_Config.m_ClDummy].m_Hook;
			pDummyInput->m_Jump = m_aInputData[g_Config.m_ClDummy].m_Jump;
			pDummyInput->m_PlayerFlags = m_aInputData[g_Config.m_ClDummy].m_PlayerFlags;
			pDummyInput->m_TargetX = m_aInputData[g_Config.m_ClDummy].m_TargetX;
			pDummyInput->m_TargetY = m_aInputData[g_Config.m_ClDummy].m_TargetY;
			pDummyInput->m_WantedWeapon = m_aInputData[g_Config.m_ClDummy].m_WantedWeapon;

			if(!g_Config.m_ClDummyControl)
				pDummyInput->m_Fire += m_aInputData[g_Config.m_ClDummy].m_Fire - m_aLastData[g_Config.m_ClDummy].m_Fire;

			pDummyInput->m_NextWeapon += m_aInputData[g_Config.m_ClDummy].m_NextWeapon - m_aLastData[g_Config.m_ClDummy].m_NextWeapon;
			pDummyInput->m_PrevWeapon += m_aInputData[g_Config.m_ClDummy].m_PrevWeapon - m_aLastData[g_Config.m_ClDummy].m_PrevWeapon;

			m_aInputData[!g_Config.m_ClDummy] = *pDummyInput;
		}

		if(g_Config.m_ClDummyControl)
		{
			CNetObj_PlayerInput *pDummyInput = &m_pClient->m_DummyInput;
			pDummyInput->m_Jump = g_Config.m_ClDummyJump;

			if(g_Config.m_ClDummyFire)
				pDummyInput->m_Fire = g_Config.m_ClDummyFire;
			else if((pDummyInput->m_Fire & 1) != 0)
				pDummyInput->m_Fire++;

			pDummyInput->m_Hook = g_Config.m_ClDummyHook;
		}

		// stress testing
#ifdef CONF_DEBUG
		if(g_Config.m_DbgStress)
		{
			float t = Client()->LocalTime();
			mem_zero(&m_aInputData[g_Config.m_ClDummy], sizeof(m_aInputData[0]));

			m_aInputData[g_Config.m_ClDummy].m_Direction = ((int)t / 2) & 1;
			m_aInputData[g_Config.m_ClDummy].m_Jump = ((int)t);
			m_aInputData[g_Config.m_ClDummy].m_Fire = ((int)(t * 10));
			m_aInputData[g_Config.m_ClDummy].m_Hook = ((int)(t * 2)) & 1;
			m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = ((int)t) % NUM_WEAPONS;
			m_aInputData[g_Config.m_ClDummy].m_TargetX = (int)(std::sin(t * 3) * 100.0f);
			m_aInputData[g_Config.m_ClDummy].m_TargetY = (int)(std::cos(t * 3) * 100.0f);
		}
#endif
		// check if we need to send input
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_Direction != m_aLastData[g_Config.m_ClDummy].m_Direction;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_Jump != m_aLastData[g_Config.m_ClDummy].m_Jump;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_Fire != m_aLastData[g_Config.m_ClDummy].m_Fire;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_Hook != m_aLastData[g_Config.m_ClDummy].m_Hook;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_WantedWeapon != m_aLastData[g_Config.m_ClDummy].m_WantedWeapon;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_NextWeapon != m_aLastData[g_Config.m_ClDummy].m_NextWeapon;
		Send = Send || m_aInputData[g_Config.m_ClDummy].m_PrevWeapon != m_aLastData[g_Config.m_ClDummy].m_PrevWeapon;
		Send = Send || time_get() > m_LastSendTime + time_freq() / 25; // send at least 25 Hz
		Send = Send || (m_pClient->m_Snap.m_pLocalCharacter && m_pClient->m_Snap.m_pLocalCharacter->m_Weapon == WEAPON_NINJA && (m_aInputData[g_Config.m_ClDummy].m_Direction || m_aInputData[g_Config.m_ClDummy].m_Jump || m_aInputData[g_Config.m_ClDummy].m_Hook));
	}

	// copy and return size
	m_aLastData[g_Config.m_ClDummy] = m_aInputData[g_Config.m_ClDummy];

	if(!Send)
		return 0;

       m_LastSendTime = time_get();
       mem_copy(pData, &m_aInputData[g_Config.m_ClDummy], sizeof(m_aInputData[0]));

       RecordPhantomTick();

       return sizeof(m_aInputData[0]);
}

void CControls::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(g_Config.m_ClAutoswitchWeaponsOutOfAmmo && !GameClient()->m_GameInfo.m_UnlimitedAmmo && m_pClient->m_Snap.m_pLocalCharacter)
	{
		// Keep track of ammo count, we know weapon ammo only when we switch to that weapon, this is tracked on server and protocol does not track that
		m_aAmmoCount[maximum(0, m_pClient->m_Snap.m_pLocalCharacter->m_Weapon % NUM_WEAPONS)] = m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount;
		// Autoswitch weapon if we're out of ammo
		if(m_aInputData[g_Config.m_ClDummy].m_Fire % 2 != 0 &&
			m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount == 0 &&
			m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_HAMMER &&
			m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_NINJA)
		{
			int Weapon;
			for(Weapon = WEAPON_LASER; Weapon > WEAPON_GUN; Weapon--)
			{
				if(Weapon == m_pClient->m_Snap.m_pLocalCharacter->m_Weapon)
					continue;
				if(m_aAmmoCount[Weapon] > 0)
					break;
			}
			if(Weapon != m_pClient->m_Snap.m_pLocalCharacter->m_Weapon)
				m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = Weapon + 1;
		}
	}

	// update target pos
	if(m_pClient->m_Snap.m_pGameInfoObj && !m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		// make sure to compensate for smooth dyncam to ensure the cursor stays still in world space if zoomed
		vec2 DyncamOffsetDelta = m_pClient->m_Camera.m_DyncamTargetCameraOffset - m_pClient->m_Camera.m_aDyncamCurrentCameraOffset[g_Config.m_ClDummy];
		float Zoom = m_pClient->m_Camera.m_Zoom;
		m_aTargetPos[g_Config.m_ClDummy] = m_pClient->m_LocalCharacterPos + m_aMousePos[g_Config.m_ClDummy] - DyncamOffsetDelta + DyncamOffsetDelta / Zoom;
	}
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_aTargetPos[g_Config.m_ClDummy] = m_pClient->m_Snap.m_SpecInfo.m_Position + m_aMousePos[g_Config.m_ClDummy];
	}
       else
       {
               m_aTargetPos[g_Config.m_ClDummy] = m_aMousePos[g_Config.m_ClDummy];
       }

       DrawFujixPrediction();
}

void CControls::DrawFujixPrediction()
{
       if(!g_Config.m_ClFujixEnable || !g_Config.m_ClShowFujixPrediction || !m_pClient->m_Snap.m_pLocalCharacter || m_pClient->m_Snap.m_SpecInfo.m_Active)
               return;

       CCharacterCore Pred = m_pClient->m_PredictedChar;
       CNetObj_PlayerInput PredInput = m_aInputData[g_Config.m_ClDummy];
       if(m_FujixTicksLeft > 0)
       {
               vec2 LPos = m_pClient->m_LocalCharacterPos;
               vec2 Dir = normalize(m_FujixTarget - LPos);
               PredInput.m_TargetX = (int)(Dir.x * GetMaxMouseDistance());
               PredInput.m_TargetY = (int)(Dir.y * GetMaxMouseDistance());
               PredInput.m_Hook = 1;
       }
       else
               PredInput.m_Hook = 0;
       Pred.m_Input = PredInput;

       vec2 OldPos = Pred.m_Pos;
       IGraphics::CLineItem aLines[64];
       int Num = 0;
       for(int i = 0; i < g_Config.m_ClFujixTicks && i < 64; i++)
       {
               Pred.Tick(true);
               Pred.Move();
               Pred.Quantize();
               aLines[Num++] = IGraphics::CLineItem(OldPos.x, OldPos.y, Pred.m_Pos.x, Pred.m_Pos.y);
               OldPos = Pred.m_Pos;
       }

       if(Num > 0)
       {
               Graphics()->TextureClear();
               Graphics()->LinesBegin();
               Graphics()->SetColor(1.0f, 0.6f, 0.0f, 0.75f);
               Graphics()->LinesDraw(aLines, Num);
               Graphics()->LinesEnd();

               CTeeRenderInfo TeeInfo = GameClient()->m_aClients[m_pClient->m_Snap.m_LocalClientId].m_RenderInfo;
               TeeInfo.m_Size *= 1.0f;
               RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1, 0), Pred.m_Pos, 0.5f);
       }
}

void CControls::StartPhantomRecord()
{
       if(m_PhantomRecording)
               return;
       char aDate[20], aFilename[IO_MAX_PATH_LENGTH];
       str_timestamp(aDate, sizeof(aDate));
       str_format(aFilename, sizeof(aFilename), "phantom/record_%s.txt", aDate);
       m_PhantomFile = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
       if(m_PhantomFile)
       {
               m_PhantomRecording = true;
               const char *pHeader = "tick x y\n";
               io_write(m_PhantomFile, pHeader, str_length(pHeader));
               m_PhantomLastTick = Client()->GameTick(g_Config.m_ClDummy);
       }
}

void CControls::StopPhantomRecord()
{
       if(!m_PhantomRecording)
               return;
       if(m_PhantomFile)
               io_close(m_PhantomFile);
       m_PhantomFile = 0;
       m_PhantomRecording = false;
}

void CControls::RecordPhantomTick()
{
       if(!m_PhantomRecording || !m_PhantomFile)
               return;
       int Tick = Client()->GameTick(g_Config.m_ClDummy);
       int Interval = maximum(1, Client()->GameTickSpeed() / g_Config.m_ClPhantomTps);
       if(Tick >= m_PhantomLastTick + Interval)
       {
               m_PhantomLastTick = Tick;
               vec2 Pos = m_pClient->m_PredictedChar.m_Pos;
               char aBuf[64];
               str_format(aBuf, sizeof(aBuf), "%d %.2f %.2f\n", Tick, Pos.x, Pos.y);
               io_write(m_PhantomFile, aBuf, str_length(aBuf));
       }
}

bool CControls::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(m_pClient->m_Snap.m_pGameInfoObj && (m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED))
		return false;

	if(CursorType == IInput::CURSOR_JOYSTICK && g_Config.m_InpControllerAbsolute && m_pClient->m_Snap.m_pGameInfoObj && !m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		vec2 AbsoluteDirection;
		if(Input()->GetActiveJoystick()->Absolute(&AbsoluteDirection.x, &AbsoluteDirection.y))
			m_aMousePos[g_Config.m_ClDummy] = AbsoluteDirection * GetMaxMouseDistance();
		return true;
	}

	float Factor = 1.0f;
	if(g_Config.m_ClDyncam && g_Config.m_ClDyncamMousesens)
	{
		Factor = g_Config.m_ClDyncamMousesens / 100.0f;
	}
	else
	{
		switch(CursorType)
		{
		case IInput::CURSOR_MOUSE:
			Factor = g_Config.m_InpMousesens / 100.0f;
			break;
		case IInput::CURSOR_JOYSTICK:
			Factor = g_Config.m_InpControllerSens / 100.0f;
			break;
		default:
			dbg_msg("assert", "CControls::OnCursorMove CursorType %d", (int)CursorType);
			dbg_break();
			break;
		}
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorId < 0)
		Factor *= m_pClient->m_Camera.m_Zoom;

	m_aMousePos[g_Config.m_ClDummy] += vec2(x, y) * Factor;
	ClampMousePos();
	return true;
}

void CControls::ClampMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorId < 0)
	{
		m_aMousePos[g_Config.m_ClDummy].x = clamp(m_aMousePos[g_Config.m_ClDummy].x, -201.0f * 32, (Collision()->GetWidth() + 201.0f) * 32.0f);
		m_aMousePos[g_Config.m_ClDummy].y = clamp(m_aMousePos[g_Config.m_ClDummy].y, -201.0f * 32, (Collision()->GetHeight() + 201.0f) * 32.0f);
	}
	else
	{
		const float MouseMin = GetMinMouseDistance();
		const float MouseMax = GetMaxMouseDistance();

		float MouseDistance = length(m_aMousePos[g_Config.m_ClDummy]);
		if(MouseDistance < 0.001f)
		{
			m_aMousePos[g_Config.m_ClDummy].x = 0.001f;
			m_aMousePos[g_Config.m_ClDummy].y = 0;
			MouseDistance = 0.001f;
		}
		if(MouseDistance < MouseMin)
			m_aMousePos[g_Config.m_ClDummy] = normalize_pre_length(m_aMousePos[g_Config.m_ClDummy], MouseDistance) * MouseMin;
		MouseDistance = length(m_aMousePos[g_Config.m_ClDummy]);
		if(MouseDistance > MouseMax)
			m_aMousePos[g_Config.m_ClDummy] = normalize_pre_length(m_aMousePos[g_Config.m_ClDummy], MouseDistance) * MouseMax;
	}
}

float CControls::GetMinMouseDistance() const
{
	return g_Config.m_ClDyncam ? g_Config.m_ClDyncamMinDistance : g_Config.m_ClMouseMinDistance;
}

float CControls::GetMaxMouseDistance() const
{
	float CameraMaxDistance = 200.0f;
	float FollowFactor = (g_Config.m_ClDyncam ? g_Config.m_ClDyncamFollowFactor : g_Config.m_ClMouseFollowfactor) / 100.0f;
	float DeadZone = g_Config.m_ClDyncam ? g_Config.m_ClDyncamDeadzone : g_Config.m_ClMouseDeadzone;
	float MaxDistance = g_Config.m_ClDyncam ? g_Config.m_ClDyncamMaxDistance : g_Config.m_ClMouseMaxDistance;
        return minimum((FollowFactor != 0 ? CameraMaxDistance / FollowFactor + DeadZone : MaxDistance), MaxDistance);
}


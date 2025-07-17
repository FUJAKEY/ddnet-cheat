#pragma once
// Минимальные заглушки для game/gamecore.h

struct CWorldCore {};
struct CTeamsCore {};

struct CNetObj_PlayerInput {
    int m_Direction;
    int m_TargetX;
    int m_TargetY;
    int m_Jump;
    int m_Fire;
    int m_Hook;
    int m_PlayerFlags;
    int m_WantedWeapon;
    int m_NextWeapon;
    int m_PrevWeapon;
};

struct CNetObj_CharacterCore {
    int m_X, m_Y;
    int m_VelX, m_VelY;
    int m_Angle;
    int m_Direction;
    int m_Jumped;
    int m_HookState;
    int m_HookTick;
    int m_HookX, m_HookY;
    int m_HookDx, m_HookDy;
    int m_HookedPlayer;
};

class CCharacterCore {
public:
    vec2 m_Pos, m_Vel;
    vec2 m_HookPos, m_HookDir;
    int m_Angle;
    int m_Direction;
    int m_HookState;
    int m_HookTick;
    int m_Jumps;
    int m_Jumped;
    int m_ActiveWeapon;
    bool m_NewHook;
    bool m_CollisionDisabled;
    bool m_Solo;
    bool m_HookHitDisabled;
    bool m_HammerHitDisabled;
    bool m_GrenadeHitDisabled;
    bool m_ShotgunHitDisabled;
    bool m_LaserHitDisabled;
    CNetObj_PlayerInput m_Input;
    
    void SetCoreWorld(CWorldCore *pWorld, ICollision *pCollision, CTeamsCore *pTeams) {}
    void Tick(bool UseInput) {}
    void Move() {}
    void Quantize() {}
    void Write(CNetObj_CharacterCore *pObjCore) {}
    int HookedPlayer() { return -1; }
    void SetHookedPlayer(int ClientId) {}
};


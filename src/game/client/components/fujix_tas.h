#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <game/client/component.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <game/generated/protocol.h>
#include <game/gamecore.h>
#include <game/client/render.h>
#include <vector>
#include <deque>

class CFujixTas : public CComponent
{
public:
    static const char *ms_pFujixDir;

private:
    struct SEntry
    {
        int m_Tick;
        CNetObj_PlayerInput m_Input;
    };

    struct SHookEvent
    {
        int m_Tick;
        int m_State;
        int m_HookedPlayer;
        int m_HookX;
        int m_HookY;
        int m_HookTick;
    };

    bool m_Recording;
    bool m_Playing;
    bool m_Testing;
    int m_StartTick;
    int m_TestStartTick;
    int m_PlayStartTick;
    char m_aFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_File;
    std::vector<SEntry> m_vEntries;
    int m_PlayIndex;
    int m_LastRecordTick;
    CNetObj_PlayerInput m_LastInput;
    CNetObj_PlayerInput m_CurrentInput;
    bool m_StopPending;
    int m_StopTick;

    char m_aHookFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_HookFile;
    std::vector<SHookEvent> m_vHookEvents;
    int m_HookPlayIndex;
    int m_LastHookState;
    int m_LastHookedPlayer;

    // Phantom
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomStep;
    CNetObj_PlayerInput m_PhantomInput;
    int m_PhantomPlayIndex;

    // Rage timers
    int m_RageHookTicks;
    int m_RageMoveDir;
    int m_RageMoveTicks;

    // Tunables
    static constexpr int RAGE_HOOK_HOLD_MIN = 6;
    static constexpr int RAGE_HOOK_HOLD_MAX = 20;
    static constexpr int RAGE_MOVE_EXTRA = 4;
    static constexpr int RAGE_PREDICT_STEPS = 50;
    static constexpr int RAGE_HOOK_DOWN_HOLD = 3;
    static constexpr float RAGE_NEAR_MARGIN = 6.f;
    static constexpr int RAGE_EXTRA_AFTER_HOLD = 10;
    static constexpr int RAGE_RELEASE_SAFE = 6;
    static constexpr int RAGE_DIR_TOTAL = 24;
    static constexpr float RAGE_PATH_STEP = 4.f;

    // Additional legit tunables
    static constexpr int LEGIT_PREDICT_STEPS = 18;      // was 12
    static constexpr float LEGIT_NEAR_MARGIN = 9.0f;    // was 6.f
    static constexpr int LEGIT_MINI_SHORT_HOLD = 3;     // 2-3 ticks mini-short hook
    static constexpr int LEGIT_CLOSE_THREAT = 4;        // threat threshold in air for strong braking
    static constexpr float LEGIT_PATH_STEP = 3.0f;      // path sampling step for freeze check

    void GetPath(char *pBuf, int Size) const;
    void GetHookPath(char *pBuf, int Size) const;
    void UpdatePlaybackInput();
    void TickPhantom();
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderFuturePath(int TicksAhead);
    void TickPhantomUpTo(int TargetTick);
    void RecordHookState(int Tick);
    void ApplyHookEvents(int PredTick, bool ToPhantom);

public:
    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConTest(IConsole::IResult *pResult, void *pUserData);

    CFujixTas();
    virtual int Sizeof() const override;

    virtual void OnConsoleInit() override;
    virtual void OnMapLoad() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;

    void StartRecord();
    void StopRecord();
    void StartPlay();
    void StopPlay();
    void StartTest();
    void StopTest();
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
    bool IsTesting() const { return m_Testing; }
    bool IsPhantomActive() const { return m_PhantomActive; }
    vec2 PhantomPos() const { return m_PhantomCore.m_Pos; }
    bool FetchPlaybackInput(CNetObj_PlayerInput *pInput);
    void RecordInput(const CNetObj_PlayerInput *pInput, int Tick);
    void MaybeFinishRecord();
    void BlockFreezeInput(CNetObj_PlayerInput *pInput);
    void BlockFreezeRageInput(CNetObj_PlayerInput *pInput);
    void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

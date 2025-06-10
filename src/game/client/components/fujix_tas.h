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
        bool m_Active; // true if any input changed this tick
    };

    struct SHookEvent
    {
        int m_StartTick;
        int m_EndTick;
        int m_TileX;
        int m_TileY;
    };

    bool m_Recording;
    bool m_Playing;
    bool m_Testing;
    int m_StartTick;
    int m_TestStartTick;
    int m_PlayStartTick;
    char m_aFilename[IO_MAX_PATH_LENGTH];
    char m_aHookFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_File;
    IOHANDLE m_HookFile;
    std::vector<SEntry> m_vEntries;
    std::vector<SHookEvent> m_vHookEvents;
    int m_PlayIndex;
    int m_PlayHookIndex;
    int m_LastRecordTick;
    CNetObj_PlayerInput m_LastInput;
    CNetObj_PlayerInput m_CurrentInput;
    int m_LastHookState;
    bool m_HookRecording;
    SHookEvent m_CurHookEvent;
    int m_PhantomHookIndex;
    bool m_StopPending;
    int m_StopTick;

    bool m_PhantomActive;
    int m_PhantomTick;
    CNetObj_PlayerInput m_PhantomInput;
    struct SInputTick
    {
        int m_Tick;
        CNetObj_PlayerInput m_Input;
    };
    std::deque<SInputTick> m_PendingInputs;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomFreezeTime;
    int m_PhantomStep;
    int m_LastPredTick;

    bool m_FreezeActive;
    float m_FreezeLevel;

    int m_OldShowOthers;

    struct SPhantomState
    {
        int m_Tick;
        CCharacterCore m_Core;
        CCharacterCore m_PrevCore;
        CNetObj_PlayerInput m_Input;
        int m_FreezeTime;
    };
    std::deque<SPhantomState> m_PhantomHistory;

    void GetPath(char *pBuf, int Size) const;
    void GetHookPath(char *pBuf, int Size) const;
    void RecordEntry(const CNetObj_PlayerInput *pInput, int Tick);
    bool FetchEntry(CNetObj_PlayerInput *pInput);
    void UpdatePlaybackInput();
    void TickPhantom();
    void TickPhantomUpTo(int TargetTick);
    void RenderFuturePath(int TicksAhead);
    bool HandlePhantomTiles(int MapIndex);
    void PhantomFreeze(int Seconds);
    void PhantomUnfreeze();
    void RollbackPhantom(int Ticks);
    void RewriteFile();
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderRecommendedRoute(int TicksAhead);

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConTest(IConsole::IResult *pResult, void *pUserData);

public:
    CFujixTas();
    virtual int Sizeof() const override { return sizeof(*this); }

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
    void StartFreeze();
    void StopFreeze();
    void UpdateFreezeInput(CNetObj_PlayerInput *pInput);
    void RenderFreezeIndicator();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

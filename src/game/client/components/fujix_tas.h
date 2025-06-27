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

    struct STasEntry
    {
        int m_Tick;
        CNetObj_PlayerInput m_Input;
    };

private:
    bool m_Recording;
    bool m_Playing;
    bool m_Testing;
    int m_StartTick;
    char m_aFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_File;

    std::vector<STasEntry> m_vEntries;
    int m_PlayIndex;
    int m_PlayStartTick;

    CNetObj_PlayerInput m_LastRecordedInput;
    CNetObj_PlayerInput m_CurrentPlaybackInput;
    
    int m_LastRecordTick;
    bool m_StopPending;
    int m_StopTick;

    // --- Phantom ---
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomStep;
    int m_LastPredTick;
    int m_PhantomFreezeTime;
    
    struct SPhantomState
    {
        int m_Tick;
        CCharacterCore m_Core;
        CCharacterCore m_PrevCore;
        CNetObj_PlayerInput m_Input;
        int m_FreezeTime;
    };
    std::deque<SPhantomState> m_PhantomHistory;
    std::deque<STasEntry> m_PhantomInputs; // Inputs for phantom simulation

    int m_OldShowOthers;

    void GetPath(char *pBuf, int Size) const;
    void RecordEntry(const CNetObj_PlayerInput *pInput, int Tick);
    void UpdatePlaybackInput();
    
    void TickPhantom();
    void TickPhantomUpTo(int TargetTick);
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderFuturePath(int TicksAhead);

    bool HandlePhantomTiles(int MapIndex);
    void PhantomFreeze(int Seconds);
    void PhantomUnfreeze();

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
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
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
    void RenderAimLines();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

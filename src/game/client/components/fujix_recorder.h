#ifndef GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H
#define GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

#include <game/client/component.h>
#include <engine/console.h>
#include <engine/demo.h>
#include <engine/shared/demo.h>
#include <engine/shared/snapshot.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <vector>
#include <memory>

class CFujixRecorder : public CComponent
{
public:
    int Sizeof() const override { return sizeof(*this); }
    void OnConsoleInit() override;
    void OnMapLoad() override;
    void OnUpdate() override;

private:
    bool m_Recording = false;
    bool m_Playing = false;
    bool m_Loading = false;
    int m_PlayIndex = 0;

    struct CInputFrame
    {
        CNetObj_PlayerInput m_Input;
    };
    std::vector<CInputFrame> m_vInputs;

    struct CInputListener : public CDemoPlayer::IListener
    {
        CFujixRecorder *m_pRecorder = nullptr;
        void OnDemoPlayerSnapshot(void *, int) override {}
        void OnDemoPlayerMessage(void *pData, int Size) override;
    } m_Listener;

    std::unique_ptr<CSnapshotDelta> m_pDelta;
    std::unique_ptr<CDemoPlayer> m_pPlayer;

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);

    void ToggleRecord();
    void StartPlay();
    bool BeginLoad(const char *pFilename);

public:
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

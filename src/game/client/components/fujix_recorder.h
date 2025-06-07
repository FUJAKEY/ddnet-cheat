#ifndef GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H
#define GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

#include <game/client/component.h>
#include <engine/console.h>
#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <vector>

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
    int m_PlayIndex = 0;

    struct CKjmHeader
    {
        char m_aMarker[4]; // "KJM1"
        int m_NumTicks;
    };

    struct CKjmTick
    {
        unsigned char m_Action;
        CNetObj_PlayerInput m_Input;
    };

    struct CInputFrame
    {
        CNetObj_PlayerInput m_Input;
    };
    std::vector<CInputFrame> m_vInputs;

    CNetObj_PlayerInput m_LastPlayInput{};
    bool m_HaveLastPlayInput = false;

    IOHANDLE m_pFile = nullptr;
    int m_NumTicks = 0;
    int m_LastRecordedTick = -1;

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);

    void ToggleRecord();
    void StartPlay();
    bool LoadRecording(const char *pFilename);

public:
    int SnapInput(int *pData);
    void RecordInput(int Tick, const CNetObj_PlayerInput &Input);
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

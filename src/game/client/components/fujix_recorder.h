#ifndef GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H
#define GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

#include <game/client/component.h>
#include <engine/console.h>
#include <engine/demo.h>
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

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);

    void ToggleRecord();
    void StartPlay();

public:
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

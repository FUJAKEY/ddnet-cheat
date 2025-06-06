#ifndef GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H
#define GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

#include <game/client/component.h>
#include <engine/console.h>
#include <vector>

class CFujixRecorder : public CComponent
{
public:
    int Sizeof() const override { return sizeof(*this); }
    void OnConsoleInit() override;
    void OnMapLoad() override;
    void OnUpdate() override;

private:
    struct CFrame
    {
        vec2 m_Pos;
        vec2 m_Vel;
        int m_Direction;
        int m_Jump;
        int m_Fire;
        int m_Hook;
        int m_TargetX;
        int m_TargetY;
    };

    std::vector<CFrame> m_Frames;
    bool m_Recording = false;
    bool m_Playing = false;
    int m_PlayPos = 0;
    vec2 m_StartPos;

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);

    void ToggleRecord();
    void StartPlay();
    void Save() const;
    bool Load();

public:
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_RECORDER_H

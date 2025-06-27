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
private:
    struct SEntry
    {
        int m_Tick;
        CNetObj_PlayerInput m_Input;
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

    // Phantom
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomStep;
    CNetObj_PlayerInput m_PhantomInput;
    int m_PhantomPlayIndex;


    void GetPath(char *pBuf, int Size) const;
    void UpdatePlaybackInput();
    void TickPhantom();
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderFuturePath(int TicksAhead);
    void TickPhantomUpTo(int TargetTick);


    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConTest(IConsole::IResult *pResult, void *pUserData);
#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

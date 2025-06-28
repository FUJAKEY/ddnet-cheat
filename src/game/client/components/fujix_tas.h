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
void BlockFreezeInput(CNetObj_PlayerInput *pInput);
void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
void RenderExtras();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

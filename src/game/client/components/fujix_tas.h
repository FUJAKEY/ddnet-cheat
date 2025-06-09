#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <deque>
#include <engine/console.h>
#include <engine/storage.h>
#include <game/client/component.h>
#include <game/client/render.h>
#include <game/gamecore.h>
#include <game/generated/protocol.h>
#include <vector>

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

	bool m_Recording;
	bool m_Playing;
	int m_StartTick;
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

	struct SPhantomState
	{
		int m_Tick;
		CCharacterCore m_Core;
		CCharacterCore m_PrevCore;
		CNetObj_PlayerInput m_Input;
		int m_FreezeTime;
	};
	std::deque<SPhantomState> m_PhantomHistory;

	struct SInputEvent
	{
		int m_Tick;
		vec2 m_Pos;
		int m_Action; // 0 = hook, 1 = left, 2 = right
		bool m_Pressed;
	};
	IOHANDLE m_EventFile;
	std::vector<SInputEvent> m_vEvents;

	void GetPath(char *pBuf, int Size) const;
	void GetEventPath(char *pBuf, int Size) const;
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

	static void ConRecord(IConsole::IResult *pResult, void *pUserData);
	static void ConPlay(IConsole::IResult *pResult, void *pUserData);

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
	bool IsRecording() const { return m_Recording; }
	bool IsPlaying() const { return m_Playing; }
	bool IsPhantomActive() const { return m_PhantomActive; }
	vec2 PhantomPos() const { return m_PhantomCore.m_Pos; }
	bool FetchPlaybackInput(CNetObj_PlayerInput *pInput);
	void RecordInput(const CNetObj_PlayerInput *pInput, int Tick);
	void RecordEvent(int Tick, vec2 Pos, int Action, bool Pressed);
	void MaybeFinishRecord();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

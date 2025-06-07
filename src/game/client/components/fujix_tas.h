#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <game/client/component.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <game/generated/protocol.h>
#include <vector>

// +++ ИСПРАВЛЕНИЕ: Включаем полный заголовок для CCharacter, а не только для его ядра +++
#include <game/client/components/characters.h>

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

    // --- Переменные для записи/воспроизведения ---
    bool m_Recording;
    bool m_Playing;
    int m_StartTick;
    int m_PlayStartTick;
    char m_aFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_File;
    std::vector<SEntry> m_vEntries;
    int m_PlayIndex;

    // +++ ИСПРАВЛЕНИЕ: Сохраняем весь объект CCharacter, а не только CCharacterCore +++
    bool m_StateSaved;
    CCharacter m_SavedCharState; // Переменная для хранения полного состояния персонажа

    void GetPath(char *pBuf, int Size) const;
    void RecordEntry(const CNetObj_PlayerInput *pInput, int Tick);
    bool FetchEntry(CNetObj_PlayerInput *pInput);

    // --- Консольные команды ---
    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConSaveState(IConsole::IResult *pResult, void *pUserData);
    static void ConLoadState(IConsole::IResult *pResult, void *pUserData);

public:
    CFujixTas();
    virtual int Sizeof() const override { return sizeof(*this); }

    virtual void OnConsoleInit() override;
    virtual void OnMapLoad() override;

    // --- Основные функции TAS ---
    void StartRecord();
    void StopRecord();
    void StartPlay();
    void StopPlay();
    bool IsRecording() const { return m_Recording; }
    bool IsPlaying() const { return m_Playing; }
    bool FetchPlaybackInput(CNetObj_PlayerInput *pInput);
    void RecordInput(const CNetObj_PlayerInput *pInput, int Tick);

    // --- Функции для работы с состоянием ---
    void SaveState();
    void LoadState();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

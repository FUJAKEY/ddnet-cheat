#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <game/client/component.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <game/generated/protocol.h>
#include <vector>

// +++ ИСПРАВЛЕНИЕ: Используем предварительное объявление вместо #include +++
// Это говорит компилятору, что класс CCharacter существует, не раскрывая его деталей.
class CCharacter;

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

    // --- Переменная для сохранения состояния ---
    bool m_StateSaved;
    // +++ ИСПРАВЛЕНИЕ: Храним указатель на состояние, а не сам объект,
    // чтобы избежать необходимости знать его полный размер здесь.
    CCharacter *m_pSavedCharState;

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
    // +++ ИСПРАВЛЕНИЕ: Добавляем деструктор для очистки памяти +++
    virtual ~CFujixTas();
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

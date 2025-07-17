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
#include <memory>

// Forward declarations
class CSmartAutopilot;

// 🆕 Структура состояния для интеграции с autopilot
struct SAutopilotState
{
    vec2 m_Position;      // Текущая позиция
    vec2 m_Velocity;      // Текущая скорость  
    vec2 m_Target;        // Целевая позиция
    bool m_OnGround;      // На земле ли персонаж
    
    SAutopilotState() : m_Position(0, 0), m_Velocity(0, 0), m_Target(0, 0), m_OnGround(false) {}
};
// 🆕 ПОЛНОСТЬЮ НОВАЯ STATE-BASED TAS СИСТЕМА
class CFujixTas : public CComponent
{
public:
    static const char *ms_pFujixDir;

private:
    // 🆕 Снимок полного состояния персонажа в один тик
    struct SStateSnapshot
    {
        int m_Tick;                    // Номер тика
        
        // Основная позиция и движение
        float m_PosX, m_PosY;         // Позиция (float для точности)
        float m_VelX, m_VelY;         // Скорость
        int m_Angle;                  // Угол взгляда
        int m_Direction;              // Направление (-1/0/1)
        
        // Состояние крюка
        int m_HookState;              // Состояние крюка (HOOK_IDLE, HOOK_FLYING, etc.)
        float m_HookPosX, m_HookPosY; // Позиция крюка
        float m_HookDirX, m_HookDirY; // Направление крюка
        int m_HookTick;               // Счетчик крюка
        int m_HookedPlayer;           // К кому прицепился крюк
        bool m_NewHook;               // Флаг нового крюка
        
        // Физические состояния
        bool m_Grounded;              // На земле ли
        int m_Jumps;                  // Количество прыжков
        bool m_Jumped;                // Прыгнул ли в этом тике
        
        // Игровые параметры
        int m_Health;                 // Здоровье
        int m_Armor;                  // Броня
        int m_Weapon;                 // Текущее оружие
        
        // Инпут который привел к этому состоянию
        CNetObj_PlayerInput m_Input;
    };
    // 🆕 Основные переменные новой TAS системы
    bool m_Recording;
    bool m_RecordingNoGhost;  // 🆕 Новая запись без phantom
    bool m_Playing;
    bool m_Testing;
    int m_StartTick;
    int m_TestStartTick;
    int m_PlayStartTick;
    char m_aFilename[IO_MAX_PATH_LENGTH];
    IOHANDLE m_File;
    std::vector<SStateSnapshot> m_vStates;  // Вектор состояний
    int m_PlayIndex;
    int m_LastRecordTick;
    bool m_StopPending;
    int m_StopTick;
    
    // 🆕 Старая система для совместимости
    struct SEntry
    {
        int m_Tick;
        CNetObj_PlayerInput m_Input;
    };
    std::vector<SEntry> m_vEntries;  // Старые записи инпута
    CNetObj_PlayerInput m_CurrentInput;
    CNetObj_PlayerInput m_LastInput;
    CNetObj_PlayerInput m_PhantomInput;

    // Phantom для предпросмотра
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomStep;
    int m_PhantomPlayIndex;
    
    // Smart autopilot integration
    std::unique_ptr<CSmartAutopilot> m_pSmartAutopilot;
    bool m_SmartAutopilotInitialized;
    
    // Rage mode
    bool m_RageActive;
    vec2 m_RageTarget;
    bool m_RagePrevEnabled;
    // 🆕 Методы для работы с состояниями
    void GetPath(char *pBuf, int Size) const;
    void CaptureCurrentState(SStateSnapshot *pSnapshot, int Tick);
    void RestoreState(const SStateSnapshot &Snapshot, CCharacterCore *pCore);
    bool LoadStates(const char *pFilename);
    void UpdateStatePlayback();
    void ApplyState(const SStateSnapshot &Snapshot);
    void InterpolateAndApplyState(const SStateSnapshot &Prev, const SStateSnapshot &Next, float Factor);
    
    // Методы для записи/воспроизведения инпута (простая система как в krx)
    void UpdatePlaybackInput();
    void TickPhantom();
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderFuturePath(int TicksAhead);
    void RenderAutopilotPath();
    void TickPhantomUpTo(int TargetTick);
    void UpdateRageTarget();

public:
    // Методы, используемые в gameclient.cpp
    bool FetchPlaybackInput(CNetObj_PlayerInput *pInput);
    void RecordInput(const CNetObj_PlayerInput *pInput, int Tick);
    void ApplyRageInput(CNetObj_PlayerInput *pInput);

    // Console commands
    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConRecordNoGhost(IConsole::IResult *pResult, void *pUserData); // 🆕 Новая команда
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConTest(IConsole::IResult *pResult, void *pUserData);
    CFujixTas();
    ~CFujixTas(); // Кастомный деструктор для unique_ptr с forward declaration
    virtual int Sizeof() const override;

    virtual void OnConsoleInit() override;
    virtual void OnMapLoad() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;

    void StartRecord();
    void StartRecordNoGhost(); // 🆕 Новый метод записи без phantom
    void StopRecord();
    void StartPlay();
    void StopPlay();
    void StartTest();
    void StopTest();
    bool IsRecording() const { return m_Recording || m_RecordingNoGhost; } // 🆕 Проверяем оба типа записи
    bool IsPlaying() const { return m_Playing; }
    bool IsTesting() const { return m_Testing; }
    bool IsRecordingNoGhost() const { return m_RecordingNoGhost; } // 🆕 Геттер для записи без phantom
    bool IsPhantomActive() const { return m_PhantomActive; }
    vec2 PhantomPos() const { return m_PhantomCore.m_Pos; }

    // 🆕 Новые интерфейсы для state-based TAS
    bool FetchPlaybackState(CCharacterCore *pCore);  // Получить состояние для воспроизведения
    void RecordCurrentState(int Tick);               // Записать текущее состояние
    void MaybeFinishRecord();
    void BlockFreezeInput(CNetObj_PlayerInput *pInput);
    void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
    void SetRageTarget(vec2 Pos) { m_RageTarget = Pos; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

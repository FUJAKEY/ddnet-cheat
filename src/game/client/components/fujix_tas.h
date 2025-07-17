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

// 🆕 НОВАЯ СТРУКТУРА: Состояние персонажа (для старой совместимости)
struct SStateSnapshot 
{
    int m_Tick;
    
    // Позиция и движение
    float m_PosX, m_PosY;
    float m_VelX, m_VelY;
    int m_Angle;
    int m_Direction;
    
    // Состояние крюка
    int m_HookState;
    float m_HookPosX, m_HookPosY;
    float m_HookDirX, m_HookDirY;
    int m_HookTick;
    int m_HookedPlayer;
    bool m_NewHook;
    
    // Физические состояния
    bool m_Grounded;
    int m_Jumps;
    bool m_Jumped;
    
    // Игровые параметры
    int m_Health, m_Armor;
    int m_Weapon;
    
    // Инпут
    CNetObj_PlayerInput m_Input;
};
// 🆕 ПОЛНОСТЬЮ НОВАЯ STATE-BASED TAS СИСТЕМА
class CFujixTas : public CComponent
{
public:
    static const char *ms_pFujixDir;

private:
    // 🆕 НОВАЯ АРХИТЕКТУРА: SERVER-STATE BASED TAS СИСТЕМА
    struct SServerStateSnapshot
    {
        int m_ServerTick;               // Серверный тик (не клиентский!)
        
        // СЕРВЕРНОЕ состояние персонажа (то что видит сервер)
        int m_X, m_Y;                   // Серверная позиция (int как в протоколе)
        int m_VelX, m_VelY;             // Серверная скорость
        int m_Angle;                    // Угол взгляда
        int m_Direction;                // Направление (-1/0/1)
        int m_Jumped;                   // Прыжки
        
        // СЕРВЕРНОЕ состояние крюка (критически важно!)
        int m_HookState;                // Состояние крюка на сервере
        int m_HookTick;                 // Тик крюка на сервере  
        int m_HookX, m_HookY;           // Позиция крюка на сервере
        int m_HookDx, m_HookDy;         // Направление крюка на сервере
        int m_HookedPlayer;             // К кому прицепился
        
        // Игровое состояние
        int m_Health, m_Armor;          // Здоровье/броня
        int m_Weapon;                   // Оружие
        int m_Ammo;                     // Патроны
        
        // КЛИЕНТСКИЙ инпут который ПРИВЕЛ к этому серверному состоянию
        CNetObj_PlayerInput m_InputUsed; // Инпут который обработал сервер
        
        // Компенсация задержек
        int m_Ping;                     // Пинг в момент записи
        int m_PredictionTime;           // Время предикции
    };

    // 🆕 Старая структура для совместимости (простая система как в krx)
    struct SEntry
    {
        int m_Tick;
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
    std::vector<SEntry> m_vEntries;                     // Вектор инпутов (старая система)
    std::vector<SStateSnapshot> m_vStates;              // 🆕 Вектор состояний (для новой совместимости)
    std::vector<SServerStateSnapshot> m_vServerStates;  // 🆕 Вектор серверных состояний (НОВАЯ СИСТЕМА)
    int m_PlayIndex;
    int m_LastRecordTick;
    bool m_StopPending;
    int m_StopTick;
    CNetObj_PlayerInput m_CurrentInput;  // Текущий инпут для воспроизведения
    CNetObj_PlayerInput m_LastInput;     // Последний записанный инпут
    
    // 🆕 НОВЫЕ ПЕРЕМЕННЫЕ ДЛЯ SERVER-STATE ЗАПИСИ
    int m_LastServerTick;                // Последний обработанный серверный тик
    bool m_RecordingServerStates;        // Флаг записи серверных состояний  
    SServerStateSnapshot m_LastServerState; // Последнее серверное состояние

	// 🆕 TAS Recording control - УБИРАЕМ TPS ОГРАНИЧЕНИЯ!
	// int m_TargetTps;                    // УДАЛЕНО: записываем КАЖДЫЙ тик
	// int64_t m_LastTasRecordTickTime;    // УДАЛЕНО: записываем КАЖДЫЙ тик  
	// bool m_NeedTasTickRecording;        // УДАЛЕНО: записываем КАЖДЫЙ тик

    // Phantom для предпросмотра
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    CNetObj_PlayerInput m_PhantomInput;  // 🆕 Добавлено: отсутствующая переменная
    int m_PhantomStep;
    int m_PhantomPlayIndex;
    // Smart autopilot integration
    std::unique_ptr<CSmartAutopilot> m_pSmartAutopilot;
    bool m_SmartAutopilotInitialized;
    
    // 🆕 НОВЫЕ МЕТОДЫ: Работа с состояниями
    // Rage mode
    bool m_RageActive;
    vec2 m_RageTarget;
    bool m_RagePrevEnabled;
    // 🆕 Методы для работы с состояниями
    void GetPath(char *pBuf, int Size) const;
    
    // 🆕 НОВЫЕ МЕТОДЫ ДЛЯ SERVER-STATE СИСТЕМЫ
    void CaptureServerState(SServerStateSnapshot *pSnapshot, int ServerTick);
    void RestoreServerState(const SServerStateSnapshot &Snapshot);
    bool LoadServerStates(const char *pFilename);
    void UpdateServerStatePlayback();
    void ApplyServerState(const SServerStateSnapshot &Snapshot);
    
    // 🆕 СТАРЫЕ МЕТОДЫ (для совместимости)
    void CaptureCurrentState(SStateSnapshot *pSnapshot, int Tick);
    void RestoreState(const SStateSnapshot &Snapshot, CCharacterCore *pCore);
    bool LoadStates(const char *pFilename);
    void UpdateStatePlayback();
    void ApplyState(const SStateSnapshot &Snapshot);
    void InterpolateAndApplyState(const SStateSnapshot &Prev, const SStateSnapshot &Next, float Factor);
    
    // 🆕 НЕДОСТАЮЩИЕ МЕТОДЫ ИЗ .CPP ФАЙЛА
    void UpdatePlaybackInput();                         // Обновление воспроизведения input
    void UpdateRageTarget();                           // Обновление rage target
    void FinishRecord();                              // Завершение записи
    void TickPhantomUpTo(int TargetTick);             // Phantom до указанного тика
    void TickPhantom();                               // Один тик phantom
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick); // Конвертация core в character
    void RenderFuturePath(int TicksAhead);            // Рендеринг пути вперед
    void RenderAutopilotPath();                       // Рендеринг autopilot пути

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
    bool IsRecordingWithPhantom() const { return m_Recording; } // 🆕 Геттер для записи только с phantom (блокирует ввод)
    bool IsPhantomActive() const { return m_PhantomActive; }
    vec2 PhantomPos() const { return m_PhantomCore.m_Pos; }

    // 🆕 Новые интерфейсы для state-based TAS
    bool FetchPlaybackState(CCharacterCore *pCore);  // Получить состояние для воспроизведения
    void RecordCurrentState(int Tick);               // Записать текущее состояние
    void RecordServerState(int ServerTick);          // 🆕 Записать серверное состояние (PUBLIC для gameclient.cpp)
    void MaybeFinishRecord();
    void BlockFreezeInput(CNetObj_PlayerInput *pInput);
    void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
    void SetRageTarget(vec2 Pos) { m_RageTarget = Pos; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

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

    // Phantom для предпросмотра
    bool m_PhantomActive;
    int m_PhantomTick;
    CCharacterCore m_PhantomCore;
    CCharacterCore m_PhantomPrevCore;
    CTeeRenderInfo m_PhantomRenderInfo;
    int m_PhantomStep;
    int m_PhantomPlayIndex;

    // 🆕 Методы для работы с состояниями
    void GetPath(char *pBuf, int Size) const;
    void CaptureCurrentState(SStateSnapshot *pSnapshot, int Tick);
    void ApplyState(const SStateSnapshot &Snapshot, bool ToPhantom = false);
    void UpdatePlaybackState();
    void TickPhantom();
    void CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick);
    void FinishRecord();
    void RenderFuturePath(int TicksAhead);
    void TickPhantomUpTo(int TargetTick);

    // 🆕 Интерполяция состояний для плавности
    void InterpolateStates(const SStateSnapshot &From, const SStateSnapshot &To, float Factor, SStateSnapshot *pResult);

    // Rage mode
    bool m_RageActive;
    vec2 m_RageTarget;
    bool m_RagePrevEnabled;
    void ApplyRageInput(CNetObj_PlayerInput *pInput);
    void UpdateRageTarget();
public:
    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);
    static void ConTest(IConsole::IResult *pResult, void *pUserData);

    CFujixTas();
    virtual int Sizeof() const override;

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

    // 🆕 Новые интерфейсы для state-based TAS
    bool FetchPlaybackState(CCharacterCore *pCore);  // Получить состояние для воспроизведения
    void RecordCurrentState(int Tick);               // Записать текущее состояние
    void MaybeFinishRecord();
    void BlockFreezeInput(CNetObj_PlayerInput *pInput);
    void UpdateFreezeInput(CNetObj_PlayerInput *pInput); // legacy compatibility
    void SetRageTarget(vec2 Pos) { m_RageTarget = Pos; }
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

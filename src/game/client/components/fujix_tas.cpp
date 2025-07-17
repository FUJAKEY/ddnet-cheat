#include "fujix_tas.h"
#include "fujix_pathfinding.h"

#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/client.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/animstate.h>
#include <base/math.h>
#include <game/gamecore.h>
#include <game/client/components/players.h>
#include <game/client/prediction/entities/character.h>
#include <base/system.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <memory>

const char *CFujixTas::ms_pFujixDir = "fujix";

// 🆕 ПРОСТАЯ СИСТЕМА КАК В KRX
CFujixTas::CFujixTas()
{
    // Инициализация основных переменных
    m_Recording = false;
    m_Playing = false;
    m_Testing = false;
    m_StartTick = 0;
    m_TestStartTick = 0;
    m_PlayStartTick = 0;
    m_File = nullptr;
    m_PlayIndex = 0;
    m_LastRecordTick = -1;
    m_aFilename[0] = '\0';
    m_StopPending = false;
    m_StopTick = -1;
    
    // Phantom для предпросмотра
    m_PhantomActive = false;
    m_PhantomTick = 0;
    m_PhantomStep = 1;
    m_PhantomPlayIndex = 0;

    // Rage mode
    m_RageActive = false;
    m_RageTarget = vec2(0.f, 0.f);
    m_RagePrevEnabled = false;
    
    // Smart autopilot
    m_pSmartAutopilot = std::make_unique<CSmartAutopilot>();
    m_SmartAutopilotInitialized = false;
    
    // Input states
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    
    // 🆕 Резервируем память для состояний (оптимизация)
    m_vStates.reserve(60 * 60 * 5); // 5 минут при 60 FPS
}

// 🆕 Деструктор для корректной работы с unique_ptr<CSmartAutopilot>
CFujixTas::~CFujixTas()
{
    // Деструктор unique_ptr корректно удалит объект CSmartAutopilot
    // когда определение CSmartAutopilot доступно через include
}
int CFujixTas::Sizeof() const
{
    return sizeof(*this);
}

// 🆕 НОВЫЙ МЕТОД: Получить путь к файлу состояний
void CFujixTas::GetPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s.fjx", ms_pFujixDir, pMap);
}

// 🆕 НОВЫЙ МЕТОД: Получить путь к файлу событий крюка
void CFujixTas::GetHookPath(char *pBuf, int Size) const
{
    const char *pMap = Client()->GetCurrentMap();
    str_format(pBuf, Size, "%s/%s_hook.fjx", ms_pFujixDir, pMap);
}

// 🆕 НОВЫЙ МЕТОД: Захват полного состояния персонажа
void CFujixTas::CaptureCurrentState(SStateSnapshot *pSnapshot, int Tick)
{
    if(!GameClient()->m_Snap.m_pLocalCharacter)
        return;

    const CCharacterCore &Core = GameClient()->m_PredictedChar;
    
    // Заполняем базовую информацию
    pSnapshot->m_Tick = Tick;
    
    // Позиция и движение с максимальной точностью
    pSnapshot->m_PosX = Core.m_Pos.x;
    pSnapshot->m_PosY = Core.m_Pos.y;
    pSnapshot->m_VelX = Core.m_Vel.x;
    pSnapshot->m_VelY = Core.m_Vel.y;
    pSnapshot->m_Angle = Core.m_Angle;
    pSnapshot->m_Direction = Core.m_Direction;
    
    // Состояние крюка
    pSnapshot->m_HookState = Core.m_HookState;
    pSnapshot->m_HookPosX = Core.m_HookPos.x;
    pSnapshot->m_HookPosY = Core.m_HookPos.y;
    pSnapshot->m_HookDirX = Core.m_HookDir.x;
    pSnapshot->m_HookDirY = Core.m_HookDir.y;
    pSnapshot->m_HookTick = Core.m_HookTick;
    pSnapshot->m_HookedPlayer = Core.HookedPlayer();
    pSnapshot->m_NewHook = Core.m_NewHook;
    
    // Физические состояния 
    pSnapshot->m_Grounded = (Core.m_Jumped & 2) == 0; // Простой способ определить на земле ли
    pSnapshot->m_Jumps = Core.m_Jumps;
    pSnapshot->m_Jumped = Core.m_Jumped != 0;
    
    // Игровые параметры
    if(GameClient()->m_Snap.m_pLocalCharacter)
    {
        pSnapshot->m_Health = GameClient()->m_Snap.m_pLocalCharacter->m_Health;
        pSnapshot->m_Armor = GameClient()->m_Snap.m_pLocalCharacter->m_Armor; 
        pSnapshot->m_Weapon = GameClient()->m_Snap.m_pLocalCharacter->m_Weapon;
    }
    else
    {
        pSnapshot->m_Health = 10;
        pSnapshot->m_Armor = 0;
        pSnapshot->m_Weapon = 0;
    }
    
    // Последний инпут - это то что привело к этому состоянию
    pSnapshot->m_Input = Core.m_Input;
}

// 🆕 НОВЫЙ МЕТОД: Записать текущее состояние
void CFujixTas::RecordCurrentState(int Tick)
{
    if(!m_Recording || !GameClient()->m_Snap.m_pLocalCharacter)
        return;
        
    SStateSnapshot Snapshot;
    CaptureCurrentState(&Snapshot, Tick);
    
    m_vStates.push_back(Snapshot);
    
    // Записываем в файл
    if(m_File)
        io_write(m_File, &Snapshot, sizeof(Snapshot));
}

// 🆕 НОВЫЙ МЕТОД: Восстановить состояние из снимка
void CFujixTas::RestoreState(const SStateSnapshot &Snapshot, CCharacterCore *pCore)
{
    if(!pCore)
        return;
        
    // Восстанавливаем позицию и движение
    pCore->m_Pos = vec2(Snapshot.m_PosX, Snapshot.m_PosY);
    pCore->m_Vel = vec2(Snapshot.m_VelX, Snapshot.m_VelY);
    pCore->m_Angle = Snapshot.m_Angle;
    pCore->m_Direction = Snapshot.m_Direction;
    
    // Восстанавливаем состояние крюка
    pCore->m_HookState = Snapshot.m_HookState;
    pCore->m_HookPos = vec2(Snapshot.m_HookPosX, Snapshot.m_HookPosY);
    pCore->m_HookDir = vec2(Snapshot.m_HookDirX, Snapshot.m_HookDirY);
    pCore->m_HookTick = Snapshot.m_HookTick;
    pCore->SetHookedPlayer(Snapshot.m_HookedPlayer);
    pCore->m_NewHook = Snapshot.m_NewHook;
    
    // Восстанавливаем физические состояния
    pCore->m_Jumps = Snapshot.m_Jumps;
    pCore->m_Jumped = Snapshot.m_Jumped ? 1 : 0;
    
    // Последний инпут
    pCore->m_Input = Snapshot.m_Input;
}

// 🆕 НОВЫЙ МЕТОД: Загрузить состояния из файла
bool CFujixTas::LoadStates(const char *pFilename)
{
    IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
        return false;
        
    m_vStates.clear();
    SStateSnapshot State;
    while(io_read(File, &State, sizeof(State)) == sizeof(State))
        m_vStates.push_back(State);
        
    io_close(File);
    return !m_vStates.empty();
}

// 🆕 НОВЫЙ МЕТОД: State-based воспроизведение  
void CFujixTas::UpdateStatePlayback()
{
    if(!m_Playing || m_vStates.empty())
        return;
        
    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    int RelativeTick = PredTick - m_PlayStartTick;
    
    // Ищем ближайшее состояние
    for(size_t i = 0; i < m_vStates.size(); i++)
    {
        if(m_vStates[i].m_Tick >= RelativeTick)
        {
            if(i > 0 && m_vStates[i].m_Tick > RelativeTick)
            {
                // Интерполируем между состояниями для плавности
                const SStateSnapshot &Prev = m_vStates[i-1];
                const SStateSnapshot &Next = m_vStates[i];
                float Factor = (float)(RelativeTick - Prev.m_Tick) / (float)(Next.m_Tick - Prev.m_Tick);
                
                InterpolateAndApplyState(Prev, Next, Factor);
            }
            else
            {
                // Применяем точное состояние
                ApplyState(m_vStates[i]);
            }
            break;
        }
    }
}

void CFujixTas::ApplyState(const SStateSnapshot &Snapshot)
{
    RestoreState(Snapshot, &GameClient()->m_PredictedChar);
    m_CurrentInput = Snapshot.m_Input;
}

// 🆕 НОВЫЙ МЕТОД: Интерполяция состояний
void CFujixTas::InterpolateAndApplyState(const SStateSnapshot &Prev, const SStateSnapshot &Next, float Factor)
{
    SStateSnapshot Interpolated;
    
    // Интерполируем позицию и скорость
    Interpolated.m_PosX = Prev.m_PosX + (Next.m_PosX - Prev.m_PosX) * Factor;
    Interpolated.m_PosY = Prev.m_PosY + (Next.m_PosY - Prev.m_PosY) * Factor;
    Interpolated.m_VelX = Prev.m_VelX + (Next.m_VelX - Prev.m_VelX) * Factor;
    Interpolated.m_VelY = Prev.m_VelY + (Next.m_VelY - Prev.m_VelY) * Factor;
    
    // Для дискретных значений используем пороговую интерполяцию
    Interpolated.m_Direction = Factor < 0.5f ? Prev.m_Direction : Next.m_Direction;
    Interpolated.m_HookState = Factor < 0.5f ? Prev.m_HookState : Next.m_HookState;
    Interpolated.m_HookedPlayer = Factor < 0.5f ? Prev.m_HookedPlayer : Next.m_HookedPlayer;
    Interpolated.m_Jumps = Factor < 0.5f ? Prev.m_Jumps : Next.m_Jumps;
    Interpolated.m_Jumped = Factor < 0.5f ? Prev.m_Jumped : Next.m_Jumped;
    
    // Интерполируем крюк
    Interpolated.m_HookPosX = Prev.m_HookPosX + (Next.m_HookPosX - Prev.m_HookPosX) * Factor;
    Interpolated.m_HookPosY = Prev.m_HookPosY + (Next.m_HookPosY - Prev.m_HookPosY) * Factor;
    Interpolated.m_HookDirX = Prev.m_HookDirX + (Next.m_HookDirX - Prev.m_HookDirX) * Factor;
    Interpolated.m_HookDirY = Prev.m_HookDirY + (Next.m_HookDirY - Prev.m_HookDirY) * Factor;
    
    // Применяем интерполированное состояние
    ApplyState(Interpolated);
}

// 🆕 ПРОСТАЯ СИСТЕМА КАК В KRX - БЕЗ ОТДЕЛЬНЫХ СОБЫТИЙ КРЮКА
void CFujixTas::UpdatePlaybackInput()
{
    if(!m_Playing && !m_Testing)
        return;

    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    int BaseTick = m_Playing ? m_PlayStartTick : m_TestStartTick;
    int *pPlayIndex = m_Playing ? &m_PlayIndex : &m_PhantomPlayIndex;

    // ПРОСТАЯ СИНХРОНИЗАЦИЯ: Ищем инпут для текущего тика
    int RelativeTick = PredTick - BaseTick;
    
    // Находим последний подходящий инпут
    CNetObj_PlayerInput *pTargetInput = m_Playing ? &m_CurrentInput : &m_PhantomInput;
    
    while(*pPlayIndex < (int)m_vEntries.size() && m_vEntries[*pPlayIndex].m_Tick <= RelativeTick)
    {
        *pTargetInput = m_vEntries[*pPlayIndex].m_Input;
        (*pPlayIndex)++;
    }

    // Проверяем завершение воспроизведения
    if(*pPlayIndex >= (int)m_vEntries.size() && 
       (m_vEntries.empty() || RelativeTick > m_vEntries.back().m_Tick + 10))
    {
        if(m_Playing)
            StopPlay();
        else if(m_Testing)
            StopTest();
    }
}

// УБРАНО: Отдельная система событий крюка - используем только простые инпуты как в krx

void CFujixTas::ApplyRageInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeRage || !GameClient()->m_Snap.m_pLocalCharacter || !m_RageActive)
        return;

    // Initialize smart autopilot if not done yet
    if(!m_SmartAutopilotInitialized && m_pSmartAutopilot && Collision())
    {
        m_pSmartAutopilot->Init(Collision());
        m_SmartAutopilotInitialized = true;
    }

    if(!m_SmartAutopilotInitialized || !m_pSmartAutopilot)
    {
        // Fallback to simple logic if smart autopilot failed to initialize
        vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
        vec2 Diff = m_RageTarget - Pos;

        if(length(Diff) < 2.0f)
        {
            pInput->m_Direction = 0;
            pInput->m_Hook = 0;
            pInput->m_Jump = 0;
            m_RageActive = false;
            return;
        }

        if(Diff.x > 2.0f)
            pInput->m_Direction = 1;
        else if(Diff.x < -2.0f)
            pInput->m_Direction = -1;
        else
            pInput->m_Direction = 0;

        if(Diff.y < -32.0f)
            pInput->m_Jump = 1;

        if(length(Diff) > 96.0f)
        {
            pInput->m_Hook = 1;
            pInput->m_TargetX = (int)(Diff.x * 256.0f);
            pInput->m_TargetY = (int)(Diff.y * 256.0f);
        }
        else
        {
            pInput->m_Hook = 0;
        }
        return;
    }

    // Use smart autopilot
    vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
    vec2 Vel = GameClient()->m_PredictedChar.m_Vel;
    
    // Check if we reached the target
    if(length(m_RageTarget - Pos) < 2.0f)
    {
        pInput->m_Direction = 0;
        pInput->m_Hook = 0;
        pInput->m_Jump = 0;
        m_RageActive = false;
        return;
    }

    // Get next action from smart autopilot
    SAutopilotState State;
    State.m_Position = Pos;
    State.m_Velocity = Vel;
    State.m_Target = m_RageTarget;
    State.m_OnGround = false; // We'll detect this properly later
    
    // Detect if on ground
    CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
    if(pLocalChar)
        State.m_OnGround = pLocalChar->IsGrounded();
    
    CNetObj_PlayerInput SmartInput;
    mem_zero(&SmartInput, sizeof(SmartInput));
    
    if(m_pSmartAutopilot->Update(State, &SmartInput))
    {
        // Copy smart input to actual input
        *pInput = SmartInput;
    }
    else
    {
        // Smart autopilot failed, keep current input but stop rage mode after some time
        static int s_FailCount = 0;
        s_FailCount++;
        if(s_FailCount > 60) // Stop after 1 second of failures
        {
            m_RageActive = false;
            s_FailCount = 0;
        }
    }
}

void CFujixTas::UpdateRageTarget()
{
    if(g_Config.m_ClFujixBlockFreezeRage != m_RagePrevEnabled)
    {
        m_RagePrevEnabled = g_Config.m_ClFujixBlockFreezeRage;
        if(!m_RagePrevEnabled)
            m_RageActive = false;
        else
        {
            m_RageTarget = vec2(Ui()->MouseWorldX(), Ui()->MouseWorldY());
            m_RageActive = true;
        }
    }

    if(!g_Config.m_ClFujixBlockFreezeRage)
        return;

    if(Input()->KeyPress(KEY_MOUSE_1))
    {
        m_RageTarget = vec2(Ui()->MouseWorldX(), Ui()->MouseWorldY());
        m_RageActive = true;
    }
}

bool CFujixTas::FetchPlaybackInput(CNetObj_PlayerInput *pInput)
{
    if(!m_Playing)
        return false;

    UpdatePlaybackInput();
    *pInput = m_CurrentInput;

    // also update the local control state so prediction uses the TAS input
    GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy] = m_CurrentInput;
    GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy] = m_CurrentInput;

    return true;
}

void CFujixTas::RecordInput(const CNetObj_PlayerInput *pInput, int Tick)
{
    if(!m_Recording || Tick < m_StartTick)
        return;

    // ПРОСТАЯ СИСТЕМА: Записываем каждый тик без оптимизации
    SEntry e = {Tick - m_StartTick, *pInput};
    if(m_File)
        io_write(m_File, &e, sizeof(e));
    m_vEntries.push_back(e);
    
    m_LastRecordTick = Tick;
    
    // Phantom синхронизация
    if((m_Testing || m_Recording) && m_PhantomActive)
    {
        m_PhantomInput = *pInput;
        m_PhantomPlayIndex = (int)m_vEntries.size();
    }
}

void CFujixTas::StartRecord()
{
    if(m_Recording)
        return;

    // Защита от множественных попыток записи
    static bool s_StartingRecord = false;
    if(s_StartingRecord)
        return;
    s_StartingRecord = true;

    GetPath(m_aFilename, sizeof(m_aFilename));
    
    if(!Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE))
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to create directory");
        s_StartingRecord = false;
        return;
    }
    
    m_File = Storage()->OpenFile(m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!m_File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for recording");
        s_StartingRecord = false;
        return;
    }

    // 🆕 ПРОСТАЯ СИСТЕМА КАК В KRX: только инпуты
    m_vEntries.clear();
    m_vEntries.reserve(60 * 60); // Резервируем место на 1 минуту (60 FPS)
    
    m_StartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_LastRecordTick = m_StartTick - 1;
    mem_zero(&m_LastInput, sizeof(m_LastInput));
    m_Recording = true;
    g_Config.m_ClFujixTasRecord = 1;

    m_RageActive = false;

    // Initialize phantom safely
    if(GameClient()->m_Snap.m_LocalClientId >= 0 && GameClient()->m_Snap.m_pLocalCharacter)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        
        // Безопасное подключение к миру
        if(Collision())
        {
            m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        }
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    else
    {
        // Если персонаж недоступен, инициализируем phantom по умолчанию
        m_PhantomCore = CCharacterCore();
        m_PhantomPrevCore = CCharacterCore();
        m_PhantomRenderInfo = CTeeRenderInfo();
    }
    
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomStep = 1;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomPlayIndex = 0;
    
    // Настройки phantom для изоляции
    m_PhantomCore.m_CollisionDisabled = false;
    m_PhantomCore.m_Solo = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_HammerHitDisabled = true;
    m_PhantomCore.m_GrenadeHitDisabled = true;
    m_PhantomCore.m_ShotgunHitDisabled = true;
    m_PhantomCore.m_LaserHitDisabled = true;
    
    m_TestStartTick = m_PhantomTick;
    m_PhantomActive = true;
    
    s_StartingRecord = false;
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "Recording started successfully");
}

void CFujixTas::FinishRecord()
{
    if(!m_Recording)
        return;

    if(m_File)
    {
        io_close(m_File);
        m_File = nullptr;
    }

    m_Recording = false;
    g_Config.m_ClFujixTasRecord = 0;
    m_LastRecordTick = -1;
    m_StopPending = false;
    m_StopTick = -1;

    m_PhantomActive = false;
}

void CFujixTas::StopRecord()
{
    if(!m_Recording || m_StopPending)
        return;

    m_StopPending = true;
    m_StopTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
}

void CFujixTas::MaybeFinishRecord()
{
    if(m_StopPending && Client()->PredGameTick(g_Config.m_ClDummy) >= m_StopTick)
        FinishRecord();
}

void CFujixTas::BlockFreezeInput(CNetObj_PlayerInput *pInput)
{
    if(!g_Config.m_ClFujixBlockFreezeLegit || !GameClient()->m_Snap.m_pLocalCharacter)
        return;

    auto PredictFreeze = [&](const CNetObj_PlayerInput &Input, int HookMode) {
        CCharacterCore Core = GameClient()->m_PredictedChar;
        Core.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        const int Steps = 12;
        for(int i = 0; i < Steps; i++)
        {
            CNetObj_PlayerInput Step = Input;
            if(HookMode == 0)
                Step.m_Hook = 0;
            else if(HookMode == 1)
                Step.m_Hook = 1;
            else if(HookMode == 2)
                Step.m_Hook = i == 0 ? 1 : 0;
            Core.m_Input = Step;
            Core.Tick(true);
            Core.Move();
            Core.Quantize();
            int Index = Collision()->GetPureMapIndex(Core.m_Pos.x, Core.m_Pos.y);
            int Tile = Collision()->GetTileIndex(Index);
            int Front = Collision()->GetFrontTileIndex(Index);
            bool Freeze = Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE ||
                          Front == TILE_FREEZE || Front == TILE_DFREEZE || Front == TILE_LFREEZE;
            if(Freeze)
                return i + 1;
        }
        return 0;
    };

    int FreezeCurrent = PredictFreeze(*pInput, -1);
    if(!FreezeCurrent)
        return;

    CNetObj_PlayerInput Adjusted = *pInput;

    int FreezeNoHook = PredictFreeze(Adjusted, 0);
    int FreezeFullHook = PredictFreeze(Adjusted, 1);
    int FreezeShortHook = PredictFreeze(Adjusted, 2);

    if(GameClient()->m_PredictedChar.m_Vel.y < 0 && FreezeFullHook &&
       (!FreezeNoHook || FreezeFullHook <= FreezeNoHook))
    {
        if(!FreezeShortHook || FreezeShortHook >= FreezeFullHook)
            Adjusted.m_Hook = 0;
    }

    if(FreezeFullHook && (!FreezeNoHook || FreezeFullHook < FreezeNoHook))
    {
        if(!(FreezeShortHook && (!FreezeNoHook || FreezeShortHook < FreezeNoHook)))
            Adjusted.m_Hook = 0;
    }
    else if(FreezeNoHook && !FreezeFullHook)
    {
        Adjusted.m_Hook = 1;
        if(GameClient()->m_PredictedChar.m_Vel.y < 0)
            Adjusted.m_Jump = 1;
    }

    CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(GameClient()->m_Snap.m_LocalClientId);
    bool OnGround = pLocalChar && pLocalChar->IsGrounded();

    if(!OnGround)
    {
        float VelX = GameClient()->m_PredictedChar.m_Vel.x;
        if(VelX > 0.5f)
            Adjusted.m_Direction = -1;
        else if(VelX < -0.5f)
            Adjusted.m_Direction = 1;
        else
            Adjusted.m_Direction = 0;

        if(FreezeCurrent <= 3)
        {
            if(VelX > 0.1f)
                Adjusted.m_Direction = -1;
            else if(VelX < -0.1f)
                Adjusted.m_Direction = 1;
        }
    }
    else
        Adjusted.m_Direction = 0;

    *pInput = Adjusted;
}

void CFujixTas::UpdateFreezeInput(CNetObj_PlayerInput *pInput)
{
    BlockFreezeInput(pInput);
}

void CFujixTas::StartPlay()
{
    if(m_Playing)
        StopPlay();

    char aPath[IO_MAX_PATH_LENGTH];
    GetPath(aPath, sizeof(aPath));
    IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for playback");
        return;
    }

    // 🆕 ПРОСТАЯ СИСТЕМА КАК В KRX: только инпуты
    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    if(m_vEntries.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "tas file is empty");
        return;
	}

    m_PlayIndex = 0;
    m_PlayStartTick = Client()->PredGameTick(g_Config.m_ClDummy) + 1;
    m_Playing = true;
    g_Config.m_ClFujixTasPlay = 1;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
}

void CFujixTas::StopPlay()
{
    m_Playing = false;
    g_Config.m_ClFujixTasPlay = 0;
    m_vEntries.clear();
    m_PlayIndex = 0;
    m_PlayStartTick = 0;
    mem_zero(&m_CurrentInput, sizeof(m_CurrentInput));
    m_RageActive = false;
}

void CFujixTas::StartTest()
{
    if(m_Testing)
        StopTest();

    char aPath[IO_MAX_PATH_LENGTH];
    GetPath(aPath, sizeof(aPath));
    IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "failed to open file for test");
        return;
    }

    // 🆕 ПРОСТАЯ СИСТЕМА КАК В KRX: только инпуты
    m_vEntries.clear();
    SEntry e;
    while(io_read(File, &e, sizeof(e)) == sizeof(e))
        m_vEntries.push_back(e);
    io_close(File);

    if(m_vEntries.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "tas file is empty");
        return;
	}

    if(GameClient()->m_Snap.m_LocalClientId >= 0)
    {
        m_PhantomCore = GameClient()->m_PredictedChar;
        m_PhantomPrevCore = m_PhantomCore;
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomRenderInfo = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderInfo;
    }
    m_PhantomTick = Client()->PredGameTick(g_Config.m_ClDummy);
    m_PhantomStep = 1;
    mem_zero(&m_PhantomInput, sizeof(m_PhantomInput));
    m_PhantomPlayIndex = 0;

    // ignore other players while keeping map collisions
    m_PhantomCore.m_CollisionDisabled = false;
    m_PhantomCore.m_Solo = true;
    m_PhantomCore.m_HookHitDisabled = true;
    m_PhantomCore.m_HammerHitDisabled = true;
    m_PhantomCore.m_GrenadeHitDisabled = true;
    m_PhantomCore.m_ShotgunHitDisabled = true;
    m_PhantomCore.m_LaserHitDisabled = true;

    m_TestStartTick = m_PhantomTick;
    m_Testing = true;
    m_PhantomActive = true;
    g_Config.m_ClFujixTasTest = 1;
}

void CFujixTas::StopTest()
{
    m_Testing = false;
    g_Config.m_ClFujixTasTest = 0;
    m_PhantomActive = false;
    m_vEntries.clear();
    m_RageActive = false;
}

void CFujixTas::TickPhantomUpTo(int TargetTick)
{
    if(!m_PhantomActive)
        return;

    while(m_PhantomTick < TargetTick)
    {
        // keep world pointers fresh in case prediction updated
        m_PhantomCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
        m_PhantomPrevCore = m_PhantomCore;

        // ВАЖНО: Сначала применяем крюк, потом инпут, затем симулируем движение
        if(m_Testing || m_Playing)
        {
            UpdatePlaybackInput(); // Это уже включает ApplyHookEvents
        }

        m_PhantomCore.m_Input = m_PhantomInput;
        m_PhantomCore.Tick(true);
        m_PhantomCore.Move();
        m_PhantomCore.Quantize();

        m_PhantomTick++;
    }
}

void CFujixTas::TickPhantom()
{
    if(!m_PhantomActive)
        return;
    int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
    TickPhantomUpTo(PredTick);
}

void CFujixTas::CoreToCharacter(const CCharacterCore &Core, CNetObj_Character *pChar, int Tick)
{
    CNetObj_CharacterCore CCore;
    Core.Write(&CCore);
    mem_zero(pChar, sizeof(*pChar));
    pChar->m_X = CCore.m_X;
    pChar->m_Y = CCore.m_Y;
    pChar->m_VelX = CCore.m_VelX;
    pChar->m_VelY = CCore.m_VelY;
    pChar->m_Angle = CCore.m_Angle;
    pChar->m_Direction = CCore.m_Direction;
    pChar->m_Weapon = Core.m_ActiveWeapon;
    pChar->m_HookState = CCore.m_HookState;
    pChar->m_HookTick = CCore.m_HookTick;
    pChar->m_HookX = CCore.m_HookX;
    pChar->m_HookY = CCore.m_HookY;
    pChar->m_HookDx = CCore.m_HookDx;
    pChar->m_HookDy = CCore.m_HookDy;
    pChar->m_HookedPlayer = CCore.m_HookedPlayer;
    pChar->m_Jumped = CCore.m_Jumped;
    pChar->m_Tick = Tick;
    pChar->m_AttackTick = Core.m_HookTick + (Client()->GameTick(g_Config.m_ClDummy) - Tick);
}

void CFujixTas::OnUpdate()
{
    if(g_Config.m_ClFujixTasRecord && !m_Recording)
        StartRecord();
    else if(!g_Config.m_ClFujixTasRecord && m_Recording)
        StopRecord();

    if(g_Config.m_ClFujixTasPlay && !m_Playing)
        StartPlay();
    else if(!g_Config.m_ClFujixTasPlay && m_Playing)
        StopPlay();

    if(g_Config.m_ClFujixTasTest && !m_Testing)
        StartTest();
    else if(!g_Config.m_ClFujixTasTest && m_Testing)
        StopTest();

    MaybeFinishRecord();
    // УБРАНО: RecordHookState - используем только простые инпуты
    TickPhantom();
}

void CFujixTas::OnRender()
{
    if(m_PhantomActive)
    {
        CNetObj_Character Prev, Curr;
        CoreToCharacter(m_PhantomPrevCore, &Prev, m_PhantomTick - 1);
        CoreToCharacter(m_PhantomCore, &Curr, m_PhantomTick);

        CTeeRenderInfo PhantomRenderInfo = m_PhantomRenderInfo;
		PhantomRenderInfo.m_ColorBody = ColorRGBA(0.7f, 0.7f, 1.0f, 0.6f);
		PhantomRenderInfo.m_ColorFeet = ColorRGBA(0.7f, 0.7f, 1.0f, 0.6f);

        GameClient()->m_Players.RenderHook(&Prev, &Curr, &PhantomRenderInfo, -2);
        GameClient()->m_Players.RenderHookCollLine(&Prev, &Curr, -2);
        GameClient()->m_Players.RenderPlayer(&Prev, &Curr, &PhantomRenderInfo, -2);

        RenderFuturePath(g_Config.m_ClFujixTasPreviewTicks);
    }
    
    // Render smart autopilot planned path in rage mode
    if(m_RageActive && m_pSmartAutopilot && m_SmartAutopilotInitialized)
    {
        RenderAutopilotPath();
    }
}

void CFujixTas::RenderFuturePath(int TicksAhead)
{
    if(TicksAhead <= 0 || !m_PhantomActive)
        return;

    // Избегаем копирования всего объекта - используем только нужные данные
    CCharacterCore TempCore = m_PhantomCore;
    TempCore.SetCoreWorld(&GameClient()->m_PredictedWorld.m_Core, Collision(), GameClient()->m_PredictedWorld.Teams());
    
    std::vector<vec2> Points;
    Points.reserve(TicksAhead + 1);
    Points.push_back(TempCore.m_Pos);

    int CurrentTick = m_PhantomTick;
    int TargetTick = CurrentTick + TicksAhead;
    int CurrentPlayIndex = m_PhantomPlayIndex;
    
    while(CurrentTick < TargetTick)
    {
        // Получаем инпут для текущего тика
        CNetObj_PlayerInput CurrentInput = m_PhantomInput;
        
        if(m_Testing || m_Playing)
        {
            // Симулируем получение инпута
            int BaseTick = m_Playing ? m_PlayStartTick : m_TestStartTick;
            while(CurrentPlayIndex < (int)m_vEntries.size() && 
                  BaseTick + m_vEntries[CurrentPlayIndex].m_Tick <= CurrentTick)
            {
                CurrentInput = m_vEntries[CurrentPlayIndex].m_Input;
                CurrentPlayIndex++;
            }
        }
        
        TempCore.m_Input = CurrentInput;
        TempCore.Tick(true);
        TempCore.Move();
        TempCore.Quantize();
        
        Points.push_back(TempCore.m_Pos);
        CurrentTick++;
    }

    if(Points.size() <= 1)
        return;

    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    Graphics()->SetColor(0.2f, 1.0f, 0.2f, 0.5f);
    for(size_t i = 1; i < Points.size(); i++)
    {
        IGraphics::CLineItem Line(Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y);
        Graphics()->LinesDraw(&Line, 1);
    }
    Graphics()->LinesEnd();
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CFujixTas::RenderAutopilotPath()
{
    if(!m_pSmartAutopilot || !GameClient()->m_Snap.m_pLocalCharacter)
        return;
    
    const std::vector<SPathNode> &PathNodes = m_pSmartAutopilot->GetCurrentPath();
    
    if(PathNodes.size() < 2)
        return;
    
    // Convert path nodes to positions
    std::vector<vec2> Path;
    Path.reserve(PathNodes.size());
    for(const SPathNode &Node : PathNodes)
    {
        Path.push_back(Node.m_Pos);
    }
    
    // Render the planned path
    Graphics()->LinesBegin();
    Graphics()->SetColor(1.0f, 0.5f, 0.0f, 0.8f); // Orange color for autopilot path
    
    for(size_t i = 1; i < Path.size(); i++)
    {
        IGraphics::CLineItem Line(Path[i - 1].x, Path[i - 1].y, Path[i].x, Path[i].y);
        Graphics()->LinesDraw(&Line, 1);
    }
    
    Graphics()->LinesEnd();
    
    // Render target marker
    Graphics()->QuadsBegin();
    Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.8f); // Red color for target
    IGraphics::CQuadItem QuadItem(m_RageTarget.x - 8.0f, m_RageTarget.y - 8.0f, 16.0f, 16.0f);
    Graphics()->QuadsDrawTL(&QuadItem, 1);
    Graphics()->QuadsEnd();
    
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f); // Reset color
}
void CFujixTas::ConRecord(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Recording)
        pSelf->StopRecord();
    else
        pSelf->StartRecord();
}

void CFujixTas::ConPlay(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Playing)
        pSelf->StopPlay();
    else
        pSelf->StartPlay();
}

void CFujixTas::ConTest(IConsole::IResult *pResult, void *pUserData)
{
    CFujixTas *pSelf = static_cast<CFujixTas *>(pUserData);
    if(pSelf->m_Testing)
        pSelf->StopTest();
    else
        pSelf->StartTest();
}

void CFujixTas::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Start/stop FUJIX TAS recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play FUJIX TAS for current map");
    Console()->Register("fujix_test", "", CFGFLAG_CLIENT, ConTest, this, "Play FUJIX TAS as phantom");
}

void CFujixTas::OnMapLoad()
{
    Storage()->CreateFolder(ms_pFujixDir, IStorage::TYPE_SAVE);
    StopPlay();
    if(m_Recording)
        FinishRecord();
    StopTest();
    m_RageActive = false;
    
    // Initialize smart autopilot with collision system
    if(m_pSmartAutopilot && Collision())
    {
        m_pSmartAutopilot->Init(Collision());
        m_SmartAutopilotInitialized = true;
    }
}

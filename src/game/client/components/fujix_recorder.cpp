#include "fujix_recorder.h"
#include <game/client/gameclient.h>
#include <game/client/components/controls.h>
#include <game/generated/protocol.h>
#include <engine/shared/jsonwriter.h>
#include <engine/shared/json.h>
#include <engine/storage.h>
#include <base/math.h>

void CFujixRecorder::ConRecord(IConsole::IResult *pResult, void *pUserData)
{
    CFujixRecorder *pSelf = static_cast<CFujixRecorder *>(pUserData);
    pSelf->ToggleRecord();
}

void CFujixRecorder::ConPlay(IConsole::IResult *pResult, void *pUserData)
{
    CFujixRecorder *pSelf = static_cast<CFujixRecorder *>(pUserData);
    pSelf->StartPlay();
}

void CFujixRecorder::OnConsoleInit()
{
    Console()->Register("fujix_record", "", CFGFLAG_CLIENT, ConRecord, this, "Toggle Fujix recording");
    Console()->Register("fujix_play", "", CFGFLAG_CLIENT, ConPlay, this, "Play Fujix recording");
}

void CFujixRecorder::OnMapLoad()
{
    m_Frames.clear();
    m_Recording = false;
    m_Playing = false;
    m_PlayPos = 0;
    g_Config.m_ClFujixRecord = 0;
}

void CFujixRecorder::ToggleRecord()
{
    if(m_Recording)
    {
        CFrame F{};
        F.m_Pos = GameClient()->m_PredictedChar.m_Pos;
        F.m_Vel = GameClient()->m_PredictedChar.m_Vel;
        const CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
        F.m_Direction = Input.m_Direction;
        F.m_Jump = Input.m_Jump & 1;
        F.m_Fire = Input.m_Fire & 1;
        F.m_Hook = Input.m_Hook & 1;
        F.m_TargetX = Input.m_TargetX;
        F.m_TargetY = Input.m_TargetY;
        m_Frames.push_back(F);

        m_Recording = false;
        Save();
        g_Config.m_ClFujixRecord = 0;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix",
                          "record saved with %d frames", (int)m_Frames.size());
    }
    else
    {
        m_Frames.clear();
        m_Recording = true;
        m_StartPos = GameClient()->m_LocalCharacterPos;
        g_Config.m_ClFujixRecord = 1;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "recording started");
    }
}

void CFujixRecorder::StartPlay()
{
    if(m_Playing)
    {
        m_Playing = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback stopped");
        return;
    }

    if(Client()->State() != IClient::STATE_ONLINE || !GameClient()->m_Snap.m_pLocalCharacter)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "not ingame");
        return;
    }

    if(m_Recording)
        ToggleRecord();
    if(!Load())
        return;
    if(distance(GameClient()->m_LocalCharacterPos, m_StartPos) > 32.0f)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "Start position mismatch");
        return;
    }
    m_Playing = true;
    m_PlayPos = 0;
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback started");
}

void CFujixRecorder::Save() const
{
    char aFilename[IO_MAX_PATH_LENGTH];
    str_format(aFilename, sizeof(aFilename), "fujix/%s.rec", Client()->GetCurrentMap());
    Storage()->CreateFolder("fujix", IStorage::TYPE_SAVE);
    IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!File)
        return;

    io_write(File, &m_StartPos.x, sizeof(m_StartPos.x));
    io_write(File, &m_StartPos.y, sizeof(m_StartPos.y));
    int32_t Count = (int32_t)m_Frames.size();
    io_write(File, &Count, sizeof(Count));
    for(const CFrame &F : m_Frames)
    {
        io_write(File, &F.m_Pos.x, sizeof(F.m_Pos.x));
        io_write(File, &F.m_Pos.y, sizeof(F.m_Pos.y));
        io_write(File, &F.m_Vel.x, sizeof(F.m_Vel.x));
        io_write(File, &F.m_Vel.y, sizeof(F.m_Vel.y));
        io_write(File, &F.m_Direction, sizeof(F.m_Direction));
        io_write(File, &F.m_Jump, sizeof(F.m_Jump));
        io_write(File, &F.m_Fire, sizeof(F.m_Fire));
        io_write(File, &F.m_Hook, sizeof(F.m_Hook));
        io_write(File, &F.m_TargetX, sizeof(F.m_TargetX));
        io_write(File, &F.m_TargetY, sizeof(F.m_TargetY));
    }
    io_close(File);
}

bool CFujixRecorder::Load()
{
    char aFilename[IO_MAX_PATH_LENGTH];
    str_format(aFilename, sizeof(aFilename), "fujix/%s.rec", Client()->GetCurrentMap());
    IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
    if(!File)
        return false;

    if(io_read(File, &m_StartPos.x, sizeof(m_StartPos.x)) != sizeof(m_StartPos.x))
    {
        io_close(File);
        return false;
    }
    if(io_read(File, &m_StartPos.y, sizeof(m_StartPos.y)) != sizeof(m_StartPos.y))
    {
        io_close(File);
        return false;
    }
    int32_t Count = 0;
    if(io_read(File, &Count, sizeof(Count)) != sizeof(Count))
    {
        io_close(File);
        return false;
    }
    m_Frames.clear();
    for(int i = 0; i < Count; i++)
    {
        CFrame F{};
        if(io_read(File, &F.m_Pos.x, sizeof(F.m_Pos.x)) != sizeof(F.m_Pos.x)) break;
        io_read(File, &F.m_Pos.y, sizeof(F.m_Pos.y));
        io_read(File, &F.m_Vel.x, sizeof(F.m_Vel.x));
        io_read(File, &F.m_Vel.y, sizeof(F.m_Vel.y));
        io_read(File, &F.m_Direction, sizeof(F.m_Direction));
        io_read(File, &F.m_Jump, sizeof(F.m_Jump));
        io_read(File, &F.m_Fire, sizeof(F.m_Fire));
        io_read(File, &F.m_Hook, sizeof(F.m_Hook));
        io_read(File, &F.m_TargetX, sizeof(F.m_TargetX));
        io_read(File, &F.m_TargetY, sizeof(F.m_TargetY));
        m_Frames.push_back(F);
    }
    io_close(File);
    return !m_Frames.empty();
}

void CFujixRecorder::OnUpdate()
{
    if(!GameClient()->m_Snap.m_pLocalCharacter)
        return;

    if(m_Recording)
    {
        CFrame F{};
        F.m_Pos = GameClient()->m_PredictedChar.m_Pos;
        F.m_Vel = GameClient()->m_PredictedChar.m_Vel;
        const CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
        F.m_Direction = Input.m_Direction;
        F.m_Jump = Input.m_Jump & 1;
        F.m_Fire = Input.m_Fire & 1;
        F.m_Hook = Input.m_Hook & 1;
        F.m_TargetX = Input.m_TargetX;
        F.m_TargetY = Input.m_TargetY;
        m_Frames.push_back(F);
    }
    else if(m_Playing)
    {
        if(m_PlayPos >= (int)m_Frames.size())
        {
            m_Playing = false;
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fujix", "playback finished");
            return;
        }
        const CFrame &F = m_Frames[m_PlayPos];

        GameClient()->m_PredictedChar.m_Pos = F.m_Pos;
        GameClient()->m_PredictedChar.m_Vel = F.m_Vel;
        GameClient()->m_PredictedPrevChar.m_Pos = F.m_Pos;
        GameClient()->m_PredictedPrevChar.m_Vel = F.m_Vel;

        CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
        Input.m_Direction = F.m_Direction;
        Input.m_Jump = (Input.m_Jump & ~1) | (F.m_Jump & 1);
        Input.m_Fire = (Input.m_Fire & ~1) | (F.m_Fire & 1);
        Input.m_Hook = (Input.m_Hook & ~1) | (F.m_Hook & 1);
        Input.m_TargetX = F.m_TargetX;
        Input.m_TargetY = F.m_TargetY;
        Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
        m_PlayPos++;

        int Tile = Collision()->GetTileIndex(Collision()->GetPureMapIndex(GameClient()->m_LocalCharacterPos));
        if((Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE) && m_PlayPos > 20)
            m_PlayPos -= 20;
    }
}

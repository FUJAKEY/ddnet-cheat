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
    str_format(aFilename, sizeof(aFilename), "fujix/%s.json", Client()->GetCurrentMap());
    Storage()->CreateFolder("fujix", IStorage::TYPE_SAVE);
    IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
    if(!File)
        return;

    CJsonFileWriter Writer(File);
    Writer.BeginObject();
    Writer.WriteAttribute("start_x");
    Writer.WriteIntValue(round_to_int(m_StartPos.x));
    Writer.WriteAttribute("start_y");
    Writer.WriteIntValue(round_to_int(m_StartPos.y));
    Writer.WriteAttribute("frames");
    Writer.BeginArray();
    for(const CFrame &F : m_Frames)
    {
        Writer.BeginObject();
        Writer.WriteAttribute("x");
        Writer.WriteIntValue(round_to_int(F.m_Pos.x));
        Writer.WriteAttribute("y");
        Writer.WriteIntValue(round_to_int(F.m_Pos.y));
        Writer.WriteAttribute("vx");
        Writer.WriteIntValue(round_to_int(F.m_Vel.x));
        Writer.WriteAttribute("vy");
        Writer.WriteIntValue(round_to_int(F.m_Vel.y));
        Writer.WriteAttribute("dir");
        Writer.WriteIntValue(F.m_Direction);
        Writer.WriteAttribute("jump");
        Writer.WriteIntValue(F.m_Jump);
        Writer.WriteAttribute("fire");
        Writer.WriteIntValue(F.m_Fire);
        Writer.WriteAttribute("hook");
        Writer.WriteIntValue(F.m_Hook);
        Writer.WriteAttribute("tx");
        Writer.WriteIntValue(F.m_TargetX);
        Writer.WriteAttribute("ty");
        Writer.WriteIntValue(F.m_TargetY);
        Writer.EndObject();
    }
    Writer.EndArray();
    Writer.EndObject();
}

bool CFujixRecorder::Load()
{
    char aFilename[IO_MAX_PATH_LENGTH];
    str_format(aFilename, sizeof(aFilename), "fujix/%s.json", Client()->GetCurrentMap());
    void *pData = nullptr;
    unsigned DataSize = 0;
    if(!Storage()->ReadFile(aFilename, IStorage::TYPE_SAVE, &pData, &DataSize))
        return false;

    json_settings Settings{};
    char aErr[256];
    json_value *pJson = json_parse_ex(&Settings, static_cast<const json_char *>(pData), DataSize, aErr);
    free(pData);
    if(!pJson || pJson->type != json_object)
    {
        if(pJson)
            json_value_free(pJson);
        return false;
    }

    const json_value &StartX = (*pJson)["start_x"];
    const json_value &StartY = (*pJson)["start_y"];
    const json_value &Frames = (*pJson)["frames"];
    if(StartX.type != json_integer || StartY.type != json_integer || Frames.type != json_array)
    {
        json_value_free(pJson);
        return false;
    }

    m_StartPos = vec2(json_int_get(&StartX), json_int_get(&StartY));
    m_Frames.clear();
    for(int i = 0; i < json_array_length(&Frames); i++)
    {
        const json_value &Fv = *json_array_get(&Frames, i);
        if(Fv.type != json_object)
            continue;
        CFrame F{};
        F.m_Pos.x = json_int_get(&Fv["x"]);
        F.m_Pos.y = json_int_get(&Fv["y"]);
        F.m_Vel.x = json_int_get(&Fv["vx"]);
        F.m_Vel.y = json_int_get(&Fv["vy"]);
        F.m_Direction = json_int_get(&Fv["dir"]);
        F.m_Jump = json_int_get(&Fv["jump"]);
        F.m_Fire = json_int_get(&Fv["fire"]);
        F.m_Hook = json_int_get(&Fv["hook"]);
        F.m_TargetX = json_int_get(&Fv["tx"]);
        F.m_TargetY = json_int_get(&Fv["ty"]);
        m_Frames.push_back(F);
    }
    json_value_free(pJson);
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
        F.m_Jump = Input.m_Jump;
        F.m_Fire = Input.m_Fire;
        F.m_Hook = Input.m_Hook;
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
        CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
        Input.m_Direction = F.m_Direction;
        Input.m_Jump = F.m_Jump;
        Input.m_Fire = F.m_Fire;
        Input.m_Hook = F.m_Hook;
        Input.m_TargetX = F.m_TargetX;
        Input.m_TargetY = F.m_TargetY;
        Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
        m_PlayPos++;

        int Tile = Collision()->GetTileIndex(Collision()->GetPureMapIndex(GameClient()->m_LocalCharacterPos));
        if((Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE) && m_PlayPos > 20)
            m_PlayPos -= 20;
    }
}

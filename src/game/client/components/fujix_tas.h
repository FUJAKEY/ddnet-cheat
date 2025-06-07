#ifndef GAME_CLIENT_COMPONENTS_FUJIX_TAS_H
#define GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

#include <game/client/component.h>

class CFujixTas : public CComponent
{
public:
    static const char *ms_pFujixDir;

private:
    bool m_Recording;
    char m_aFilename[IO_MAX_PATH_LENGTH];

    void GetPath(char *pBuf, int Size) const;

    static void ConRecord(IConsole::IResult *pResult, void *pUserData);
    static void ConPlay(IConsole::IResult *pResult, void *pUserData);

public:
    CFujixTas();
    virtual int Sizeof() const override { return sizeof(*this); }

    virtual void OnConsoleInit() override;
    virtual void OnMapLoad() override;

    void StartRecord();
    void StopRecord();
    void Play();
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_TAS_H

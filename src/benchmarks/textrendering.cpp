#include "SDL.h"
#ifdef main
#undef main
#endif

#include <engine/kernel.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <base/math.h>

#if defined(CONF_PLATFORM_MACOSX) || defined(__ANDROID__)
extern "C" int SDL_main(int argc, char **argv_) // ignore_convention
{
	const char **argv = const_cast<const char **>(argv_);
#else
int main(int argc, const char **argv) // ignore_convention
{
#endif
	dbg_logger_stdout();
	
	IKernel *pKernel = IKernel::Create();

	// init SDL
	{
		if(SDL_Init(0) < 0)
		{
			dbg_msg("client", "unable to init SDL base: %s", SDL_GetError());
			return 1;
		}

		atexit(SDL_Quit); // ignore_convention
	}

	// init graphics
	IEngineGraphics *pGraphics = CreateEngineGraphicsThreaded();
	pKernel->RegisterInterface(static_cast<IEngineGraphics*>(pGraphics));
	pKernel->RegisterInterface(static_cast<IGraphics*>(pGraphics));
	if(pGraphics->Init() != 0)
	{
		dbg_msg("client", "couldn't init graphics");
		return 1;
	}
	
	// init textrender
	IEngineTextRender *pTextRender = CreateEngineTextRender();
	pKernel->RegisterInterface(static_cast<IEngineTextRender*>(pTextRender));
	pKernel->RegisterInterface(static_cast<ITextRender*>(pTextRender));
	pTextRender->Init();
	
	CFont *pDefaultFont = pTextRender->LoadFont("data/fonts/DejaVuSansCJKName.ttf");
	pTextRender->SetDefaultFont(pDefaultFont);
	
	pGraphics->BlendNormal();
	
	dbg_msg("benchmark", "ready!");
	
	// text from https://fr.wikibooks.org/wiki/Translinguisme/Par_expression/Bonjour
	static const char s_aaText[][32] =
	{
		"azertyuiop",
		"ըթժիլխծկհձ",
		"ءحآخغأدؤذإ",
		"ႠႡႢႣႤႥႦႧႨႩ",
		"ΑΒΓΔΕΖΗΘΙΚ",
		"٠١٢٣٤٥٦٧٨٩",
		"ЀЎМЪЁЏНЫЂА",
		"QSDFGHJKLM",
		"رئزاسبشفةص",
		"ﬡﬢﬣﬤﬥﬦﬧﬨ﬩שׁ",
		"αβγδεζηθικ",
		"ႭႮႯႰႱႲႳႴႵႶ",
		"ՋՍՌՎՏՐՑՒՓՔ",
		"ОЬЃБПЭЄВРЮ",
		"0123456789",
		"wxcvbnqsdf",
		"قتضكثطلجظم",
		"ΛΜΝΞΟΠΡΣΤΥ",
		"ნოპჟრსტუფქ",
		"еужфзхицйч",
		"ԱԲԳԴԵԶԷԸԹԺ",
		"כלםמןנסעףפ",
		"AZERTYUIOP",
	};
	static const int s_NbText = sizeof(s_aaText) / sizeof(s_aaText[0]);
	static const int s_Duration = 20; // in seconds
	
	float Fps = 0.0f;
	unsigned int FrameCounter = 0;
	
	{
		int64 StartTime = time_get();
		while(1)
		{
			int64 CurrentTime = time_get();
			double RenderTime = 0.1 * (CurrentTime - StartTime) / (double)time_freq();

			pGraphics->Clear(0.5f, 0.5f, 0.5f);
			
			float Height = 300.0f;
			float Width = Height * pGraphics->ScreenAspect();
			pGraphics->MapScreen(0.0f, 0.0f, Width, Height);
			
			for(int j = 0; j < 3; j++)
			{
				for(int i = 0; i < s_NbText; i++)
				{
					float TimeWrap = fmod(RenderTime + i / (double)(s_NbText), 1.0f);
					
					float PositionX = j * Width / 3.0f;
					float PositionY = TimeWrap * Height * 0.8f;
					float Size = 8 + TimeWrap * 40.0;
					
					CTextCursor Cursor;
					pTextRender->SetCursor(&Cursor, PositionX, PositionY, Size, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
					Cursor.m_LineWidth = Width;
					pTextRender->TextEx(&Cursor, s_aaText[i % s_NbText], -1);
				}
			}
			
			pGraphics->Swap();
			
			FrameCounter++;
			int64 TimeDiff = CurrentTime - StartTime;
			if(TimeDiff > time_freq() * s_Duration)
			{
				double Time = TimeDiff / (double)time_freq();
				Fps = (double)FrameCounter / Time;
				break;
			}
		}
	}

	dbg_msg("Benchmark", "Result: %f fps", Fps);

	delete pTextRender;
	delete pGraphics;
	delete pKernel;

	return 0;
}

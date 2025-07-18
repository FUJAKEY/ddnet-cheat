/* FUJIX Settings - GEROS BOT Implementation */

#include <base/color.h>
#include <base/math.h>

#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/animstate.h>
#include <game/client/ui.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>

void CMenus::RenderSettingsFujix(CUIRect MainView)
{
	CUIRect Button, Label;
	char aBuf[128];

	// Header
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Label, &MainView);
	str_format(aBuf, sizeof(aBuf), "FUJIX - Advanced Prediction Bot");
	Ui()->DoLabel(&Label, aBuf, 20.0f, TEXTALIGN_MC);
	
	// Description
	MainView.HSplitTop(10.0f, nullptr, &MainView);
	MainView.HSplitTop(30.0f, &Label, &MainView);
	str_format(aBuf, sizeof(aBuf), "Intelligent auto-rescue system with 8-tick prediction");
	Ui()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_MC);
	
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	
	// GEROS BOT Enable/Disable
	CUIRect GerosSection;
	MainView.HSplitTop(300.0f, &GerosSection, &MainView);
	GerosSection.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	GerosSection.Margin(20.0f, &GerosSection);
	
	GerosSection.HSplitTop(30.0f, &Label, &GerosSection);
	str_format(aBuf, sizeof(aBuf), "🤖 GEROS BOT - Ultimate Survival System");
	Ui()->DoLabel(&Label, aBuf, 16.0f, TEXTALIGN_ML);
	
	GerosSection.HSplitTop(10.0f, nullptr, &GerosSection);
	
	// Enable checkbox - УВЕЛИЧЕННАЯ КНОПКА
	static CButtonContainer s_GerosEnable;
	CUIRect BigButton;
	GerosSection.HSplitTop(45.0f, &BigButton, &GerosSection);
	BigButton.Draw(g_Config.m_FujixGerosBot ? ColorRGBA(0.2f, 0.8f, 0.2f, 0.3f) : ColorRGBA(0.8f, 0.2f, 0.2f, 0.3f), IGraphics::CORNER_ALL, 8.0f);
	BigButton.Margin(5.0f, &BigButton);
	
	char aBufButton[128];
	str_format(aBufButton, sizeof(aBufButton), "%s GEROS BOT", g_Config.m_FujixGerosBot ? "🟢 DISABLE" : "🔴 ENABLE");
	if(DoButton_Menu(&s_GerosEnable, aBufButton, 0, &BigButton, 0, nullptr, IGraphics::CORNER_ALL))
	{
		g_Config.m_FujixGerosBot ^= 1;
		// Принудительно сохраняем конфиг
		GameClient()->m_Menus.m_NeedSendinfo = true;
	}
	
	// 🧪 ТЕСТОВАЯ КНОПКА ДЛЯ ПРОВЕРКИ РАБОТЫ БОТА
	GerosSection.HSplitTop(5.0f, nullptr, &GerosSection);
	GerosSection.HSplitTop(30.0f, &Button, &GerosSection);
	static CButtonContainer s_TestBot;
	if(DoButton_Menu(&s_TestBot, "🧪 TEST BOT (Force Emergency Mode)", 0, &Button, 0, nullptr, IGraphics::CORNER_ALL))
	{
		CGameClient *pGameClient = GameClient();
		if(pGameClient && g_Config.m_FujixGerosBot)
		{
			// Принудительно активируем emergency mode для теста
			pGameClient->m_FujixGerosBot.ForceEmergencyMode();
		}
	}
	// Status indicator
	GerosSection.HSplitTop(10.0f, nullptr, &GerosSection);
	GerosSection.HSplitTop(20.0f, &Label, &GerosSection);
	if(g_Config.m_FujixGerosBot)
	{
		TextRender()->TextColor(0.3f, 1.0f, 0.3f, 1.0f);
		str_format(aBuf, sizeof(aBuf), "● Status: ACTIVE - Protecting player from death");
		
		// 🔍 ДОБАВЛЯЕМ ОТЛАДОЧНУЮ ИНФОРМАЦИЮ
		CGameClient *pGameClient = GameClient();
		if(pGameClient && pGameClient->m_FujixGerosBot.IsActive())
		{
			bool EmergencyState = pGameClient->m_FujixGerosBot.IsInEmergencyState();
			bool ShouldOverride = pGameClient->m_FujixGerosBot.ShouldOverrideInput();
			bool EmergencyMode = pGameClient->m_FujixGerosBot.IsInEmergencyMode();
			
			char aDebugBuf[256];
			str_format(aDebugBuf, sizeof(aDebugBuf), "  🤖 Emergency: %s | Override: %s | Mode: %s", 
				EmergencyState ? "YES" : "NO",
				ShouldOverride ? "YES" : "NO", 
				EmergencyMode ? "ACTIVE" : "STANDBY"
			);
			
			Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);
			GerosSection.HSplitTop(15.0f, &Label, &GerosSection);
			TextRender()->TextColor(0.7f, 0.7f, 1.0f, 1.0f);
			Ui()->DoLabel(&Label, aDebugBuf, 10.0f, TEXTALIGN_ML);
		}
		else
		{
			Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);
		}
	}
	else
	{
		TextRender()->TextColor(1.0f, 0.3f, 0.3f, 1.0f);
		str_format(aBuf, sizeof(aBuf), "● Status: DISABLED - Click button above to enable");
		Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);
	}
	GerosSection.HSplitTop(20.0f, nullptr, &GerosSection);
	str_format(aBuf, sizeof(aBuf), "Features:");
	Ui()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
	
	const char *apFeatures[] = {
		"• 8-tick future prediction algorithm",
		"• Automatic hook deployment for escaping danger",
		"• Smart movement and jumping to avoid projectiles",
		"• Death prevention even from intentional suicide attempts",
		"• Real-time threat analysis and response",
		"• Adaptive rescue strategies based on map layout"
	};
	
	for(int i = 0; i < (int)(sizeof(apFeatures) / sizeof(apFeatures[0])); i++)
	{
		GerosSection.HSplitTop(18.0f, &Label, &GerosSection);
		Ui()->DoLabel(&Label, apFeatures[i], 12.0f, TEXTALIGN_ML);
	}
	
	// Advanced settings (if enabled)
	if(g_Config.m_FujixGerosBot)
	{
		MainView.HSplitTop(20.0f, nullptr, &MainView);
		
		CUIRect AdvancedSection;
		MainView.HSplitTop(200.0f, &AdvancedSection, &MainView);
		AdvancedSection.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.15f), IGraphics::CORNER_ALL, 10.0f);
		AdvancedSection.Margin(20.0f, &AdvancedSection);
		
		AdvancedSection.HSplitTop(25.0f, &Label, &AdvancedSection);
		str_format(aBuf, sizeof(aBuf), "⚙️ Advanced Configuration");
		Ui()->DoLabel(&Label, aBuf, 14.0f, TEXTALIGN_ML);
		
		AdvancedSection.HSplitTop(10.0f, nullptr, &AdvancedSection);
		
		// Aggressiveness slider
		static int s_GerosAggressiveness = 0;
		AdvancedSection.HSplitTop(20.0f, &Button, &AdvancedSection);
		Ui()->DoScrollbarOption(&s_GerosAggressiveness, &g_Config.m_FujixGerosAggressiveness, &Button, "Rescue Aggressiveness", 1, 10, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_MULTILINE);
		
		AdvancedSection.HSplitTop(10.0f, nullptr, &AdvancedSection);
		
		// Prediction distance
		static int s_GerosPrediction = 0;
		AdvancedSection.HSplitTop(20.0f, &Button, &AdvancedSection);
		Ui()->DoScrollbarOption(&s_GerosPrediction, &g_Config.m_FujixGerosPredictionTicks, &Button, "Prediction Ticks", 4, 12, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_MULTILINE);
		
		AdvancedSection.HSplitTop(15.0f, nullptr, &AdvancedSection);
		
		// Anti-suicide protection
		static int s_GerosAntiSuicide = 0;
		DoButton_CheckBoxAutoVMarginAndSet(&s_GerosAntiSuicide, "Anti-Suicide Protection (prevents intentional death)", &g_Config.m_FujixGerosAntiSuicide, &AdvancedSection, 5.0f);
		
		// Performance info
		AdvancedSection.HSplitTop(10.0f, nullptr, &AdvancedSection);
		AdvancedSection.HSplitTop(15.0f, &Label, &AdvancedSection);
		TextRender()->TextColor(0.7f, 0.7f, 0.7f, 1.0f);
		str_format(aBuf, sizeof(aBuf), "Performance: ~0.1ms per prediction cycle");
		Ui()->DoLabel(&Label, aBuf, 10.0f, TEXTALIGN_ML);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
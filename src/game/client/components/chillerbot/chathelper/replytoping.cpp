// chillerbot-ux reply to ping

#include "game/client/gameclient.h"

#include "replytoping.h"

CLangParser &CReplyToPing::LangParser() { return ChatHelper()->LangParser(); }

CReplyToPing::CReplyToPing(CChatHelper *pChatHelper, const char *pMessageAuthor, const char *pMessageAuthorClan, const char *pMessage, char *pResponse, long unsigned int SizeOfResponse)
{
	m_pChatHelper = pChatHelper;

	m_pMessageAuthor = pMessageAuthor;
	m_pMessageAuthorClan = pMessageAuthorClan;
	m_pMessage = pMessage;
	m_pResponse = pResponse;
	m_SizeOfResponse = SizeOfResponse;
}

bool CReplyToPing::WhyWar(const char *pVictim)
{
	if(!pVictim)
		return false;

	bool HasWar = true;
	// aVictim also has to hold the full own name to match the chop off
	char aVictim[MAX_NAME_LENGTH + 3 + MAX_NAME_LENGTH];
	str_copy(aVictim, pVictim, sizeof(aVictim));
	if(!ChatHelper()->GameClient()->m_WarList.IsWarlist(aVictim) && !ChatHelper()->GameClient()->m_WarList.IsTraitorlist(aVictim))
	{
		HasWar = false;
		while(str_endswith(aVictim, "?")) // cut off the question marks from the victim name
			aVictim[str_length(aVictim) - 1] = '\0';
		// cut off own name from the victime name if question in this format "why do you war foo (your name)"
		char aOwnName[MAX_NAME_LENGTH + 3];
		// main tee
		str_format(aOwnName, sizeof(aOwnName), " %s", ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[0]].m_aName);
		if(str_endswith_nocase(aVictim, aOwnName))
			aVictim[str_length(aVictim) - str_length(aOwnName)] = '\0';
		if(ChatHelper()->GameClient()->Client()->DummyConnected())
		{
			str_format(aOwnName, sizeof(aOwnName), " %s", ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[1]].m_aName);
			if(str_endswith_nocase(aVictim, aOwnName))
				aVictim[str_length(aVictim) - str_length(aOwnName)] = '\0';
		}

		// cut off descriptions like this
		// why do you block foo he is new here!
		// why do you block foo she is my friend!!
		for(int i = 0; i < str_length(aVictim); i++)
		{
			// c++ be like...
			if(i < 2)
				continue;
			if(aVictim[i - 1] != ' ')
				continue;
			if((aVictim[i] != 'h' || !aVictim[i + 1] || aVictim[i + 1] != 'e' || !aVictim[i + 2] || aVictim[i + 2] != ' ') &&
				(aVictim[i] != 's' || !aVictim[i + 1] || aVictim[i + 1] != 'h' || !aVictim[i + 2] || aVictim[i + 2] != 'e' || !aVictim[i + 3] || aVictim[i + 3] != ' '))
				continue;

			aVictim[i - 1] = '\0';
			break;
		}

		// do not kill my friend foo
		const char *pFriend = NULL;
		if((pFriend = str_find_nocase(aVictim, " friend ")))
			pFriend += str_length(" friend ");
		else if((pFriend = str_find_nocase(aVictim, " frint ")))
			pFriend += str_length(" frint ");
		else if((pFriend = str_find_nocase(aVictim, " mate ")))
			pFriend += str_length(" mate ");
		else if((pFriend = str_find_nocase(aVictim, " bff ")))
			pFriend += str_length(" bff ");
		else if((pFriend = str_find_nocase(aVictim, " girlfriend ")))
			pFriend += str_length(" girlfriend ");
		else if((pFriend = str_find_nocase(aVictim, " boyfriend ")))
			pFriend += str_length(" boyfriend ");
		else if((pFriend = str_find_nocase(aVictim, " dog ")))
			pFriend += str_length(" dog ");
		else if((pFriend = str_find_nocase(aVictim, " gf ")))
			pFriend += str_length(" gf ");
		else if((pFriend = str_find_nocase(aVictim, " bf ")))
			pFriend += str_length(" bf ");

		if(pFriend)
			str_copy(aVictim, pFriend, sizeof(aVictim));
	}

	char aWarReason[128];
	if(HasWar || ChatHelper()->GameClient()->m_WarList.IsWarlist(aVictim) || ChatHelper()->GameClient()->m_WarList.IsTraitorlist(aVictim))
	{
		ChatHelper()->GameClient()->m_WarList.GetWarReason(aVictim, aWarReason, sizeof(aWarReason));
		if(aWarReason[0])
			str_format(m_pResponse, m_SizeOfResponse, "%s: %s has war because: %s", m_pMessageAuthor, aVictim, aWarReason);
		else
			str_format(m_pResponse, m_SizeOfResponse, "%s: the name %s is on my warlist.", m_pMessageAuthor, aVictim);
		return true;
	}
	for(auto &Client : ChatHelper()->GameClient()->m_aClients)
	{
		if(!Client.m_Active)
			continue;
		if(str_comp(Client.m_aName, aVictim))
			continue;

		if(ChatHelper()->GameClient()->m_WarList.IsWarClanlist(Client.m_aClan))
		{
			str_format(m_pResponse, m_SizeOfResponse, "%s i war %s because his clan %s is on my warlist.", m_pMessageAuthor, aVictim, Client.m_aClan);
			return true;
		}
	}
	return false;
}

bool CReplyToPing::Reply()
{
	if(!m_pResponse)
		return false;
	m_pResponse[0] = '\0';
	if(m_pMessageAuthor[0] == '\0')
		return false;
	if(m_pMessage[0] == '\0')
		return false;

	int MsgLen = str_length(m_pMessage);
	int NameLen = 0;
	const char *pName = ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[0]].m_aName;
	const char *pDummyName = ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[1]].m_aName;
	const char *pClan = ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[0]].m_aClan;
	const char *pDummyClan = ChatHelper()->GameClient()->m_aClients[ChatHelper()->GameClient()->m_LocalIDs[1]].m_aClan;

	if(ChatHelper()->LineShouldHighlight(m_pMessage, pName))
		NameLen = str_length(pName);
	else if(ChatHelper()->GameClient()->Client()->DummyConnected() && ChatHelper()->LineShouldHighlight(m_pMessage, pDummyName))
		NameLen = str_length(pDummyName);

	// ping without further context
	if(MsgLen < NameLen + 2)
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s ?", m_pMessageAuthor);
		return true;
	}
	// greetings
	if(LangParser().IsGreeting(m_pMessage))
	{
		str_format(m_pResponse, m_SizeOfResponse, "hi %s", m_pMessageAuthor);
		return true;
	}
	if(LangParser().IsBye(m_pMessage))
	{
		str_format(m_pResponse, m_SizeOfResponse, "bye %s", m_pMessageAuthor);
		return true;
	}
	// can i join your clan?
	if(str_find_nocase(m_pMessage, "clan") &&
		(str_find_nocase(m_pMessage, "enter") ||
			str_find_nocase(m_pMessage, "join") ||
			str_find_nocase(m_pMessage, "let me") ||
			str_find_nocase(m_pMessage, "beitreten") ||
			str_find_nocase(m_pMessage, " in ") ||
			str_find_nocase(m_pMessage, "can i") ||
			str_find_nocase(m_pMessage, "can me") ||
			str_find_nocase(m_pMessage, "me you") ||
			str_find_nocase(m_pMessage, "me is") ||
			str_find_nocase(m_pMessage, "into")))
	{
		char aResponse[1024];
		if(ChatHelper()->HowToJoinClan(pClan, aResponse, sizeof(aResponse)) || (ChatHelper()->GameClient()->Client()->DummyConnected() && ChatHelper()->HowToJoinClan(pDummyClan, aResponse, m_SizeOfResponse)))
		{
			str_format(m_pResponse, m_SizeOfResponse, "%s %s", m_pMessageAuthor, aResponse);
			return true;
		}
	}
	// check if a player has war or not
	// TODO:

	// check war reason for others
	const char *pWhy = str_find_nocase(m_pMessage, "why has ");
	if(pWhy)
		pWhy = pWhy + str_length("why has ");
	if(!pWhy)
		if((pWhy = str_find_nocase(m_pMessage, "why")))
			pWhy = pWhy + str_length("why");
	if(!pWhy)
		if((pWhy = str_find_nocase(m_pMessage, "warum hat ")))
			pWhy = pWhy + str_length("warum hat ");
	if(!pWhy)
		if((pWhy = str_find_nocase(m_pMessage, "warum")))
			pWhy = pWhy + str_length("warum");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "stop");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "do not");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "don't");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "do u");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "do you");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "is u");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "is you");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "are u");
	if(!pWhy)
		pWhy = str_find_nocase(m_pMessage, "are you");
	if(pWhy)
	{
		const char *pKill = NULL;
		pKill = str_find_nocase(pWhy, "kill "); // why do you kill foo?
		if(pKill)
			pKill = pKill + str_length("kill ");
		else if((pKill = str_find_nocase(pWhy, "kil "))) // why kil foo?
			pKill = pKill + str_length("kil ");
		else if((pKill = str_find_nocase(pWhy, "killed "))) // why killed foo?
			pKill = pKill + str_length("killed ");
		else if((pKill = str_find_nocase(pWhy, "block "))) // why do you block foo?
			pKill = pKill + str_length("block ");
		else if((pKill = str_find_nocase(pWhy, "blocked "))) // why do you blocked foo?
			pKill = pKill + str_length("blocked ");
		else if((pKill = str_find_nocase(pWhy, "help "))) // why no help foo?
			pKill = pKill + str_length("help ");
		else if((pKill = str_find_nocase(pWhy, "war with "))) // why do you have war with foo?
			pKill = pKill + str_length("war with ");
		else if((pKill = str_find_nocase(pWhy, "war "))) // why war foo?
			pKill = pKill + str_length("war ");
		else if((pKill = str_find_nocase(pWhy, "wared "))) // why wared foo?
			pKill = pKill + str_length("wared ");
		else if((pKill = str_find_nocase(pWhy, "waring "))) // why waring foo?
			pKill = pKill + str_length("waring ");
		else if((pKill = str_find_nocase(pWhy, "bully "))) // why bully foo?
			pKill = pKill + str_length("bully ");
		else if((pKill = str_find_nocase(pWhy, "bullying "))) // why bullying foo?
			pKill = pKill + str_length("bullying ");
		else if((pKill = str_find_nocase(pWhy, "bullied "))) // why bullied foo?
			pKill = pKill + str_length("bullied ");
		else if((pKill = str_find_nocase(pWhy, "freeze "))) // why freeze foo?
			pKill = pKill + str_length("freeze ");

		if(WhyWar(pKill))
			return true;

		// "why foo war?"
		// chop off the "war" at the end
		char aWhy[128];
		str_copy(aWhy, pWhy, sizeof(aWhy));

		int CutOffWar = -1;
		if((CutOffWar = LangParser().StrFindIndex(aWhy, " war")) != -1)
			aWhy[CutOffWar] = '\0';
		else if((CutOffWar = LangParser().StrFindIndex(aWhy, " kill")) != -1)
			aWhy[CutOffWar] = '\0';

		// trim
		int trim = 0;
		while(aWhy[trim] && aWhy[trim] == ' ')
			trim++;

		if(CutOffWar != -1)
			if(WhyWar(aWhy + trim))
				return true;
	}
	// why? (check war for self)
	if(LangParser().IsQuestionWhy(m_pMessage) || (str_find(m_pMessage, "?") && MsgLen < NameLen + 4) ||
		((str_find(m_pMessage, "stop") || str_find_nocase(m_pMessage, "help")) && (ChatHelper()->GameClient()->m_WarList.IsWarlist(m_pMessageAuthor) || ChatHelper()->GameClient()->m_WarList.IsTraitorlist(m_pMessageAuthor))))
	{
		char aWarReason[128];
		if(ChatHelper()->GameClient()->m_WarList.IsWarlist(m_pMessageAuthor) || ChatHelper()->GameClient()->m_WarList.IsTraitorlist(m_pMessageAuthor))
		{
			ChatHelper()->GameClient()->m_WarList.GetWarReason(m_pMessageAuthor, aWarReason, sizeof(aWarReason));
			if(aWarReason[0])
				str_format(m_pResponse, m_SizeOfResponse, "%s has war because: %s", m_pMessageAuthor, aWarReason);
			else
				str_format(m_pResponse, m_SizeOfResponse, "%s you are on my warlist.", m_pMessageAuthor);
			return true;
		}
		else if(ChatHelper()->GameClient()->m_WarList.IsWarClanlist(m_pMessageAuthorClan))
		{
			str_format(m_pResponse, m_SizeOfResponse, "%s your clan is on my warlist.", m_pMessageAuthor);
			return true;
		}
	}

	// spec me
	if(str_find_nocase(m_pMessage, "spec") || str_find_nocase(m_pMessage, "watch") || (str_find_nocase(m_pMessage, "look") && !str_find_nocase(m_pMessage, "looks")) || str_find_nocase(m_pMessage, "schau"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "/pause %s", m_pMessageAuthor);
		str_format(m_pResponse, m_SizeOfResponse, "%s ok i am watching you", m_pMessageAuthor);
		return true;
	}
	// wanna? (always say no automated if motivated to do something type yes manually)
	if(str_find_nocase(m_pMessage, "wanna") || str_find_nocase(m_pMessage, "want"))
	{
		// TODO: fix tone
		// If you get asked to be given something "no sorry" sounds weird
		// If you are being asked to do something together "no thanks" sounds weird
		// the generic "no" might be a bit dry
		str_format(m_pResponse, m_SizeOfResponse, "%s no", m_pMessageAuthor);
		return true;
	}
	// help
	if(str_find_nocase(m_pMessage, "help") || str_find_nocase(m_pMessage, "hilfe"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s where? what?", m_pMessageAuthor);
		return true;
	}
	// small talk
	if(str_find_nocase(m_pMessage, "how are you") ||
		str_find_nocase(m_pMessage, "how r u") ||
		str_find_nocase(m_pMessage, "how ru ") ||
		str_find_nocase(m_pMessage, "how ru?") ||
		str_find_nocase(m_pMessage, "how r you") ||
		str_find_nocase(m_pMessage, "how are u") ||
		str_find_nocase(m_pMessage, "how is it going") ||
		str_find_nocase(m_pMessage, "ca va") ||
		(str_find_nocase(m_pMessage, "как") && str_find_nocase(m_pMessage, "дела")))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s good, and you? :)", m_pMessageAuthor);
		return true;
	}
	if(str_find_nocase(m_pMessage, "wie gehts") || str_find_nocase(m_pMessage, "wie geht es") || str_find_nocase(m_pMessage, "was geht"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s gut, und dir? :)", m_pMessageAuthor);
		return true;
	}
	if(str_find_nocase(m_pMessage, "about you") || str_find_nocase(m_pMessage, "and you") || str_find_nocase(m_pMessage, "and u") ||
		(str_find_nocase(m_pMessage, "u?") && MsgLen < NameLen + 5) ||
		(str_find_nocase(m_pMessage, "wbu") && MsgLen < NameLen + 8) ||
		(str_find_nocase(m_pMessage, "hbu") && MsgLen < NameLen + 8))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s good", m_pMessageAuthor);
		return true;
	}
	// chillerbot-ux features
	if(LangParser().IsQuestionHow(m_pMessage))
	{
		// feature: auto_drop_money
		if(str_find_nocase(m_pMessage, "drop") && (str_find_nocase(m_pMessage, "money") || str_find_nocase(m_pMessage, "moni") || str_find_nocase(m_pMessage, "coin") || str_find_nocase(m_pMessage, "cash") || str_find_nocase(m_pMessage, "geld")))
		{
			str_format(m_pResponse, m_SizeOfResponse, "%s I auto drop money using \"auto_drop_money\" in chillerbot-ux", m_pMessageAuthor);
			return true;
		}
		// feature: auto reply
		if((str_find_nocase(m_pMessage, "reply") && str_find_nocase(m_pMessage, "chat")) || (str_find_nocase(m_pMessage, "auto chat") || str_find_nocase(m_pMessage, "autochat")) ||
			str_find_nocase(m_pMessage, "message") ||
			((str_find_nocase(m_pMessage, "fast") || str_find_nocase(m_pMessage, "quick")) && str_find_nocase(m_pMessage, "chat")))
		{
			str_format(m_pResponse, m_SizeOfResponse, "%s I bound the chillerbot-ux command \"reply_to_last_ping\" to automate chat", m_pMessageAuthor);
			return true;
		}
	}
	// advertise chillerbot
	if(str_find_nocase(m_pMessage, "what client") || str_find_nocase(m_pMessage, "which client") || str_find_nocase(m_pMessage, "wat client") ||
		str_find_nocase(m_pMessage, "good client") || str_find_nocase(m_pMessage, "download client") || str_find_nocase(m_pMessage, "link client") || str_find_nocase(m_pMessage, "get client") ||
		str_find_nocase(m_pMessage, "where chillerbot") || str_find_nocase(m_pMessage, "download chillerbot") || str_find_nocase(m_pMessage, "link chillerbot") || str_find_nocase(m_pMessage, "get chillerbot") ||
		str_find_nocase(m_pMessage, "chillerbot url") || str_find_nocase(m_pMessage, "chillerbot download") || str_find_nocase(m_pMessage, "chillerbot link") || str_find_nocase(m_pMessage, "chillerbot website") ||
		((str_find_nocase(m_pMessage, "ddnet") || str_find_nocase(m_pMessage, "vanilla")) && str_find_nocase(m_pMessage, "?")))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s I use chillerbot-ux ( https://chillerbot.github.io )", m_pMessageAuthor);
		return true;
	}
	// whats your setting (mousesense, distance, dyn)
	if((str_find_nocase(m_pMessage, "?") ||
		   str_find_nocase(m_pMessage, "what") ||
		   str_find_nocase(m_pMessage, "which") ||
		   str_find_nocase(m_pMessage, "wat") ||
		   str_find_nocase(m_pMessage, "much") ||
		   str_find_nocase(m_pMessage, "many") ||
		   str_find_nocase(m_pMessage, "viel") ||
		   str_find_nocase(m_pMessage, "hoch")) &&
		(str_find_nocase(m_pMessage, "sens") || str_find_nocase(m_pMessage, "sesn") || str_find_nocase(m_pMessage, "snse") || str_find_nocase(m_pMessage, "senes") || str_find_nocase(m_pMessage, "inp") || str_find_nocase(m_pMessage, "speed")))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s my current inp_mousesens is %d", m_pMessageAuthor, g_Config.m_InpMousesens);
		return true;
	}
	if((str_find_nocase(m_pMessage, "?") || str_find_nocase(m_pMessage, "what") || str_find_nocase(m_pMessage, "which") || str_find_nocase(m_pMessage, "wat") || str_find_nocase(m_pMessage, "much") || str_find_nocase(m_pMessage, "many")) &&
		str_find_nocase(m_pMessage, "dist"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s my current cl_mouse_max_distance is %d", m_pMessageAuthor, g_Config.m_ClMouseMaxDistance);
		return true;
	}
	if((str_find_nocase(m_pMessage, "?") || str_find_nocase(m_pMessage, "do you") || str_find_nocase(m_pMessage, "do u")) &&
		str_find_nocase(m_pMessage, "dyn"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s my dyncam is currently %s", m_pMessageAuthor, g_Config.m_ClDyncam ? "on" : "off");
		return true;
	}
	// compliments
	if(str_find_nocase(m_pMessage, "good") ||
		str_find_nocase(m_pMessage, "happy") ||
		str_find_nocase(m_pMessage, "congrats") ||
		str_find_nocase(m_pMessage, "nice") ||
		str_find_nocase(m_pMessage, "pro"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s thanks", m_pMessageAuthor);
		return true;
	}
	// impatient
	if(str_find_nocase(m_pMessage, "answer") || str_find_nocase(m_pMessage, "ignore") || str_find_nocase(m_pMessage, "antwort") || str_find_nocase(m_pMessage, "ignorier"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s i am currently busy (automated reply)", m_pMessageAuthor);
		return true;
	}
	// ask to ask
	if(LangParser().IsAskToAsk(m_pMessage, m_pMessageAuthor, m_pResponse, m_SizeOfResponse))
		return true;
	// got weapon?
	if(str_find_nocase(m_pMessage, "got") || str_find_nocase(m_pMessage, "have") || str_find_nocase(m_pMessage, "hast"))
	{
		int Weapon = -1;
		if(str_find_nocase(m_pMessage, "hammer"))
			Weapon = WEAPON_HAMMER;
		else if(str_find_nocase(m_pMessage, "gun"))
			Weapon = WEAPON_GUN;
		else if(str_find_nocase(m_pMessage, "sg") || str_find_nocase(m_pMessage, "shotgun") || str_find_nocase(m_pMessage, "shotty"))
			Weapon = WEAPON_SHOTGUN;
		else if(str_find_nocase(m_pMessage, "nade") || str_find_nocase(m_pMessage, "rocket") || str_find_nocase(m_pMessage, "bazooka"))
			Weapon = WEAPON_GRENADE;
		else if(str_find_nocase(m_pMessage, "rifle") || str_find_nocase(m_pMessage, "laser") || str_find_nocase(m_pMessage, "sniper"))
			Weapon = WEAPON_LASER;
		if(CCharacter *pChar = ChatHelper()->GameClient()->m_GameWorld.GetCharacterByID(ChatHelper()->GameClient()->m_LocalIDs[g_Config.m_ClDummy]))
		{
			char aWeapons[1024];
			aWeapons[0] = '\0';
			if(pChar->GetWeaponGot(WEAPON_HAMMER))
				str_append(aWeapons, "hammer", sizeof(aWeapons));
			if(pChar->GetWeaponGot(WEAPON_GUN))
				str_append(aWeapons, aWeapons[0] ? ", gun" : "gun", sizeof(aWeapons));
			if(pChar->GetWeaponGot(WEAPON_SHOTGUN))
				str_append(aWeapons, aWeapons[0] ? ", shotgun" : "shotgun", sizeof(aWeapons));
			if(pChar->GetWeaponGot(WEAPON_GRENADE))
				str_append(aWeapons, aWeapons[0] ? ", grenade" : "grenade", sizeof(aWeapons));
			if(pChar->GetWeaponGot(WEAPON_LASER))
				str_append(aWeapons, aWeapons[0] ? ", rifle" : "rifle", sizeof(aWeapons));

			if(pChar->GetWeaponGot(Weapon))
				str_format(m_pResponse, m_SizeOfResponse, "%s Yes I got those weapons: %s", m_pMessageAuthor, aWeapons);
			else
				str_format(m_pResponse, m_SizeOfResponse, "%s No I got those weapons: %s", m_pMessageAuthor, aWeapons);
			return true;
		}
	}
	// weeb
	if(str_find_nocase(m_pMessage, "uwu"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s OwO", m_pMessageAuthor);
		return true;
	}
	if(str_find_nocase(m_pMessage, "owo"))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s UwU", m_pMessageAuthor);
		return true;
	}
	// no u
	if(MsgLen < NameLen + 8 && (str_find_nocase(m_pMessage, "no u") ||
					   str_find_nocase(m_pMessage, "no you") ||
					   str_find_nocase(m_pMessage, "noob") ||
					   str_find_nocase(m_pMessage, "nob") ||
					   str_find_nocase(m_pMessage, "nuub") ||
					   str_find_nocase(m_pMessage, "nub") ||
					   str_find_nocase(m_pMessage, "bad")))
	{
		str_format(m_pResponse, m_SizeOfResponse, "%s no u", m_pMessageAuthor);
		return true;
	}
	return false;
}

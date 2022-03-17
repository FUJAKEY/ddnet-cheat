#ifndef GAME_CLIENT_COMPONENTS_CHILLERBOT_PLAYERPICS_H
#define GAME_CLIENT_COMPONENTS_CHILLERBOT_PLAYERPICS_H

// nobo copy and edit of countryflags.h
#include <base/tl/sorted_array.h>
#include <base/vmath.h>
#include <game/client/component.h>

#include <game/client/components/nameplates.h>

struct SChillerNamePlate
{
	SChillerNamePlate()
	{
		Reset();
	}

	void Reset()
	{
		m_NameTextContainerIndex = m_WarReasonTextContainerIndex = -1;
		m_aName[0] = 0;
		m_aWarReason[0] = 0;
		m_NameTextWidth = m_WarReasonTextWidth = 0.f;
		m_NameTextFontSize = m_WarReasonTextFontSize = 0;
	}

	char m_aName[MAX_NAME_LENGTH];
	float m_NameTextWidth;
	int m_NameTextContainerIndex;
	float m_NameTextFontSize;

	char m_aWarReason[128];
	float m_WarReasonTextWidth;
	int m_WarReasonTextContainerIndex;
	float m_WarReasonTextFontSize;
};

class CPlayerPics : public CComponent
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }

	struct CPlayerPic
	{
		char m_aPlayerName[32];
		IGraphics::CTextureHandle m_Texture;

		bool operator<(const CPlayerPic &Other) const { return str_comp(m_aPlayerName, Other.m_aPlayerName) < 0; }
	};

	virtual void OnInit() override;

	int Num() const;
	const CPlayerPic *GetByName(const char *pName) const;
	const CPlayerPic *GetByIndex(int Index) const;
	void Render(const char *pName, const vec4 *pColor, float x, float y, float w, float h);

private:
	enum
	{
		CODE_LB = -1,
		CODE_UB = 999,
		CODE_RANGE = CODE_UB - CODE_LB + 1,
	};
	sorted_array<CPlayerPic> m_aPlayerPics;

	static int LoadImageByName(const char *pImgName, int IsDir, int DirType, void *pUser);
	void LoadPlayerpicsIndexfile();

	void MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup);
	SChillerNamePlate m_aNamePlates[MAX_CLIENTS];
	void RenderNameplate(
		const CNetObj_Character *pPrevChar,
		const CNetObj_Character *pPlayerChar,
		const CNetObj_PlayerInfo *pPlayerInfo);
	void RenderNameplatePos(vec2 Position, const CNetObj_PlayerInfo *pPlayerInfo, float Alpha);

	virtual void OnRender() override;
};
#endif

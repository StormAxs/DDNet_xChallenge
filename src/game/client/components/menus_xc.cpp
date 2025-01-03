#include <base/log.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/shared/localization.h>
#include <engine/shared/protocol7.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/updater.h>

#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/chat.h>
#include <game/client/components/menu_background.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/skin.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

#include <array>
#include <chrono>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

void CMenus::RenderSettingsXc(CUIRect MainView)
{
	using namespace FontIcons;
	using namespace std::chrono_literals;

	static CScrollRegion s_ScrollRegion;
	static vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 120.0f;

	// Begin the scroll region
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

	// Adjust MainView based on the scroll offset
	MainView.y += ScrollOffset.y;

	const float FontSize = 14.0f;
	const float Margin = 10.0f;
	const float HeaderHeight = FontSize + 5.0f + Margin;
	const float LineSize = 20.0f;
	const float ColorPickerLineSize = 25.0f;
	const float HeadlineFontSize = 20.0f;
	const float HeadlineHeight = 30.0f;
	const float MarginSmall = 5.0f;
	const float MarginBetweenViews = 20.0f;
	const float ColorPickerLabelSize = 13.0f;
	const float ColorPickerLineSpacing = 5.0f;

	ColorRGBA Rainbow = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 50) % 255 / 255.f, 1.f, 1.f));
	ColorRGBA Default = ColorRGBA(1.f, 1.f, 1.f, 1.f);

	CUIRect GeneralGroup, ConsoleGroup, VisualGroup;
	MainView.y -= 10.0f;

	// General Settings Section
	static SFoldableSection s_InGameGroup;
	MainView.HSplitTop(Margin, nullptr, &GeneralGroup);
	DoFoldableSection(&s_InGameGroup, Localize("General"), FontSize, &GeneralGroup, &MainView, 5.0f, [&]() -> int {
	    GeneralGroup.VMargin(Margin, &GeneralGroup);
	    GeneralGroup.HMargin(Margin, &GeneralGroup);

	    // Displaying key press status and global time
	    char aBuf[64];
	    str_format(aBuf, sizeof(aBuf), "Key is pressed (ALT): %d", Input()->KeyIsPressed(KEY_LALT));
	    Ui()->DoLabel(&GeneralGroup, aBuf, FontSize, TEXTALIGN_TL);
	    GeneralGroup.HSplitTop(LineSize, nullptr, &GeneralGroup);

	    char aBuf2[64];
	    str_format(aBuf2, sizeof(aBuf2), "Global time: %f", Client()->GlobalTime());
	    Ui()->DoLabel(&GeneralGroup, aBuf2, FontSize, TEXTALIGN_TL);

	    int TotalHeight = 40.0f; // section height
	    return TotalHeight + Margin;
	});
	s_ScrollRegion.AddRect(GeneralGroup);

	// Console Settings Section
	static SFoldableSection s_InConsoleGroup;
	MainView.HSplitTop(Margin, nullptr, &ConsoleGroup);
	DoFoldableSection(&s_InConsoleGroup, Localize("Console"), FontSize, &ConsoleGroup, &MainView, 5.0f, [&]() -> int {
	    ConsoleGroup.VMargin(Margin, &ConsoleGroup);
	    ConsoleGroup.HMargin(Margin, &ConsoleGroup);

	    int TotalHeight = 100.0f; // section height
	    CUIRect Button;

	    // Custom Console options
	    ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
	    if (g_Config.m_XcCustomConsole) {
		TextRender()->TextColor(1, 1, 1, 0.6);
		if (DoButton_CheckBox(&g_Config.m_XcCustomConIcons, Localize("Enable .png support for console"), g_Config.m_XcCustomConIcons, &Button)) {
		    g_Config.m_XcCustomConIcons ^= 1;
		    g_Config.m_XcCustomConsole = 0;
		}
		TextRender()->TextColor(1, 1, 1, 1);
	    } else if (DoButton_CheckBox(&g_Config.m_XcCustomConIcons, Localize("Enable .png support for console"), g_Config.m_XcCustomConIcons, &Button)) {
		g_Config.m_XcCustomConIcons ^= 1;
		g_Config.m_XcCustomConsole = 0;
	    }

	    ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
	    if (g_Config.m_XcCustomConIcons) {
		TextRender()->TextColor(1, 1, 1, 0.6);
		if (DoButton_CheckBox(&g_Config.m_XcCustomConsole, Localize("Enable custom console"), g_Config.m_XcCustomConsole, &Button)) {
		    g_Config.m_XcCustomConsole ^= 1;
		    g_Config.m_XcCustomConIcons = 0;
		}
		TextRender()->TextColor(1, 1, 1, 1);
	    } else if (DoButton_CheckBox(&g_Config.m_XcCustomConsole, Localize("Enable custom console"), g_Config.m_XcCustomConsole, &Button)) {
		g_Config.m_XcCustomConsole ^= 1;
		g_Config.m_XcCustomConIcons = 0;
	    }

	    ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
	    if (DoButton_CheckBox(&g_Config.m_XcCustomConsoleBar, Localize("Enable custom console bar"), g_Config.m_XcCustomConsoleBar, &Button)) {
		g_Config.m_XcCustomConsoleBar ^= 1;
	    }

	    ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
	    if (DoButton_CheckBox(&g_Config.m_XcConsoleAnimation, Localize("Enable custom console speed"), g_Config.m_XcConsoleAnimation, &Button)) {
		g_Config.m_XcConsoleAnimation ^= 1;
	    }

	    int i = 0;
	    static CButtonContainer s_aResetIDs[24];
	    if (g_Config.m_XcCustomConsole) {
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &ConsoleGroup, Localize("Local console color"), &g_Config.m_XcLocalConsoleColor, ColorRGBA(0.2f, 0.2f, 0.2f), false);
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &ConsoleGroup, Localize("Remote console color"), &g_Config.m_XcRemoteConsoleColor, ColorRGBA(0.4f, 0.2f, 0.2f), false);
		TotalHeight += 100.0f;
	    }
	    if (g_Config.m_XcCustomConsoleBar) {
		DoLine_ColorPicker(&s_aResetIDs[i++], ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &ConsoleGroup, Localize("Console bar color"), &g_Config.m_XcConsoleBarColor, ColorRGBA(8883654), false);
		TotalHeight += 20.0f;
	    }

	    if (g_Config.m_XcCustomConIcons) {
		ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
		Ui()->DoScrollbarOption(&g_Config.m_XcLocalConsoleAlpha, &g_Config.m_XcLocalConsoleAlpha, &Button, Localize("Local Console Opacity"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");
		ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
		Ui()->DoScrollbarOption(&g_Config.m_XcCustomConsoleBrightness, &g_Config.m_XcCustomConsoleBrightness, &Button, Localize("Console Wallpaper Brightness"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");

		TotalHeight += 35;
	    }

	    if (g_Config.m_XcCustomConsole) {
		ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
		Ui()->DoScrollbarOption(&g_Config.m_XcLocalConsoleAlpha, &g_Config.m_XcLocalConsoleAlpha, &Button, Localize("Local Console Opacity"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");

		// Remote console opacity
		ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
		Ui()->DoScrollbarOption(&g_Config.m_XcRemoteConsoleAlpha, &g_Config.m_XcRemoteConsoleAlpha, &Button, Localize("Remote Console Opacity"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");

		TotalHeight += 35;
	    }

	    return TotalHeight;
	});
	s_ScrollRegion.AddRect(ConsoleGroup);

static SFoldableSection s_InVisualGroup;
CUIRect VisualGroup_Bullets, VisualGroup_Crosshair;
MainView.HSplitTop(Margin, nullptr, &VisualGroup);
DoFoldableSection(&s_InVisualGroup, Localize("Visual"), FontSize, &VisualGroup, &MainView, 5.0f, [&]() -> int {
    int TotalHeightVisual = 90.0f; // Section height
    VisualGroup.VMargin(Margin, &VisualGroup);
    VisualGroup.HMargin(Margin, &VisualGroup);

    // BulletTrails Section
	//TODO: fix
    static SFoldableSection s_InVisualGroup_Bullets;
    VisualGroup.HSplitTop(Margin, nullptr, &VisualGroup_Bullets);
    DoFoldableSection(&s_InVisualGroup_Bullets, Localize("BulletTrails"), FontSize, &VisualGroup_Bullets, &VisualGroup, 5.0f, [&]() -> int {
        int TotalHeightVisual_Bullets = 25.0f;
        TotalHeightVisual += 35.0f;

        VisualGroup_Bullets.VMargin(Margin, &VisualGroup_Bullets);
        VisualGroup_Bullets.HMargin(Margin, &VisualGroup_Bullets);

        // Bullet Smoke Trail Checkbox
        CUIRect Button;
        VisualGroup_Bullets.HSplitTop(LineSize, &Button, &VisualGroup_Bullets);
        if (DoButton_CheckBox(&g_Config.m_XcBulletSmokeTrail, Localize("Enable custom bullet trails"), g_Config.m_XcBulletSmokeTrail, &Button)) {
            g_Config.m_XcBulletSmokeTrail ^= 1;
        }

        // Bullet Smoke Trail Lifespan
        VisualGroup_Bullets.HSplitTop(LineSize, &Button, &VisualGroup_Bullets);
        if (g_Config.m_XcBulletSmokeTrail) {
            Ui()->DoScrollbarOption(&g_Config.m_XcBulletSmokeTrailLifeSpan,
                                    &g_Config.m_XcBulletSmokeTrailLifeSpan, &Button,
                                    Localize("Bullet trail lifespan"), 0, 200,
                                    &CUi::ms_LinearScrollbarScale, 0u, "");
            TotalHeightVisual_Bullets += 25;
            TotalHeightVisual += 25;
        }

        // Done with BulletTrails section
        return TotalHeightVisual_Bullets + Margin;
    });
    s_ScrollRegion.AddRect(VisualGroup_Bullets);

    // Crosshair Section
    static SFoldableSection s_InVisualGroup_Crosshair;
    VisualGroup.HSplitTop(Margin, nullptr, &VisualGroup_Crosshair);
    DoFoldableSection(&s_InVisualGroup_Crosshair, Localize("Crosshair"), FontSize, &VisualGroup_Crosshair, &VisualGroup_Bullets, 5.0f, [&]() -> int {
        int TotalHeightVisual_Crosshair = 25.0f; // Crosshair section height
        TotalHeightVisual += 35.0f; // Increment for Crosshair section

        VisualGroup_Crosshair.VMargin(Margin, &VisualGroup_Crosshair);
        VisualGroup_Crosshair.HMargin(Margin, &VisualGroup_Crosshair);

        CUIRect Button;

        // Crosshair Size
        VisualGroup_Crosshair.HSplitTop(LineSize, &Button, &VisualGroup_Crosshair);
          	Ui()->DoScaledScrollbarOption(&g_Config.m_XcCursorSizeMultiplier, &g_Config.m_XcCursorSizeMultiplier,
          		&Button, Localize("Crosshair multiplier"), 1, 100, 0.1f,
          		&CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, "x");
	ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);

    	TotalHeightVisual_Crosshair += 25;
            TotalHeightVisual += 25;

        return TotalHeightVisual_Crosshair + Margin;
    });
    s_ScrollRegion.AddRect(VisualGroup_Crosshair);


    return TotalHeightVisual + Margin;
});
s_ScrollRegion.AddRect(VisualGroup);


	static SFoldableSection s_InPdfGroup;
	CUIRect PdfGroup;
	MainView.HSplitTop(Margin, nullptr, &PdfGroup);
	MainView.HSplitTop(Margin, nullptr, &PdfGroup);
	DoFoldableSection(&s_InPdfGroup, Localize("Hitbox settings"), FontSize, &PdfGroup, &MainView, 5.0f, [&]() -> int {
	    PdfGroup.VMargin(Margin, &PdfGroup);
	    PdfGroup.HMargin(Margin, &PdfGroup);

	    CUIRect Button, Preview;

	    int TotalHeightHitbox = 270.0f;

	    // Show hitbox of own tee
	    PdfGroup.HSplitTop(LineSize, &Button, &PdfGroup);
	    if (DoButton_CheckBox(&g_Config.m_XcShowTeeHitboxOwn, Localize("Show hitbox of own tee"), g_Config.m_XcShowTeeHitboxOwn, &Button))
	    	g_Config.m_XcShowTeeHitboxOwn = g_Config.m_XcShowTeeHitboxOwn ? 0 : 1;

		// Show hitbox of other tees
		PdfGroup.HSplitTop(LineSize, &Button, &PdfGroup);
		if (DoButton_CheckBox(&g_Config.m_XcShowTeeHitboxOther, Localize("Show hitbox of other tees"), g_Config.m_XcShowTeeHitboxOther, &Button))
			g_Config.m_XcShowTeeHitboxOther = g_Config.m_XcShowTeeHitboxOther ? 0 : 1;

		    // Tee hitbox colors
		    const ColorRGBA TeeHitboxInnerColorDefault(1.f, 0.7f, 1.f, 0.3f);
		    const ColorRGBA TeeHitboxOuterColorDefault(1.f, 1.f, 0.7f, 0.3f);
		    static CButtonContainer s_TeeHitboxInner, s_TeeHitboxOuter;
		    DoLine_ColorPicker(&s_TeeHitboxInner, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &PdfGroup, Localize("Tee hitbox inner color"), &g_Config.m_XcShowTeeHitboxInnerColor, TeeHitboxInnerColorDefault, false, nullptr, true);
		    DoLine_ColorPicker(&s_TeeHitboxOuter, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &PdfGroup, Localize("Tee hitbox outer color"), &g_Config.m_XcShowTeeHitboxOuterColor, TeeHitboxOuterColorDefault, false, nullptr, true);

		    // Show hammer hitbox of own tee
		    PdfGroup.HSplitTop(LineSize, &Button, &PdfGroup);
		    if (DoButton_CheckBox(&g_Config.m_XcShowHammerHitboxOwn, Localize("Show hammer hitbox of own tee"), g_Config.m_XcShowHammerHitboxOwn, &Button))
		    	g_Config.m_XcShowHammerHitboxOwn = g_Config.m_XcShowHammerHitboxOwn ? 0 : 1;

			// Show hammer hitbox of other tees
			PdfGroup.HSplitTop(LineSize, &Button, &PdfGroup);
			if (DoButton_CheckBox(&g_Config.m_XcShowHammerHitboxOther, Localize("Show hammer hitbox of other tees"), g_Config.m_XcShowHammerHitboxOther, &Button))
				g_Config.m_XcShowHammerHitboxOther = g_Config.m_XcShowHammerHitboxOther ? 0 : 1;

			    // Hammer hitbox color
			    const ColorRGBA HammerHitboxColorDefault(1.f, 1.f, 1.f, 0.3f);
			    static CButtonContainer s_HammerHitbox;
			    DoLine_ColorPicker(&s_HammerHitbox, ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing, &PdfGroup, Localize("Hammer hitbox color"), &g_Config.m_XcShowHammerHitboxColor, HammerHitboxColorDefault, false, nullptr, true);

			    // skin prewiew
			    CTeeRenderInfo OwnSkinInfo;
			    OwnSkinInfo.Apply(m_pClient->m_Skins.Find(g_Config.m_ClPlayerSkin));
			    OwnSkinInfo.ApplyColors(g_Config.m_ClPlayerUseCustomColor, g_Config.m_ClPlayerColorBody, g_Config.m_ClPlayerColorFeet);
			    OwnSkinInfo.m_Size = 50.0f;

			    CTeeRenderInfo DummySkinInfo;
			    DummySkinInfo.Apply(m_pClient->m_Skins.Find(g_Config.m_ClDummySkin));
			    DummySkinInfo.ApplyColors(g_Config.m_ClDummyUseCustomColor, g_Config.m_ClDummyColorBody, g_Config.m_ClDummyColorFeet);
	 		    DummySkinInfo.m_Size = 50.0f;

			    auto DoHitboxPreviewTee = [=](vec2 Pos, vec2 Dir, bool HammerHitbox, bool TeeHitbox, const CTeeRenderInfo *pInfo) {
				if (HammerHitbox)
					RenderTools()->RenderHammerHitbox(Pos, Dir);
				    if (TeeHitbox)
				    	RenderTools()->RenderTeeHitbox(Pos);
					else
						RenderTools()->RenderTee(CAnimState::GetIdle(), pInfo, 0, Dir, Pos);
					};

					PdfGroup.VSplitRight(170.f, &PdfGroup, &Preview);
					vec2 Center = vec2(Preview.x, Preview.y);

					// dummies
					vec2 MainTeePos = Center - vec2(40.f, 0.f);
					DoHitboxPreviewTee(MainTeePos, direction(angle(Ui()->MousePos() - MainTeePos)), g_Config.m_XcShowHammerHitboxOwn, g_Config.m_XcShowTeeHitboxOwn, &OwnSkinInfo);
					vec2 DummyTeePos = Center + vec2(40.f, 0.f);
					DoHitboxPreviewTee(DummyTeePos, direction(angle(Ui()->MousePos() - DummyTeePos)), g_Config.m_XcShowHammerHitboxOther, g_Config.m_XcShowTeeHitboxOther, &DummySkinInfo);

					// floor
					Graphics()->TextureClear();
					Graphics()->TextureSet(m_pClient->m_MapImages.GetEntities(MAP_IMAGE_ENTITY_LAYER_TYPE_ALL_EXCEPT_SWITCH));
					Graphics()->BlendNormal();
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					for (int i = -2; i <= 2; i++)
						RenderTools()->RenderTile(Center.x - (i * 32.f) - 16.f, Center.y + 14.f, TILE_SOLID, 32.f, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));

					    Preview.HSplitTop(65.0f, &Preview, &PdfGroup);
					    TextRender()->TextColor(Rainbow);
					    Ui()->DoLabel(&Preview, Localize("Inspired by AniClient(AnimePDF)"), 12.0f, TEXTALIGN_BR);
					    TextRender()->TextColor(Default);

					    return TotalHeightHitbox;

					});
	s_ScrollRegion.AddRect(PdfGroup);

	s_ScrollRegion.End();
}
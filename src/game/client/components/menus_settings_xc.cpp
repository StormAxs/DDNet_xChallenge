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

void CMenus::RenderSettingsXChallange(CUIRect MainView)
{
    using namespace FontIcons;
    using namespace std::chrono_literals;

    static CScrollRegion s_ScrollRegion;
    vec2 ScrollOffset(0.0f, 0.0f);
    CScrollRegionParams ScrollParams;
    ScrollParams.m_ScrollUnit = 120.0f;
    s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
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

    CUIRect GeneralGroup, ConsoleGroup;
    MainView.y -= 10.0f;

    // Begining of the section
    static SFoldableSection s_InGameGroup;
    MainView.HSplitTop(Margin, nullptr, &GeneralGroup);
    DoFoldableSection(&s_InGameGroup, Localize("General"), FontSize, &GeneralGroup, &MainView, 5.0f, [&]() -> int {
        GeneralGroup.VMargin(Margin, &GeneralGroup);
        GeneralGroup.HMargin(Margin, &GeneralGroup);
        // Code stuff
        char aBuf[64];
        str_format(aBuf, sizeof(aBuf), "Key is pressed (ALT): %d", Input()->KeyIsPressed(KEY_LALT));
        Ui()->DoLabel(&GeneralGroup, aBuf, FontSize, TEXTALIGN_TL);
        GeneralGroup.HSplitTop(LineSize, nullptr, &GeneralGroup);
        char aBuf2[64];
        str_format(aBuf2, sizeof(aBuf2), "Global time: %f", Client()->GlobalTime());
        Ui()->DoLabel(&GeneralGroup, aBuf2, FontSize, TEXTALIGN_TL);
        // Done with code
        int TotalHeight = 40.0f; // section height
        return TotalHeight + Margin;
    });
    s_ScrollRegion.AddRect(GeneralGroup);
    //Section:end


    // Begining of the section
    static SFoldableSection s_InConsoleGroup;
    MainView.HSplitTop(Margin, nullptr, &ConsoleGroup);
    DoFoldableSection(&s_InConsoleGroup, Localize("Console"), FontSize, &ConsoleGroup, &MainView, 5.0f, [&]() -> int {
        ConsoleGroup.VMargin(Margin, &ConsoleGroup);
        ConsoleGroup.HMargin(Margin, &ConsoleGroup);

        int TotalHeight = 75.0f; // section height

        // Code stuff
        CUIRect Button;

        ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
        if(DoButton_CheckBox(&g_Config.m_XcCustomConsole, Localize("Enable custom console"), g_Config.m_XcCustomConsole, &Button))
         {
             g_Config.m_XcCustomConsole ^= 1;
         }

        ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
        if(DoButton_CheckBox(&g_Config.m_XcCustomConsoleBar, Localize("Enable custom console bar"), g_Config.m_XcCustomConsoleBar, &Button))
            {
               g_Config.m_XcCustomConsoleBar ^= 1;
            }

        ConsoleGroup.HSplitTop(LineSize, &Button, &ConsoleGroup);
        if(DoButton_CheckBox(&g_Config.m_XcConsoleAnimation, Localize("Enable custom console speed"), g_Config.m_XcConsoleAnimation, &Button))
        {
           g_Config.m_XcConsoleAnimation ^= 1;
        }


        int i = 0;
          static CButtonContainer s_aResetIDs[24];
        if(g_Config.m_XcCustomConsole) {
            DoLine_ColorPicker(&s_aResetIDs[i++],
                    ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing,
                    &ConsoleGroup, Localize("Local console color"),
                    &g_Config.m_XcLocalConsoleColor,
                    ColorRGBA(0.2f, 0.2f, 0.2f),
                    false);

            DoLine_ColorPicker(&s_aResetIDs[i++],
                    ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing,
                    &ConsoleGroup, Localize("Remote console color"),
                    &g_Config.m_XcRemoteConsoleColor,
                    ColorRGBA(0.4f, 0.2f, 0.2f),
                    false);

            TotalHeight += 100.0f;
        }
        if(g_Config.m_XcCustomConsoleBar) {
            DoLine_ColorPicker(&s_aResetIDs[i++],
                          ColorPickerLineSize, ColorPickerLabelSize, ColorPickerLineSpacing,
                          &ConsoleGroup, Localize("Console bar color"),
                          &g_Config.m_XcConsoleBarColor,
                          ColorRGBA(8883654),
                          false);

            TotalHeight += 20.0f;
        }
        if(g_Config.m_XcCustomConsole) {
            ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
            Ui()->DoScrollbarOption(&g_Config.m_XcLocalConsoleAlpha, &g_Config.m_XcLocalConsoleAlpha, &Button, Localize("Local Console Opacity"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");
            //rcon
            ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
            Ui()->DoScrollbarOption(&g_Config.m_XcRemoteConsoleAlpha, &g_Config.m_XcRemoteConsoleAlpha, &Button, Localize("Remote Console Opacity"), 0, 100, &CUi::ms_LinearScrollbarScale, 0u, "%");
        }
        if(g_Config.m_XcConsoleAnimation) {
            ConsoleGroup.HSplitTop(20.f, &Button, &ConsoleGroup);
            Ui()->DoScrollbarOption(&g_Config.m_XcConsoleAnimationSpeed, &g_Config.m_XcConsoleAnimationSpeed, &Button, Localize("Console opening speed"), 5, 0, &CUi::ms_LinearScrollbarScale, 0u, "");

            TotalHeight += 30.0f;
        }
        // Done with code
        return TotalHeight + Margin;
    });
    s_ScrollRegion.AddRect(ConsoleGroup);
    //Section:end


    s_ScrollRegion.End();
}

/*
		CUIRect DirectoryButton;
		Left.HSplitBottom(20.0f, &Left, &DirectoryButton);
		Left.HSplitBottom(5.0f, &Left, nullptr);
		static CButtonContainer s_ThemesButtonId;
		if(DoButton_Menu(&s_ThemesButtonId, Localize("Themes directory"), 0, &DirectoryButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "themes", aBuf, sizeof(aBuf));
			Storage()->CreateFolder("themes", IStorage::TYPE_SAVE);
			Client()->ViewFile(aBuf);
		}
		GameClient()->m_Tooltips.DoToolTip(&s_ThemesButtonId, &DirectoryButton, Localize("Open the directory to add custom themes"));
	
 */
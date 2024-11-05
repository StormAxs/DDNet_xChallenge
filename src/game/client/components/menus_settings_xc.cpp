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

    CUIRect GeneralGroup;
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
    s_ScrollRegion.End();
}
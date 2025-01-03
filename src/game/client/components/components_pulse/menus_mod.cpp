#include <base/math.h>
#include <base/system.h>

#include <engine/demo.h>
#include <engine/favorites.h>
#include <engine/friends.h>
#include <engine/ghost.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/textrender.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/countryflags.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>
#include "rcon_parse.h"
#include "game/client/components/menus.h"
#include "game/client/components/motd.h"
#include "game/client/components/voting.h"

#include "game/client/components/ghost.h"
#include <engine/keys.h>
#include <engine/storage.h>

#include <chrono>
#include <gmock/gmock-matchers.h>
#include <oneapi/tbb/detail/_task.h>

using namespace FontIcons;
using namespace std::chrono_literals;


void CMenus::RenderServerModMenu(CUIRect MainView) {

	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(20.0f, &MainView);
	CUIRect LeftView, RightView, Line, Rect;
	MainView.VSplitMid(&LeftView, &RightView, 10.f);

	static int s_SelectedPlayerIndex = -1; // Selected player index
	static bool isOpen = false;

	if(Client()->RconAuthed()) {
		// FIX THIS KOSTYL
		if(!isOpen){
			m_pClient->m_GameConsoleParse.RconAuthenticated(true);
			m_pClient->m_GameConsoleParse.Refresh();

			isOpen = true;
		}

		// Players search
		LeftView.HSplitTop(20.f, &Line, &LeftView);
		Line.VSplitRight(20.f, &Line, &Rect);
		Line.VSplitRight(5.f, &Line, nullptr);
		static CLineInputBuffered<MAX_NAME_LENGTH> s_Input;
		s_Input.SetEmptyText(Localize("Player search. Type name or clan."));
		Ui()->DoEditBox(&s_Input, &Line, 16.0f);

		static CButtonContainer s_ButtonEditWidgets;
		if(DoButton_Menu(&s_ButtonEditWidgets, "X", 0, &Rect))
			s_Input.Clear();

		LeftView.HSplitTop(5.f, nullptr, &LeftView);

		// Clients list
		static CScrollRegion s_ScrollRegion;
		vec2 ScrollOffset(0.0f, 0.0f);
		CScrollRegionParams ScrollParams;
		ScrollParams.m_ScrollUnit = 120.0f;
		s_ScrollRegion.Begin(&LeftView, &ScrollOffset, &ScrollParams);
		LeftView.y += ScrollOffset.y;

for (int i = 0; i < MAX_CLIENTS; i++) {
    CGameClient::CClientData &Client = GameClient()->m_aClients[i];

    if (!Client.m_Active)
        continue;

    if (!s_Input.IsEmpty() && !str_find_nocase(Client.m_aName, s_Input.GetString()) && !str_find_nocase(Client.m_aClan, s_Input.GetString()))
        continue;

    LeftView.HSplitTop(30.f, &Line, &LeftView);
    LeftView.HSplitTop(5.f, nullptr, &LeftView);
    if (!s_ScrollRegion.AddRect(Line))
        continue;

    // Invisible button for selecting player
    static CButtonContainer s_aButtons[MAX_CLIENTS];
    bool PlayerSelected = DoButton_Menu(&s_aButtons[i], "", 0, &Line, 0x0, 0, 0, 0, ColorRGBA(0.f, 0.f, 0.f, 0.2f));

    // Background for player line
    Line.Draw(ColorRGBA(1.f, 1.f, 1.f, s_SelectedPlayerIndex == i ? 0.4f : 0.2f), IGraphics::CORNER_ALL, 5);

	ClientInfo pClient = m_pClient->m_GameConsoleParse.GetClientById(s_SelectedPlayerIndex);


    // Player skin
    Line.VSplitLeft(30.f, &Rect, &Line);
    CTeeRenderInfo RenderInfo = Client.m_RenderInfo;
    RenderInfo.m_Size = 25.f;
    vec2 OffsetToMid;
    CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &RenderInfo, OffsetToMid);
    const vec2 TeeRenderPos = vec2(Rect.x + Rect.h / 2, Rect.y + Rect.h / 2 + OffsetToMid.y);
    RenderTools()->RenderTee(CAnimState::GetIdle(), &RenderInfo, EMOTE_NORMAL, vec2(1, 0), TeeRenderPos);

    // Player name
    Line.HMargin(3.f, &Rect);
    Ui()->DoLabel(&Rect, Client.m_aName, 15.f, TEXTALIGN_LEFT);
    Line.HSplitBottom(12.f, nullptr, &Rect);
    Ui()->DoLabel(&Rect, Client.m_aClan, 8.f, TEXTALIGN_LEFT);

    // Create a "Mute" button positioned to the left of the "Kick" button
    Line.VSplitRight(Line.w / 7, &Line, &Rect);
    Rect.h = 25.f; // Set button height
    Rect.y += (30.f - Rect.h) / 2; // Center the button vertically within the 30.f height
	Rect.w = 60.f; // Set width for the "Mute" button
//TODO: 4:3 support (console resize support from xc_client)
	if (DoButton_Menu(&s_aButtons[MAX_CLIENTS + i], "Ban", 0, &Rect, 0x0, IGraphics::CORNER_R, 6, 0, ColorRGBA(1.f, 0.0f, 0.0f, 0.3f))) {
        PlayerSelected = false; // Prevent player selection
		m_pClient->m_GameConsoleParse.Refresh();
    }

    // Create a "Kick" button positioned to the right of the "Mute" button
    Line.VSplitRight(60.f, &Line, &Rect); // Adjust 60.f for the button width
    Rect.h = 25.f; // Set button height
    Rect.y += (30.f - Rect.h) / 2; // Center the button vertically within the 30.f height

    // Kick button
    if (DoButton_Menu(&s_aButtons[MAX_CLIENTS + i + MAX_CLIENTS], "Kick", 0, &Rect, 0x0, 0, 0, 0, ColorRGBA(1.f, 0.5f, 0.f, 0.5f))) {
        PlayerSelected = false; // Prevent player selection
    }

    // Create a "Ban" button positioned to the right of the "Kick" button
    Line.VSplitRight(60.f, &Line, &Rect); // Adjust 60.f for the button width
    Rect.h = 25.f; // Set button height
	Rect.y += (30.f - Rect.h) / 2; // Center the button vertically within the 30.f height

    // Ban button
    if (DoButton_Menu(&s_aButtons[MAX_CLIENTS + i + 2 * MAX_CLIENTS], "Mute", 0, &Rect, 0x0, IGraphics::CORNER_L, 6, 0, ColorRGBA(0.14, 0.144f, 0.91f, 0.5f))) {
        PlayerSelected = false; // Prevent player selection
    }

    // If the player was selected, update the selected index
    if (PlayerSelected) {
        s_SelectedPlayerIndex = i;
    }
}
			s_ScrollRegion.End();
	}
		//---> Right side
		// Display selected player information
		CUIRect PlayerInfo, CopyView;
		if(s_SelectedPlayerIndex != -1) {


			CGameClient::CClientData &Client = GameClient()->m_aClients[s_SelectedPlayerIndex];
			ClientInfo pClient = m_pClient->m_GameConsoleParse.GetClientById(s_SelectedPlayerIndex);

			RightView.HSplitTop(RightView.h / 3, &PlayerInfo, &RightView);
			PlayerInfo.Draw(ColorRGBA(0.f, 0.f, 0.f, 0.3f), IGraphics::CORNER_R, 5);


			CTeeRenderInfo RenderInfo = Client.m_RenderInfo;
			RenderInfo.m_Size = 60.f;
			vec2 OffsetToMid;
			CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &RenderInfo, OffsetToMid);
			const vec2 TeeRenderPos = vec2(PlayerInfo.x + PlayerInfo.h / 4, PlayerInfo.y + PlayerInfo.h / 4 + OffsetToMid.y);
			RenderTools()->RenderTee(CAnimState::GetIdle(), &RenderInfo, EMOTE_NORMAL, vec2(1, 0), TeeRenderPos);

			PlayerInfo.HSplitBottom(PlayerInfo.h / 3, &PlayerInfo, &CopyView);
			CUIRect id, name, addr, kog_id, client, dnsbl;

			PlayerInfo.HSplitTop(PlayerInfo.h / 3, &id, &PlayerInfo);
			id.VSplitMid(&id, &addr);
			char bBuf[8];
			id.VSplitMid(nullptr, &id);
			str_format(bBuf, sizeof(bBuf), "ID:%d", pClient.id);
			Ui()->DoLabel(&id, bBuf, 20.0f, TEXTALIGN_LEFT);

			char cBuf[64];
			str_format(cBuf, sizeof(cBuf), "IP:%s", pClient.addr);
			Ui()->DoLabel(&addr, cBuf, 20.0f, TEXTALIGN_LEFT);

			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Selected Player: %s", Client.m_aName);
			Ui()->DoLabel(&RightView, aBuf, 10.0f, TEXTALIGN_TL);
		}
}
//TODO: Нофис сосал1
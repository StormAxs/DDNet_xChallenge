/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "scoreboard.h"

#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <sstream>
#include <thread>

#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/statboard.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/generated/client_data7.h>
#include <game/localization.h>
#include <game/client/components/components_pulse/rcon_parse.h>

struct SPopupProperties
{
	static constexpr float ms_Width = 220.0f;

	static constexpr float ms_HeadlineFontSize = 24.0f;
	static constexpr float ms_FontSize = 24.0f;
	static constexpr float ms_IconFontSize = 33.0f;
	static constexpr float ms_Padding = 20.0f;
	static constexpr float ms_Rounding = 10.0f;

	static constexpr float ms_ItemSpacing = 5.0f;
	static constexpr float ms_GroupSpacing = 15.0f;

	static constexpr float ms_QuickActionsHeight = 50.0f;
	static constexpr float ms_ButtonHeight = 35.0f;

	static ColorRGBA WindowColor() { return ColorRGBA(0.451f, 0.451f, 0.451f, 0.9f); };
	static ColorRGBA GeneralButtonColor() { return ColorRGBA(0.541f, 0.561f, 0.48f, 0.8f); };
	static ColorRGBA GeneralActiveButtonColor() { return ColorRGBA(0.53f, 0.78f, 0.53f, 0.8f); };

	static ColorRGBA ActionGeneralButtonColor() { return ColorRGBA(0.541f, 0.561f, 0.48f, 0.8f); };
	static ColorRGBA ActionActiveButtonColor() { return ColorRGBA(0.53f, 0.78f, 0.53f, 0.8f); };
	static ColorRGBA ActionAltActiveButtonColor() { return ColorRGBA(1.0f, 0.42f, 0.42f, 0.8f); };

	static ColorRGBA TeamsGeneralButtonColor() { return ColorRGBA(0.32f, 0.32f, 0.72f, 0.8f); };
	static ColorRGBA TeamsActiveButtonColor() { return ColorRGBA(0.31f, 0.52f, 0.78f, 0.8f); };

	static ColorRGBA ModGeneralActiveButtonColor() { return ColorRGBA(0.3f, 0.f, 0.f, 0.8f); };
	static ColorRGBA ModActiveButtonColor() { return ColorRGBA(0.3f, 0.f, 0.f, 0.8f); };

};

void CScoreboard::DoIconLabeledButton(CUIRect *pRect, const char *pTitle, const char *pIcon, float TextSize, float Height, ColorRGBA IconColor) const
{
	CUIRect Label;
	pRect->VSplitLeft(Height, &Label, pRect);
	DoIconButton(&Label, pIcon, TextSize, IconColor);
	Ui()->DoLabel(pRect, pTitle, TextSize, TEXTALIGN_MC);
}
void CScoreboard::DoIconButton(CUIRect *pRect, const char *pIcon, float TextSize, ColorRGBA IconColor) const
{
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	TextRender()->TextColor(IconColor);
	Ui()->DoLabel(pRect, pIcon, TextSize, TEXTALIGN_MC);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}

CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pSelf = static_cast<CScoreboard *>(pUserData);
	pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::OnReset()
{
	m_Active = false;
	m_Mouse.reset();
	m_Popup.reset();
	m_ServerRecord = -1.0f;
}

void CScoreboard::OnRelease()
{
	m_Active = false;
	m_Mouse.reset();
	m_Popup.reset();
}

void CScoreboard::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_RECORD)
	{
		CNetMsg_Sv_Record *pMsg = static_cast<CNetMsg_Sv_Record *>(pRawMsg);
		m_ServerRecord = pMsg->m_ServerTimeBest / 100.0f;
	}
	else if(MsgType == NETMSGTYPE_SV_RECORDLEGACY)
	{
		CNetMsg_Sv_RecordLegacy *pMsg = static_cast<CNetMsg_Sv_RecordLegacy *>(pRawMsg);
		m_ServerRecord = pMsg->m_ServerTimeBest / 100.0f;
	}
}

bool CScoreboard::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!Active() || !m_Mouse.m_Unlocked)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);

	m_Mouse.m_Position.x += x;
	m_Mouse.m_Position.y += y;

	const float ScreenWidth = 400.0f * 3.0f * Graphics()->ScreenAspect();
	const float ScreenHeight = 400.0f * 3.0f;

	m_Mouse.clampPosition(ScreenWidth, ScreenHeight);

	return true;
}

bool CScoreboard::OnInput(const IInput::CEvent &Event)
{
	if(!Active())
		return false;

	if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_MOUSE_3) //TODO: binder
		m_Mouse.m_Unlocked = !m_Mouse.m_Unlocked;

	m_Mouse.m_LastMouseInput = m_Mouse.m_MouseInput;
	m_Mouse.m_MouseInput = m_Mouse.m_Unlocked && (Event.m_Flags & IInput::FLAG_PRESS && (Event.m_Key == KEY_MOUSE_1 || Event.m_Key == KEY_MOUSE_2));
	m_Mouse.m_Clicked = !m_Mouse.m_LastMouseInput && m_Mouse.m_MouseInput;

	return m_Mouse.m_Clicked;
}

void CScoreboard::SPlayerPopup::toggle(const bool Show, const vec2 Pos, const int Id)
{
	m_Visible = Show;
	if(Show)
	{
		m_Position = Pos;
		m_PlayerId = Id;
	}
}

bool CScoreboard::SPlayerPopup::shouldHide(const SMouseState &Mouse, const bool PlayerHovered) const
{
	return (!PlayerHovered && Mouse.m_Clicked) ||
	       !Mouse.m_Unlocked;
}

void CScoreboard::RenderTitle(CUIRect TitleBar, int Team, const char *pTitle)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;

	char aScore[128] = "";
	if(GameClient()->m_GameInfo.m_TimeScore)
	{
		if(m_ServerRecord > 0)
		{
			str_time_float(m_ServerRecord, TIME_HOURS, aScore, sizeof(aScore));
		}
	}
	else if(pGameInfoObj && (pGameInfoObj->m_GameFlags & GAMEFLAG_TEAMS))
	{
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if(pGameDataObj)
		{
			str_format(aScore, sizeof(aScore), "%d", Team == TEAM_RED ? pGameDataObj->m_TeamscoreRed : pGameDataObj->m_TeamscoreBlue);
		}
	}
	else
	{
		if(GameClient()->m_Snap.m_SpecInfo.m_Active &&
			GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW &&
			GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId])
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_apPlayerInfos[GameClient()->m_Snap.m_SpecInfo.m_SpectatorId]->m_Score);
		}
		else if(GameClient()->m_Snap.m_pLocalInfo)
		{
			str_format(aScore, sizeof(aScore), "%d", GameClient()->m_Snap.m_pLocalInfo->m_Score);
		}
	}

	const float TitleFontSize = 40.0f;
	const float ScoreTextWidth = TextRender()->TextWidth(TitleFontSize, aScore);

	TitleBar.VMargin(20.0f, &TitleBar);
	CUIRect TitleLabel, ScoreLabel;
	if(Team == TEAM_RED)
	{
		TitleBar.VSplitRight(ScoreTextWidth, &TitleLabel, &ScoreLabel);
		TitleLabel.VSplitRight(10.0f, &TitleLabel, nullptr);
	}
	else
	{
		TitleBar.VSplitLeft(ScoreTextWidth, &ScoreLabel, &TitleLabel);
		TitleLabel.VSplitLeft(10.0f, nullptr, &TitleLabel);
	}

	{
		SLabelProperties Props;
		Props.m_MaxWidth = TitleLabel.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&TitleLabel, pTitle, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_ML : TEXTALIGN_MR, Props);
	}

	if(aScore[0] != '\0')
	{
		Ui()->DoLabel(&ScoreLabel, aScore, TitleFontSize, Team == TEAM_RED ? TEXTALIGN_MR : TEXTALIGN_ML);
	}
}

void CScoreboard::RenderGoals(CUIRect Goals)
{
	Goals.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);
	Goals.VMargin(10.0f, &Goals);

	const float FontSize = 20.0f;
	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	char aBuf[64];

	if(pGameInfoObj->m_ScoreLimit)
	{
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), pGameInfoObj->m_ScoreLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_ML);
	}

	if(pGameInfoObj->m_TimeLimit)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), pGameInfoObj->m_TimeLimit);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MC);
	}

	if(pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)
	{
		str_format(aBuf, sizeof(aBuf), Localize("Round %d/%d"), pGameInfoObj->m_RoundCurrent, pGameInfoObj->m_RoundNum);
		Ui()->DoLabel(&Goals, aBuf, FontSize, TEXTALIGN_MR);
	}
}

void CScoreboard::RenderSpectators(CUIRect Spectators)
{
	Spectators.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);
	Spectators.Margin(10.0f, &Spectators);

	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, Spectators.x, Spectators.y, 22.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = Spectators.w;
	Cursor.m_MaxLines = round_truncate(Spectators.h / Cursor.m_FontSize);

	int RemainingSpectators = 0;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;
		++RemainingSpectators;
	}

	TextRender()->TextEx(&Cursor, Localize("Spectators"));

	if(RemainingSpectators > 0)
	{
		TextRender()->TextEx(&Cursor, ": ");
	}

	bool CommaNeeded = false;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(CommaNeeded)
		{
			TextRender()->TextEx(&Cursor, ", ");
		}

		if(Cursor.m_LineCount == Cursor.m_MaxLines && RemainingSpectators >= 2)
		{
			// This is less expensive than checking with a separate invisible
			// text cursor though we waste some space at the end of the line.
			char aRemaining[64];
			str_format(aRemaining, sizeof(aRemaining), Localize("%d others…", "Spectators"), RemainingSpectators);
			TextRender()->TextEx(&Cursor, aRemaining);
			break;
		}

		if(g_Config.m_ClShowIds)
		{
			char aClientId[16];
			GameClient()->FormatClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::NO_INDENT);
			TextRender()->TextEx(&Cursor, aClientId);
		}

		{
			const char *pClanName = GameClient()->m_aClients[pInfo->m_ClientId].m_aClan;

			if(pClanName[0] != '\0')
			{
				if(str_comp(pClanName, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor)));
				}
				else
				{
					TextRender()->TextColor(ColorRGBA(0.7f, 0.7f, 0.7f));
				}

				TextRender()->TextEx(&Cursor, pClanName);
				TextRender()->TextEx(&Cursor, " ");

				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
		}

		if(GameClient()->m_aClients[pInfo->m_ClientId].m_AuthLevel)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor)));
		}
		else if(GameClient()->m_aClients[pInfo->m_ClientId].m_Friend)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_XcScoreboardColorFriends)));
		}
		else if(GameClient()->m_aClients[pInfo->m_ClientId].m_Foe)
		{
			TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_XcScoreboardColorFoes)));
		}



		TextRender()->TextEx(&Cursor, GameClient()->m_aClients[pInfo->m_ClientId].m_aName);
		TextRender()->TextColor(TextRender()->DefaultTextColor());

		CommaNeeded = true;
		--RemainingSpectators;
	}
}

bool CScoreboard::RenderScoreboard(CUIRect Scoreboard, int Team, int CountStart, int CountEnd, CScoreboardRenderState &State, bool playerHovered)
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
	const bool TimeScore = GameClient()->m_GameInfo.m_TimeScore;
	const int NumPlayers = CountEnd - CountStart;
	const bool LowScoreboardWidth = Scoreboard.w < 700.0f;

	bool Race7 = Client()->IsSixup() && pGameInfoObj && pGameInfoObj->m_GameFlags & protocol7::GAMEFLAG_RACE;

	// calculate measurements
	float LineHeight;
	float TeeSizeMod;
	float Spacing;
	float RoundRadius;
	float FontSize;
	if(NumPlayers <= 8)
	{
		LineHeight = 60.0f;
		TeeSizeMod = 1.0f;
		Spacing = 16.0f;
		RoundRadius = 10.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 12)
	{
		LineHeight = 50.0f;
		TeeSizeMod = 0.9f;
		Spacing = 5.0f;
		RoundRadius = 10.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 16)
	{
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 24)
	{
		LineHeight = 27.0f;
		TeeSizeMod = 0.6f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 20.0f;
	}
	else if(NumPlayers <= 32)
	{
		LineHeight = 20.0f;
		TeeSizeMod = 0.4f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 16.0f;
	}
	else if(LowScoreboardWidth)
	{
		LineHeight = 15.0f;
		TeeSizeMod = 0.25f;
		Spacing = 0.0f;
		RoundRadius = 2.0f;
		FontSize = 14.0f;
	}
	else
	{
		LineHeight = 10.0f;
		TeeSizeMod = 0.2f;
		Spacing = 0.0f;
		RoundRadius = 2.0f;
		FontSize = 10.0f;
	}

	const float ScoreOffset = Scoreboard.x + 40.0f;
	const float ScoreLength = TextRender()->TextWidth(FontSize, TimeScore ? "00:00:00" : "99999");
	const float TeeOffset = ScoreOffset + ScoreLength + 20.0f;
	const float TeeLength = 60.0f * TeeSizeMod;
	const float NameOffset = TeeOffset + TeeLength;
	const float NameLength = (LowScoreboardWidth ? 180.0f : 300.0f) - TeeLength;
	const float CountryLength = (LineHeight - Spacing - TeeSizeMod * 5.0f) * 2.0f;
	const float PingLength = 55.0f;
	const float PingOffset = Scoreboard.x + Scoreboard.w - PingLength - 20.0f;
	const float CountryOffset = PingOffset - CountryLength;
	const float ClanOffset = NameOffset + NameLength + 5.0f;
	const float ClanLength = CountryOffset - ClanOffset - 5.0f;

	// render headlines

	const float HeadlineFontsize = 22.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	const char *pScore = TimeScore ? Localize("T") : Localize("Score");
	TextRender()->Text(ScoreOffset + ScoreLength - TextRender()->TextWidth(HeadlineFontsize, pScore), HeadlineY, HeadlineFontsize, pScore);
	TextRender()->Text(NameOffset, HeadlineY, HeadlineFontsize, Localize("N"));
	const char *pClanLabel = Localize("C");
	TextRender()->Text(ClanOffset + (ClanLength - TextRender()->TextWidth(HeadlineFontsize, pClanLabel)) / 2.0f, HeadlineY, HeadlineFontsize, pClanLabel);
	const char *pPingLabel = Localize(FontIcons::FONT_ICON_SERVER);
	TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(HeadlineFontsize, pPingLabel), HeadlineY, HeadlineFontsize, pPingLabel);
;

	// render player entries
	int CountRendered = 0;
	int PrevDDTeam = -1;
	int &CurrentDDTeamSize = State.m_CurrentDDTeamSize;

	char aBuf[64];
	int MaxTeamSize = m_pClient->Config()->m_SvMaxTeamSize;

	for(int RenderDead = 0; RenderDead < 2; RenderDead++)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// make sure that we render the correct team
			const CNetObj_PlayerInfo *pInfo = GameClient()->m_Snap.m_apInfoByDDTeamScore[i];
			if(!pInfo || pInfo->m_Team != Team)
				continue;

			if(CountRendered++ < CountStart)
				continue;

			int DDTeam = GameClient()->m_Teams.Team(pInfo->m_ClientId);
			int NextDDTeam = 0;
			bool IsDead = Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_DEAD;
			if(!RenderDead && IsDead)
				continue;
			if(RenderDead && !IsDead)
				continue;

			ColorRGBA TextColor = TextRender()->DefaultTextColor();
			TextColor.a = RenderDead ? 0.5f : 1.0f;
			TextRender()->TextColor(TextColor);

			for(int j = i + 1; j < MAX_CLIENTS; j++)
			{
				const CNetObj_PlayerInfo *pInfoNext = GameClient()->m_Snap.m_apInfoByDDTeamScore[j];
				if(!pInfoNext || pInfoNext->m_Team != Team)
					continue;

				NextDDTeam = GameClient()->m_Teams.Team(pInfoNext->m_ClientId);
				break;
			}

			if(PrevDDTeam == -1)
			{
				for(int j = i - 1; j >= 0; j--)
				{
					const CNetObj_PlayerInfo *pInfoPrev = GameClient()->m_Snap.m_apInfoByDDTeamScore[j];
					if(!pInfoPrev || pInfoPrev->m_Team != Team)
						continue;

					PrevDDTeam = GameClient()->m_Teams.Team(pInfoPrev->m_ClientId);
					break;
				}
			}

			CUIRect RowAndSpacing, Row;
			Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
			RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);

			// team background
			if(DDTeam != TEAM_FLOCK)
			{
				const ColorRGBA Color = GameClient()->GetDDTeamColor(DDTeam).WithAlpha(0.5f);
				int TeamRectCorners = 0;
				if(PrevDDTeam != DDTeam)
				{
					TeamRectCorners |= IGraphics::CORNER_T;
					State.m_TeamStartX = Row.x;
					State.m_TeamStartY = Row.y;
				}
				if(NextDDTeam != DDTeam)
					TeamRectCorners |= IGraphics::CORNER_B;
				RowAndSpacing.Draw(Color, TeamRectCorners, RoundRadius);

				CurrentDDTeamSize++;

				if(NextDDTeam != DDTeam)
				{
					const float TeamFontSize = FontSize / 1.5f;

					if(NumPlayers > 8)
					{
						if(DDTeam == TEAM_SUPER)
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize <= 1)
							str_format(aBuf, sizeof(aBuf), "%d", DDTeam);
						else
							str_format(aBuf, sizeof(aBuf), Localize("%d\n(%d/%d)", "Team and size"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						TextRender()->Text(State.m_TeamStartX, maximum(State.m_TeamStartY + Row.h / 2.0f - TeamFontSize, State.m_TeamStartY + 3.0f /* padding top */), TeamFontSize, aBuf);
					}
					else
					{
						if(DDTeam == TEAM_SUPER)
							str_copy(aBuf, Localize("Super"));
						else if(CurrentDDTeamSize > 1)
							str_format(aBuf, sizeof(aBuf), Localize("Team %d (%d/%d)"), DDTeam, CurrentDDTeamSize, MaxTeamSize);
						else
							str_format(aBuf, sizeof(aBuf), Localize("Team %d"), DDTeam);
						TextRender()->Text(Row.x + Row.w / 2.0f - TextRender()->TextWidth(TeamFontSize, aBuf) / 2.0f + 10.0f, Row.y + Row.h, TeamFontSize, aBuf);
					}

					CurrentDDTeamSize = 0;
				}
			}
			PrevDDTeam = DDTeam;

			// background so it's easy to find the local player or the followed one in spectator mode
			if((!GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_SpectatorId == SPEC_FREEVIEW && pInfo->m_Local) ||
				(GameClient()->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientId == GameClient()->m_Snap.m_SpecInfo.m_SpectatorId))
			{
				Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, RoundRadius);
			}

			const CGameClient::CClientData &ClientData = GameClient()->m_aClients[pInfo->m_ClientId];

			if(!playerHovered)
			{
				if(Hovered(&Row) && !Hovered(&m_Popup.m_Rect))
					Row.Draw(ColorRGBA(.7f, .7f, .7f, .7f), IGraphics::CORNER_ALL, RoundRadius);
				if(DoButtonLogic(&Row))
				{
					m_Popup.toggle(true, m_Mouse.m_Position, pInfo->m_ClientId);
					playerHovered = true;
				}
			}

			// score
			if(Race7)
			{
				if(pInfo->m_Score == -1)
				{
					aBuf[0] = '\0';
				}
				else
				{
					// 0.7 uses milliseconds and ddnets str_time wants centiseconds
					// 0.7 servers can also send the amount of precision the client should use
					// we ignore that and always show 3 digit precision
					str_time((int64_t)absolute(pInfo->m_Score / 10), TIME_MINS_CENTISECS, aBuf, sizeof(aBuf));
				}
			}
			else if(TimeScore)
			{
				if(pInfo->m_Score == -9999)
				{
					aBuf[0] = '\0';
				}
				else
				{
					str_time((int64_t)absolute(pInfo->m_Score) * 100, TIME_HOURS, aBuf, sizeof(aBuf));
				}
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 99999));
			}
			TextRender()->Text(ScoreOffset + ScoreLength - TextRender()->TextWidth(FontSize, aBuf), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aBuf);

			// CTF flag
			if(pGameInfoObj && (pGameInfoObj->m_GameFlags & GAMEFLAG_FLAGS) &&
				pGameDataObj && (pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientId || pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId))
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientId ? GameClient()->m_GameSkin.m_SpriteFlagBlue : GameClient()->m_GameSkin.m_SpriteFlagRed);
				Graphics()->QuadsBegin();
				Graphics()->QuadsSetSubset(1.0f, 0.0f, 0.0f, 1.0f);
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y - 5.0f - Spacing / 2.0f, Row.h / 2.0f, Row.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// skin
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(client_data7::g_pData->m_aImages[client_data7::IMAGE_DEADTEE].m_Id);
				Graphics()->QuadsBegin();
				if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_TEAMS)
				{
					ColorRGBA Color = m_pClient->m_Skins7.GetTeamColor(true, 0, m_pClient->m_aClients[pInfo->m_ClientId].m_Team, protocol7::SKINPART_BODY);
					Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
				}
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientId].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				IGraphics::CQuadItem QuadItem(TeeOffset, Row.y, TeeInfo.m_Size, TeeInfo.m_Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = ClientData.m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				vec2 OffsetToMid;
				CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &TeeInfo, OffsetToMid);
				const vec2 TeeRenderPos = vec2(TeeOffset + TeeLength / 2, Row.y + Row.h / 2.0f + OffsetToMid.y);
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
			}

			// name
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, NameOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
				Cursor.m_LineWidth = NameLength;
				if(ClientData.m_AuthLevel)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClAuthedPlayerColor)));
				}
				else if(ClientData.m_Friend)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_XcScoreboardColorFriends)));
				}
				else if(ClientData.m_Foe)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_XcScoreboardColorFoes)));
				}

				if(g_Config.m_ClShowIds)
				{
					char aClientId[16];
					GameClient()->FormatClientId(pInfo->m_ClientId, aClientId, EClientIdFormat::INDENT_AUTO);
					TextRender()->TextEx(&Cursor, aClientId);
				}


				TextRender()->TextEx(&Cursor, ClientData.m_aName);

				// ready / watching
				if(Client()->IsSixup() && Client()->m_TranslationContext.m_aClients[pInfo->m_ClientId].m_PlayerFlags7 & protocol7::PLAYERFLAG_READY)
				{
					TextRender()->TextColor(0.1f, 1.0f, 0.1f, TextColor.a);
					TextRender()->TextEx(&Cursor, "✓");
				}
			}

			// clan
			{
				if(str_comp(ClientData.m_aClan, GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_aClan) == 0)
				{
					TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClSameClanColor)));
				}
				else
				{
					TextRender()->TextColor(TextColor);
				}
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, ClanOffset + (ClanLength - minimum(TextRender()->TextWidth(FontSize, ClientData.m_aClan), ClanLength)) / 2.0f, Row.y + (Row.h - FontSize) / 2.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
				Cursor.m_LineWidth = ClanLength;
				TextRender()->TextEx(&Cursor, ClientData.m_aClan);
			}

			// country flag
			GameClient()->m_CountryFlags.Render(ClientData.m_Country, ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f),
				CountryOffset, Row.y + (Spacing + TeeSizeMod * 5.0f) / 2.0f, CountryLength, Row.h - Spacing - TeeSizeMod * 5.0f);

			// ping
			if(g_Config.m_ClEnablePingColor)
			{
				TextRender()->TextColor(color_cast<ColorRGBA>(ColorHSLA((300.0f - clamp(pInfo->m_Latency, 0, 300)) / 1000.0f, 1.0f, 0.5f)));
			}
			else
			{
				TextRender()->TextColor(TextRender()->DefaultTextColor());
			}
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 999));
			TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(FontSize, aBuf), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aBuf);
			TextRender()->TextColor(TextRender()->DefaultTextColor());

			if(CountRendered == CountEnd)
				break;
		}
	}

	return playerHovered;
}

void CScoreboard::RenderRecordingNotification(float x)
{
	char aBuf[512] = "";

	const auto &&AppendRecorderInfo = [&](int Recorder, const char *pName) {
		if(GameClient()->DemoRecorder(Recorder)->IsRecording())
		{
			char aTime[32];
			str_time((int64_t)GameClient()->DemoRecorder(Recorder)->Length() * 100, TIME_HOURS, aTime, sizeof(aTime));
			str_append(aBuf, pName);
			str_append(aBuf, " ");
			str_append(aBuf, aTime);
			str_append(aBuf, "  ");
		}
	};

	AppendRecorderInfo(RECORDER_MANUAL, Localize("Manual"));
	AppendRecorderInfo(RECORDER_RACE, Localize("Race"));
	AppendRecorderInfo(RECORDER_AUTO, Localize("Auto"));
	AppendRecorderInfo(RECORDER_REPLAYS, Localize("Replay"));

	if(aBuf[0] == '\0')
		return;

	const float FontSize = 20.0f;

	CUIRect Rect = {x, 0.0f, TextRender()->TextWidth(FontSize, aBuf) + 60.0f, 50.0f};
	Rect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.4f), IGraphics::CORNER_B, 15.0f);
	Rect.VSplitLeft(20.0f, nullptr, &Rect);
	Rect.VSplitRight(10.0f, &Rect, nullptr);

	CUIRect Circle;
	Rect.VSplitLeft(20.0f, &Circle, &Rect);
	Circle.HMargin((Circle.h - Circle.w) / 2.0f, &Circle);
	Circle.Draw(ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), IGraphics::CORNER_ALL, Circle.h / 2.0f);

	Rect.VSplitLeft(10.0f, nullptr, &Rect);
	Ui()->DoLabel(&Rect, aBuf, FontSize, TEXTALIGN_ML);
}
float CScoreboard::CalculatePopupHeight()
{
	int GeneralButtons = 2;
	int TeamButtons = 0;
	{
		bool LocalIsTarget = GameClient()->m_aLocalIds[g_Config.m_ClDummy] == m_Popup.m_PlayerId;
		int LocalTeam = GameClient()->m_Teams.Team(GameClient()->m_aLocalIds[g_Config.m_ClDummy]);
		int TargetTeam = GameClient()->m_Teams.Team(m_Popup.m_PlayerId);
		bool IsMod = Client()->RconAuthed();

		bool LocalInTeam = LocalTeam != TEAM_FLOCK && LocalTeam != TEAM_SUPER;
		bool TargetInTeam = TargetTeam != TEAM_FLOCK && TargetTeam != TEAM_SUPER;
		if(g_Config.m_XcScoreboardActionsProfile)
			TeamButtons++;
		if(g_Config.m_XcScoreboardActionsCopyName)
			TeamButtons++;
		if(g_Config.m_XcScoreboardActionsVoteKick)
			TeamButtons++;
		if(g_Config.m_XcScoreboardActionsTeamActions)
		{
			if(LocalInTeam && LocalTeam == TargetTeam)
				TeamButtons++;
			if(TargetInTeam && LocalTeam != TargetTeam)
				TeamButtons++;
			if(LocalInTeam && TargetTeam != LocalTeam)
				TeamButtons++;
			if(!LocalIsTarget && LocalInTeam && TargetTeam == LocalTeam)
				TeamButtons++;
			if(LocalInTeam && LocalTeam == TargetTeam)
				TeamButtons++;
		}
		if(IsMod)
			TeamButtons++;
	}

	const float Height = (SPopupProperties::ms_Padding * 2) +
			     (SPopupProperties::ms_HeadlineFontSize) +
			     (SPopupProperties::ms_QuickActionsHeight) +
			     (SPopupProperties::ms_ItemSpacing * (GeneralButtons + TeamButtons + 1)) +
			     (SPopupProperties::ms_ButtonHeight * (GeneralButtons + TeamButtons)) +
			     (TeamButtons > 0 ? SPopupProperties::ms_GroupSpacing : 0);

	return Height;
}

static std::string encodeUTF8(const std::string &Input)
{
	std::ostringstream Encoded;
	const char *ptr = Input.c_str();

	while(*ptr != '\0')
	{
		int CodePoint = str_utf8_decode(&ptr);
		if(CodePoint < 0)
		{
			Encoded << "-ERROR-";
			break;
		}

		if(CodePoint <= 127 && std::isalnum(static_cast<unsigned char>(CodePoint)))
		{
			Encoded << static_cast<char>(CodePoint);
		}
		else
		{
			Encoded << '-' << CodePoint << '-';
		}
	}

	return Encoded.str();
}

void CScoreboard::RenderPlayerPopUp()
{
	const char *pPlayerName = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aName;

	CUIRect Base, Label;

	Base.x = m_Popup.m_Position.x;
	Base.y = m_Popup.m_Position.y;
	Base.h = CalculatePopupHeight();
	Base.w = SPopupProperties::ms_Width;

	vec2 ScreenTL, ScreenBR;
	Graphics()->GetScreen(&ScreenTL.x, &ScreenTL.y, &ScreenBR.x, &ScreenBR.y);

	if(Base.y + Base.h > ScreenBR.y)
	{
		Base.y -= Base.y + Base.h - ScreenBR.y;
	}
	if(Base.x + Base.w > ScreenBR.x)
	{
		Base.x -= Base.x + Base.w - ScreenBR.x;
	}

	m_Popup.m_Rect = Base;

	Base.Draw(SPopupProperties::WindowColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	Base.Margin(SPopupProperties::ms_Padding, &Base);

	Base.HSplitTop(SPopupProperties::ms_HeadlineFontSize, &Label, &Base);
	Ui()->DoLabel(&Label, pPlayerName, SPopupProperties::ms_HeadlineFontSize, TEXTALIGN_ML);

	RenderQuickActions(&Base);
	RenderGeneralActions(&Base);
	RenderTeamActions(&Base);
	if(Client()->RconAuthed())
		RenderModActions(&Base);

}

void CScoreboard::RenderQuickActions(CUIRect *pBase)
{
	CUIRect Container, Action;

	const char *pPlayerName = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aName;
	const char *pPlayerClan = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aClan;

	pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
	pBase->HSplitTop(SPopupProperties::ms_QuickActionsHeight, &Container, pBase);

	float ActionSpacing = (pBase->w - (3 * SPopupProperties::ms_QuickActionsHeight)) / 2;

	Container.VSplitLeft(SPopupProperties::ms_QuickActionsHeight, &Action, &Container);

	// Friend
	const bool IsFriend = Client()->Friends()->IsFriend(pPlayerName, pPlayerClan, true);
	if(Hovered(&Action))
	{
		Action.Draw(IsFriend ? SPopupProperties::ActionAltActiveButtonColor() : SPopupProperties::ActionActiveButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		DoIconButton(&Action, IsFriend ? FontIcons::FONT_ICON_HEART_CRACK : FontIcons::FONT_ICON_HEART, SPopupProperties::ms_IconFontSize, TextRender()->DefaultTextColor());
	}
	else
	{
		Action.Draw(IsFriend ? SPopupProperties::ActionActiveButtonColor() : SPopupProperties::ActionGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		DoIconButton(&Action, FontIcons::FONT_ICON_HEART, SPopupProperties::ms_IconFontSize, TextRender()->DefaultTextColor());
	}
	if(DoButtonLogic(&Action))
	{
		if(IsFriend)
			Client()->Friends()->RemoveFriend(pPlayerName, pPlayerClan);
		else
			Client()->Friends()->AddFriend(pPlayerName, pPlayerClan);
	}

	Container.VSplitLeft(ActionSpacing, nullptr, &Container);
	Container.VSplitLeft(SPopupProperties::ms_QuickActionsHeight, &Action, &Container);

	// Foe
	const bool IsFoe = Client()->Foes()->IsFriend(pPlayerName, pPlayerClan, true);
	Action.Draw(IsFoe ? SPopupProperties::ActionAltActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	if(Hovered(&Action))
		Action.Draw(IsFoe ? SPopupProperties::ActionActiveButtonColor() : SPopupProperties::ActionAltActiveButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	DoIconButton(&Action, FontIcons::FONT_ICON_BAN, SPopupProperties::ms_IconFontSize, TextRender()->DefaultTextColor());
	if(DoButtonLogic(&Action))
	{
		if(IsFoe)
			Client()->Foes()->RemoveFriend(pPlayerName, pPlayerClan);
		else
			Client()->Foes()->AddFriend(pPlayerName, pPlayerClan);
	}

	Container.VSplitRight(SPopupProperties::ms_QuickActionsHeight, &Container, &Action);

	// Hidden
	const bool IsHidden = Client()->Hidden()->IsFriend(pPlayerName, pPlayerClan, true);
	if(Hovered(&Action))
	{
		Action.Draw(IsHidden ? SPopupProperties::ActionActiveButtonColor() : SPopupProperties::ActionAltActiveButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		DoIconButton(&Action, IsHidden ? FontIcons::FONT_ICON_EYE : FontIcons::FONT_ICON_EYE_SLASH, SPopupProperties::ms_IconFontSize, TextRender()->DefaultTextColor());
	}
	else
	{
		Action.Draw(IsHidden ? SPopupProperties::ActionAltActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		DoIconButton(&Action, IsHidden ? FontIcons::FONT_ICON_EYE_SLASH : FontIcons::FONT_ICON_EYE, SPopupProperties::ms_IconFontSize, TextRender()->DefaultTextColor());
	}
	if(DoButtonLogic(&Action))
	{
		if(IsHidden)
			Client()->Hidden()->RemoveFriend(pPlayerName, pPlayerClan);
		else
			Client()->Hidden()->AddFriend(pPlayerName, pPlayerClan);
	}
}

void CScoreboard::RenderGeneralActions(CUIRect *pBase)
{
	CUIRect Button;

	const char *pPlayerName = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aName;

	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	int Community = (str_comp(ServerInfo.m_aCommunityId, "kog") == 0) ? 1 : (str_comp(ServerInfo.m_aCommunityId, "unique") == 0) ? 2 : 0;
	char aCommunityLink[512];
	if(Community == 1)
		str_format(aCommunityLink, sizeof(aCommunityLink), "https://kog.tw/#p=players&player=%s", pPlayerName);
	else if(Community == 2)
		str_format(aCommunityLink, sizeof(aCommunityLink), "https://uniqueclan.net/ranks/player/%s", pPlayerName);
	else
		str_format(aCommunityLink, sizeof(aCommunityLink), "https://ddnet.org/players/%s", encodeUTF8(pPlayerName).c_str());

	if(g_Config.m_XcScoreboardActionsProfile)
	{
		pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
		pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
		Button.Draw(Hovered(&Button) ? SPopupProperties::GeneralActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		Ui()->DoLabel(&Button, Localize("Profile"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
		if(DoButtonLogic(&Button))
		{
			Client()->ViewLink(aCommunityLink);
		}
	}

	pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
	pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
	Button.Draw(Hovered(&Button) ? SPopupProperties::GeneralActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	Ui()->DoLabel(&Button, Localize("Whisper"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
	if(DoButtonLogic(&Button))
	{
		char aWhisperBuf[512];
		str_format(aWhisperBuf, sizeof(aWhisperBuf), "chat all /whisper %s ", pPlayerName);
		Console()->ExecuteLine(aWhisperBuf);
	}

	pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
	pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
	Button.Draw(Hovered(&Button) ? SPopupProperties::GeneralActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	Ui()->DoLabel(&Button, Localize("Spectate"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
	if(DoButtonLogic(&Button))
	{
		char aBuf[128];
		if(!GameClient()->m_Snap.m_SpecInfo.m_Active)
		{
			str_format(aBuf, sizeof(aBuf), "say /spec %s", pPlayerName);
			Console()->ExecuteLine(aBuf);
		}
		GameClient()->m_Spectator.Spectate(m_Popup.m_PlayerId);
	}
	if(g_Config.m_XcScoreboardActionsCopyName)
	{
		pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
		pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
		Button.Draw(Hovered(&Button) ? SPopupProperties::GeneralActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		Ui()->DoLabel(&Button, Localize("Copy Name"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
		if(DoButtonLogic(&Button))
		{
			Input()->SetClipboardText(pPlayerName);
		}
	}
	if(g_Config.m_XcScoreboardActionsVoteKick)
	{
		pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
		pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
		Button.Draw(Hovered(&Button) ? SPopupProperties::GeneralActiveButtonColor() : SPopupProperties::GeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
		Ui()->DoLabel(&Button, Localize("Vote Kick"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
		if(DoButtonLogic(&Button))
		{
			GameClient()->m_Voting.CallvoteKick(m_Popup.m_PlayerId, "");
		}
	}
}

void CScoreboard::RenderTeamActions(CUIRect *pBase)
{
	CUIRect Button;

	bool LocalIsTarget = GameClient()->m_aLocalIds[g_Config.m_ClDummy] == m_Popup.m_PlayerId;
	int LocalTeam = GameClient()->m_Teams.Team(GameClient()->m_aLocalIds[g_Config.m_ClDummy]);
	int TargetTeam = GameClient()->m_Teams.Team(m_Popup.m_PlayerId);

	bool LocalInTeam = LocalTeam != TEAM_FLOCK && LocalTeam != TEAM_SUPER;
	bool TargetInTeam = TargetTeam != TEAM_FLOCK && TargetTeam != TEAM_SUPER;
	if(g_Config.m_XcScoreboardActionsTeamActions){
		pBase->HSplitTop(SPopupProperties::ms_GroupSpacing, nullptr, pBase);

		if(LocalInTeam && LocalTeam == TargetTeam)
		{
			pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
			pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
			Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::TeamsGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
			Ui()->DoLabel(&Button, Localize("Exit"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
			if(DoButtonLogic(&Button))
			{
				Console()->ExecuteLine("say /team 0");
			}
		}
		if(TargetInTeam && LocalTeam != TargetTeam)
		{
			pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
			pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
			Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::TeamsGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
			Ui()->DoLabel(&Button, Localize("Join"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
			if(DoButtonLogic(&Button))
			{
				char aCmdBuf[128];
				str_format(aCmdBuf, sizeof(aCmdBuf), "say /team %d", TargetTeam);
				Console()->ExecuteLine(aCmdBuf);
			}
		}

		if(LocalInTeam && TargetTeam != LocalTeam)
		{
			pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
			pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
			Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::TeamsGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
			Ui()->DoLabel(&Button, Localize("Invite"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
			if(DoButtonLogic(&Button))
			{
				char aCmdBuf[128];
				str_format(aCmdBuf, sizeof(aCmdBuf), "say /invite %s", GameClient()->m_aClients[m_Popup.m_PlayerId].m_aName);
				Console()->ExecuteLine(aCmdBuf);
			}
		}
		if(!LocalIsTarget && LocalInTeam && TargetTeam == LocalTeam)
		{
			pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
			pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
			Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::TeamsGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
			Ui()->DoLabel(&Button, Localize("Kick"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
			if(DoButtonLogic(&Button))
			{
				GameClient()->m_Voting.CallvoteKick(m_Popup.m_PlayerId, "");
			}
		}

		if(LocalInTeam && LocalTeam == TargetTeam)
		{
			pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
			pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
			Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::TeamsGeneralButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
			Ui()->DoLabel(&Button, Localize("Lock"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
			if(DoButtonLogic(&Button))
			{
				Console()->ExecuteLine("say /lock");
			}
		}
	}
}

void CScoreboard::RenderModActions(CUIRect *pBase)
{
	static bool IsOpen = false;
	static bool RconCommandsExecuted = false;

	if (Client()->RconAuthed())
	{
		if (!IsOpen)
		{
			m_pClient->m_GameConsoleParse.RconAuthenticated(true);
			m_pClient->m_GameConsoleParse.Refresh();
			IsOpen = true;
			Console()->ExecuteLine("rcon show_ips 1");
			Console()->ExecuteLine("rcon status");
			RconCommandsExecuted = true;
		}
		if (!RconCommandsExecuted)
		{
			Console()->ExecuteLine("rcon show_ips 1");
			Console()->ExecuteLine("rcon status");
			RconCommandsExecuted = true;
		}

	}
	//TODO: Somehow parsing info when tab is pressed??? //XD
	//TODO: pop up a new window with custom ban
	CUIRect Button;

	CGameClient::CClientData &Client = GameClient()->m_aClients[m_Popup.m_PlayerId];
	ClientInfo pClient = m_pClient->m_GameConsoleParse.GetClientById(m_Popup.m_PlayerId);

	CServerInfo ServerInfo;
	pBase->HSplitTop(SPopupProperties::ms_GroupSpacing, nullptr, pBase);
	const char *pPlayerName = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aName;
	const char *pPlayerClan = GameClient()->m_aClients[m_Popup.m_PlayerId].m_aClan;

	int Community = (str_comp(ServerInfo.m_aCommunityId, "kog") == 0) ? 1 : (str_comp(ServerInfo.m_aCommunityId, "ddnet") == 0) ? 2 : 0;
	char aBanBuffer[1024];
	if(Community == 1)
		str_format(aBanBuffer, sizeof(aBanBuffer),  "$admin banip \"%s\" \"%s\" \"Botting\" 180d", pClient.addr, pPlayerName);
	else if(Community == 2)
		str_format(aBanBuffer, sizeof(aBanBuffer), "!ban %s \"%s\" 1w Botting", pClient.addr, pPlayerName);

	//TODO: redirect to mod menus when mod actions is been pressed
	pBase->HSplitTop(SPopupProperties::ms_ItemSpacing, nullptr, pBase);
	pBase->HSplitTop(SPopupProperties::ms_ButtonHeight, &Button, pBase);
	Button.Draw(Hovered(&Button) ? SPopupProperties::TeamsActiveButtonColor() : SPopupProperties::ModActiveButtonColor(), IGraphics::CORNER_ALL, SPopupProperties::ms_Rounding);
	Ui()->DoLabel(&Button, Localize("CopyBAN"), SPopupProperties::ms_FontSize, TEXTALIGN_MC);
	if(DoButtonLogic(&Button))
	{
		Input()->SetClipboardText(aBanBuffer);
	}
}



void CScoreboard::OnRender()
{
	//TODO:Frieds priority in scoreboard
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(!Active())
	{
		m_Popup.toggle(false);
		return;
	}

	// if the score board is active, then we should clear the motd message as well
	if(GameClient()->m_Motd.IsActive())
		GameClient()->m_Motd.Clear();

	const float Height = 400.0f * 3.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	const bool Teams = pGameInfoObj && (pGameInfoObj->m_GameFlags & GAMEFLAG_TEAMS);
	const auto &aTeamSize = GameClient()->m_Snap.m_aTeamSize;
	const int NumPlayers = Teams ? maximum(aTeamSize[TEAM_RED], aTeamSize[TEAM_BLUE]) : aTeamSize[TEAM_RED];

	const float ScoreboardSmallWidth = 750.0f + 20.0f;
	const float ScoreboardWidth = !Teams && NumPlayers <= 16 ? ScoreboardSmallWidth : 1500.0f;
	const float TitleHeight = 60.0f;

	CUIRect Scoreboard = {(Width - ScoreboardWidth) / 2.0f, 150.0f, ScoreboardWidth, 710.0f + TitleHeight};
	CScoreboardRenderState RenderState{};

	bool playerHovered = m_Popup.m_Visible && Hovered(&m_Popup.m_Rect);

	if(Teams)
	{
		const char *pRedTeamName = GetTeamName(TEAM_RED);
		const char *pBlueTeamName = GetTeamName(TEAM_BLUE);

		// Game over title
		const CNetObj_GameData *pGameDataObj = GameClient()->m_Snap.m_pGameDataObj;
		if((pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER) && pGameDataObj)
		{
			char aTitle[256];
			if(pGameDataObj->m_TeamscoreRed > pGameDataObj->m_TeamscoreBlue)
			{
				TextRender()->TextColor(ColorRGBA(0.975f, 0.17f, 0.17f, 1.0f));
				if(pRedTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Red team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pRedTeamName);
				}
			}
			else if(pGameDataObj->m_TeamscoreBlue > pGameDataObj->m_TeamscoreRed)
			{
				TextRender()->TextColor(ColorRGBA(0.17f, 0.46f, 0.975f, 1.0f));
				if(pBlueTeamName == nullptr)
				{
					str_copy(aTitle, Localize("Blue team wins!"));
				}
				else
				{
					str_format(aTitle, sizeof(aTitle), Localize("%s wins!"), pBlueTeamName);
				}
			}
			else
			{
				TextRender()->TextColor(ColorRGBA(0.91f, 0.78f, 0.33f, 1.0f));
				str_copy(aTitle, Localize("Draw!"));
			}

			const float TitleFontSize = 72.0f;
			CUIRect GameOverTitle = {Scoreboard.x, Scoreboard.y - TitleFontSize - 12.0f, Scoreboard.w, TitleFontSize};
			Ui()->DoLabel(&GameOverTitle, aTitle, TitleFontSize, TEXTALIGN_MC);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}

		CUIRect RedScoreboard, BlueScoreboard, RedTitle, BlueTitle;
		Scoreboard.VSplitMid(&RedScoreboard, &BlueScoreboard, 15.0f);
		RedScoreboard.HSplitTop(TitleHeight, &RedTitle, &RedScoreboard);
		BlueScoreboard.HSplitTop(TitleHeight, &BlueTitle, &BlueScoreboard);

		RedTitle.Draw(ColorRGBA(0.975f, 0.17f, 0.17f, 0.5f), IGraphics::CORNER_T, 15.0f);
		BlueTitle.Draw(ColorRGBA(0.17f, 0.46f, 0.975f, 0.5f), IGraphics::CORNER_T, 15.0f);
		RedScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 15.0f);
		BlueScoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_B, 15.0f);

		RenderTitle(RedTitle, TEAM_RED, pRedTeamName == nullptr ? Localize("Red team") : pRedTeamName);
		RenderTitle(BlueTitle, TEAM_BLUE, pBlueTeamName == nullptr ? Localize("Blue team") : pBlueTeamName);
		bool HoveredThisCall = RenderScoreboard(RedScoreboard, TEAM_RED, 0, NumPlayers, RenderState, playerHovered);
		playerHovered = HoveredThisCall || playerHovered;
		HoveredThisCall = RenderScoreboard(BlueScoreboard, TEAM_BLUE, 0, NumPlayers, RenderState, playerHovered);
		playerHovered = HoveredThisCall || playerHovered;
	}
	else
	{
		Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);

		const char *pTitle;
		if(pGameInfoObj && (pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER))
		{
			pTitle = Localize("Game over");
		}
		else
		{
			pTitle = Client()->GetCurrentMap();
		}

		CUIRect Title;
		Scoreboard.HSplitTop(TitleHeight, &Title, &Scoreboard);
		RenderTitle(Title, TEAM_RED, pTitle);

		if(NumPlayers <= 16)
		{
			bool HoveredThisCall = RenderScoreboard(Scoreboard, TEAM_RED, 0, NumPlayers, RenderState, playerHovered);
			playerHovered = HoveredThisCall || playerHovered;
		}
		else if(NumPlayers <= 64)
		{
			int PlayersPerSide;
			if(NumPlayers <= 24)
				PlayersPerSide = 12;
			else if(NumPlayers <= 32)
				PlayersPerSide = 16;
			else if(NumPlayers <= 48)
				PlayersPerSide = 24;
			else
				PlayersPerSide = 32;

			CUIRect LeftScoreboard, RightScoreboard;
			Scoreboard.VSplitMid(&LeftScoreboard, &RightScoreboard);

			bool HoveredThisCall = RenderScoreboard(LeftScoreboard, TEAM_RED, 0, PlayersPerSide, RenderState, playerHovered);
			playerHovered = HoveredThisCall || playerHovered;
			HoveredThisCall = RenderScoreboard(RightScoreboard, TEAM_RED, PlayersPerSide, 2 * PlayersPerSide, RenderState, playerHovered);
			playerHovered = HoveredThisCall || playerHovered;
		}
		else
		{
			const int NumColumns = 3;
			const int PlayersPerColumn = std::ceil(128.0f / NumColumns);
			CUIRect RemainingScoreboard = Scoreboard;
			for(int i = 0; i < NumColumns; ++i)
			{
				CUIRect Column;
				RemainingScoreboard.VSplitLeft(Scoreboard.w / NumColumns, &Column, &RemainingScoreboard);
				bool HoveredThisCall = RenderScoreboard(Column, TEAM_RED, i * PlayersPerColumn, (i + 1) * PlayersPerColumn, RenderState, playerHovered);
				playerHovered = HoveredThisCall || playerHovered;
			}
		}
	}

	CUIRect Spectators = {(Width - ScoreboardSmallWidth) / 2.0f, Scoreboard.y + Scoreboard.h + 10.0f, ScoreboardSmallWidth, 200.0f};
	if(pGameInfoObj && (pGameInfoObj->m_ScoreLimit || pGameInfoObj->m_TimeLimit || (pGameInfoObj->m_RoundNum && pGameInfoObj->m_RoundCurrent)))
	{
		CUIRect Goals;
		Spectators.HSplitTop(50.0f, &Goals, &Spectators);
		Spectators.HSplitTop(10.0f, nullptr, &Spectators);
		RenderGoals(Goals);
	}
	RenderSpectators(Spectators);

	RenderRecordingNotification((Width / 7) * 4 + 20);

	if(m_Popup.shouldHide(m_Mouse, playerHovered))
		m_Popup.toggle(false);
	if(m_Popup.m_Visible)
		RenderPlayerPopUp();
	if(m_Mouse.m_Unlocked)
		RenderTools()->RenderCursor(m_Mouse.m_Position, 24.0f * ((400.0f * 3.0f) / 600.0f));
}

bool CScoreboard::Active() const
{
	// if statboard is active don't show scoreboard
	if(GameClient()->m_Statboard.IsActive())
		return false;

	if(m_Active)
		return true;

	const CNetObj_GameInfo *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	if(GameClient()->m_Snap.m_pLocalInfo && !GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		// we are not a spectator, check if we are dead and the game isn't paused
		if(!GameClient()->m_Snap.m_pLocalCharacter && g_Config.m_ClScoreboardOnDeath &&
			!(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED))
			return true;
	}

	// if the game is over
	if(pGameInfoObj && pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
		return true;

	return false;
}

const char *CScoreboard::GetTeamName(int Team) const
{
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Team invalid");

	int ClanPlayers = 0;
	const char *pClanName = nullptr;
	for(const CNetObj_PlayerInfo *pInfo : GameClient()->m_Snap.m_apInfoByScore)
	{
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = GameClient()->m_aClients[pInfo->m_ClientId].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(GameClient()->m_aClients[pInfo->m_ClientId].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return nullptr;
		}
	}

	if(ClanPlayers > 1 && pClanName[0] != '\0')
		return pClanName;
	else
		return nullptr;
}

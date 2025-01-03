#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif
//Console_flex
MACRO_CONFIG_COL(XcLocalConsoleColor, xc_local_con_color, 51, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local console color")
MACRO_CONFIG_COL(XcRemoteConsoleColor, xc_rcon_color, 21837, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rcon color")
MACRO_CONFIG_COL(XcConsoleBarColor, xc_con_bar_color, 10902439, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Consolebar color")
MACRO_CONFIG_INT(XcLocalConsoleAlpha, xc_local_con_a, 30, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Local console alpha")
MACRO_CONFIG_INT(XcRemoteConsoleAlpha, xc_rcon_a, 30, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rcon alpha")
MACRO_CONFIG_INT(XcConsoleAnimationSpeed, xc_con_anim_speed, 1, 0, 5, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Console opening speed")
MACRO_CONFIG_INT(XcConsoleAnimation, xc_con_anim, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom speed when console is open")
MACRO_CONFIG_INT(XcCustomConIcons, xc_con_png_support, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable .png support for console")
MACRO_CONFIG_INT(XcCustomConsoleBrightness, xc_custom_con_brth, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Brightness of custom console")
MACRO_CONFIG_INT(XcCustomConsole, xc_custom_console, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable customizable console")
MACRO_CONFIG_INT(XcCustomConsoleBar, xc_custom_console_bar, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable customizable console bar")
MACRO_CONFIG_INT(XcWallpaperConsoleScaling, xc_icon_con_scale, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom pic scaling in console")
MACRO_CONFIG_INT(XcWallpaperConsoleScalingW, xc_icon_con_scale_w, 1920, 800, 4096, CFGFLAG_CLIENT | CFGFLAG_SAVE, "custom pic scaling (width")
MACRO_CONFIG_INT(XcWallpaperConsoleScalingH, xc_icon_con_scale_h, 1080, 600, 2160, CFGFLAG_CLIENT | CFGFLAG_SAVE, "custom pic scaling (height")

//BulletTrails
MACRO_CONFIG_INT(XcBulletSmokeTrail, xc_bullet_smoke_trail, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom bullet trail")
MACRO_CONFIG_INT(XcBulletSmokeTrailLifeSpan, xc_bullet_smoke_trail_lifespan, 50, 0, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Custom bullet trail: Smoke lifespan")

//Turn off snowflakes
MACRO_CONFIG_INT(XcSnowflakes, xc_snowflakes, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable snowflakes(while freezed)")

//Console laod
MACRO_CONFIG_STR(ClAssetConsole, xc_asset_console, 50, "console_bg", CFGFLAG_SAVE | CFGFLAG_CLIENT, "The asset for console")

//aniclient
// TODO: Remade

MACRO_CONFIG_INT(XcShowHookCollOwnOverride, xc_show_hook_coll_own_override, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Override server's GAMEINFOFLAG_ALLOW_HOOK_COLL flag")

MACRO_CONFIG_INT(XcApplySoloOnUnique, xc_apply_solo_unique, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Make client think everyone has solo modificator on unique servers")

MACRO_CONFIG_INT(XcCursorSizeMultiplier, xc_cursor_size_multiplier, 10, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Scale of weapon cursor (crosshair)")

MACRO_CONFIG_INT(XcHideIgnoredInAnyCondition, xc_hide_ignored_any, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hide ignored players even if they are onthe same team")

MACRO_CONFIG_COL(XcShowTeeHitboxInnerColor, xc_show_tee_hitbox_inner_color, 0xFFB3FE4D, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Own tee hitbox inner circle color");
MACRO_CONFIG_COL(XcShowTeeHitboxOuterColor, xc_show_tee_hitbox_outer_color, 0xFEFFB34D, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Own tee hitbox outer circle color");
MACRO_CONFIG_INT(XcShowTeeHitboxOwn, xc_show_tee_hitbox_own, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show hitbox of local tee")
MACRO_CONFIG_INT(XcShowTeeHitboxOther, xc_show_tee_hitbox_other, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show hitbox of other tees")

MACRO_CONFIG_COL(XcShowHammerHitboxColor, xc_show_hammer_hitbox_color, 0xFFFFFF4D, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Hammer hitbox color");
MACRO_CONFIG_INT(XcShowHammerHitboxOwn, xc_show_hammer_hitbox_own, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show hitbox of hammer for local tee")
MACRO_CONFIG_INT(XcShowHammerHitboxOther, xc_show_hammer_hitbox_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show hitbox of hammer for other tees")

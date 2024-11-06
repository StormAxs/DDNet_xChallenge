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

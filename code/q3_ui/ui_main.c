// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/


#include "ui_local.h"

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		UI_Init();
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent( arg0, arg1 );
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
//anticheat
		//UI_CheckForCheats();
//anticheat
		return UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;
	case UI_HASUNIQUECDKEY:				// mod authors need to observe this
		return qfalse;  // bk010117 - change this to qfalse for mods!
	}

	return -1;
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

vmCvar_t	ui_ffa_fraglimit;
vmCvar_t	ui_ffa_timelimit;

vmCvar_t	ui_tourney_fraglimit;
vmCvar_t	ui_tourney_timelimit;

vmCvar_t	ui_team_fraglimit;
vmCvar_t	ui_team_timelimit;
vmCvar_t	ui_team_friendly;

vmCvar_t	ui_ctf_capturelimit;
vmCvar_t	ui_ctf_timelimit;
vmCvar_t	ui_ctf_friendly;

vmCvar_t	ui_arenasFile;
vmCvar_t	ui_botsFile;
vmCvar_t	ui_spScores1;
vmCvar_t	ui_spScores2;
vmCvar_t	ui_spScores3;
vmCvar_t	ui_spScores4;
vmCvar_t	ui_spScores5;
vmCvar_t	ui_spAwards;
vmCvar_t	ui_spVideos;
vmCvar_t	ui_spSkill;

vmCvar_t	ui_spSelection;

vmCvar_t	ui_browserMaster;
vmCvar_t	ui_browserGameType;
vmCvar_t	ui_browserSortKey;
vmCvar_t	ui_browserShowFull;
vmCvar_t	ui_browserShowEmpty;

vmCvar_t	ui_brassTime;
vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;

vmCvar_t	ui_server1;
vmCvar_t	ui_server2;
vmCvar_t	ui_server3;
vmCvar_t	ui_server4;
vmCvar_t	ui_server5;
vmCvar_t	ui_server6;
vmCvar_t	ui_server7;
vmCvar_t	ui_server8;
vmCvar_t	ui_server9;
vmCvar_t	ui_server10;
vmCvar_t	ui_server11;
vmCvar_t	ui_server12;
vmCvar_t	ui_server13;
vmCvar_t	ui_server14;
vmCvar_t	ui_server15;
vmCvar_t	ui_server16;

vmCvar_t	ui_cdkeychecked;

//freeze
vmCvar_t	ui_iceChips;
vmCvar_t	ui_weather;
vmCvar_t	ui_chatFilter;
//freeze
//unlagged
vmCvar_t	ui_delag;
//unlagged

// bk001129 - made static to avoid aliasing.
static cvarTable_t		cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },

	{ &ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM },

//freeze
	{ &ui_iceChips, "cg_iceChips", "1", CVAR_ARCHIVE },
	{ &ui_weather, "cg_weather", "1", CVAR_ARCHIVE },
	{ &ui_chatFilter, "cg_chatFilter", "0", CVAR_ARCHIVE },
//freeze
//unlagged
	{ &ui_delag, "cg_delag", "1", CVAR_ARCHIVE },
//unlagged
};

// bk001129 - made static to avoid aliasing
static int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
static char *ui_keyList[] = {
	"0x00",
	"0x01",
	"0x02",
	"0x03",
	"0x04",
	"0x05",
	"0x06",
	"0x07",
	"0x08",
	"TAB",
	"0x0a",
	"0x0b",
	"0x0c",
	"ENTER",
	"0x0e",
	"0x0f",
	"0x10",
	"0x11",
	"0x12",
	"0x13",
	"0x14",
	"0x15",
	"0x16",
	"0x17",
	"0x18",
	"0x19",
	"0x1a",
	"ESCAPE",
	"0x1c",
	"0x1d",
	"0x1e",
	"0x1f",
	"SPACE",
	"!",
	"0x22",
	"#",
	"$",
	"%",
	"&",
	"'",
	"(",
	")",
	"*",
	"+",
	",",
	"-",
	".",
	"/",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	":",
	"SEMICOLON",
	"<",
	"=",
	">",
	"?",
	"@",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"[",
	"\"",
	"]",
	"_",
	"`",
	"0x60",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"{",
	"|",
	"}",
	"~",
	"BACKSPACE",
	"COMMAND",
	"CAPSLOCK",
	"0x82",
	"PAUSE",
	"UPARROW",
	"DOWNARROW",
	"LEFTARROW",
	"RIGHTARROW",
	"ALT",
	"CTRL",
	"SHIFT",
	"INS",
	"DEL",
	"PGDN",
	"PGUP",
	"HOME",
	"END",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"0x9d",
	"0x9e",
	"0x9f",
	"KP_HOME",
	"KP_UPARROW",
	"KP_PGUP",
	"KP_LEFTARROW",
	"KP_5",
	"KP_RIGHTARROW",
	"KP_END",
	"KP_DOWNARROW",
	"KP_PGDN",
	"KP_ENTER",
	"KP_INS",
	"KP_DEL",
	"KP_SLASH",
	"KP_MINUS",
	"KP_PLUS",
	"KP_NUMLOCK",
	"KP_STAR",
	"KP_EQUALS",
	"MOUSE1",
	"MOUSE2",
	"MOUSE3",
	"MOUSE4",
	"MOUSE5",
	"MWHEELDOWN",
	"MWHEELUP",
	"JOY1",
	"JOY2",
	"JOY3",
	"JOY4",
	"JOY5",
	"JOY6",
	"JOY7",
	"JOY8",
	"JOY9",
	"JOY10",
	"JOY11",
	"JOY12",
	"JOY13",
	"JOY14",
	"JOY15",
	"JOY16",
	"JOY17",
	"JOY18",
	"JOY19",
	"JOY20",
	"JOY21",
	"JOY22",
	"JOY23",
	"JOY24",
	"JOY25",
	"JOY26",
	"JOY27",
	"JOY28",
	"JOY29",
	"JOY30",
	"JOY31",
	"JOY32",
	"AUX1",
	"AUX2",
	"AUX3",
	"AUX4",
	"AUX5",
	"AUX6",
	"AUX7",
	"AUX8",
	"AUX9",
	"AUX10",
	"AUX11",
	"AUX12",
	"AUX13",
	"AUX14",
	"AUX15",
	"AUX16",
	"0xe9",
	"0xea",
	"0xeb",
	"0xec",
	"0xed",
	"0xee",
	"0xef",
	"0xf0",
	"0xf1",
	"0xf2",
	"0xf3",
	"0xf4",
	"0xf5",
	"0xf6",
	"0xf7",
	"0xf8",
	"0xf9",
	"0xfa",
	"0xfb",
	"0xfc",
	"0xfd",
	"0xfe",
	"0xff",
};

static char *ui_knownButtons[] = {
	"+attack", 
	"+back", 
	"+button2", 
	"+button3", 
	"+button5", 
	"+forward", 
	"+info", 
	"+left", 
	"+lookdown", 
	"+lookup", 
	"+mlook", 
	"+movedown", 
	"+moveleft", 
	"+moveright", 
	"+moveup", 
	"+right", 
	"+scores", 
	"+speed", 
	"+strafe", 
	"+zoom", 
	"-attack", 
	"-back", 
	"-button2", 
	"-button3", 
	"-button5", 
	"-forward", 
	"-info", 
	"-left", 
	"-lookdown", 
	"-lookup", 
	"-mlook", 
	"-movedown", 
	"-moveleft", 
	"-moveright", 
	"-moveup", 
	"-right", 
	"-scores", 
	"-speed", 
	"-strafe", 
	"-zoom", 
};
static const int ui_nKnownButtons = sizeof(ui_knownButtons) / sizeof(char *);

static char *ui_exemptCmds[] = {
	"devmap", 
	"map", 
	"music", 
	"play", 
	"record", 
	"say", 
	"say_team", 
	"spdevmap", 
	"spmap", 
	"tell", 
	"tell_attacker", 
	"tell_target", 
	"vosay", 
	"vosay_team", 
	"votell", 
	"vsay", 
	"vsay_team", 
	"vtaunt", 
	"vtell", 
	"vtell_attacker", 
	"vtell_target", 
};
static const int ui_nExemptCmds = sizeof(ui_exemptCmds) / sizeof(char *);

static char *ui_cheatCmds[] = {
	"ogc_shoot",
	"ogc_aim", 
	"ogc_ignorewalls", 
	"ogc_pingpredict", 
	"ogc_glow", 
	"ogc_mode", 
	"ogc_fov", 
	"ogc_names", 
	"ogc_weapons", 
};
static const int ui_nCheatCmds = sizeof(ui_cheatCmds) / sizeof(char *);

static void UI_CheatError( char *key, char *arg1, char *arg2 ) {
	trap_Cvar_Set( "cheatkey", key );
	trap_Cvar_Set( "cheatarg1", arg1 );
	trap_Cvar_Set( "cheatarg2", arg2 );
}

// forward declaration for recursive stuff
static qboolean UI_CheckBinding( char *key, char *bind, int depth );

#define FILESIZE 65536
#define NUMFILESDEEP 8

static char g_bytes[NUMFILESDEEP][FILESIZE];

static qboolean UI_CheckExec( char *key, char *filename, int depth ) {
	fileHandle_t f;
	int len;
	char *curr, *line;

	if ( depth >= NUMFILESDEEP ) {
		Com_Printf("Cannot execute more than %d files deep.\n", NUMFILESDEEP );
		UI_CheatError( key, "exec", filename );
		return qtrue;
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( f == 0 ) {
		return qfalse;
	}

	if ( len == 0 ) {
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	if ( len > FILESIZE ) {
		trap_FS_FCloseFile( f );
		Com_Printf("Cannot exec a file larger than %d bytes from a bind.\n", FILESIZE );
		UI_CheatError( key, "exec", filename );
		return qtrue;
	}

	trap_FS_Read( g_bytes[depth], len, f );
	trap_FS_FCloseFile( f );

	curr = line = g_bytes[depth];
	while ( curr < &g_bytes[depth][len] ) {
		// did we find a linefeed?
		if ( *curr == '\n' ) {
			// yeah, so terminate the "line" string and check it out
			*curr = '\0';
			if ( UI_CheckBinding( key, line, depth + 1 ) ) {
				return qtrue;				
			}

			// advance until we get past the whitespace
			while ( *curr <= ' ' && curr < &g_bytes[depth][len] ) {
				curr++;
			}

			// this should be the start of the next line
			line = curr;
		}
		else {
			// no line termination, proceed normally
			curr++;
		}
	}

	// check the last line
	if ( line[0] != '\0' ) {
		return UI_CheckBinding( key, line, depth + 1 );
	}

	return qfalse;
}

static qboolean UI_ParseCommand( char *key, char *cmd, int depth ) {
	int i, j;
	char *end;

	// get past whitespace
	while ( *cmd <= ' ' ) {
		cmd++;
	}

	// find the end of the string
	end = &cmd[strlen(cmd)];

	// go backwards
	while ( *end <= ' ' && end > cmd ) {
		*end = '\0';
		end--;
	}

	// do we have nothing left after trimming?
	if ( end == cmd ) {
		return qfalse;
	}

//	Com_Printf("Command: %s\n", cmd );

	// is it a button?
	if ( cmd[0] == '+' || cmd[0] == '-' ) {
		for ( i = 0; i < ui_nKnownButtons; i++ ) {
			// is it a known button?
			if ( Q_stricmpn( cmd, ui_knownButtons[i], strlen(ui_knownButtons[i]) ) == 0 ) {
				// yep, we know it!
				return qfalse;
			}
		}

		// nope, it's unknown
		UI_CheatError( key, "using", va("\"%s\"", cmd) );
		return qtrue;
	}

	// some commands are exempt from cheat-checking, like "say"
	for ( i = 0; i < ui_nExemptCmds; i++ ) {
		if ( Q_stricmp( cmd, ui_exemptCmds[i] ) == 0 ) {
			// it's exempt
			return qfalse;
		}
	}

	// exec commands need have files opened and checked
	if ( Q_stricmpn( cmd, "exec", 4 ) == 0 && cmd[4] <= ' ' ) {
		cmd += 5;

		// get past whitespace
		while ( *cmd <= ' ' ) {
			cmd++;
		}

		return UI_CheckExec( key, cmd, depth );
	}

	// vstr commands need to be dealt with recursively
	if ( Q_stricmpn( cmd, "vstr", 4 ) == 0 && cmd[4] <= ' ' ) {
		char val[MAX_STRING_CHARS];

		cmd += 5;

		// get past whitespace
		while ( *cmd <= ' ' ) {
			cmd++;
		}

		trap_Cvar_VariableStringBuffer( cmd, val, sizeof(val) );

		return UI_CheckBinding( key, val, depth );
	}

	// try to find a known cheat command inside the remaining stuff
	for ( i = 0; i < ui_nCheatCmds; i++ ) {
		char *cheat = ui_cheatCmds[i];
		int len = strlen(cheat);

		for ( j = 0; cmd[j] != '\0'; j++ ) {
			if ( Q_stricmpn( &cmd[j], cheat, len ) == 0 ) {
				UI_CheatError( key, "tried", va("\"%s\"", cmd) );
				return qtrue;
			}
		}
	}

	// didn't find any cheats in it
	return qfalse;
}

static qboolean UI_CheckBinding( char *key, char *bind, int depth ) {
	int i;
	qboolean quote = qfalse;
	char *lastCmd = bind;

	// parse the binding
	for ( i = 0; bind[i] != '\0'; i++ ) {
		// check for double quotes (which will override a semicolon command separator)
		if ( bind[i] == '"' ) {
			quote = !quote;
			continue;
		}

		// separated command?
		if ( !quote && bind[i] == ';' ) {
			bind[i] = '\0';
			if ( lastCmd[0] != '\0' ) {
				if ( UI_ParseCommand( key, lastCmd, depth ) ) {
					return qtrue;
				}
			}
			lastCmd = &bind[i + 1];
		}
	}

	// catch the last one
	if ( lastCmd[0] != '\0' ) {
		if ( UI_ParseCommand( key, lastCmd, depth ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

void UI_CheckForCheats( void ) {
	int key;

	// check for keypresses
	for ( key = 0; key <= 255; key++ ) {
		// if the key is down
		if ( trap_Key_IsDown( key ) ) {
			char bind[MAX_STRING_CHARS];

			// grab the binding
			trap_Key_GetBindingBuf( key, bind, sizeof(bind) );

			if ( UI_CheckBinding( ui_keyList[key], bind, 0 ) ) {
				return;
			}
		}
	}
}
*/

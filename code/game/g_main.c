// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

level_locals_t	level;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
  qboolean teamShader;        // track and if changed, update shader state
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

vmCvar_t	g_gametype;
vmCvar_t	g_dmflags;
vmCvar_t	g_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_log;
vmCvar_t	g_logSync;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	g_rankings;
vmCvar_t	g_listEntity;
#ifdef MISSIONPACK
vmCvar_t	g_obeliskHealth;
vmCvar_t	g_obeliskRegenPeriod;
vmCvar_t	g_obeliskRegenAmount;
vmCvar_t	g_obeliskRespawnDelay;
vmCvar_t	g_cubeTimeout;
vmCvar_t	g_redteam;
vmCvar_t	g_blueteam;
vmCvar_t	g_singlePlayer;
vmCvar_t	g_enableDust;
vmCvar_t	g_enableBreath;
vmCvar_t	g_proxMineTimeout;
//freeze
#else
vmCvar_t	g_enableBreath;
//freeze
#endif
//freeze
vmCvar_t	g_grapple;
vmCvar_t	g_grappleSpeed;
vmCvar_t	g_grapplePullSpeed;
vmCvar_t	g_wpflags;
vmCvar_t	g_weaponlimit;
vmCvar_t	g_doReady;
vmCvar_t	g_startArmor;
vmCvar_t	g_reloadFactor;
vmCvar_t	g_noVoiceChatSounds;
vmCvar_t	g_railJumpKnock;
vmCvar_t	g_pogoJumps;
vmCvar_t	g_thawTime;
vmCvar_t	g_autoThawTime;
vmCvar_t	g_friendlyAutoThawTime;
vmCvar_t	g_noFreezeTime;
vmCvar_t	g_noDamageTime;
vmCvar_t	g_noAttackTime;
vmCvar_t	g_doSuddenDeath;
vmCvar_t	g_doTimeouts;
vmCvar_t	g_doTeamLocks;
vmCvar_t	g_maxTimeout;
vmCvar_t	g_numTimeouts;
vmCvar_t	g_readyPercent;
vmCvar_t	g_refPassword;
vmCvar_t	g_refAutologin;
vmCvar_t	g_allowThirdPerson;
vmCvar_t	g_forceNoFloat;
vmCvar_t	g_allowFollowEnemy;
vmCvar_t	g_hardcore;
vmCvar_t	g_llamaPenalty;
vmCvar_t	g_startFrozen;
vmCvar_t	g_ufreezeVersion;
vmCvar_t	sv_floodProtect;
vmCvar_t	g_floodProtect;
vmCvar_t	sv_cheats;
vmCvar_t	g_huntMode;
vmCvar_t	g_huntTime;
vmCvar_t	vote_capturelimit;
vmCvar_t	vote_timelimit;
vmCvar_t	vote_warmup;
vmCvar_t	vote_friendlyfire;
vmCvar_t	vote_gametype;
vmCvar_t	vote_grapple;
vmCvar_t	vote_lagcomp;
vmCvar_t	vote_kick;
vmCvar_t	vote_remove;
vmCvar_t	vote_putblue;
vmCvar_t	vote_putred;
vmCvar_t	vote_map;
vmCvar_t	vote_restart;
vmCvar_t	vote_nextmap;
vmCvar_t	vote_railjump;
vmCvar_t	vote_opinion;
vmCvar_t	vote_config;
vmCvar_t	vote_huntmode;
vmCvar_t	ammo_mg;
vmCvar_t	ammo_sg;
vmCvar_t	ammo_gl;
vmCvar_t	ammo_rl;
vmCvar_t	ammo_lg;
vmCvar_t	ammo_rg;
vmCvar_t	ammo_pg;
vmCvar_t	ammo_bfg;
//freeze
//unlagged - server options
vmCvar_t	g_delagHitscan;
vmCvar_t	g_unlaggedVersion;
vmCvar_t	g_truePing;
vmCvar_t	g_lightningDamage;
vmCvar_t	sv_fps;
//unlagged - server options
//anticheat
vmCvar_t	sv_mapname;
//anticheat
//ssdemo
vmCvar_t	ss_continueRecordName;
vmCvar_t	ss_continueRecordSize;
vmCvar_t	ss_continueRecordTime;
vmCvar_t	ss_continueDemoName;
vmCvar_t	ss_continueDemoLoc;
vmCvar_t	ss_demoLimit;
vmCvar_t	ss_mapRestartThresh;
vmCvar_t	g_timescale;
//ssdemo

// bk001129 - made static to avoid aliasing
static cvarTable_t		gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ NULL, "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
/*freeze
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
freeze*/
	{ &g_gametype, "g_gametype", "3", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse },
//freeze

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", 0, 0, qtrue  },
	{ &g_log, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "320", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "5", 0, 0, qtrue },
	{ &g_forcerespawn, "g_forcerespawn", "20", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

#ifdef MISSIONPACK
	{ &g_obeliskHealth, "g_obeliskHealth", "2500", 0, 0, qfalse },
	{ &g_obeliskRegenPeriod, "g_obeliskRegenPeriod", "1", 0, 0, qfalse },
	{ &g_obeliskRegenAmount, "g_obeliskRegenAmount", "15", 0, 0, qfalse },
	{ &g_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO, 0, qfalse },

	{ &g_cubeTimeout, "g_cubeTimeout", "30", 0, 0, qfalse },
	{ &g_redteam, "g_redteam", "Stroggs", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue },
	{ &g_blueteam, "g_blueteam", "Pagans", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue  },
	{ &g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse, qfalse  },

	{ &g_enableDust, "g_enableDust", "0", CVAR_SERVERINFO, 0, qtrue, qfalse },
	{ &g_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO, 0, qtrue, qfalse },
	{ &g_proxMineTimeout, "g_proxMineTimeout", "20000", 0, 0, qfalse },
//freeze
#else
	{ &g_enableBreath, "g_enableBreath", "1", CVAR_SERVERINFO, 0, qtrue, qfalse },
//freeze
#endif
//freeze
	{ &g_grapple, "g_grapple", "0", CVAR_SERVERINFO, 0, qfalse },
	{ &g_grappleSpeed, "g_grappleSpeed", "1800", CVAR_SERVERINFO, 0, qfalse },
	{ &g_grapplePullSpeed, "g_grapplePullSpeed", "1200", CVAR_SERVERINFO, 0, qfalse },
	{ &g_wpflags, "wpflags", "0", 0, 0, qfalse },
	{ &g_weaponlimit, "weaponlimit", "0", 0, 0, qfalse },
	{ &g_doReady, "g_doReady", "0", CVAR_SERVERINFO, 0, qfalse },
	{ &g_startArmor, "g_startArmor", "0", 0, 0, qfalse },
	{ &g_reloadFactor, "g_reloadFactor", "1.0", CVAR_SERVERINFO, 0, qtrue },
	{ &g_noVoiceChatSounds, "g_noVoiceChatSounds", "0", CVAR_SERVERINFO, 0, qtrue },
	{ &g_railJumpKnock, "g_railJumpKnock", "0", CVAR_SERVERINFO, 0, qtrue },
	{ &g_pogoJumps, "g_pogoJumps", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue },
	{ &g_thawTime, "g_thawTime", "3.0", CVAR_SERVERINFO, 0, qtrue },
	{ &g_autoThawTime, "g_autoThawTime", "120", CVAR_SERVERINFO, 0, qtrue },
	{ &g_friendlyAutoThawTime, "g_friendlyAutoThawTime", "120", 0, 0, qtrue },
	{ &g_noFreezeTime, "g_noFreezeTime", "2.0", 0, 0, qtrue },
	{ &g_noDamageTime, "g_noDamageTime", "0.0", 0, 0, qtrue },
	{ &g_noAttackTime, "g_noAttackTime", "0.0", 0, 0, qtrue },
	{ &g_doSuddenDeath, "g_doSuddenDeath", "1", 0, 0, qtrue  },
	{ &g_doTimeouts, "g_doTimeouts", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_doTeamLocks, "g_doTeamLocks", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_maxTimeout, "g_maxTimeout", "120", 0, 0, qfalse },
	{ &g_numTimeouts, "g_numTimeouts", "3", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_readyPercent, "g_readyPercent", "50.0", 0, 0, qfalse },
	{ &g_refPassword, "ref_password", "none", 0, 0, qfalse },
	{ &g_refAutologin, "g_refAutologin", "0", 0, 0, qfalse },
	{ &g_allowThirdPerson, "g_allowThirdPerson", "1", CVAR_SERVERINFO, 0, qtrue },
	{ &g_forceNoFloat, "g_forceNoFloat", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_allowFollowEnemy, "g_allowFollowEnemy", "1", 0, 0, qfalse },
	{ &g_hardcore, "g_hardcore", "0", CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_llamaPenalty, "g_llamaPenalty", "3.0", 0, 0, qtrue },
	{ &g_startFrozen, "g_startFrozen", "0", 0, 0, qfalse },
	{ &g_ufreezeVersion, "g_ufreezeVersion", "1.2", CVAR_ROM | CVAR_SERVERINFO, 0, qfalse },
	{ &sv_floodProtect, "sv_floodProtect", "0", 0, 0, qfalse },
	{ &g_floodProtect, "g_floodProtect", "1", 0, 0, qfalse },
	{ &sv_cheats, "sv_cheats", "0", 0, 0, qfalse },
	{ &g_huntMode, "g_huntMode", "0", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_huntTime, "g_huntTime", "60", 0, 0, qfalse },
	{ &vote_capturelimit, "vote_capturelimit", "1", 0, 0, qfalse },
	{ &vote_timelimit, "vote_timelimit", "1", 0, 0, qfalse },
	{ &vote_warmup, "vote_warmup", "1", 0, 0, qfalse },
	{ &vote_friendlyfire, "vote_friendlyfire", "1", 0, 0, qfalse },
	{ &vote_gametype, "vote_gametype", "1", 0, 0, qfalse },
	{ &vote_grapple, "vote_grapple", "1", 0, 0, qfalse },
	{ &vote_lagcomp, "vote_lagcomp", "1", 0, 0, qfalse },
	{ &vote_kick, "vote_kick", "1", 0, 0, qfalse },
	{ &vote_remove, "vote_remove", "1", 0, 0, qfalse },
	{ &vote_putblue, "vote_putblue", "1", 0, 0, qfalse },
	{ &vote_putred, "vote_putred", "1", 0, 0, qfalse },
	{ &vote_map, "vote_map", "1", 0, 0, qfalse },
	{ &vote_restart, "vote_restart", "1", 0, 0, qfalse },
	{ &vote_nextmap, "vote_nextmap", "1", 0, 0, qfalse },
	{ &vote_railjump, "vote_railjump", "1", 0, 0, qfalse },
	{ &vote_opinion, "vote_opinion", "1", 0, 0, qfalse },
	{ &vote_config, "vote_config", "1", 0, 0, qfalse },
	{ &vote_huntmode, "vote_huntmode", "0", 0, 0, qfalse },
	{ &ammo_mg, "ammo_mg", "100", 0, 0, qfalse },
	{ &ammo_sg, "ammo_sg", "10", 0, 0, qfalse },
	{ &ammo_gl, "ammo_gl", "5", 0, 0, qfalse },
	{ &ammo_rl, "ammo_rl", "5", 0, 0, qfalse },
	{ &ammo_lg, "ammo_lg", "60", 0, 0, qfalse },
	{ &ammo_rg, "ammo_rg", "10", 0, 0, qfalse },
	{ &ammo_pg, "ammo_pg", "30", 0, 0, qfalse },
	{ &ammo_bfg, "ammo_bfg", "15", 0, 0, qfalse },
	{ NULL, "sv_pure", "1", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse },
//freeze
//unlagged - server options
	{ &g_delagHitscan, "g_delagHitscan", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue },
	{ &g_unlaggedVersion, "g_unlaggedVersion", "2.0", CVAR_ROM | CVAR_SERVERINFO, 0, qfalse },
	{ &g_truePing, "g_truePing", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_lightningDamage, "g_lightningDamage", "8", 0, 0, qtrue },
	// it's CVAR_SYSTEMINFO so the client's sv_fps will be automagically set to its value
	{ &sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse },
//unlagged - server options
//anticheat
	{ &sv_mapname, "mapname", "nomap", 0, 0, qfalse },
//anticheat
	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},
//ssdemo
	{ &ss_continueRecordName, "##_continueRecordName", "", CVAR_LATCH, 0, qfalse },
	{ &ss_continueRecordSize, "##_continueRecordSize", "", CVAR_LATCH, 0, qfalse },
	{ &ss_continueRecordTime, "##_continueRecordTime", "", CVAR_LATCH, 0, qfalse },
	{ &ss_continueDemoName, "##_continueDemoName", "", CVAR_LATCH, 0, qfalse },
	{ &ss_continueDemoLoc, "##_continueDemoLoc", "0", CVAR_LATCH, 0, qfalse },
	{ &ss_demoLimit, "ss_demoLimit", "100", CVAR_ARCHIVE, 0, qfalse },
	{ &ss_mapRestartThresh, "ss_mapRestartThresh", "20", CVAR_ARCHIVE, 0, qfalse },
	{ &g_timescale, "timescale", "1", 0, 0, qfalse },
//ssdemo

	{ &g_rankings, "g_rankings", "0", 0, 0, qfalse}

};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (int)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
//ssdemo
		// play demo data instead of running a frame
		if ( level.inDemoFile != 0 ) {
			G_RunDemoLoop( arg0 );
			return 0;
		}
//ssdemo
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
//ssdemo
		// don't let bots think if playing a server-side demo
		if ( level.inDemoFile != 0 ) {
			return 0;
		}
//ssdemo
		return BotAIStartFrame( arg0 );
	}

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders() {
#ifdef MISSIONPACK
	char string[1024];
	float f = level.time * 0.001;
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f); 
	AddRemap("textures/ctf2/redteam02", string, f); 
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f); 
	AddRemap("textures/ctf2/blueteam02", string, f); 
	G_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig()); //ssdemo
#endif
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
	}

	level.warmupModificationCount = g_warmup.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					G_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", 
						cv->cvarName, cv->vmCvar->string ) ); //ssdemo
				}

				if (cv->teamShader) {
					remapped = qtrue;
				}
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;
	char				map[MAX_QPATH];
	char				serverinfo[MAX_INFO_STRING];
//freeze
	int ufreezeFlags, len;
	fileHandle_t f;
	char buffer[MAX_STRING_CHARS];
//freeze

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);
	G_Printf ("levelTime: %d\n", levelTime);

//freeze
	//exec per-map cfg's - if they don't exist, this will fail
	//we'll get a nice little 'cant exec cfg' which is what's supposed to happen
	
	trap_GetServerinfo( serverinfo, sizeof(serverinfo) );
	Q_strncpyz( map, Info_ValueForKey( serverinfo, "mapname" ), sizeof(map) );

	buffer[0] = '\0';	
	Com_sprintf(buffer,MAX_STRING_CHARS,"exec map-cfgs\\%s.cfg;\n","map-default");
	trap_SendConsoleCommand(EXEC_APPEND,buffer);

	buffer[0] = '\0';	
	Com_sprintf(buffer,MAX_STRING_CHARS,"exec map-cfgs\\%s.cfg;\n",map);
	trap_SendConsoleCommand(EXEC_APPEND,buffer);
//freeze

	srand( randomSeed );

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

//freeze
	level.roundStartTime = levelTime;

	level.lastHuntedTeam = -1;

	// open our game state file
	len = trap_FS_FOpenFile( "ufreeze.gamestate", &f, FS_READ );

	// init the buffer
	buffer[0] = '\0';

	// if it's long enough and short enough, read it in
	if ( len && len <= MAX_STRING_CHARS ) {
		trap_FS_Read( buffer, len, f );
		// close off the buffer
		buffer[len] = '\0';
	}

	// close it
	trap_FS_FCloseFile( f );

	// zero it out (wish we could delete it)
	trap_FS_FOpenFile( "ufreeze.gamestate", &f, FS_WRITE );
	trap_FS_FCloseFile( f );

	// if we've got data
	if ( *buffer ) {
		ufreezeFlags = atoi( buffer );

		// lock teams based on flags
		if ( ufreezeFlags & 1 ) {
			level.redlocked = qtrue;
		}
		if ( ufreezeFlags & 2 ) {
			level.bluelocked = qtrue;
		}
	}

	if ( g_autoThawTime.integer <= 60 ) {
		float rnd = random();
		level.rain = rnd * 120 > g_autoThawTime.integer;
	}
	else {
		float rnd = random();
		level.rain = rnd * 150 - 15 > g_autoThawTime.integer;
	}

	BG_WriteDescriptionTxt();
//freeze

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if ( g_gametype.integer != GT_SINGLE_PLAYER && g_log.string[0] ) {
		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( g_gametype.integer >= GT_TEAM ) {
		G_CheckTeamItems();
	}

	SaveRegisteredItems();

	G_Printf ("-----------------------------------\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

//ssdemo
	// continue recording a demo if one was recording
	G_ContinueRecord();

	// continue running a demo if one was running
	G_ContinueDemo();
//ssdemo
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	G_Printf ("==== ShutdownGame ====\n");

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

//ssdemo
	// shut down any recording or playing demos cleanly
	G_ShutdownDemosClean();
//ssdemo
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	G_Printf ("%s", text);
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ||  client->sess.sessionTeam != TEAM_REDCOACH ||  client->sess.sessionTeam != TEAM_BLUECOACH ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f", qfalse );
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s", qfalse );
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s", qfalse );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	clientNum = level.sortedClients[0];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged( clientNum );
	}

	clientNum = level.sortedClients[1];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged( clientNum );
	}

}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorTime < cb->sess.spectatorTime ) {
			return -1;
		}
		if ( ca->sess.spectatorTime > cb->sess.spectatorTime ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gclient_t	*cl;

//ssdemo
	int		max;
	if ( level.inDemoFile != 0 ) {
		max = MAX_CLIENTS;
	}
	else {
		max = level.maxclients;
	}
//ssdemo

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots
	for ( i = TEAM_FREE; i < TEAM_NUM_TEAMS; i++ ) {
		level.numteamVotingClients[i] = 0;
	}
//ssdemo
	//for ( i = 0 ; i < level.maxclients ; i++ ) {
	for ( i = 0 ; i < max ; i++ ) {
//ssdemo
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR && level.clients[i].sess.sessionTeam != TEAM_REDCOACH && level.clients[i].sess.sessionTeam != TEAM_BLUECOACH ) {
				level.numNonSpectatorClients++;
			
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {	
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) ); //ssdemo
		G_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) ); //ssdemo
	} else {
		if ( level.numConnectedClients == 0 ) {
			G_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) ); //ssdemo
			G_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) ); //ssdemo
		} else if ( level.numConnectedClients == 1 ) {
			G_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) ); //ssdemo
			G_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) ); //ssdemo
		} else {
			G_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) ); //ssdemo
			G_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) ); //ssdemo
		}
	}

	// see if it is time to end the level
/*freeze
	CheckExitRules();
freeze*/

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ||
			ent->client->sess.spectatorState == SPECTATOR_FROZEN ) {
		StopFollowing( ent );
	}


	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( NULL, vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		trap_Cvar_Set("ui_singlePlayerActive", "0");
		UpdateTournamentInfo();
	}
#else
	// if single player game
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
	}
#endif

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			respawn(client);
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_TOURNAMENT  ) {
		if ( !level.restarted ) {
			RemoveTournamentLoser();
//freeze
			SaveGameState();
//freeze
			trap_SendConsoleCommand( EXEC_APPEND, va("map_restart 0\n") );
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;	
	}

//freeze
	SaveGameState();
//freeze
	trap_SendConsoleCommand( EXEC_APPEND, va("vstr nextmap\n") );
	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof(string), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	vsprintf( string +7 , fmt,argptr );
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
#ifdef MISSIONPACK // bk001205
	qboolean won = qtrue;
#endif
	G_LogPrintf( "Exit: %s\n", string );

//freeze
	G_SendAllClientStats();
//freeze

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	G_SetConfigstring( CS_INTERMISSION, "1" ); //ssdemo

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR || cl->sess.sessionTeam == TEAM_REDCOACH || cl->sess.sessionTeam == TEAM_BLUECOACH ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],	cl->pers.netname );
#ifdef MISSIONPACK
		if (g_singlePlayer.integer && g_gametype.integer == GT_TOURNAMENT) {
			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
				won = qfalse;
			}
		}
#endif

	}

#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		if (g_gametype.integer >= GT_CTF) {
			won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
		}
		trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
#endif


}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
//freeze
	int			readyMask[4];
//freeze

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask[0] = readyMask[1] = readyMask[2] = readyMask[3] = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) {
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			readyMask[i / 16] |= 1 << (i % 16);
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
//freeze
		cl->ps.stats[STAT_CLIENTS_READY0] = readyMask[0];
		cl->ps.stats[STAT_CLIENTS_READY1] = readyMask[1];
		cl->ps.stats[STAT_CLIENTS_READY2] = readyMask[2];
		cl->ps.stats[STAT_CLIENTS_READY3] = readyMask[3];
	}

	//send the 'level is about to change' so everyone knows to do an autoscreenshot
	//and stop autorecording, and whever else goes in
	if((level.time > level.intermissiontime + 4000) && level.bDoneMsgSent == qfalse){
		//client -1 is by definition every connected client
		trap_SendServerCommand( -1, "ROUNDDONE" );
		level.bDoneMsgSent = qtrue;
	}
//freeze


	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// if nobody wants to go, clear timer
/*freeze
	if ( !ready ) {
freeze*/
	if ( !ready && notReady ) {
//freeze
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}
	
	if ( g_gametype.integer >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if ( level.intermissionQueued ) {
#ifdef MISSIONPACK
		int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#else
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#endif
		return;
	}

//freeze
	CheckDelay();
//freeze
	// check for sudden death
	if ( ScoreIsTied() && g_doSuddenDeath.integer ) {
		// always wait for sudden death
		return;
	}

	if ( g_timelimit.integer && !level.warmupTime ) {
		if ( level.time - level.startTime - level.totaltimeout >= g_timelimit.integer*60000 ) {
			G_SendServerCommand( -1, "print \"Timelimit hit.\n\""); //ssdemo
			LogExit( "Timelimit hit." );
			return;
		}
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( g_gametype.integer < GT_TEAM && g_fraglimit.integer ) {
		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Fraglimit hit." );
				G_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"", //ssdemo
					cl->pers.netname ) );
				return;
			}
		}
	}

	if ( g_gametype.integer >= GT_TEAM && g_capturelimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			G_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" ); //ssdemo
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			G_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" ); //ssdemo
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT ) {

		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 ) {
			AddTournamentPlayer();
		}

		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
				G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );

			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	} else if ( g_gametype.integer != GT_SINGLE_PLAYER && level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( g_gametype.integer > GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

//freeze
		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}
		else {
			notEnough = readyCheck();
//freeze
			if ( notEnough ) {
				if ( level.warmupTime != -2 ) {
					level.warmupTime = -2;
					G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
					G_LogPrintf( "Warmup:\n" );
				}
				return; // still waiting for team members
			}
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
//freeze
			SaveGameState();
//freeze
			trap_SendConsoleCommand( EXEC_APPEND, va("map_restart 0\n") );
			level.restarted = qtrue;
			return;
		}
	}
}


/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
//freeze
		SaveGameState();
//freeze
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString) );
	}
	if ( !level.voteTime ) {
		return;
	}
//freeze
	// force votes to take a minimum amount of time
	if ( level.time - level.voteTime < VOTE_MIN_TIME ) {
		return;
	}
	else if ( level.time < level.voteTime ) {
		return;
//freeze
	}
	else if ( level.time - level.voteTime >= VOTE_TIME ) {
		G_SendServerCommand( -1, "print \"Vote failed.\n\"" ); //ssdemo
	}
	else {
		if ( level.voteYes > 0 && level.voteYes > level.numVotingClients / 2 ) {
			// execute the command, then remove the vote
			G_SendServerCommand( -1, "print \"Vote passed.\n\"" ); //ssdemo
			level.voteExecuteTime = level.time + 3000;
		}
		else if ( level.voteNo > 0 && level.voteNo >= level.numVotingClients / 2 ) {
			// same behavior as a timeout
			G_SendServerCommand( -1, "print \"Vote failed.\n\"" ); //ssdemo
		}
		else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	G_SetConfigstring( CS_VOTE_TIME, "" ); //ssdemo
}

/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s ^7is the new team leader\n\"", level.clients[client].pers.netname) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		G_SendServerCommand( -1, "print \"Team vote failed.\n\"" ); //ssdemo
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			G_SendServerCommand( -1, "print \"Team vote passed.\n\"" ); //ssdemo
			//
			if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				//set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			G_SendServerCommand( -1, "print \"Team vote failed.\n\"" ); //ssdemo
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	G_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" ); //ssdemo
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}
	
	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;
int start, end;
//freeze
	gentity_t *iter;
	int red, blue, refs;
//freeze

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();

//anticheat
/*
	// perform initial unfixups on the players :)
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse && ent->client ) {
			G_UnhashWeapon( ent );
		}
	}
*/
//anticheat

//freeze
	// unhash all the bodies' hashed stuff
	// clients don't need this, because the data we hash is always copied from more
	// accurate sources at the end of each frame
	for ( ent = &g_entities[MAX_CLIENTS]; ent < &g_entities[MAX_GENTITIES]; ent++ ) {
		if ( !ent->inuse || ent->s.eType != ET_PLAYER ) continue;

		if ( ent->s.number >= MAX_CLIENTS ) {
			if ( is_body_freeze(ent) ) {
				ent->s.eFlags &= ~EF_DEAD;
			}
			else {
				ent->s.eFlags |= EF_DEAD;
			}
		}

		if ( ent->s.eFlags & EF_HASHED ) {
			BG_HashOrigin( ent->s.pos.trBase, sv_mapname.string, ent->s.clientNum, -1 );
			ent->s.eFlags &= ~EF_HASHED;
		}
	}

	// give the teams 30 seconds to get in before resetting the team locks
	red = blue = 0;
	if ( level.time - level.startTime > 30000 ) {
		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			if ( !iter->inuse || !iter->client || ( iter->r.svFlags & SVF_BOT ) ||
					iter->client->pers.connected == CON_DISCONNECTED ) {
				continue;
			}

			if ( iter->client->sess.sessionTeam == TEAM_RED ) {
				red++;
			}
			else if ( iter->client->sess.sessionTeam == TEAM_BLUE ) {
				blue++;
			}
		}

		if ( red == 0 && level.redlocked ) {
			level.redlocked = qfalse;
			G_SendServerCommand( -1, va("print \"Red team unlocked\n\"")); //ssdemo
		}

		if ( blue == 0 && level.bluelocked ) {
			level.bluelocked = qfalse;
			G_SendServerCommand( -1, va("print \"Blue team unlocked\n\"")); //ssdemo
		}
	}

	// make sure we have refs and team members holding timeouts in place
	red = blue = refs = 0;
	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || ( iter->r.svFlags & SVF_BOT ) ||
				iter->client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		if ( iter->client->sess.sessionTeam == TEAM_RED ) {
			red++;
		}
		else if ( iter->client->sess.sessionTeam == TEAM_BLUE ) {
			blue++;
		}

		if ( iter->client->sess.referee ) {
			refs++;
		}
	}

	if ( red == 0 && level.timein - level.time > 3000 && level.timeoutteam == TEAM_RED ) {
		G_SendServerCommand( -1, va("cp \"Empty red team: ending the timeout\n\"")); //ssdemo
		level.timein = level.time + 3000;
		SendTimeoutEvent();
	}

	if ( blue == 0 && level.timein - level.time > 3000 && level.timeoutteam == TEAM_BLUE ) {
		G_SendServerCommand( -1, va("cp \"Empty blue team: ending the timeout\n\"")); //ssdemo
		level.timein = level.time + 3000;
		SendTimeoutEvent();
	}

	if ( refs == 0 && level.timein - level.time > 3000 && level.timeoutteam == TEAM_FREE ) {
		G_SendServerCommand( -1, va("cp \"No referees: ending the timeout\n\"")); //ssdemo
		level.timein = level.time + 3000;
		SendTimeoutEvent();
	}

	if ( level.timein > level.time ) {
		level.totaltimeout += msec;
	}

	if ( level.timeoutEnt == NULL ) {
		level.timeoutEnt = G_Spawn();
	}

	G_SetOrigin( level.timeoutEnt, vec3_origin );
	level.timeoutEnt->s.eType = ET_TIMEOUT;
	level.timeoutEnt->s.time = level.timeout;
	level.timeoutEnt->s.time2 = level.timein;
	level.timeoutEnt->s.powerups = level.totaltimeout % 32768;
	level.timeoutEnt->s.generic1 = level.totaltimeout / 32768;
	level.timeoutEnt->s.eventParm = level.timeoutteam;
	level.timeoutEnt->r.svFlags |= SVF_BROADCAST;

	G_LinkEntity(level.timeoutEnt); //ssdemo

	// resolve any ignoring problems
	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		gentity_t *iter;

		// we're looking for disconnected players
		if ( ent->inuse ) continue;

		// loop through the clients
		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			// if the client isn't connected, skip it
			if ( !iter->inuse ) continue;

			// unignore the disconnected player
			iter->client->pers.ignoring[ent - g_entities] = qfalse;
		}
	}

	// advance the end of warmup if we're in timeout
	if ( level.warmupTime > 0 && level.timein > level.time ) {
		level.warmupTime += level.time - level.previousTime;
		G_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) ); //ssdemo
	}

	if ( sv_floodProtect.integer ) {
		G_Printf( "sv_floodProtect has been disabled.  Please use g_floodProtect instead.\n" );
		trap_Cvar_Set( "sv_floodProtect", "0" );
	}

	if ( g_huntMode.integer == 1) {
		qboolean foundInvis = qfalse;

		for ( iter = g_entities; iter < &g_entities[MAX_GENTITIES]; iter++ ) {
			if ( !iter->inuse ) continue;

			// looking for dropped or spawned invisibility powerups
			if ( iter->item && iter->item->giType == IT_POWERUP && iter->item->giTag == PW_INVIS ) {
				foundInvis = qtrue;
				break;
			}
			// looking for players with invisibility
			else if ( iter->client && iter->client->ps.powerups[PW_INVIS] ) {
				foundInvis = qtrue;
				break;
			}
		}

		if ( !foundInvis ) {
			int player = (int)Com_Clamp(0.0f, level.numConnectedClients + 0.99f, random() * level.numConnectedClients);
			ent = &g_entities[level.sortedClients[player]];
			
			if ( !IsClientFrozen(ent) && ent->client->ps.stats[STAT_HEALTH] > 0 &&
					ent->client->sess.sessionTeam != level.lastHuntedTeam && ent->client->sess.sessionTeam < TEAM_SPECTATOR ) {
				gentity_t	*te;

				te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
				te->s.eventParm = BG_FindItemForPowerup( PW_INVIS ) - bg_itemlist;
				te->r.svFlags |= SVF_BROADCAST;

				ent->client->ps.powerups[PW_INVIS] = level.time + g_huntTime.integer * 1000;
				level.lastHuntedTeam = ent->client->sess.sessionTeam;

				trap_SendServerCommand( ent - g_entities, va("cp \"^3You are THE HUNTER!\n\""));
				trap_SendServerCommand( ent - g_entities, va("print \"^3You are THE HUNTER!\n\""));

				for ( i = 0; i < level.maxclients; i++ ) {
					if ( g_entities[i].inuse && i != ent - g_entities ) {
						trap_SendServerCommand( i, va("cp \"%s ^3is THE HUNTER!\n\"", ent->client->pers.netname));
						trap_SendServerCommand( i, va("print \"%s ^3is THE HUNTER!\n\"", ent->client->pers.netname));
					}
				}
			}
		}
	}
	else if ( g_huntMode.integer == 2 ) {
		qboolean foundSight = qfalse;

		for ( iter = g_entities; iter < &g_entities[MAX_CLIENTS]; iter++ ) {
			if ( iter->inuse && iter->client ) {
				iter->client->ps.powerups[PW_INVIS] = 9999999;
			}
		}

		for ( iter = g_entities; iter < &g_entities[MAX_GENTITIES]; iter++ ) {
			if ( !iter->inuse ) continue;

			// looking for dropped or spawned invisibility powerups
			if ( iter->item && iter->item->giType == IT_POWERUP && iter->item->giTag == PW_SIGHT ) {
				foundSight = qtrue;
				break;
			}
			// looking for players with invisibility
			else if ( iter->client && iter->client->ps.powerups[PW_SIGHT] ) {
				foundSight = qtrue;
				break;
			}
		}

		if ( !foundSight ) {
			int player = (int)Com_Clamp(0.0f, level.numConnectedClients + 0.99f, random() * level.numConnectedClients);
			ent = &g_entities[level.sortedClients[player]];

			if ( !IsClientFrozen(ent) && ent->client->ps.stats[STAT_HEALTH] > 0 &&
					ent->client->sess.sessionTeam != level.lastHuntedTeam && ent->client->sess.sessionTeam < TEAM_SPECTATOR ) {
				gentity_t	*te;

				te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
				te->s.eventParm = BG_FindItemForPowerup( PW_SIGHT ) - bg_itemlist;
				te->r.svFlags |= SVF_BROADCAST;

				ent->client->ps.powerups[PW_SIGHT] = level.time + g_huntTime.integer * 1000;
				level.lastHuntedTeam = ent->client->sess.sessionTeam;

				trap_SendServerCommand( ent - g_entities, va("cp \"^3You are THE SEER!\n\""));
				trap_SendServerCommand( ent - g_entities, va("print \"^3You are THE SEER!\n\""));

				for ( i = 0; i < level.maxclients; i++ ) {
					if ( g_entities[i].inuse && i != ent - g_entities ) {
						trap_SendServerCommand( i, va("cp \"%s ^3is THE SEER!\n\"", ent->client->pers.netname));
						trap_SendServerCommand( i, va("print \"%s ^3is THE SEER!\n\"", ent->client->pers.netname));
					}
				}
			}
		}
	}
//freeze

	//
	// go through all allocated objects
	//
	start = trap_Milliseconds();
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				G_UnlinkEntity( ent ); //ssdemo
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

//unlagged - backward reconciliation #2
		// we'll run missiles separately to save CPU in backward reconciliation
/*
		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}
*/
//unlagged - backward reconciliation #2

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			if ( ent->s.number == 107 ) {
				int bob = 0;
			}
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}

//unlagged - backward reconciliation #2
	// NOW run the missiles, with all players backward-reconciled
	// to the positions they were in exactly 50ms ago, at the end
	// of the last server frame
	G_TimeShiftAllClients( level.previousTime, NULL );

	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
		}
	}

	G_UnTimeShiftAllClients( NULL );
//unlagged - backward reconciliation #2

end = trap_Milliseconds();

start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
end = trap_Milliseconds();

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

//anticheat
	if ( level.nextkeychange == 0 || level.time >= level.nextkeychange ) {
		level.nextkeychange = random() * 1000 + 1000 + level.time;

		level.lastcheatkey = level.cheatkey;
		level.cheatkey = random() * 32767 * 65536 + random() * 32767;
	}
//anticheat

	G_SetFreezeStats(); //ssdemo - bunch of stuff moved to G_SetFreezeStats()

//ssdemo
	// make sure there's no userinfo >= 32
	G_CleanUserinfo();

	// make sure the demo state configstring is blank
	G_SetDemoState( qfalse );

	// record the frame if a demo is recording
	G_RecordSDemoData();
//ssdemo

//multiview
	G_ReconcileClientNumIssues();
    G_ManagePipWindows();
//multiview

//anticheat
	for ( ent = g_entities; ent < &g_entities[MAX_GENTITIES]; ent++ ) {
		if ( !ent->inuse ) continue;

		if ( ent->s.eType == ET_PLAYER ) {
			if ( ent->s.number >= MAX_CLIENTS ) {
				if ( is_body_freeze(ent) ) {
					ent->s.eFlags &= ~EF_DEAD;
				}
				else {
					ent->s.eFlags |= EF_DEAD;
				}
			}

			BG_HashOrigin( ent->s.pos.trBase, sv_mapname.string, ent->s.clientNum, 1 );
			BG_HashDeadFlag( &ent->s.eFlags, sv_mapname.string, ent->s.number );
			ent->s.eFlags |= EF_HASHED;
		}
		else {
			ent->s.eFlags &= ~EF_HASHED;
		}
	}
//anticheat

//unlagged - backward reconciliation #4
	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
//unlagged - backward reconciliation #4
}

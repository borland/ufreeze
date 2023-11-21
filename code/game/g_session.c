// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	char s[MAX_STRING_CHARS];
	const char	*var;
	int i;

	Com_sprintf( s, sizeof(s), "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %s %i", 
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
//freeze
		client->sess.referee,
		client->sess.guestref,
		client->sess.invisible,
		client->sess.wallhack,
		client->sess.gamestats.numFreezes,
		client->sess.gamestats.numThaws,
		client->sess.gamestats.numFrozen,
		client->sess.localClient,
		client->sess.ip,
		client->sess.autoFollow
//freeze
		);

	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) {
		char weapstr[MAX_STRING_CHARS];

		Com_sprintf( weapstr, sizeof(weapstr), "%s %i %i %i %i %i %i %i %i", 
			s, 
			client->sess.accstats.totalShots[i], client->sess.accstats.totalEnemyHits[i],
			client->sess.accstats.totalTeamHits[i], client->sess.accstats.totalCorpseHits[i],
			client->sess.accstats.totalKills[i], client->sess.accstats.totalDeaths[i],
			client->sess.accstats.damageGiven[i], client->sess.accstats.damageReceived[i] );

		strcpy( s, weapstr );
	}

	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

//freeze
	int i;
	char *p;
//freeze

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

//freeze
	p = s;
	while ( *p ) {
		if ( *p == ' ' ) {
			*p = '\0';
		}

		p++;
	}

	p = s;
	sessionTeam = atoi( p );

	while ( *(p++) );
	client->sess.spectatorTime = atoi( p );

	while ( *(p++) );
	spectatorState = atoi( p );

	while ( *(p++) );
	client->sess.spectatorClient = atoi( p );

	while ( *(p++) );
	client->sess.wins = atoi( p );

	while ( *(p++) );
	client->sess.losses = atoi( p );

	while ( *(p++) );
	teamLeader = atoi( p );

	while ( *(p++) );
	client->sess.referee = atoi( p );

	while ( *(p++) );
	client->sess.guestref = atoi( p );

	while ( *(p++) );
	client->sess.invisible = atoi( p );

	while ( *(p++) );
	client->sess.wallhack = atoi( p );

	while ( *(p++) );
	client->sess.gamestats.numFreezes = atoi( p );

	while ( *(p++) );
	client->sess.gamestats.numThaws = atoi( p );

	while ( *(p++) );
	client->sess.gamestats.numFrozen = atoi( p );

	while ( *(p++) );
	client->sess.localClient = atoi( p );

	while ( *(p++) );
	Q_strncpyz( client->sess.ip, p, sizeof(client->sess.ip) );

	if ( Q_strncmp( client->sess.ip, "172.21.", 7 ) == 0 ||
			Q_strncmp( client->sess.ip, "192.168.", 8 ) == 0 ||
			Q_strncmp( client->sess.ip, "10.", 3 ) == 0 ||
			Q_strncmp( client->sess.ip, "127.", 4 ) == 0 ) {
		client->sess.privateNetwork = qtrue;
	}

	while ( *(p++) );
	client->sess.autoFollow = atoi( p );

	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) {
		while ( *(p++) );
		client->sess.accstats.totalShots[i] = atoi( p );
		
		while ( *(p++) );
		client->sess.accstats.totalEnemyHits[i] = atoi( p );

		while ( *(p++) );
		client->sess.accstats.totalTeamHits[i] = atoi( p );
		
		while ( *(p++) );
		client->sess.accstats.totalCorpseHits[i] = atoi( p );

		while ( *(p++) );
		client->sess.accstats.totalKills[i] = atoi( p );
		
		while ( *(p++) );
		client->sess.accstats.totalDeaths[i] = atoi( p );

		while ( *(p++) );
		client->sess.accstats.damageGiven[i] = atoi( p );
		
		while ( *(p++) );
		client->sess.accstats.damageReceived[i] = atoi( p );
	}

/*
	sscanf( s, "%i %i %i %i %i %i %i",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorTime,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader)                  // bk010221 - format
*/
//freeze

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;	
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

//freeze
	sess->ip[0] = '\0';
	sess->referee = qfalse;
	sess->invisible = qfalse;
	sess->autoFollow = qfalse;
	memset( &sess->accstats, 0, sizeof(accStats_t) );
	memset( &sess->gamestats, 0, sizeof(gameStats_t) );
//freeze

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}

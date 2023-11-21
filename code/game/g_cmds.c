// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "../../ui/menudef.h"			// for the voice chats

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1048];
	char		string[1400];
	int			stringlength;
	int			i, j, numSent;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
//anticheat
	numSent = 0;
//anticheat
	
	for (i=0 ; i < numSorted ; i++) {
		int		ping;
//unlagged - true ping
		int		realPing;
//unlagged - true ping

		cl = &level.clients[level.sortedClients[i]];

//anticheat
		if ( cl->sess.invisible ) {
			continue;
		}

		numSent++;
//anticheat

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
//unlagged - true ping
			realPing = -1;
//unlagged - true ping
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
//unlagged - true ping
			realPing = cl->pers.realPing < 999 ? cl->pers.realPing : 999;
//unlagged - true ping
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;
//freeze
		scoreFlags = cl->sess.wins;
//freeze

//freeze - added new score info
		Com_sprintf (entry, sizeof(entry),
//unlagged - true ping
			//" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			//cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],cl->ps.persistant[PERS_KILLED], ping, realPing, (level.time - cl->pers.enterTime)/60000,
//unlagged - true ping
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			cl->ps.persistant[PERS_CAPTURES],
//freeze
			cl->ps.persistant[PERS_TOTALFROZEN] / 60000,
			cl->sess.referee);
//freeze
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s",
		numSent, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
//ssdemo
		//if ( idnum < 0 || idnum >= level.maxclients ) {
		if ( idnum < 0 || (level.inDemoFile == 0 && idnum >= level.maxclients) || (level.inDemoFile != 0 && idnum >= MAX_CLIENTS) ) {
//ssdemo
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 /*- 
			( 1 << WP_GRAPPLING_HOOK ) */ - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = 999;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}

//freeze
void Cmd_GiveAll_f( gentity_t *ent ) {
	gentity_t *iter;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || is_spectator(iter->client) ) {
			continue;
		}

		Cmd_Give_f( iter );
	}
}
//freeze

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
//freeze
	if ( level.timein > level.time ) {
		trap_SendServerCommand( ent-g_entities, "print \"Suicide is not allowed in a timeout\n\"" );
		return;
	}
//freeze

/*freeze
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
freeze*/
	if ( is_spectator( ent->client ) ) {
//freeze
		return;
	}
	if (ent->health <= 0) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_KILL);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	if ( client->sess.sessionTeam == TEAM_RED ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"",
			client->pers.netname) ); //ssdemo
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
			client->pers.netname)); //ssdemo
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
			client->pers.netname)); //ssdemo
	} else if ( client->sess.sessionTeam == TEAM_REDCOACH ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " is now Coaching the Red team.\n\"",
			client->pers.netname)); //ssdemo
	} else if ( client->sess.sessionTeam == TEAM_BLUECOACH ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " is now Coaching the Blue team.\n\"",
			client->pers.netname)); //ssdemo
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		G_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"",
			client->pers.netname)); //ssdemo
	}
}

/*
=================
SetTeamCoach
=================
*/
qboolean SetTeamCoach( gentity_t *ent, char *s, qboolean force ) {
	int					team, oldTeam,i;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;

	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_FOLLOW;

	oldTeam = client->sess.sessionTeam;
	
	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR && oldTeam != TEAM_REDCOACH && oldTeam != TEAM_BLUECOACH ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
//freeze
		if ( g_gametype.integer >= GT_TEAM ) {
			AddScore(ent, NULL, 1);
		}
//freeze
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
	}

	if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
		team = TEAM_REDCOACH;
	} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
		team = TEAM_BLUECOACH;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	//find the first person on the appropriate team
	for(i = 0; i < level.maxclients;i++){
		if(client->sess.sessionTeam == TEAM_REDCOACH){
			if(level.clients[i].sess.sessionTeam == TEAM_RED){
				specClient = i;
				break;
			}
		}
		if(client->sess.sessionTeam == TEAM_BLUECOACH){
			if(level.clients[i].sess.sessionTeam == TEAM_BLUE){
				specClient = i;
				break;
			}
		}
	}
	client->sess.spectatorClient = specClient;

	// make sure there is a team leader on the team the player came from

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum, qfalse );

	return qtrue;
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam( gentity_t *ent, char *s, qboolean force ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		if ( g_teamForceBalance.integer && !force ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"print \"Red team has too many players.\n\"" );
				return qfalse; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"print \"Blue team has too many players.\n\"" );
				return qfalse; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

//ssdemo
	// if playing a demo
	if ( level.inDemoFile != 0 ) {
		// don't let the player join a team
		if ( team == TEAM_BLUE || team == TEAM_RED ) {
			trap_SendServerCommand( ent->client->ps.clientNum, 
					"print \"You may not join a team while a demo is in progress.\n\"" );
			team = TEAM_SPECTATOR;
		}
	}
//ssdemo

	oldTeam = client->sess.sessionTeam;

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_TOURNAMENT)
		&& level.numNonSpectatorClients >= 2 ) {

		trap_SendServerCommand( ent->client->ps.clientNum, 
				"print \"There are already two players in the game.\n\"" );

		return qfalse; // ignore the request
	} else if ( g_maxGameClients.integer > 0 && 
			level.numNonSpectatorClients >= g_maxGameClients.integer &&
			!force && oldTeam == TEAM_SPECTATOR && oldTeam != TEAM_REDCOACH && oldTeam != TEAM_BLUECOACH ) {

		trap_SendServerCommand( ent->client->ps.clientNum, 
				va("print \"You may not join until there are less than %d players in the game.\n\"", g_maxGameClients.integer) );

		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return qfalse;
	}

//freeze
	if ( oldTeam == TEAM_SPECTATOR && client->sess.invisible ) {
		client->sess.invisible = qfalse;
		G_SendServerCommand( -1, va("print \"%s ^7connected\n\"", ent->client->pers.netname) ); //ssdemo
	}
//freeze

	//
	// execute the team change
	//

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR && oldTeam != TEAM_REDCOACH && oldTeam != TEAM_BLUECOACH ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
//freeze
		if ( g_gametype.integer >= GT_TEAM ) {
			AddScore(ent, NULL, 1);
		}
//freeze
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum, qfalse );

	return qtrue;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	//coaches aren't allowed to freefloat
	if(ent->client->ps.persistant[ PERS_TEAM ] == TEAM_REDCOACH || ent->client->ps.persistant[ PERS_TEAM ] == TEAM_BLUECOACH)
		return;

	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
/*freeze
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
freeze*/
	SetClientViewAngle( ent, ent->client->ps.viewangles );
	ent->client->ps.stats[ STAT_HEALTH ] = ent->health = 100;
	memset( ent->client->ps.powerups, 0, sizeof ( ent->client->ps.powerups ) );
//freeze
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, "print \"You may not switch teams more than once per 5 seconds.\n\"" );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

//freeze
	if ( ent->freezeState ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW || 
				ent->client->sess.spectatorState == SPECTATOR_FROZEN ) {
			StopFollowing( ent );
		}
		return;
	}
//freeze
	trap_Argv( 1, s, sizeof( s ) );

//freeze
	if ( (!Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" )) && level.redlocked ) {
		trap_SendServerCommand( ent-g_entities, "print \"Red team is locked\n\"" );
		return;
	}
	else if ( (!Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" )) && level.bluelocked ) {
		trap_SendServerCommand( ent-g_entities, "print \"Blue team is locked\n\"" );
		return;
	}
//freeze

	if ( SetTeam( ent, s, qfalse ) ) {
		ent->client->switchTeamTime = level.time + 5000;
	}
}

qboolean CanFollow( gentity_t *ent, gentity_t *target ) {

//ssdemo
	if ( target == NULL || target->client == NULL ) { 
		return qfalse;
	}
//ssdemo

	// only follow connected clients
	if ( target->client->pers.connected != CON_CONNECTED ) {
		return qfalse;
	}

	// if the follower is a spectator
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// if the target is a spectator
		if ( target->client->sess.sessionTeam == TEAM_SPECTATOR ||
				target->client->sess.sessionTeam == TEAM_REDCOACH ||
				target->client->sess.sessionTeam == TEAM_BLUECOACH ) {
			return qfalse;
		}

		// if the target is the follower
		if ( target == ent ) {
			return qfalse;
		}

		// if the follower is not a referee
		if ( !ent->client->sess.referee ) {
			// if the target is red and the red team is speclocked
			if ( target->client->sess.sessionTeam == TEAM_RED && level.redspeclocked ) {
				return qfalse;
			}

			// if the target is blue and the blue team is speclocked
			if ( target->client->sess.sessionTeam == TEAM_BLUE && level.bluespeclocked ) {
				return qfalse;
			}
		}
	}
	// if the follower is not a spectator
	//they are a red coach
	else if ( ent->client->sess.sessionTeam == TEAM_REDCOACH ) {
		// if the target is the follower
		if ( target == ent ) {
			return qfalse;
		}
		// if the target is a spectator
		if ( target->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return qfalse;
		}
		//if the target is blue
		if (target->client->sess.sessionTeam == TEAM_BLUE){
			return qfalse;
		}

	//else they are a blue coach
	}else if ( ent->client->sess.sessionTeam == TEAM_BLUECOACH ) {
		// if the target is the follower
		if ( target == ent ) {
			return qfalse;
		}
		//if the target is red
		if (target->client->sess.sessionTeam == TEAM_RED){
			return qfalse;
		}
		// if the target is a spectator
		if ( target->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return qfalse;
		}

	//they are something else! 
	}else{
		// if the target is on a different team (including spectator)
		if ( target->client->sess.sessionTeam != ent->client->sess.sessionTeam ) {
			return qfalse;
		}

		// for testing - this will only be run if the above is not commented out
		// if the target is a spectator
		if ( target->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return qfalse;
		}
	}

	return qtrue;
}

static void FollowPlayer( gentity_t *ent, gentity_t *target ) {
	// if the target is frozen
	if ( IsClientFrozen( target ) && target->target_ent ) {
		vec3_t angles;

		ent->client->sess.spectatorState = SPECTATOR_FROZEN;

		// toggle the teleport bit so the client knows to not lerp
		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

		VectorCopy( target->target_ent->s.apos.trBase, angles );
		angles[PITCH] = 45.0f;

		SetClientViewAngle( ent, angles );
	}
	else {
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	}

	ent->client->sess.spectatorClient = target->s.number;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( !is_spectator( ent->client ) ) {
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW || 
				ent->client->sess.spectatorState == SPECTATOR_FROZEN ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// if following self AND self is not frozen AND self is not a spectator
	if ( i == ent->s.number && !IsClientFrozen(ent) && ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->sess.sessionTeam != TEAM_REDCOACH && ent->client->sess.sessionTeam != TEAM_BLUECOACH) {
		StopFollowing( ent );
		return;
	}

//freeze
	if ( CanFollow( ent, &g_entities[i] ) ) {
		// if they are playing a tournement game, count as a loss
		if ( (g_gametype.integer == GT_TOURNAMENT )
			&& ent->client->sess.sessionTeam == TEAM_FREE ) {
			ent->client->sess.losses++;
		}

		// first set them to spectator
		if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
			SetTeam( ent, "spectator", qfalse );
		}

		FollowPlayer( ent, &g_entities[i] );
	}
//freeze
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;

	// if they're not spectating, return
	if ( !is_spectator( ent->client ) ) {
		return;
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;

//ssdemo
		//if ( clientnum >= level.maxclients ) {
		if ( clientnum >= MAX_CLIENTS ) {
//ssdemo
			clientnum = 0;
		}

		if ( clientnum < 0 ) {
//ssdemo
			//clientnum = level.maxclients - 1;
			clientnum = MAX_CLIENTS - 1;
//ssdemo
		}

//freeze
		if ( CanFollow( ent, &g_entities[clientnum] ) ) {
			FollowPlayer( ent, &g_entities[clientnum] );
			return;
		}
//freeze
	} while ( clientnum != original );

	// leave it where it was
}

/*
From OSP documentation:

#A - Armor
	Current armor value (lower-case "a" will print the value
	with color coding, depending on the level of the value)
	(Note: the docs have it backwards)

#C - Corpse
	The location where you last died
	/say_team "Overrun at #C"

#D - Damaged by
	The last player to score a hit on you
	/say_team "#D is here and he's heavily armed!"

#F - nearest Friend
	Reports closest teammate in team-based gametypes.

#H - Health
	Current health value (lower-case "h" will print the value
	with color coding, depending on the level of the value)
	/say_team "Hurting bad - #H/#A"
	(Note: the docs have it backwards)

#I - Item (nearest available)
	Shows the nearest "significant" (weapon, armor, powerup, or MH)
	available item, and that includes dropped items.  Note, you must
      be able to *see* the center of the item (not just an edge).. this
      requires you to be facing in the general direction of the object.
	/say_team "#I available here"

#K - ammopacK (nearest available)
	Just like #I, except it reports only closest available ammopack.

#L - Location
	Many maps have terrible target_location entities
	e.g. PG on PRO-DM6 shows as YA
	"(pF.arQon) (YA): Weapon available" is a bit crap, neh?
	This shows the nearest "significant" item spawn
	(weapon, armor, powerup, or MH), whether the item is there or not

#M - aMmo wanted
	Lists all types of ammo for weapons you have that are empty or
	are nearly emptry (between 0 and 5 ammo).
	/say_team "Need #M"

#P - last item Picked up
	Reports last item picked up.  Useful for reporting when you pick
	up an important item (i.e. quad).

#R - health/aRmor (nearest available)
	Just like #I, except it reports only closest available health/armor.

#T - Target
	The last player you hit
	/say_team "#T is weak - finish him!"

#U - powerUps
	Lists all powerups you currently possess

#W - current Weapon held
	Prints the abbreviated name of the weapon you are currently holding
*/

static qboolean IsChatToken( const char *tok ) {
	if ( *tok == '\0' ) {
		return qfalse;
	}

	if ( *tok != '#' ) {
		return qfalse;
	}

	if ( strchr("AaCcDdFfHhIiKkLlMmPpRrTtUuWw", *(tok + 1)) ) {
		return qtrue;
	}

	return qfalse;
}

static char *g_pszWeaponShortNames[WP_NUM_WEAPONS] = {
	"",
	"GAUNT",
	"MG",
	"SG",
	"GL",
	"RL",
	"LG",
	"RG",
	"PG",
	"BFG"
};

static char *g_pszAmmoNames[WP_NUM_WEAPONS] = {
	"",
	"",
	"bullets",
	"shells",
	"grenades",
	"rockets",
	"lightning ammo",
	"slugs",
	"cells",
	"BFG ammo"
};

static char *g_pszPowerupNames[PW_NUM_POWERUPS] = {
	"",
	"Quad Damage",
	"Battle Suit",
	"Haste",
	"Invisibility",
	"Regeneration",
	"Flight",
	"Red Flag",
	"Blue Flag",
};

#define FT_MINOR_HEALTH  0x00000001
#define FT_MAJOR_HEALTH  0x00000002
#define FT_ARMOR         0x00000004
#define FT_AMMO          0x00000008
#define FT_WEAPON        0x00000010
#define FT_POWERUP       0x00000020
#define FT_TEAM          0x00000040

static gentity_t *G_FindNearestItem( gentity_t *ent, int findflags ) {
	vec3_t forward, delta;
	float dist, bestdist;
	gentity_t *iter, *best;
	trace_t trace;

	best = NULL;
	bestdist = 8192.0f * 8192.0f;

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	for (iter = g_entities; iter < &g_entities[MAX_GENTITIES]; iter++) {
		qboolean match = qfalse;

		if (!iter->inuse || !iter->item || (iter->s.eFlags & EF_NODRAW)) continue;

		if ( (findflags & FT_MINOR_HEALTH) &&
				(iter->item->giType == IT_HEALTH && iter->item->quantity < 100 && iter->item->quantity >= 25 ) ) {
			match = qtrue;
		}
		else if ( (findflags & FT_MAJOR_HEALTH) &&
				(iter->item->giType == IT_HEALTH && iter->item->quantity == 100) ) {
			match = qtrue;
		}
		else if ( (findflags & FT_ARMOR) &&
				(iter->item->giType == IT_ARMOR && iter->item->quantity >= 50) ) {
			match = qtrue;
		}
		else if ( (findflags & FT_AMMO) && iter->item->giType == IT_AMMO ) {
			match = qtrue;
		}
		else if ( (findflags & FT_WEAPON) && iter->item->giType == IT_WEAPON ) {
			match = qtrue;
		}
		else if ( (findflags & FT_POWERUP) &&
				(iter->item->giType == IT_POWERUP || iter->item->giType == IT_HOLDABLE) ) {
			match = qtrue;
		}
		else if ( (findflags & FT_TEAM) && iter->item->giType == IT_TEAM ) {
			match = qtrue;
		}

		if (!match) continue;

		VectorSubtract( iter->r.currentOrigin, ent->r.currentOrigin, delta );
		dist = VectorNormalize( delta );

		// check FOV
		if ( DotProduct( forward, delta ) < 0.5f ) {
			continue;
		}

		// check against other distances
		if (dist > bestdist) {
			continue;
		}

		// check line-of-sight
		trap_Trace( &trace, ent->r.currentOrigin, NULL, NULL, iter->r.currentOrigin, ent->s.number, MASK_SOLID );
		if ( trace.fraction < 1.0f ) {
			continue;
		}

		bestdist = dist;
		best = iter;
	}

	return best;
}

static void G_ExpandText( gentity_t *ent, int color, char *dest, const char *text, int destsize) {
	int i, j;

	for ( i = 0, j = 0; text[i] != '\0' && j < destsize; i++ ) {
		if ( !IsChatToken(&text[i]) ) {
			dest[j] = text[i];
			j++;
		}
		else {
			char tok = text[++i];
			char val[MAX_SAY_TEXT];

			val[0] = '\0';

			switch (tok)
			{
				case 'a': {
					// current armor
					Com_sprintf( val, sizeof(val), "%d", ent->client->ps.stats[STAT_ARMOR] );
				} break;

				case 'A': {
					// current armor in color
					char acolor;
					int armor = ent->client->ps.stats[STAT_ARMOR];

					if ( armor < 50 ) {
						acolor = '1';
					}
					else if ( armor < 100 ) {
						acolor = '3';
					}
					else {
						acolor = '7';
					}

					Com_sprintf( val, sizeof(val), "%c%c%d%c%c", Q_COLOR_ESCAPE, acolor, armor, Q_COLOR_ESCAPE, color );
				} break;

				case 'C': case 'c': {
					if ( ent->client->pers.lastCorpse[0] != 0.0f || ent->client->pers.lastCorpse[1] != 0.0f || ent->client->pers.lastCorpse[2] != 0.0f ) {
						Team_GetLocationMsgFromOrigin( ent->client->pers.lastCorpse, val, sizeof(val), qtrue );
					}
				} break;

				case 'D': case 'd': {
					// last attacker
					int attacker = ent->client->ps.persistant[PERS_ATTACKER];

					if ( attacker < MAX_CLIENTS && attacker != ent->s.number &&
							g_entities[attacker].inuse && g_entities[attacker].client ) {
						Com_sprintf( val, sizeof(val), "%c7%s%c%c", Q_COLOR_ESCAPE, g_entities[attacker].client->pers.netname, Q_COLOR_ESCAPE, color );
					}
					else {
						strcpy( val, "nobody" );
					}
				} break;

				case 'F': case 'f': {
					// closest friend
					if ( g_gametype.integer >= GT_TEAM ) {
						gentity_t *iter, *match = NULL;
						float dist, bestdist = 64000.0f;
						vec3_t delta;

						for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
							if ( !iter->inuse || !iter->client || iter == ent ||
									iter->client->ps.persistant[PERS_TEAM] != ent->client->sess.sessionTeam ) {
								continue;
							}

							VectorSubtract( ent->r.currentOrigin, iter->r.currentOrigin, delta );
							dist = VectorLength( delta );
							if ( dist < bestdist ) {
								bestdist = dist;
								match = iter;
							}
						}

						if ( match != NULL ) {
							Com_sprintf( val, sizeof(val), "%c7%s%c%c", Q_COLOR_ESCAPE, match->client->pers.netname, Q_COLOR_ESCAPE, color );
						}
					}
				} break;

				case 'h': {
					// current health
					Com_sprintf( val, sizeof(val), "%d", ent->client->ps.stats[STAT_HEALTH] );
				} break;

				case 'H': {
					// current health in color
					char hcolor;
					int health = ent->client->ps.stats[STAT_HEALTH];

					if ( health < 50 ) {
						hcolor = '1';
					}
					else if ( health < 100 ) {
						hcolor = '3';
					}
					else {
						hcolor = '7';
					}

					Com_sprintf( val, sizeof(val), "%c%c%d%c%c", Q_COLOR_ESCAPE, hcolor, health, Q_COLOR_ESCAPE, color );
				} break;

				case 'I': case 'i': {
					gentity_t *item = G_FindNearestItem( ent, FT_WEAPON | FT_ARMOR | FT_MAJOR_HEALTH | FT_POWERUP );
					if ( item != NULL ) {
						strcpy( val, item->item->pickup_name );
					}
				} break;

				case 'K': case 'k': {
					gentity_t *item = G_FindNearestItem( ent, FT_AMMO );
					if ( item != NULL ) {
						strcpy( val, item->item->pickup_name );
					}
				} break;

				case 'L': case 'l': {
					if ( !Team_GetLocationMsg(ent, val, sizeof(val), qtrue) ) {
						strcpy( val, "unknown" );
					}
				} break;

				case 'M': case 'm': {
					// 'need ammo' list
					int i;
					qboolean bFirst = qtrue;

					for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
						if ( (ent->client->ps.stats[STAT_WEAPONS] & (1 << i)) && ent->client->ps.ammo[i] <= 5 ) {
							if ( bFirst ) {
								strcat(val, g_pszAmmoNames[i]);
								bFirst = qfalse;
							}
							else {
								strcat(val, ", ");
								strcat(val, g_pszAmmoNames[i]);
							}
						}
					}
				} break;

				case 'P': case 'p': {
					// last item picked up
					if ( ent->client->lastItem != NULL ) {
						Q_strncpyz( val, ent->client->lastItem, sizeof(val) );
					}
					else {
						strcpy( val, "nothing" );
					}
				} break;

				case 'R': case 'r': {
					gentity_t *item = G_FindNearestItem( ent, FT_MINOR_HEALTH | FT_MAJOR_HEALTH | FT_ARMOR );
					if ( item != NULL ) {
						strcpy( val, item->item->pickup_name );
					}
				} break;

				case 'T': case 't': {
					// last attacker
					int attacked = ent->client->lastAttacked;

					if ( attacked < MAX_CLIENTS && attacked != ent->s.number &&
							g_entities[attacked].inuse && g_entities[attacked].client ) {
						Com_sprintf( val, sizeof(val), "%c7%s%c%c", Q_COLOR_ESCAPE, g_entities[attacked].client->pers.netname, Q_COLOR_ESCAPE, color );
					}
					else {
						strcpy( val, "nobody" );
					}
				} break;

				case 'U': case 'u': {
					// powerup list
					int i;
					qboolean bFirst = qtrue;

					for ( i = PW_QUAD; i < PW_NUM_POWERUPS; i++ ) {
						if ( ent->client->ps.powerups[i] ) {
							if ( bFirst ) {
								strcat(val, g_pszPowerupNames[i]);
								bFirst = qfalse;
							}
							else {
								strcat(val, ", ");
								strcat(val, g_pszPowerupNames[i]);
							}
						}
					}
				} break;

				case 'W': case 'w': {
					// weapon
					Com_sprintf( val, sizeof(val), "%s", g_pszWeaponShortNames[ent->client->ps.weapon] );
				} break;
			}

			strncpy( &dest[j], val, destsize - j );
			j += min( strlen(val), destsize - j );
		}
	}

	// close it off
	dest[j] = '\0';
}

/*
==================
G_SayTo
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	char *cmd;
	char expandedText[MAX_SAY_TEXT];

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	if ( mode == SAY_REF && !other->client->sess.referee && ent != other ) {
		return;
	}
	if ( mode == SAY_RED && other->client->sess.sessionTeam != TEAM_RED &&
			!other->client->sess.referee && ent != other ) {
		return;
	}
	if ( mode == SAY_BLUE && other->client->sess.sessionTeam != TEAM_BLUE &&
			!other->client->sess.referee && ent != other ) {
		return;
	}
	if ( mode != SAY_REF && other->client->pers.ignoring[ent->s.number] ) {
		return;
	}
	if ( mode != SAY_REF && ent->client->pers.muted ) {
		return;
	}

	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		cmd = "chat";
		break;
	case SAY_TEAM:
		cmd = "tchat";
		break;
	case SAY_EMOTE:
		cmd = "emote";
		break;
	case SAY_EMOTETEAM:
		cmd = "temote";
		break;
	}

	G_ExpandText( ent, color, expandedText, message, sizeof(expandedText) );

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, expandedText));
}

#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[67];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

//ssdemo
	if ( level.inDemoFile != 0 && mode == SAY_ALL ) {
		mode = SAY_TEAM;
	}
//ssdemo

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_EMOTE:
		G_LogPrintf( "me: %s %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), " : %s%c%c"EC" ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
//ssdemo
		if ( level.inDemoFile != 0 ) {
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (demo spectator)"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		else
//ssdemo
		if (Team_GetLocationMsg(ent, location, sizeof(location), qfalse))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_EMOTETEAM:
		G_LogPrintf( "me: %s %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), " : %s%c%c"EC" ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location), qfalse))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	case SAY_REF:
		G_LogPrintf( "sayref: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%c%creferee channel%c%c)"EC": ", 
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_YELLOW, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_YELLOW;
		break;
	case SAY_RED:
		G_LogPrintf( "sayred: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%c%cred channel%c%c)"EC": ", 
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_RED, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_BLUE:
		G_LogPrintf( "sayblue: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%c%cblue channel%c%c)"EC": ", 
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_BLUE, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}

//ssdemo
	if ( mode == SAY_ALL ) {
		char expandedText[MAX_SAY_TEXT];

		G_ExpandText( ent, color, expandedText, text, sizeof(expandedText) );

		// record global chat as truly global, and make it magenta
		G_RecordServerCommand( -1, va("chat \"%s%c%c%s\"", name, Q_COLOR_ESCAPE, color, expandedText));
	}
//ssdemo
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	if ( mode == SAY_EMOTE || mode == SAY_EMOTETEAM ) {
		if ( strchr(p, '^') ) {
			trap_SendServerCommand( ent-g_entities, "print \"You may not use colors in /me or /me_team.\n\"" );
			return;
		}
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_GAUNTLET) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}



static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

static const char *gameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag CTF",
	"Overload",
	"Harvester"
};

//freeze
static const char *voteVars[] = {
	"capturelimit",
	"timelimit",
	"warmup",
	"friendlyfire",
	"gametype",
	"grapple",
	"lagcomp",
	"kick",
	"remove",
	"putblue",
	"putred",
	"map",
	"restart",
	"nextmap",
	"railjump",
	"opinion",
	"config",
	"huntmode",
};
static const int nVoteVars = sizeof(voteVars) / sizeof(const char *);

static const char *voteVarMap[] = {
	"capturelimit",
	"timelimit",
	"g_doWarmup",
	"g_friendlyFire",
	"g_gametype",
	"g_grapple",
	"g_delagHitscan",
	"clientkick",
	"remove",
	"putblue",
	"putred",
	"map",
	"map_restart",
	"nextmap",
	"g_railjumpknock",
	"opinion",
	"exec",
	"g_huntMode",
};
static const int nVoteVarMap = sizeof(voteVarMap) / sizeof(const char *);

static const char *voteValues[] = {
	"<num>",
	"<num>",
	"<0/1>",
	"<0/1>",
	"<3/4>",
	"<0/1>",
	"<0/1>",
	"<num>",
	"<num>",
	"<num>",
	"<num>",
	"<name>",
	"",
	"",
	"<knock>",
	"<text>",
	"<cfg>",
	"<0-2>",
};
static const int nVoteValues = sizeof(voteValues) / sizeof(const char *);

static qboolean IsVoteEnabled( const char *voteVar ) {
	char enabler[MAX_STRING_CHARS];

	Com_sprintf( enabler, sizeof(enabler), "vote_%s", voteVar );
	return (trap_Cvar_VariableIntegerValue( enabler ) != 0);
}

static void PrintVoteHelp( gentity_t *ent ) {
	int			i, j, k, l;
	char		temp[256];
	const char	*spaces1 = "                ";
	const int	nSpaces1 = strlen( spaces1 );
	const char	*spaces2 = "          ";
	const int	nSpaces2 = strlen( spaces2 );
	const int	nCols = 3;
	qboolean lastCR = qfalse;

	if ( nVoteVars != nVoteValues ) {
		trap_SendServerCommand( ent-g_entities, "print \"Sanity check failed: nVoteVars != nVoteValues\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"^2Vote commands are:\n\"" );

	for ( i = 0, l = 0; i < nVoteVars; i++ ) {
		if ( !IsVoteEnabled( voteVars[i] ) ) {
			continue;
		}

		j = strlen( voteVars[i] );
		if ( j > nSpaces1 )
			j = nSpaces1;

		k = strlen( voteValues[i] );
		if ( k > nSpaces2 )
			k = nSpaces2;

		if ( l % nCols == nCols - 1 ) {
			Com_sprintf( temp, sizeof(temp), "print \"^3%s%s^5%s\n\"", voteVars[i], &spaces1[j], voteValues[i] );
			lastCR = qtrue;
		}
		else {
			Com_sprintf( temp, sizeof(temp), "print \"^3%s%s^5%s%s\"", voteVars[i], &spaces1[j], voteValues[i], &spaces2[k] );
			lastCR = qfalse;
		}
		trap_SendServerCommand( ent-g_entities, temp );

		l++;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s^2Use /clients to get a list of clients when a client number is required.\n\"", lastCR ? "" : "\n") );
}

static const char *GetVoteVar( const char *cmd ) {
	int		i;

	for ( i = 0; i < nVoteVars; i++ ) {
		if ( Q_stricmp( cmd, voteVars[i] ) == 0 ) {
			if ( !IsVoteEnabled( voteVars[i] ) ) {
				return "";
			}
			return voteVarMap[i];
		}
	}

	return "";
}

static qboolean PrintSpecificVoteHelp( gentity_t *ent, const char *cmd ) {
	char reply[MAX_STRING_CHARS];

	reply[0] = '\0';

	if ( !Q_stricmp( cmd, "capturelimit" ) ) {
	}
	else if ( !Q_stricmp( cmd, "timelimit" ) ) {
	}
	else if ( !Q_stricmp( cmd, "g_doWarmup" ) ) {
	}
	else if ( !Q_stricmp( cmd, "g_friendlyFire" ) ) {
	}
	else if ( !Q_stricmp( cmd, "g_gametype" ) ) {
	}
	else if ( !Q_stricmp( cmd, "g_grapple" ) ) {
	}
	else if ( !Q_stricmp( cmd, "g_delagHitscan" ) ) {
	}
	else if ( !Q_stricmp( cmd, "clientkick" ) ) {
	}
	else if ( !Q_stricmp( cmd, "remove" ) ) {
	}
	else if ( !Q_stricmp( cmd, "putblue" ) ) {
	}
	else if ( !Q_stricmp( cmd, "putred" ) ) {
	}
	else if ( !Q_stricmp( cmd, "map" ) ) {
		// could list available maps here
	}
	else if ( !Q_stricmp( cmd, "g_railjumpknock" ) ) {
	}
	else if ( !Q_stricmp( cmd, "opinion" ) ) {
		strcpy( reply, "What, you haven't got an opinion?" );
	}
	else if ( !Q_stricmp( cmd, "exec" ) ) {
		char filelist[MAX_STRING_CHARS];
		int numfiles = trap_FS_GetFileList( "votecfgs", ".cfg", filelist, sizeof(filelist) );

		if ( numfiles != 0 ) {
			char *curr = filelist;
			int i;

			strcpy( reply, "You may vote on the following configs:^3\n" );

			for ( i = 0; i < numfiles; i++ ) {
				int skip = strlen( curr ) + 1;
				char *dot = Q_strrchr( curr, '.' );

				if ( dot == NULL || Q_stricmp( dot + 1, "cfg" ) != 0 ) {
					curr += skip;
					continue;
				}

				*dot = '\0';

				if ( strlen( reply ) + strlen( curr ) + 2 >= MAX_STRING_CHARS ) {
					break;
				}

				strcat( reply, curr );
				if ( i < numfiles - 1 ) {
					strcat( reply, "\n" );
				}

				curr += skip;
			}
		}
		else {
			strcpy( reply, "There are no valid configuration files to vote on." );
		}
	}
	else if ( !Q_stricmp( cmd, "g_huntMode" ) ) {
		strcpy( reply, "0: Regular Freeze Tag.\n1: Teams take turns having an invisible player.\n2: Everyone is invisible, and teams take turns having a seer." );
	}

	if ( reply[0] == '\0' ) {
		return qfalse;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"^2%s\n\"", reply) );
	return qtrue;
}

static qboolean CheckValid( const char *cmd, const char *value, char **err) {
	int			i;

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( cmd, "g_gametype" ) ) {
		i = atoi( value );
		if( i < GT_TEAM || i > GT_CTF ) {
			*err = "print \"Invalid gametype.\n\"";
			return qfalse;
		}
	}
	else if ( !Q_stricmp( cmd, "clientkick" ) || !Q_stricmp( cmd, "putblue" ) ||
			!Q_stricmp( cmd, "putred" ) || !Q_stricmp( cmd, "remove" ) ) {
		gentity_t *ent;
		if ( value[0] == '\0' ) {
			*err = "print \"Use /clients to get a list of clients on the server.\n\"";
			return qfalse;
		}
		if ( value[0] < '0' || value[0] > '9' ) {
			*err = "print \"Value must be a positive integer between 0 and 63.\n\"";
			return qfalse;
		}
		i = atoi( value );
		if ( i < 0 ) {
			*err = "print \"Value must be a positive integer between 0 and 63.\n\"";
			return qfalse;
		}
		if ( i >= MAX_CLIENTS ) {
			*err = "print \"Value must be a positive integer between 0 and 63.\n\"";
			return qfalse;
		}
		ent = &g_entities[i];
		if ( !ent->inuse || !ent->client || ent->client->sess.invisible ) {
			*err = "print \"Client slot is unused.\n\"";
			return qfalse;
		}
		if ( ent->client->sess.referee ) {
			*err = "print \"You cannot do this to a referee.\n\"";
			return qfalse;
		}
	}
	else if ( !Q_stricmp( cmd, "fraglimit" ) || !Q_stricmp( cmd, "timelimit" ) ||
			!Q_stricmp( cmd, "capturelimit" ) || !Q_stricmp( cmd, "g_railjumpknock" ) ) {
		i = atoi( value );
		if ( i < 0 ) {
			*err = "print \"Value must be a positive integer.\n\"";
			return qfalse;
		}
		if ( value[0] < '0' || value[0] > '9' ) {
			*err = "print \"Value must be a positive integer.\n\"";
			return qfalse;
		}
	}
	else if ( !Q_stricmp( cmd, "g_doWarmup" ) || !Q_stricmp( cmd, "g_friendlyFire" ) || 
			!Q_stricmp( cmd, "g_grapple" )) {
		if ( strlen( value ) == 0 || strlen( value ) > 1 ) {
			*err = "print \"Valid values are 0 and 1.\n\"";
			return qfalse;
		}
		if ( value[0] != '0' && value[0] != '1' ) {
			*err = "print \"Valid values are 0 and 1.\n\"";
			return qfalse;
		}
	}
	else if ( !Q_stricmp( cmd, "map_restart" ) || !Q_stricmp( cmd, "nextmap" ) ) {
		if ( strlen( value ) > 0 ) {
			*err = "print \"This command requires no argument.\n\"";
			return qfalse;
		}
	}
	else if ( !Q_stricmp( cmd, "exec" ) ) {
		// make sure they don't go back or forward a directory or two
		if ( strstr( value, "." ) || strchr( value, '\\' ) || strchr( value, '/' ) ) {
			*err = "print \"Invalid configuration file name.\n\"";
			return qfalse;
		}
	}

	return qtrue;
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int			i;
	char		arg1[MAX_STRING_TOKENS];
	char		arg2[MAX_STRING_TOKENS];
	const char	*cmd;
	char		*err;

	if ( nVoteVars != nVoteVarMap ) {
		trap_SendServerCommand( ent-g_entities, "print \"Sanity check failed: nVoteVars != nVoteVarMap\n\"" );
		return;
	}

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}

	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && !ent->client->sess.referee ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	cmd = GetVoteVar( arg1 );
	if ( cmd[0] == '\0' ) {
		PrintVoteHelp( ent );
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
//freeze
		SaveGameState();
//freeze
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}

	// if there's no arg2, try to print some specific vote help
	if ( arg2[0] == '\0' && PrintSpecificVoteHelp( ent, cmd ) ) {
		return;
	}

	if ( !CheckValid( cmd, arg2, &err ) ) {
		trap_SendServerCommand( ent-g_entities, err );
		return;
	}

	// set the vote string
	if ( !Q_stricmp( cmd, "map" ) ) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", cmd, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", cmd, arg2 );
		}
	}
	else if ( !Q_stricmp( cmd, "nextmap" ) ) {
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
	}
	else if ( !Q_stricmp( cmd, "opinion" ) ) {
		level.voteString[0] = '\0';
	}
	else if ( !Q_stricmp( cmd, "exec" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"votecfgs/%s.cfg\"", cmd, arg2 );
	}
	else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", cmd, arg2 );
	}

	// set the vote display string
	if ( !Q_stricmp( cmd, "g_gametype" ) ) {
		i = atoi( arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", cmd, gameNames[i] );
	}
	else if ( !Q_stricmp( cmd, "clientkick" ) || !Q_stricmp( cmd, "putblue" ) ||
			!Q_stricmp( cmd, "putred" ) || !Q_stricmp( cmd, "remove" ) ) {
		int i = atoi( arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, g_entities[i].client->pers.netname );
	}
	else if ( !Q_stricmp( cmd, "opinion" ) ) {
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "opinion: %s", ConcatArgs(2) );
	}
	else {
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, arg2 );
	}

	G_SendServerCommand( -1, va("print \"%s called a vote.\n\"", ent->client->pers.netname ) ); //ssdemo

	// mark all clients as not voted
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}

	// start the voting
	if ( level.time - level.startTime < 30000 ) {
		level.voteTime = level.time + (30000 - (level.time - level.startTime));
	}
	else {
		level.voteTime = level.time;
	}

	// the caller automatically votes yes if not a referee
	if ( !ent->client->sess.referee ) {
		level.voteYes = 1;
		ent->client->ps.eFlags |= EF_VOTED;
	}
	else {
		level.voteYes = 0;
	}

	level.voteNo = 0;

	G_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) ); //ssdemo
	G_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString ); //ssdemo
	G_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) ); //ssdemo
	G_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) ); //ssdemo
}
//freeze

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent, qboolean ref ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote already cast.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && !ref ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
//freeze
		if ( ref ) {
			level.voteYes += 100;
		}
		else {
//freeze
			level.voteYes++;
		}
		G_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) ); //ssdemo
	} else {
//freeze
		if ( ref ) {
			level.voteNo += 100;
		}
		else {
//freeze
			level.voteNo++;
		}
		G_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) ); //ssdemo
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, team, cs_offset;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, "print \"A team vote is already in progress.\n\"" );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, "print \"You have called the maximum number of team votes.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	G_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) ); //ssdemo
	G_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] ); //ssdemo
	G_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) ); //ssdemo
	G_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) ); //ssdemo
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, "print \"No team vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Team vote already cast.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Team vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		G_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) ); //ssdemo
	} else {
		level.teamVoteNo[cs_offset]++;
		G_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) ); //ssdemo
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}


//freeze
void Cmd_RegisterCheat_f( gentity_t *ent ) {
	char		var[MAX_TOKEN_CHARS];
	char		val[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 3 ) {
		G_LogPrintf( "Cheat: received bunk registercheat command: not enough arguments\n" );
		return;
	}

	if ( !ent->client ) {
		G_LogPrintf( "Cheat: received bunk registercheat command: no client record\n" );
		return;
	}

	trap_Argv( 1, var, sizeof( var ) );
	trap_Argv( 2, val, sizeof( val ) );

	G_LogPrintf( "Cheat: %i %s at %s: %s %s\n", ent->s.number, ent->client->pers.netname, ent->client->sess.ip, var, val );

	// are we on the internet?
	if ( !ent->client->sess.privateNetwork ) {
		// yeah, so let everyone know, including the cheater
		G_SendServerCommand( -1, va("cp \"%s " S_COLOR_WHITE "is a cheater:\n%s %s\n\"", ent->client->pers.netname, var, val ) ); //ssdemo
		G_SendServerCommand( -1, va("print \"%s " S_COLOR_WHITE "is a cheater:\n%s %s\n\"", ent->client->pers.netname, var, val ) ); //ssdemo
	}
	else {
		// no, so let everyone but the cheater know
		gentity_t *iter;

		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			if ( !iter->inuse || !iter->client || iter == ent ) {
				continue;
			}

			trap_SendServerCommand( iter - g_entities, va("cp \"%s " S_COLOR_WHITE "is a cheater:\n%s %s\n\"", ent->client->pers.netname, var, val ) );
			trap_SendServerCommand( iter - g_entities, va("print \"%s " S_COLOR_WHITE "is a cheater:\n%s %s\n\"", ent->client->pers.netname, var, val ) );
		}
	}
}

void Cmd_RegisterStealthCheat_f( gentity_t *ent ) {
	gentity_t *iter;
	char		var[MAX_TOKEN_CHARS];
	char		val[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 3 ) {
		G_LogPrintf( "Cheat: received bunk registercheat command: not enough arguments\n" );
		return;
	}

	if ( !ent->client ) {
		G_LogPrintf( "Cheat: received bunk registercheat command: no client record\n" );
		return;
	}

	trap_Argv( 1, var, sizeof( var ) );
	trap_Argv( 2, val, sizeof( val ) );

	G_LogPrintf( "Cheat: %i %s at %s: %s %s\n", ent->s.number, ent->client->pers.netname, ent->client->sess.ip, var, val );

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || !iter->client->sess.referee || iter == ent ) continue;

		trap_SendServerCommand( iter - g_entities, va("rn \"^1Referee Notification\n^7%s " S_COLOR_WHITE "may be a cheater:\n%s %s\n\"", ent->client->pers.netname, var, val ) );
		trap_SendServerCommand( iter - g_entities, va("print \"^1Referee Notification\n^7%s " S_COLOR_WHITE "may be a cheater: %s %s\n\"", ent->client->pers.netname, var, val ) );
	}
}

//freeze


/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

//ssdemo
	if ( level.inDemoFile != 0 && G_CantExecDuringDemo(cmd) ) {
		trap_SendServerCommand( ent-g_entities, "print \"You cannot execute this command during demo playback.\n\"" );
		return;
	}
//ssdemo

	if (Q_stricmp (cmd, "say") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Tell_f ( ent );
		return;
	}
	if (Q_stricmp (cmd, "vsay") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_VoiceTaunt_f ( ent );
		return;
	}
	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}
//freeze
	if (Q_stricmp (cmd, "me") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f (ent, SAY_EMOTE, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "me_team") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f (ent, SAY_EMOTETEAM, qfalse);
		return;
	}
	if ( Q_stricmp( cmd, "ignore" ) == 0 ) {
		Cmd_Ignore_f( ent, qtrue );
		return;
	}
	if ( Q_stricmp( cmd, "unignore" ) == 0 ) {
		Cmd_Ignore_f( ent, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "stats" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Stats_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsacc" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsAcc_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsblue" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsred" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsall" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsAll_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "topshots" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Topshots_f( ent, qtrue );
		return;
	}
	if ( Q_stricmp( cmd, "bottomshots" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Topshots_f( ent, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "statsteam" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsTeam_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsteamred" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsTeamRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsteamblue" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsTeamBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "statsteamall" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_StatsTeamAll_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "clients" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Clients_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "lock" ) == 0 ) {
		Cmd_Lock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "unlock" ) == 0 ) {
		Cmd_Unlock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref" ) == 0 ) {
		Cmd_Ref_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "unref" ) == 0 ) {
		Cmd_Unref_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_kick" ) == 0 ) {
		Cmd_RefKick_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_remove" ) == 0 ) {
		Cmd_RefRemove_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_putred" ) == 0 ) {
		Cmd_RefPutRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_putblue" ) == 0 ) {
		Cmd_RefPutBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_putbluecoach" ) == 0 ) {
		Cmd_RefPutBlueCoach_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_putredcoach" ) == 0 ) {
		Cmd_RefPutRedCoach_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_swap" ) == 0 ) {
		Cmd_RefSwap_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_makeref" ) == 0 ) {
		Cmd_RefMakeRef_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_lock" ) == 0 ) {
		Cmd_RefLock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_lockblue" ) == 0 ) {
		Cmd_RefLockBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_lockred" ) == 0 ) {
		Cmd_RefLockRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_unlock" ) == 0 ) {
		Cmd_RefUnlock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_unlockblue" ) == 0 ) {
		Cmd_RefUnlockBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_unlockred" ) == 0 ) {
		Cmd_RefUnlockRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_speclock" ) == 0 ) {
		Cmd_RefSpecLock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_speclockblue" ) == 0 ) {
		Cmd_RefSpecLockBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_speclockred" ) == 0 ) {
		Cmd_RefSpecLockRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_specunlock" ) == 0 ) {
		Cmd_RefSpecUnlock_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_specunlockblue" ) == 0 ) {
		Cmd_RefSpecUnlockBlue_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_specunlockred" ) == 0 ) {
		Cmd_RefSpecUnlockRed_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_ban" ) == 0 ) {
		Cmd_RefBan_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_mute" ) == 0 ) {
		Cmd_RefMute_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_clients" ) == 0 ) {
		Cmd_RefClients_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_restart" ) == 0 ) {
		Cmd_RefRestart_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_map" ) == 0 ) {
		Cmd_RefMap_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_nextmap" ) == 0 ) {
		Cmd_RefNextmap_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_invisible" ) == 0 ) {
		Cmd_RefInvisible_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_wallhack" ) == 0 ) {
		Cmd_RefWallhack_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "coach" ) == 0 ) {
		Cmd_Coach_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "coachinvite" ) == 0 ) {
		Cmd_CoachInvite_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "say_ref" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f( ent, SAY_REF, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_red" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f( ent, SAY_RED, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_blue" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f( ent, SAY_BLUE, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "ref_addip" ) == 0 ) {
		Cmd_RefAddIP_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_removeip" ) == 0 ) {
		Cmd_RefRemoveIP_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_listip" ) == 0 ) {
		Cmd_RefListIP_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "registercheat" ) == 0 ) {
		Cmd_RegisterCheat_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "uinfo" ) == 0 ) {
//		G_LogPrintf( "Uinfo: %d (%s requested new userinfo string be sent because of a mismatch)\n", ent->s.number, ent->client->pers.netname );
		ClientUserinfoChanged( ent->s.number );
		return;
	}
	if ( Q_stricmp( cmd, "freezeangles" ) == 0 ) {
		ent->client->ps.stats[STAT_FLAGS] |= SF_FREEZEANGLES;
		return;
	}
	if ( Q_stricmp( cmd, "unfreezeangles" ) == 0 ) {
		ent->client->ps.stats[STAT_FLAGS] &= ~SF_FREEZEANGLES;
		return;
	}
//freeze
//multiview
	if ( Q_stricmp( cmd, "pipreq" ) == 0 ) {
		char req[MAX_STRING_CHARS];
		trap_Argv( 1, req, sizeof(req) );
		G_ParsePipWindowRequest( ent, req );
		return;
	}
//multiview

//anticheat
	{
		char key[8], lastkey[8];
		strcpy( key, BG_IntToCheatKey( level.cheatkey, ent->client->ps.stats[STAT_FLAGS] ) );
		strcpy( lastkey, BG_IntToCheatKey( level.lastcheatkey, ent->client->ps.stats[STAT_FLAGS] ) );

		if ( Q_stricmp( cmd, key ) == 0 || Q_stricmp( cmd, lastkey ) == 0 ) {
			Cmd_RegisterStealthCheat_f( ent );
			return;
		}
	}
//anticheat
	
	if ( Q_stricmp( cmd, "autofollow" ) == 0 ) {
		Cmd_Autofollow_f( ent );
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {		
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Say_f (ent, qfalse, qtrue);
		return;
	}

	if (Q_stricmp (cmd, "give") == 0) {
		Cmd_Give_f (ent);
		return;
	}
//freeze
	if (Q_stricmp (cmd, "giveall") == 0) {
		Cmd_GiveAll_f (ent);
		return;
	}
//freeze
	if (Q_stricmp (cmd, "god") == 0) {
		Cmd_God_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "notarget") == 0) {
		Cmd_Notarget_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "noclip") == 0) {
		Cmd_Noclip_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "kill") == 0) {
		Cmd_Kill_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "teamtask") == 0) {
		Cmd_TeamTask_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "levelshot") == 0) {
		Cmd_LevelShot_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "follow") == 0) {
		Cmd_Follow_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "follownext") == 0) {
		Cmd_FollowCycle_f (ent, 1);
		return;
	}
	if (Q_stricmp (cmd, "followprev") == 0) {
		Cmd_FollowCycle_f (ent, -1);
		return;
	}
	if (Q_stricmp (cmd, "team") == 0) {
		Cmd_Team_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "where") == 0) {
		Cmd_Where_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "callvote") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_CallVote_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "vote") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Vote_f (ent, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "callteamvote") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_CallTeamVote_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "teamvote") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_TeamVote_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "gc") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_GameCommand_f( ent );
		return;
	}
	if (Q_stricmp (cmd, "setviewpos") == 0) {
		Cmd_SetViewpos_f( ent );
		return;
	}
//freeze
	if (Q_stricmp (cmd, "ref_vote") == 0) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_RefVote_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "drop" ) == 0 ) {
		Cmd_Drop_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ready" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Ready_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "notready" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_NotReady_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "timein" ) == 0 || Q_stricmp( cmd, "unpause" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_TimeIn_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "timeout" ) == 0 || Q_stricmp( cmd, "pause" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_TimeOut_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_allready" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_RefAllReady_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_allnotready" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_RefAllNotReady_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_timein" ) == 0 || Q_stricmp( cmd, "ref_unpause" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_RefTimein_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "ref_timeout" ) == 0 || Q_stricmp( cmd, "ref_pause" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_RefTimeout_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "help" ) == 0 ) {
		if ( !G_CheckFPCmd(ent) ) return;
		Cmd_Help_f( ent );
		return;
	}
//freeze

	trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
}

// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

// g_client.c -- client functions that don't happen every frame

vec3_t	playerMins = {-15, -15, -24};
vec3_t	playerMaxs = {15, 15, 32};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
PointWouldTelefrag

================
*/
qboolean PointWouldTelefrag( vec3_t origin ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( origin, playerMins, mins );
	VectorAdd( origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( gentity_t *ent, vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;
	int			team = (ent == NULL ? TEAM_SPECTATOR : ent->client->sess.sessionTeam);

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( gentity_t *ent, vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	team_t team = (ent == NULL ? TEAM_SPECTATOR : ent->client->sess.sessionTeam);
//freeze
	if ( g_gametype.integer == GT_TEAM && (team == TEAM_RED || team == TEAM_BLUE) ) {
		return SelectBestFreezeSpawnPoint( ent, avoidPoint, origin, angles );
	}
	else {
//freeze
		return SelectRandomFurthestSpawnPoint( ent, avoidPoint, origin, angles );
	}

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( ent, vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > 6500 ) {
		// the body ques are never actually freed, they are just unlinked
		G_UnlinkEntity( ent ); //ssdemo
		ent->physicsObject = qfalse;
		return;	
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
#ifdef MISSIONPACK
	gentity_t	*e;
	int i;
#endif
	gentity_t		*body;
	int			contents;

	G_UnlinkEntity (ent); //ssdemo

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	G_UnlinkEntity (body); //ssdemo

	body->s = ent->s;
	body->s.eFlags = 0;		// clear EF_TALK, etc
#ifdef MISSIONPACK
	if ( ent->s.eFlags & EF_KAMIKAZE ) {
		body->s.eFlags |= EF_KAMIKAZE;

		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < MAX_GENTITIES; i++) {
			e = &g_entities[i];
			if (!e->inuse)
				continue;
			if (e->activator != ent)
				continue;
			if (strcmp(e->classname, "kamikaze timer"))
				continue;
			e->activator = body;
			break;
		}
	}
#endif
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	G_LinkEntity (body); //ssdemo
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	gentity_t	*tent;

//freeze
	// if not intermission time and we're frozen
	if ( !level.intermissiontime && ent->freezeState ) {
		// if a bot
		if ( ent->r.svFlags & SVF_BOT ) {
			// don't let them respawn normally
			ent->client->respawnTime = INT_MAX;
		} else if ( !is_spectator( ent->client ) ) {
			// not a bot
			vec3_t origin, angles;

			// save the origin
			VectorCopy( ent->r.currentOrigin, origin );

			// save the angles
			angles[ YAW ] = ent->client->ps.stats[ STAT_DEAD_YAW ];
			angles[ PITCH ] = 0;
			angles[ ROLL ] = 0;

			// spawn normally
			ClientSpawn( ent );

			// set the origin and angles back
			VectorCopy( origin, ent->client->ps.origin );
			SetClientViewAngle( ent, angles );

			// set as a spectator to the client
			ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;

			// set as a free spectator
			ent->client->sess.spectatorTime = level.time;
			ent->client->sess.spectatorState = SPECTATOR_FREE;
			ent->client->sess.spectatorClient = 0;
		}

		// unlink so we don't flash in for one frame
		G_UnlinkEntity( ent ); //ssdemo
	}
	else {
//freeze
		CopyToBodyQue (ent);
		ClientSpawn(ent);

		// add a teleportation effect
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] && !level.redlocked ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] && !level.bluelocked ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] && !level.redlocked ) {
		return TEAM_RED;
	}
	else if ( !level.bluelocked ) {
		return TEAM_BLUE;
	}

	return TEAM_SPECTATOR;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int		len, colorlessLen;
	char	ch;
	char	*p;
	int		spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}

		// don't allow leading spaces
		if( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if( !*in ) {
				break;
			}

			// don't allow black in a name, period
			if( ColorIndex(*in) == 0 ) {
				in++;
				continue;
			}

			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if( ch == ' ' ) {
			spaces++;
			if( spaces > 3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "UnnamedPlayer", outSize );
	}
}
*/

static qboolean IsIllegalChar( char ch ) {
	return ( ch == 34 || ch == 59 || ch == 92 );
}

static void ClientCleanName( const char *in, char *out, int outSize ) {
	char *p, *firstchar, *firstcolor, *lastchar;
	int i;
	char copy[MAX_STRING_CHARS];

	Q_strncpyz( copy, in, sizeof(copy) );

	firstchar = firstcolor = lastchar = NULL;

	// leave room for the null
	outSize--;

	// look for illegal characters
	for ( p = (char *)in; *p != '\0'; p++ ) {
		if ( IsIllegalChar(*p) ) {
			Q_strncpyz( out, "UnnamedHacker", outSize );
			return;
		}
	}

	// look for the first and last actual characters
	for ( p = (char *)copy; *p != '\0'; p++ ) {
		// if it's a caret
		if ( *p == Q_COLOR_ESCAPE && *(p + 1) != '\0' ) {
			// if we haven't found the first non-space character
			if ( firstchar == NULL ) {
				firstcolor = p;
			}

			// skip the color character and continue
			p++;
			continue;
		}

		// if it's a space, ignore it
		if ( *p == ' ' ) {
			continue;
		}

		// if we haven't found the first character - here it is!
		if ( firstchar == NULL ) {
			firstchar = p;
		}

		// this will be left on the last non-space character at the end
		lastchar = p;
	}

	// check for an empty name
	if ( firstchar == NULL ) {
		// did they try to make a blank one?
		if ( firstcolor != NULL ) {
			// yeah, so call them a name
			Q_strncpyz( out, "UnnamedImbecile", outSize );
			return;
		}
		else {
			// no, so just unname them
			Q_strncpyz( out, "UnnamedPlayer", outSize );
			return;
		}
	}

	// the out counter
	i = 0;

	// is there a leading color?
	if ( firstcolor != NULL ) {
		// yeah, so fill it in
		out[i++] = firstcolor[0];
		out[i++] = firstcolor[1];
	}

	// add to out from the first character to the last
	for ( p = firstchar; p <= lastchar && i < outSize; p++, i++ ) {
		out[i] = *p;		
	}

	// close it off
	out[i] = '\0';

	// fail-safe - this shouldn't happen
	if ( out[0] == '\0' ) {
		// yeah, so just unname them
		Q_strncpyz( out, "UnnamedPlayer", outSize );
		return;
	}
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	char	*s;
	char	model[MAX_QPATH];
	char	headModel[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	char	redTeam[MAX_INFO_STRING];
	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
//anticheat
	static int fakeTeam = TEAM_RED;

	if ( fakeTeam == TEAM_RED ) {
		fakeTeam = TEAM_BLUE;
	}
	else {
		fakeTeam = TEAM_RED;
	}
//anticheat

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->sess.localClient = qtrue;
//freeze
		Q_strncpyz( client->sess.ip, s, sizeof(client->sess.ip) );
		G_LogPrintf( "IP_Addr: %d %s\n", clientNum, client->sess.ip );
//freeze
	}
//freeze
	else if ( s[0] >= '0' && s[0] <= '9' ) {
		char *colon;

		Q_strncpyz( client->sess.ip, s, sizeof(client->sess.ip) );

		colon = strchr( client->sess.ip, ':' );
		if ( colon != NULL ) {
			*colon = '\0';
		}

		if ( Q_strncmp( client->sess.ip, "172.21.", 7 ) == 0 ||
				Q_strncmp( client->sess.ip, "192.168.", 8 ) == 0 ||
				Q_strncmp( client->sess.ip, "10.", 3 ) == 0 ||
				Q_strncmp( client->sess.ip, "127.", 4 ) == 0 ) {
			client->sess.privateNetwork = qtrue;
		}

		G_LogPrintf( "IP_Addr: %d %s\n", clientNum, client->sess.ip );
	}
//freeze

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

//unlagged - client options
	// see if the player has opted out
	s = Info_ValueForKey( userinfo, "cg_delag" );
	if ( !atoi( s ) ) {
		client->pers.delag = 0;
	} else {
		client->pers.delag = atoi( s );
	}

	// see if the player is nudging his shots
	s = Info_ValueForKey( userinfo, "cg_cmdTimeNudge" );
	client->pers.cmdTimeNudge = atoi( s );

	// see if the player wants to debug the backward reconciliation
	s = Info_ValueForKey( userinfo, "cg_debugDelag" );
	if ( !atoi( s ) ) {
		client->pers.debugDelag = qfalse;
	}
	else {
		client->pers.debugDelag = qtrue;
	}

	// see if the player is simulating incoming latency
	s = Info_ValueForKey( userinfo, "cg_latentSnaps" );
	client->pers.latentSnaps = atoi( s );

	// see if the player is simulating outgoing latency
	s = Info_ValueForKey( userinfo, "cg_latentCmds" );
	client->pers.latentCmds = atoi( s );

	// see if the player is simulating outgoing packet loss
	s = Info_ValueForKey( userinfo, "cg_plOut" );
	client->pers.plOut = atoi( s );
//unlagged - client options

//freeze
	s = Info_ValueForKey( userinfo, "cg_noStats" );
	if ( atoi( s ) ) {
		client->pers.noStats = qtrue;
	}

	if ( g_refAutologin.integer ) {
		s = Info_ValueForKey( userinfo, "ref_login" );
		if ( g_refPassword.string[0] && Q_stricmp(g_refPassword.string, "none") && !Q_stricmp(g_refPassword.string, s) ) {
			client->sess.referee = qtrue;
			client->sess.guestref = qfalse;
		}
	}
//freeze

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_REDCOACH || client->sess.sessionTeam == TEAM_BLUECOACH ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) {
			G_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname, 
				client->pers.netname) ); //ssdemo
		}
	}

	// set max health
#ifdef MISSIONPACK
	if (client->ps.powerups[PW_GUARD]) {
		client->pers.maxHealth = 200;
	} else {
		health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
		client->pers.maxHealth = health;
		if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
			client->pers.maxHealth = 100;
		}
	}
#else
	health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
#endif
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

//pmskins - these are never used as it is, and they only confuse people
	/*
	// set model
	if( g_gametype.integer >= GT_TEAM ) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "team_model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "team_headmodel"), sizeof( headModel ) );
	} else {
	*/
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );
	//}
//pmskins

	// bots set their team a few frames later
/*freeze
	if (g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
	}
	else {
*/
		team = client->sess.sessionTeam;
//	}

//anticheat
	team = BG_HashTeam( team, client->pers.netname );
//anticheat

#ifdef MISSIONPACK
	if (g_gametype.integer >= GT_TEAM) {
		client->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		} else {
			client->pers.teamInfo = qfalse;
		}
	}
#else
	// teamInfo
	s = Info_ValueForKey( userinfo, "teamoverlay" );
	if ( ! *s || atoi( s ) != 0 ) {
		client->pers.teamInfo = qtrue;
	} else {
		client->pers.teamInfo = qfalse;
	}
#endif

	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	client->pers.pmoveFixed = atoi( s );

//pmove_accurate
	s = Info_ValueForKey( userinfo, "cg_pmove_accurate" );
	client->pers.pmoveAccurate = atoi( s );
//pmove_accurate

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));

	strcpy(redTeam, Info_ValueForKey( userinfo, "g_redteam" ));
	strcpy(blueTeam, Info_ValueForKey( userinfo, "g_blueteam" ));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\"TEAM_KEY"\\%i\\model\\%s\\hmodel\\%s\\"FAKE_TEAM_KEY"\\%i\\"HANDICAP_KEY"\\%i\\"COLOR1_KEY"\\%s\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d\\"COLOR2_KEY"\\%s",
			client->pers.netname, team, model, headModel, fakeTeam, client->pers.maxHealth, c1, 
			client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader, c2 );
	} else {
		if ( client->sess.invisible ) {
			s = "";
		}
		else {
			s = va("n\\%s\\"TEAM_KEY"\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\"FAKE_TEAM_KEY"\\%i\\"HANDICAP_KEY"\\%i\\"COLOR1_KEY"\\%s\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\"COLOR2_KEY"\\%s",
				client->pers.netname, team, model, headModel, redTeam, blueTeam, fakeTeam,
				client->pers.maxHealth, c1, client->sess.wins, client->sess.losses, teamTask, teamLeader, c2 );
		}
	}

	G_SetConfigstring( CS_PLAYERS+clientNum, s ); //ssdemo

	// log a different subset of the userinfo keys so we don't screw up log parsers
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			client->pers.netname, team, model, headModel, c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader );
	} else {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
			client->pers.netname, client->sess.sessionTeam, model, headModel, redTeam, blueTeam, c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader);
	}

	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;

//ssdemo
	// do not let more than 32 clients join
	if ( clientNum >= HALF_CLIENTS ) {
		return "This server cannot serve more than 32 clients.";
	}
//ssdemo

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

 	// IP filtering
 	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if ( G_FilterPacket( value ) ) {
		return "You are banned from this server.";
	}

  // we don't check password for bots and local client
  // NOTE: local client <-> "ip" "localhost"
  //   this means this client is not running in our current process
	if ( !( ent->r.svFlags & SVF_BOT ) && (strcmp(value, "localhost") != 0)) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo );
	}
	G_ReadSessionData( client );
//freeze
	if ( g_gametype.integer != GT_TOURNAMENT ) {
		client->sess.wins = 0;
	}
	ent->freezeState = qfalse;
	ent->readyBegin = qfalse;
	ent->s.eFlags |= EF_CONNECTION;
//freeze

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		G_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) ); //ssdemo
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

//unlagged - backward reconciliation #5
	// announce it
	if ( g_delagHitscan.integer ) {
		trap_SendServerCommand( clientNum, "print \"This server is Unlagged: full lag compensation is ON!\n\"" );
	}
	else {
		trap_SendServerCommand( clientNum, "print \"This server is Unlagged: full lag compensation is OFF!\n\"" );
	}
//unlagged - backward reconciliation #5

//freeze
	client->ps.persistant[PERS_TOTALFROZEN] = 0;
	client->connectTime = level.time;
//freeze

	return NULL;
}

static int NumPlayersOnTeam( team_t team ) {
	gentity_t *ent;
	int total = 0;

	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( ent->inuse && ent->client && ent->client->sess.sessionTeam == team ) {
			total++;
		}
	}

	return total;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum, qboolean newplayer ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags;
//freeze
	int score, time, frozen;
//freeze

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		G_UnlinkEntity( ent ); //ssdemo
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
//freeze
	if ( newplayer ) {
//freeze
		client->pers.enterTime = level.time;
	}
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
//freeze
	if ( g_gametype.integer >= GT_TEAM ) {
		score = client->ps.persistant[PERS_SCORE];
		frozen = client->ps.persistant[PERS_TOTALFROZEN];
		time = client->pers.enterTime;
	}
	else {
		score = 0;
	}
//freeze
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

//freeze
	client->ps.persistant[PERS_SCORE] = score;
	client->ps.persistant[PERS_TOTALFROZEN] = frozen;
	client->pers.enterTime = time;
//freeze

	// locate ent at a spawn point
	ClientSpawn( ent );

//ssdemo
	if ( level.inDemoFile == 0 ) {
//ssdemo
		// if players are to start frozen AND we're doing warmups AND we're not in warmup time AND
		// it's been more than 1 second since the map started AND the player is on a team AND
		// the number of players on that team is greater than 1 (we can't have a player starting
		// frozen on an empty team)
		if ( g_startFrozen.integer && g_doWarmup.integer && !level.warmupTime &&
				level.time - level.startTime > 1000 && 
				(ent->client->sess.sessionTeam == TEAM_RED || ent->client->sess.sessionTeam == TEAM_BLUE) &&
				NumPlayersOnTeam( ent->client->sess.sessionTeam ) > 1 ) {
			// keep track of the old weapon
			int oldweapon = ent->s.weapon;
			// set the player's weapon to none
			ent->s.weapon = WP_NONE;
			// try to freeze him
			player_freeze( ent, ent, MOD_KILL );
			// if he's not frozen
			if ( !ent->freezeState ) {
				// set his weapon back
				ent->s.weapon = oldweapon;
			}
			else {
				// otherwise, make the freezing sound
				G_AddEvent( ent, EV_FREEZING, 0 );
			}
		}

		if ( client->sess.sessionTeam != TEAM_SPECTATOR || client->sess.sessionTeam != TEAM_REDCOACH || client->sess.sessionTeam != TEAM_BLUECOACH ) {
			// send event
			tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
			tent->s.clientNum = ent->s.clientNum;

			if ( g_gametype.integer != GT_TOURNAMENT  ) {
				G_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) ); //ssdemo
			}
		}
//ssdemo
	}
//ssdemo
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	int		flags;
	int		savedPing;
//	char	*savedAreaBits;
	int		accuracy_hits, accuracy_shots;
	int		eventSequence;
	char	userinfo[MAX_INFO_STRING];
//freeze
	int		savedConnectTime;
//freeze

	index = ent - g_entities;
	client = ent->client;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_BLUECOACH || client->sess.sessionTeam == TEAM_REDCOACH ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF ) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( 
						ent, client->pers.teamState.state, 
						spawn_origin, spawn_angles);
	} else {
//		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->sess.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( ent, spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint ( 
					ent, client->ps.origin, 
					spawn_origin, spawn_angles);
			}
//freeze
// This is just silly.  A much better place to put this is in the individual
// spawn point-choosing functions to avoid an infinite loop.
/*
			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
*/
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// always clear the kamikaze flag
	//ent->s.eFlags &= ~EF_KAMIKAZE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

//unlagged - backward reconciliation #3
	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
	// and this is as good a time as any to clear the saved state
	ent->saved.leveltime = 0;
//unlagged - backward reconciliation #3

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	savedConnectTime = client->connectTime;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->connectTime = savedConnectTime;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	client->pers.maxHealth = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_MACHINEGUN );
/*freeze
	if ( g_gametype.integer == GT_TEAM ) {
		client->ps.ammo[WP_MACHINEGUN] = 50;
	} else {
freeze*/
		client->ps.ammo[WP_MACHINEGUN] = ammo_mg.integer;
//freeze	}

	client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GAUNTLET );
	client->ps.ammo[WP_GAUNTLET] = -1;
//	client->ps.ammo[WP_GRAPPLING_HOOK] = -1;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] + 25;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

/*freeze
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
freeze*/
	if ( is_spectator( client ) ) {
//freeze

	} else {
		G_KillBox( ent );
		G_LinkEntity (ent); //ssdemo

		// force the base weapon up
		client->ps.weapon = WP_MACHINEGUN;
		client->ps.weaponstate = WEAPON_READY;

//freeze
		SpawnWeapon( client );
//freeze
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

//freeze
	// don't allow firing for a bit
	client->ps.weaponTime = g_noAttackTime.value * 1000;
//freeze

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
//freeze
		if ( !( g_dmflags.integer & 1024 ) )
//freeze
		G_UseTargets( spawnPoint, ent );

		// select the highest weapon number available, after any
		// spawn given items have fired
		client->ps.weapon = 1;
		for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
			if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
				client->ps.weapon = i;
				break;
			}
		}
//freeze
		if ( client->ps.stats[ STAT_WEAPONS ] & ( 1 << WP_ROCKET_LAUNCHER ) ) {
			client->ps.weapon = WP_ROCKET_LAUNCHER;
		}

		if ( g_startArmor.integer > 0 ) {
			client->ps.stats[ STAT_ARMOR ] += g_startArmor.integer;
			if ( client->ps.stats[ STAT_ARMOR ] > client->ps.stats[ STAT_MAX_HEALTH ] * 2 ) {
				client->ps.stats[ STAT_ARMOR ] = client->ps.stats[ STAT_MAX_HEALTH ] * 2;
			}
		}
//freeze
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
/*freeze
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
freeze*/
	if ( !is_spectator( client ) ) {
//freeze
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		G_LinkEntity( ent ); //ssdemo
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( is_spectator( &level.clients[ i ] ) &&
				(level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW ||
				level.clients[i].sess.spectatorState == SPECTATOR_FROZEN) &&
				level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED &&
			!is_spectator( ent->client ) ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
#ifdef MISSIONPACK
		TossClientPersistantPowerups( ent );
		if( g_gametype.integer == GT_HARVESTER ) {
			TossClientCubes( ent );
		}
#endif

	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == clientNum ) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );
	}

	G_UnlinkEntity (ent); //ssdemo
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	G_SetConfigstring( CS_PLAYERS + clientNum, ""); //ssdemo

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}
}



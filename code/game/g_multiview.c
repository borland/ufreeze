#include "g_local.h"
#include "g_multiview.h"

static void G_AddPipPortal( ssPipWindow_t *pip, int clientNum ) {
	// don't set up a new portal if there already is one
	if ( pip->portal != NULL ) {
		return;
	}

	// if playing a demo
	if ( level.inDemoFile != 0 ) {
		// spawn from the end of the list to cut down on collisions with demo entities
		pip->portal = G_SpawnReverse();
	}
	else {
		// otherwise spawn normally
		pip->portal = G_Spawn();
	}

	//G_Printf("spawned portal at %d, entity number %d\n", level.time, pip->portal - g_entities);

	if ( pip->portal != NULL ) {
		pip->portal->r.svFlags = SVF_PORTAL | SVF_CLIENTMASK;
		pip->portal->r.singleClient = (1 << clientNum);
		pip->portal->s.eType = ET_INVISIBLE;
		VectorClear( pip->portal->r.mins );
		VectorClear( pip->portal->r.maxs );

		pip->portal->reference = &pip->portal;

		// we won't link or set origins until frame end
	}
}

static void G_AddPip( gentity_t *ent, int clientNum ) {
	int i;

	if ( !CanFollow(ent, &g_entities[clientNum]) ) {
		trap_SendServerCommand( ent - g_entities, va("badpip %d\n", clientNum) );
		return;
	}

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		ssPipWindow_t *pip = &ent->client->pers.ssPipWindows[i];
		if ( pip->valid && pip->clientNum == clientNum ) {
			pip->frame = level.framenum;
			return;
		}
	}

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		ssPipWindow_t *pip = &ent->client->pers.ssPipWindows[i];
		if ( !pip->valid ) {
			pip->valid = qtrue;
			pip->clientNum = clientNum;
			pip->snapshot = qfalse;
			pip->frame = level.framenum;
			G_AddPipPortal( pip, ent - g_entities );
			return;
		}
	}
}

static void G_RemovePip( gentity_t *ent, int clientNum ) {
	int i;

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		ssPipWindow_t *pip = &ent->client->pers.ssPipWindows[i];
		if ( pip->valid && pip->clientNum == clientNum ) {
			if ( pip->portal != NULL ) {
				G_FreeEntity( pip->portal );
			}
			memset( pip, 0, sizeof(ssPipWindow_t) );
			return;
		}
	}
}

void G_ParsePipWindowRequest( gentity_t *ent, const char *str ) {
	int i;

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		char *s = Info_ValueForKey( str, va("%d", i) );

		if ( s && s[0] != '\0' ) {
			G_AddPip( ent, atoi(s) );
		}
	}

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		ssPipWindow_t *pip = &ent->client->pers.ssPipWindows[i];
		if ( pip->valid && pip->frame != level.framenum ) {
			G_RemovePip( ent, pip->clientNum );
		}
	}
}

/*
static void G_SerializePlayerData( playerData_t *pd, char *ret, int *size ) {
	void *curr = ret;
	int pack;

	// always change
	SERIALIZE_INT(pd->commandTime, curr);

	// high degree of change
	SERIALIZE_VEC3S(pd->origin, curr);
	SERIALIZE_VEC3S(pd->velocity, curr);
	SERIALIZE_VEC3S(pd->viewangles, curr);
	SERIALIZE_CHAR(pd->bobCycle, curr);
	SERIALIZE_SHORT(pd->weaponTime, curr);
	//SERIALIZE_CHAR(pd->pmove_framecount, curr); // not needed?

	// medium-high degree of change
	SERIALIZE_VEC3S(pd->grapplePoint, curr);
	SERIALIZE_INT(pd->eventSequence, curr);

	pack = (pd->events[0] & 1023) | ((pd->events[1] & 1023) << 10);
	SERIALIZE_CHARARRAY((char *)&pack, 3, curr);

	SERIALIZE_CHAR(pd->eventParms[0], curr);
	SERIALIZE_CHAR(pd->eventParms[1], curr);
	SERIALIZE_INT(pd->entityEventSequence, curr);
	SERIALIZE_SHORTARRAY(pd->ammo, WP_NUM_WEAPONS - 1, curr);
	SERIALIZE_CHAR(pd->movementDir, curr);

	// medium degree of change
	SERIALIZE_CHAR(pd->legsAnim, curr);
	SERIALIZE_CHAR(pd->torsoAnim, curr);
	SERIALIZE_SHORT(pd->eFlags, curr);
	SERIALIZE_SHORT(pd->pm_time, curr);
	SERIALIZE_SHORT(pd->pm_flags, curr);
	SERIALIZE_SHORT(pd->grappleTime, curr);

	pack = (pd->grappleEntity & 1023) | ((pd->weapon & 15) << 10) | ((pd->weaponstate & 3) << 14);
	SERIALIZE_SHORT((short)pack, curr);

	SERIALIZE_CHAR(pd->damageEvent, curr);
	SERIALIZE_CHAR(pd->damageYaw, curr);
	SERIALIZE_CHAR(pd->damagePitch, curr);
	SERIALIZE_CHAR(pd->damageCount, curr);
	SERIALIZE_SHORTARRAY(pd->stats, STAT_NUM_SSDEMO_STATS, curr);
	SERIALIZE_SHORTARRAY(pd->persistant, PERS_NUM_PERSISTANT, curr);
	SERIALIZE_VEC3S(pd->grappleVelocity, curr);

	// medium-low degree of change
	SERIALIZE_SHORT(pd->groundEntityNum, curr);
	SERIALIZE_SHORTARRAY(pd->delta_angles, 3, curr);
	SERIALIZE_SHORT(pd->legsTimer, curr);
	SERIALIZE_SHORT(pd->torsoTimer, curr);
	SERIALIZE_SHORT(pd->externalEvent, curr);
	SERIALIZE_CHAR(pd->externalEventParm, curr);
	SERIALIZE_INT(pd->externalEventTime, curr);
	//SERIALIZE_CHAR(pd->jumppad_frame, curr); // not needed?

	// low degree of change
	SERIALIZE_CHAR(pd->viewheight, curr);
	SERIALIZE_INTARRAY(pd->powerups, PW_NUM_POWERUPS - 1, curr);
	SERIALIZE_INT(pd->generic1, curr);
	SERIALIZE_SHORT(pd->loopSound, curr);

	pack = (pd->jumppad_ent & 1023) | ((pd->pm_type & 7) << 10);
	SERIALIZE_SHORT((short)pack, curr);

	// almost never change
	SERIALIZE_CHAR(pd->clientNum, curr);
	SERIALIZE_SHORT(pd->gravity, curr);
	SERIALIZE_SHORT(pd->speed, curr);

	*size = (char *)curr - ret;
}

static void G_StuffPlayerData( playerState_t *ps, playerData_t *pd ) {
	int i;

	// always change
	pd->commandTime = ps->commandTime;

	// high degree of change
	Vec3ToShort( ps->origin, pd->origin, 1.0f );
	Vec3ToShort( ps->velocity, pd->velocity, 1.0f );
	Vec3ToShort( ps->viewangles, pd->viewangles, 4.0f );
	pd->bobCycle = ps->bobCycle;
	pd->weaponTime = ps->weaponTime;

	// medium-high degree of change
	pd->movementDir = ps->movementDir;
	Vec3ToShort( ps->grapplePoint, pd->grapplePoint, 1.0f );
	pd->eventSequence = ps->eventSequence;
	pd->events[0] = ps->events[0];
	pd->events[1] = ps->events[1];
	pd->eventParms[0] = ps->eventParms[0];
	pd->eventParms[1] = ps->eventParms[1];
	pd->entityEventSequence = ps->entityEventSequence;
	for ( i = 1; i < WP_NUM_WEAPONS; i++ ) {
		pd->ammo[i - 1] = ps->ammo[i];
	}
	
	// medium degree of change
	pd->legsAnim = ps->legsAnim;
	pd->torsoAnim = ps->torsoAnim;
	pd->eFlags = ps->eFlags;
	pd->pm_time = ps->pm_time;
	pd->pm_flags = ps->pm_flags;
	pd->weapon = ps->weapon;
	pd->weaponstate = ps->weaponstate;
	pd->damageEvent = ps->damageEvent;
	pd->damageYaw = ps->damageYaw;
	pd->damagePitch = ps->damagePitch;
	pd->damageCount = ps->damageCount;
	for ( i = 0; i < STAT_NUM_SSDEMO_STATS; i++ ) {
		pd->stats[i] = ps->stats[i];
	}
	for ( i = 0; i < PERS_NUM_PERSISTANT; i++ ) {
		pd->persistant[i] = ps->persistant[i];
	}
	Vec3ToShort( ps->grappleVelocity, pd->grappleVelocity, 4.0f );
	pd->grappleTime = ps->grappleTime;
	pd->grappleEntity = ps->grappleEntity;

	// medium-low degree of change
	pd->groundEntityNum = ps->groundEntityNum;
	pd->delta_angles[0] = ps->delta_angles[0];
	pd->delta_angles[1] = ps->delta_angles[1];
	pd->delta_angles[2] = ps->delta_angles[2];
	pd->legsTimer = ps->legsTimer;
	pd->torsoTimer = ps->torsoTimer;
	pd->externalEvent = ps->externalEvent;
	pd->externalEventParm = ps->externalEventParm;
	pd->externalEventTime = ps->externalEventTime;

	// low degree of change
	pd->pm_type = ps->pm_type;
	pd->viewheight = ps->viewheight;
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		pd->powerups[i - 1] = ps->powerups[i];
	}
	pd->generic1 = ps->generic1;
	pd->loopSound = ps->loopSound;
	pd->jumppad_ent = ps->jumppad_ent;

	// almost never change
	pd->clientNum = ps->clientNum;
	pd->gravity = ps->gravity;
	pd->speed = ps->speed;
}
*/

static void G_SerializePipPlayerData( pipPlayerData_t *pd, int clientNum, char *ret, int *size ) {
	void *curr = ret;
	int pack;

	SERIALIZE_CHAR(clientNum, curr);

	// medium-high degree of change
	SERIALIZE_SHORT(pd->ammo, curr);

	// medium degree of change
	pack = (pd->weapon & 15) | ((pd->health & 1023) << 4) | ((pd->armor & 1023) << 14);
	SERIALIZE_CHARARRAY((char *)&pack, 3, curr);

	// low degree of change
	SERIALIZE_CHAR(pd->viewheight, curr);
	SERIALIZE_SHORT(pd->deadyaw, curr);
	SERIALIZE_INTARRAY(pd->powerups, PW_NUM_POWERUPS - 1, curr);

	*size = (char *)curr - ret;
}

static void G_StuffPipPlayerData( gentity_t *ent, pipPlayerData_t *pd ) {
	playerState_t *ps = &ent->client->ps;
	int i;

	// medium-high degree of change
	pd->ammo = ps->ammo[ps->weapon];

	// medium degree of change
	pd->weapon = ps->weapon;
	pd->health = ps->stats[STAT_HEALTH] < 0 ? 0 : ps->stats[STAT_HEALTH];
	pd->armor = ps->stats[STAT_ARMOR] < 0 ? 0 : ps->stats[STAT_ARMOR];

	// low degree of change
	pd->viewheight = ps->viewheight;
	pd->deadyaw = ps->stats[STAT_DEAD_YAW];
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		pd->powerups[i - 1] = ps->powerups[i];
	}
}

static pipPlayerData_t g_pipPlayerData[MAX_CLIENTS];
static pipPlayerData_t g_pipPlayerDeltas[MAX_CLIENTS];

static void G_SendPipPlayerSnapshot( gentity_t *ent, playerState_t *ps, int clientNum ) {
	char data[2048];
	int datasize = sizeof(data);
	char rle[2048];
	int rlesize = sizeof(rle);
	char base64[2048];
	int base64size = sizeof(base64);

	G_SerializePipPlayerData( &g_pipPlayerData[clientNum], clientNum, data, &datasize );

	BG_RLE_Compress( (unsigned char *)data, datasize, (unsigned char *)rle, &rlesize );

	if ( rlesize < 0 ) {
		G_Error( "RLE compression error\n" );
	}

	if ( !BG_Base64_Encode( rle, rlesize, base64, &base64size ) ) {
		G_Error( "Base64 encoding error\n" );
	}

	trap_SendServerCommand( ent - g_entities, va("pds %s\n", base64) );
}

static void G_SendPipPlayerDelta( gentity_t *ent, playerState_t *ps, int clientNum ) {
	char data[2048];
	int datasize = sizeof(data);
	char rle[2048];
	int rlesize = sizeof(rle);
	char base64[2048];
	int base64size = sizeof(base64);

	// if the delta is all zeros, forget it
	if ( BG_IsAllChar(&g_pipPlayerDeltas[clientNum], 0, sizeof(pipPlayerData_t)) ) {
		return;
	}

	G_SerializePipPlayerData( &g_pipPlayerDeltas[clientNum], clientNum, data, &datasize );

	BG_RLE_Compress( (unsigned char *)data, datasize, (unsigned char *)rle, &rlesize );

	if ( rlesize < 0 ) {
		G_Error( "RLE compression error\n" );
	}

	if ( !BG_Base64_Encode( rle, rlesize, base64, &base64size ) ) {
		G_Error( "Base64 encoding error\n" );
	}

	trap_SendServerCommand( ent - g_entities, va("pdd %s\n", base64) );
}

static void G_CalculatePipPlayerData( void ) {
	pipPlayerData_t pd;
	gentity_t *pip;
	int max;

	if ( level.inDemoFile != 0 ) {
		max = MAX_CLIENTS;
	}
	else {
		max = level.maxclients;
	}

	for ( pip = g_entities; pip < &g_entities[max]; pip++ ) {
		int clientNum = pip - g_entities;

		if ( !pip->inuse || pip->client == NULL ) {
			memset( &g_pipPlayerData[clientNum], 0, sizeof(pipPlayerData_t) );
			memset( &g_pipPlayerDeltas[clientNum], 0, sizeof(pipPlayerData_t) );
			continue;
		}

		// put the player state data into the player data structure
		G_StuffPipPlayerData( pip, &pd );

		// do a delta and store it globally
		BG_DoXORDelta( (char *)&g_pipPlayerData[clientNum], (char *)&pd, (char *)&g_pipPlayerDeltas[clientNum], sizeof(pipPlayerData_t) );

		// store the player data gobally
		memcpy( &g_pipPlayerData[clientNum], &pd, sizeof(pipPlayerData_t) );
	}
}

void G_ManagePipWindows( void ) {
	gentity_t *ent;
	int i;

	G_CalculatePipPlayerData();

	// loop through all connected clients
	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		char data[2048];
		int datasize;
		char rle[2048];
		int rlesize = sizeof(rle);
		char base64[2048];
		int base64size = sizeof(base64);
		int alldatasize = 0;

		if ( ent->client->pers.connected != CON_CONNECTED ) continue;

		// loop through the client's PIP views
		for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
			ssPipWindow_t *pip = &ent->client->pers.ssPipWindows[i];
			gentity_t *pipEnt = &g_entities[pip->clientNum];

			// if it's not valid or the viewed client is not connected yet, forget it
			if ( !pip->valid || pipEnt->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			// if the viewer is not a spectator
			if ( !is_spectator(ent->client) ) {
				// make sure the next bit of player state data is a snapshot (not a delta)
				pip->snapshot = qfalse;
				// deallocate this portal if it's allocated
				if ( pip->portal != NULL ) {
					G_FreeEntity( pip->portal );
					pip->portal = NULL;
				}
				continue;
			}

			// if the viewer is not allowed to follow the viewed client
			if ( !CanFollow(ent, pipEnt) ) {
				// tell the client to close the window
				trap_SendServerCommand( ent - g_entities, va("badpip %d\n", pip->clientNum) );
				// deallocate the portal
				if ( pip->portal != NULL ) {
					G_FreeEntity( pip->portal );
				}
				// reset this PIP record
				memset( pip, 0, sizeof(ssPipWindow_t) );
				continue;
			}

			// tunnel the missing player state data
			if ( !pip->snapshot ) {
				// send a snapshot to start off
				G_SendPipPlayerSnapshot( ent, &pipEnt->client->ps, pip->clientNum );
				// mark that we've sent one
				pip->snapshot = qtrue;
			}
			else {
				// if it's not all zeros
				if ( !BG_IsAllChar(&g_pipPlayerDeltas[pip->clientNum], 0, sizeof(pipPlayerData_t)) ) {
					// serialize the viewed player data
					G_SerializePipPlayerData( &g_pipPlayerDeltas[pip->clientNum], pip->clientNum, &data[alldatasize], &datasize );
					alldatasize += datasize;
				}
			}

			// make sure there's a portal for this view
			G_AddPipPortal( pip, ent - g_entities );

			// if we got one
			if ( pip->portal != NULL ) {
				// put the origin at the viewer's origin
				VectorCopy( ent->client->ps.origin, pip->portal->r.currentOrigin );

				// if the viewed client is frozen
				if ( IsClientFrozen(pipEnt) && pipEnt->target_ent != NULL ) {
					// put the destination at the frozen body's origin
					VectorCopy( pipEnt->target_ent->r.currentOrigin, pip->portal->s.origin2 );
				}
				else {
					// otherwise put the destination at the viewed client's origin
					VectorCopy( pipEnt->client->ps.origin, pip->portal->s.origin2 );
				}

				// save bandwidth - snapping into map geometry shouldn't be a problem
				trap_SnapVector( pip->portal->r.currentOrigin );
				trap_SnapVector( pip->portal->s.origin2 );

				// link it
				G_LinkEntity( pip->portal );
			}
		}

		// if a delta or three was serialized
		if ( alldatasize > 0 ) {
			// compress it
			BG_RLE_Compress( (unsigned char *)data, alldatasize, (unsigned char *)rle, &rlesize );

			if ( rlesize < 0 ) {
				// this shouldn't happen
				G_Error( "RLE compression error\n" );
			}

			if ( !BG_Base64_Encode( rle, rlesize, base64, &base64size ) ) {
				// this also shouldn't happen
				G_Error( "Base64 encoding error\n" );
			}

			// send the deltas
			trap_SendServerCommand( ent - g_entities, va("pdd %s\n", base64) );
		}
	}
}

/*
G_ReconcileClientNumIssues

This function became necessary when we made sure ps->clientNum was always equal
to the client slot.

ps->clientNum is used to determine which clients to send SVF_SINGLECLIENT,
SVF_NOTSINGLECLIENT, and SVF_CLIENTMASK entities to.  It used to be that When
you followed a player, the game would copy his player state into yours, including
clientNum.  (See CopyFollowedPlayerState().)  This meant that players following
other players would receive the followed player's events, etc.

That's bad for the portal tricks we use to do PIP.  We use SVF_SINGLECLIENT
to ensure that the portals are only applied to the PIP user.  Before the
change to make ps->clientNum immutable, it would NOT send the portals if the
PIP user was following another player.

ps->clientNum is also used to NOT send a player's entity to him.  There's no
point in doing so, because the player is already receiving his player state.
In fact, the client game expects such behavior, and will behave badly itself if
this doesn't happen.

Anyway, here's what this function does:

1) Reconciles SVF_SINGLECLIENT and SVF_NOTSINGLECLIENT by changing them into
SVF_CLIENTMASK, adding (for SVF_SINGLECLIENT) or removing (for SVF_NOTSINGLECLIENT)
clients who are following.

Note that it does nothing to SVF_CLIENTMASK entities.  Also note that if you
want to change the singleClient that an SVF_SINGLECLIENT entity is sent to,
you also have to set the SVF_SINGLECLIENT flag again, and unset SVF_CLIENTMASK.

2) Makes sure a player's entity is not sent when the player is already receiving
the player state.  This is also done by making them into SVF_CLIENTMASK entities.
*/
void G_ReconcileClientNumIssues( void ) {
	gentity_t *ent, *spec;
	int singleClient;

	//
	// turn SVF_SINGLECLIENT and SVF_NOTSINGLECLIENT into SVF_CLIENTMASK
	//

	// loop through every client
	for ( ent = g_entities; ent < &g_entities[ENTITYNUM_MAX_NORMAL]; ent++ ) {
		if ( !ent->inuse ) continue;

		// if it's an SVF_SINGLECLIENT
		if ( ent->r.svFlags & SVF_SINGLECLIENT ) {
			// grab the current singleClient because we'll change it
			singleClient = ent->r.singleClient;

			// change the flags
			ent->r.svFlags &= ~SVF_SINGLECLIENT;
			ent->r.svFlags |= SVF_CLIENTMASK;
			// set this up initially to only send to the singleClient
			ent->r.singleClient = 1 << singleClient;

			// loop through all clients
			for ( spec = g_entities; spec < &g_entities[level.maxclients]; spec++ ) {
				if ( !spec->inuse || spec->client == NULL ) continue;

				// if the client is spectating the singleClient
				if ( spec->client->ps.stats[STAT_SPEC_CLIENTNUM] == singleClient ) {
					// add him to the list
					ent->r.singleClient |= 1 << spec->client->ps.clientNum;
				}
			}
		}
		else if ( ent->r.svFlags & SVF_NOTSINGLECLIENT ) {
			// grab the current singleClient because we'll change it
			singleClient = ent->r.singleClient;

			// change the flags
			ent->r.svFlags &= ~SVF_NOTSINGLECLIENT;
			ent->r.svFlags |= SVF_CLIENTMASK;
			// set this up initially to only send to everyone but the singleClient
			ent->r.singleClient = ~(1 << singleClient);

			// loop through all clients
			for ( spec = g_entities; spec < &g_entities[level.maxclients]; spec++ ) {
				if ( !spec->inuse || spec->client == NULL ) continue;

				// if the client is spectating the singleClient
				if ( spec->client->ps.stats[STAT_SPEC_CLIENTNUM] == singleClient ) {
					// remove him from the list
					ent->r.singleClient &= ~(1 << spec->client->ps.clientNum);
				}
			}
		}
	}

	//
	// keep the player entity from being sent with the player state
	//

	// first, reset every player entity to send it to all clients
	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( !ent->inuse || ent->client == NULL ) continue;

		ent->r.svFlags |= SVF_CLIENTMASK;
		ent->r.singleClient = -1;
	}

	// then, remove a client's spectators from the client mask
	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( !ent->inuse || ent->client == NULL ) continue;

		g_entities[ent->client->ps.stats[STAT_SPEC_CLIENTNUM]].r.singleClient &= ~(1 << (ent - g_entities));
	}
}

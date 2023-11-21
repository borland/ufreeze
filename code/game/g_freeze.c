#include "g_local.h"

int	check_time;
static vec3_t	redflag;
static vec3_t	blueflag;


void SendTimeoutEvent( void ) {
	gentity_t *tent;

	tent = G_TempEntity( vec3_origin, EV_TIMEOUT );
	tent->r.svFlags |= SVF_BROADCAST;
}

void SendLastPlayerEvent( gentity_t *last, int team ) {
	gentity_t *tent, *iter;

	// don't do this during warmup
	if ( level.warmupTime ) {
		return;
	}

	tent = G_TempEntity( vec3_origin, EV_LAST_PLAYER );
	tent->s.eventParm = team;
	tent->r.svFlags |= SVF_BROADCAST;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( iter->inuse && iter->client && iter->client->sess.sessionTeam == TEAM_SPECTATOR &&
				iter->client->sess.autoFollow ) {
			iter->client->bestAutofollow = last;
			iter->client->autofollowTime = level.time + 1000; // give it some time
			iter->client->forceScoreboardTime = level.time + 3000; // show the scoreboard
		}
	}
}


void SaveGameState() {
	int flags = 0, count = 0;
	char buffer[MAX_STRING_CHARS];
	fileHandle_t f;

	trap_FS_FOpenFile( "ufreeze.gamestate", &f, FS_WRITE );

	if ( level.redlocked ) flags |= 1;
	if ( level.bluelocked ) flags |= 2;

	Com_sprintf( buffer, sizeof(buffer), "%d\n", flags );
	trap_FS_Write( buffer, strlen(buffer), f );

	trap_FS_FCloseFile( f );
}


qboolean is_spectator( gclient_t *client ) {
	if ( client == NULL ) return qfalse;
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) return qtrue;
	if ( client->sess.sessionTeam == TEAM_REDCOACH ) return qtrue;
	if ( client->sess.sessionTeam == TEAM_BLUECOACH ) return qtrue;
	if ( client->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR ) return qtrue;
	if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) return qtrue;
	if ( client->sess.spectatorState == SPECTATOR_FROZEN ) return qtrue;
	return qfalse;
}


static void FollowClient( gentity_t *ent, gentity_t *other ) {
	// if he's not the same player and the follower is a spectator of some sort
	if ( ent != other && is_spectator( ent->client ) ) {
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		ent->client->sess.spectatorClient = other->s.number;
	}
}


void RespawnSpectatingClient( gentity_t *ent ) {
	gentity_t	*tent;

	ent->client->sess.spectatorState = SPECTATOR_NOT;
	ent->client->sess.spectatorClient = 0;
	ClientSpawn( ent );

	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
	tent->s.clientNum = ent->s.clientNum;
}


void CopyFollowedPlayerState( gentity_t *ent, playerState_t *ps, clientSession_t *sess, gclient_t *cl ) {
	gentity_t *other = &g_entities[sess->spectatorClient];
	qboolean wasfollow = qfalse;

	// if the follower is supposed to be following a live player
	if ( sess->spectatorState == SPECTATOR_FOLLOW ) {
		// if the followed client is frozen and has a corpsicle
		if ( IsClientFrozen( other ) && other->target_ent ) {
			// start following his corpsicle
			vec3_t angles;

			sess->spectatorState = SPECTATOR_FROZEN;
			// toggle the teleport bit so the client knows to not lerp
			ps->eFlags ^= EF_TELEPORT_BIT;

			VectorCopy( other->target_ent->s.apos.trBase, angles );
			angles[PITCH] = 45.0f;

			if ( ent != NULL ) {
				SetClientViewAngle( ent, angles );
			}
			wasfollow = qtrue;
		}
	}

	// if the follower is supposed to be following a corpsicle
	if ( sess->spectatorState == SPECTATOR_FROZEN ) {
		// if it's frozen
		if ( IsClientFrozen( other ) ) {
			// set his player state up
			ps->pm_type = PM_FREEZE;
			ps->stats[STAT_SPEC_CLIENTNUM] = sess->spectatorClient;
			ps->weapon = WP_NONE;
			ps->persistant[PERS_TEAM] = TEAM_SPECTATOR;
			ps->viewheight = DEFAULT_VIEWHEIGHT;

			// copy pending events if the followed player was alive last frame
			if ( wasfollow ) {
				memcpy( ps->events, other->client->ps.events, sizeof(ps->events) );
				memcpy( ps->eventParms, other->client->ps.eventParms, sizeof(ps->eventParms) );
				ps->eventSequence = other->client->ps.eventSequence;
				ps->externalEvent = other->client->ps.externalEvent;
				ps->externalEventParm = other->client->ps.externalEventParm;
				ps->externalEventTime = other->client->ps.externalEventTime;
			}

			// clear powerups
			memset( ps->powerups, 0, sizeof(ps->powerups) );

			// if he's got a corpsicle
			if ( other->target_ent ) {
				// copy its origin in
				VectorCopy( other->target_ent->r.currentOrigin, ps->origin );
			}

			return;
		}
		else {
			// if it's not frozen, set to follow mode
			sess->spectatorState = SPECTATOR_FOLLOW;
			// and drop through to the follow code
		}
	}

	// if we're in follow mode
	if ( sess->spectatorState == SPECTATOR_FOLLOW ) {
		int	i, savedPing, savedFlags, savedTeam, savedClient, persistant[ MAX_PERSISTANT ];

		// save the ping and some other stuff
		savedPing = ps->ping;
		for ( i = 0; i < MAX_PERSISTANT; i++ ) {
			persistant[ i ] = ps->persistant[ i ];
		}
		savedFlags = ps->stats[STAT_FLAGS];
		savedTeam = ps->stats[STAT_TEAM];
		savedClient = ps->clientNum;

		// copy the player state
		*ps = cl->ps;

		// reset the ping
		ps->ping = savedPing;
		// and the other stuff - except PERS_HITS, PERS_TEAM, and PERS_ATTACKER
		for ( i = 0; i < MAX_PERSISTANT; i++ ) {
			switch ( i ) {
			case PERS_HITS:
			case PERS_TEAM:
			case PERS_ATTACKER:
				continue;
			}
			ps->persistant[ i ] = persistant[ i ];
		}
		ps->stats[STAT_FLAGS] = savedFlags;
		ps->stats[STAT_TEAM] = savedTeam;
		ps->clientNum = savedClient;
		ps->stats[STAT_SPEC_CLIENTNUM] = sess->spectatorClient;
	}
}

static void player_free( gentity_t *ent ) {
	if ( !ent || !ent->inuse ) return;
	if ( !ent->freezeState ) return;
	ent->freezeState = qfalse;
	ent->client->respawnTime = level.time + 1700;
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW || 
			ent->client->sess.spectatorState == SPECTATOR_FROZEN ) {
		StopFollowing( ent );
		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		ent->client->ps.pm_time = 100;
	}
	ent->client->inactivityTime = level.time + g_inactivity.integer * 1000;
}

void Body_free( gentity_t *self ) {
/*
	int	i;
	gentity_t	*ent;
*/

	if ( self->freezeState ) {
		player_free( self->target_ent );
	}
/*
	if ( self->s.eFlags & EF_KAMIKAZE ) {
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			ent = &g_entities[ i ];
			if ( !ent->inuse ) continue;
			if ( ent->activator != self ) continue;
			if ( strcmp( ent->classname, "kamikaze timer" ) ) continue;
			G_FreeEntity( ent );
			break;
		}
	}
*/
	self->s.powerups = 0;
	G_FreeEntity( self );
}

static void Body_Explode( gentity_t *self ) {
	int	i;
	gentity_t	*e, *tent;
	vec3_t	point;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( e->health < 1 ) continue;
		if ( e->client->sess.sessionTeam != self->spawnflags ) continue;
		VectorSubtract( self->r.currentOrigin, e->r.currentOrigin, point );
		if ( VectorLength( point ) > 100 ) continue;
		if ( is_spectator( e->client ) ) continue;
		if ( !self->count ) {
			self->count = level.time + g_thawTime.value * 1000;
			G_Sound( self, CHAN_AUTO, self->noise_index );
			self->unfreezer = e;
		} else if ( self->count < level.time) {
			if ( self->unfreezer != e ) {
				continue;
			}

			tent = G_TempEntity( self->target_ent->r.currentOrigin, EV_OBITUARY );
			tent->s.eventParm = MOD_UNKNOWN;
			tent->s.otherEntityNum = self->target_ent->s.number;
			tent->s.otherEntityNum2 = e->s.number;
			tent->r.svFlags = SVF_BROADCAST;

			G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n", e->s.number, self->target_ent->s.number, MOD_UNKNOWN, e->client->pers.netname, self->target_ent->client->pers.netname, "MOD_UNKNOWN" );
			AddScore( e, self->s.pos.trBase, 1 );

			if ( !level.warmupTime ) {
				e->client->sess.wins++;
				e->client->sess.gamestats.numThaws++;
				e->client->pers.teamstats[e->client->sess.sessionTeam].numThaws++;
			}
			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
		}
		return;
	}
	self->count = 0;
	self->unfreezer = NULL;
}

static void TeleportBody( gentity_t *body, vec3_t origin, vec3_t angles ) {

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	G_TempEntity( body->r.currentOrigin, EV_PLAYER_TELEPORT_OUT );
	G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );

	// unlink to make sure it can't possibly interfere with G_KillBox
	G_UnlinkEntity( body ); //ssdemo

	VectorCopy( origin, body->r.currentOrigin );
	body->r.currentOrigin[2] += 1;

	VectorCopy( body->r.currentOrigin, body->s.pos.trBase );
	body->s.pos.trTime = level.time;
	body->s.pos.trType = TR_LINEAR;

	// spit the body out
	AngleVectors( angles, body->s.pos.trDelta, NULL, NULL );
	VectorScale( body->s.pos.trDelta, 400, body->s.pos.trDelta );
	body->s.pos.trDelta[2] += 200;

	// set angles
	VectorCopy( angles, body->r.currentAngles );
	VectorCopy( angles, body->s.apos.trBase );
	body->s.apos.trType = TR_STATIONARY;

	G_KillBox( body );

	G_LinkEntity( body ); //ssdemo
}


static void Body_WorldEffects( gentity_t *self ) {
	vec3_t	point;
	int	contents;
	int	i, num;
	int	touch[ MAX_GENTITIES ];
	gentity_t	*hit;
	vec3_t	mins, maxs;
	int	previous_waterlevel;
	trace_t tr;

	VectorCopy( self->r.currentOrigin, point );
	point[ 2 ] -= 23;

	contents = trap_PointContents( point, -1 );
	if ( contents & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
		if ( g_hardcore.integer ) {
			if ( self->llamatime == 0 ) {
				self->llamatime = level.time;
			}
		}
		else {
			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
			return;
		}
	}
	else if ( contents & CONTENTS_NODROP ) {
		if ( g_hardcore.integer ) {
			if ( self->llamatime == 0 ) {
				self->llamatime = level.time;
			}
		}
		else {
			Body_free( self );
			return;
		}
	}
	else if ( self->llamatime > 0 ) {
		self->llamatime = 0;
	}

	if ( self->llamatime && level.time - self->llamatime >= g_llamaPenalty.value * 1000 ) {
		trap_SendServerCommand( self->target_ent-g_entities, va("cp \"You have been thawed\n\""));
		G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
		return;
	}

	previous_waterlevel = self->waterlevel;
	self->waterlevel = 0;
	if ( contents & MASK_WATER ) {
		self->waterlevel = 3;
	}
	self->watertype = contents;
	if ( !previous_waterlevel && self->waterlevel ) {
		G_AddEvent( self, EV_WATER_TOUCH, 0 );
	}
	if ( previous_waterlevel && !self->waterlevel ) {
		G_AddEvent( self, EV_WATER_LEAVE, 0 );
	}

	VectorCopy( self->r.currentOrigin, point );
	point[2] -= 4;
	trap_Trace( &tr, self->r.currentOrigin, self->r.mins, self->r.maxs, point, self->s.number, MASK_PLAYERSOLID );
	self->s.groundEntityNum = tr.entityNum;

	if ( self->s.groundEntityNum == ENTITYNUM_WORLD ) {
		if ( tr.plane.normal[2] <= 0.7071f ) {
			self->s.groundEntityNum = -1;
		}
	}

	VectorAdd( self->r.currentOrigin, self->r.mins, mins );
	VectorAdd( self->r.currentOrigin, self->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[ touch[ i ] ];
		if ( !hit->touch ) {
			continue;
		}
		switch ( hit->s.eType ) {
			case ET_PUSH_TRIGGER: {
				if ( self->jumppad_ent != hit->s.number ) {
					G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/world/jumppad.wav" ) );
				}

				VectorCopy( hit->s.origin2, self->s.pos.trDelta );
				VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
				self->s.pos.trType = TR_LINEAR;
				self->s.pos.trTime = level.time;

				self->jumppad_ent = hit->s.number;
				self->jumppad_frame = level.framenum;
			} break;

			case ET_TELEPORT_TRIGGER: {
				if ( !( hit->spawnflags & 1 ) ) {
					gentity_t *dest = G_PickTarget( hit->target );
					if (!dest) {
						G_Printf ("Couldn't find teleporter destination\n");
						break;
					}

					TeleportBody( self, dest->s.origin, dest->s.angles );
				}
			} break;
		}

		if ( hit->touch == Touch_DoorTrigger ) {
			trace_t trace;
			memset( &trace, 0, sizeof(trace) );
			hit->touch( hit, self, &trace );
		}
	}

	if ( level.framenum - self->jumppad_frame > 4 ) {
		self->jumppad_ent = 0;
		self->jumppad_frame = 0;		
	}
}

void Kamikaze_DeathTimer( gentity_t *self );

static void TossBody( gentity_t *self ) {
	int	anim;
	static int	n;

	self->timestamp = level.time;
	self->nextthink = level.time + 5000;
#ifdef MISSIONPACK
	if ( self->s.eFlags & EF_KAMIKAZE ) {
		Kamikaze_DeathTimer( self );
	}
#endif
	self->s.eFlags |= EF_DEAD;
	self->s.powerups = 0;
	self->r.maxs[ 2 ] = -8;
	self->r.contents = CONTENTS_CORPSE;
	self->groundFriction = 10.0f;
	self->freezeState = qfalse;
	self->s.weapon = WP_NONE;
	self->s.otherEntityNum = ENTITYNUM_NONE;

	switch ( n ) {
	case 0:
		anim = BOTH_DEATH1;
		break;
	case 1:
		anim = BOTH_DEATH2;
		break;
	case 2:
	default:
		anim = BOTH_DEATH3;
		break;
	}
	n = ( n + 1 ) % 3;

	self->s.torsoAnim = self->s.legsAnim = anim;

	if ( !g_blood.integer ) {
		self->takedamage = qfalse;
	}

	// adding this fixed body stumble problem
	G_LinkEntity( self ); //ssdemo
}

void Body_think( gentity_t *self ) {
	self->nextthink = level.time + 50;

	if ( !self->target_ent || !self->target_ent->client || !self->target_ent->inuse ) {
		Body_free( self );
		return;
	}
	if ( self->s.otherEntityNum != self->target_ent->s.number ) {
		Body_free( self );
		return;
	}
	if ( level.intermissiontime || level.intermissionQueued ) {
		return;
	}

	if ( (level.time - self->timestamp) > (g_autoThawTime.integer * 1000) ) {
		trap_SendServerCommand( self->target_ent-g_entities, va("cp \"You have been thawed\n\""));
		player_free( self->target_ent );
		TossBody( self );
		return;
	}

	if ( self->target_ent && self->target_ent->client ) {
		self->target_ent->client->ps.persistant[PERS_TOTALFROZEN] += 50;
	}

	if ( self->freezeState ) {
		if ( !self->target_ent->freezeState ) {
			TossBody( self );
			return;
		}
		Body_Explode( self );
		Body_WorldEffects( self );
		return;
	}

	if ( level.time - self->timestamp > 6500 ) {
		Body_free( self );
	} else {
		self->s.pos.trBase[ 2 ] -= 1;
	}
}

static void Body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t	*tent;

	if ( self->health > GIB_HEALTH ) {
		return;
	}

	if ( self->freezeState && !g_blood.integer ) {
		player_free( self->target_ent );
		TossBody( self );
		tent = G_TempEntity( self->r.currentOrigin, EV_EXPLODING_ICE );		
		return;
	}

	tent = G_TempEntity( self->r.currentOrigin, EV_GIB_PLAYER );
	if ( self->freezeState ) {
		tent->s.eventParm = 255;
	}
	Body_free( self );
}

qboolean DamageBody( gentity_t *targ, gentity_t *attacker, vec3_t dir, int damage, int mod ) {
	float		mass;
	vec3_t	kvel;

	// play with the body mass for certain weapons
	switch ( mod ) {
		case MOD_MACHINEGUN:
			mass = 125;
			break;
		case MOD_SHOTGUN:
		case MOD_LIGHTNING:
		case MOD_RAILGUN:
			mass = 150;
			break;
		default:
			mass = 200;
			break;
	}

	if ( g_dmflags.integer & 1024 ) {
		mass *= 4;
	}

	if ( attacker && targ->freezeState ) {
		if ( damage != 0 && dir != NULL ) {
			VectorScale( dir, g_knockback.value * (float)damage / mass, kvel );
			VectorAdd( targ->damageVelocity, kvel, targ->damageVelocity );

			targ->pain_debounce_time = level.time;
		}
		if ( (mod == MOD_GAUNTLET || mod == MOD_RAILGUN) && attacker->client && g_allowFollowEnemy.integer ) {
			FollowClient( targ, attacker );
		}
		return qtrue;
	}
	return qfalse;
}

qboolean is_body( gentity_t *ent ) {
	if ( !ent || !ent->inuse ) return qfalse;
	return ( ent->classname && !Q_stricmp( ent->classname, "freezebody" ) );
}

qboolean is_body_freeze( gentity_t *ent ) {
	if ( is_body( ent ) ) {
		return ent->freezeState;
	}
	return qfalse;
}

#ifdef MISSIONPACK
void G_ExplodeMissile( gentity_t *ent );

static void ProximityMine_ExplodeOnBody( gentity_t *mine ) {
	gentity_t	*body;

	if ( !is_body_freeze( mine->enemy ) ) {
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	body = mine->enemy;
	body->s.eFlags &= ~EF_TICKING;

	body->s.loopSound = 0;

	G_SetOrigin( mine, body->s.pos.trBase );
	mine->r.svFlags &= ~SVF_NOCLIENT;
	mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	G_ExplodeMissile( mine );
}

void ProximityMine_Body( gentity_t *mine, gentity_t *body ) {
	if ( mine->s.eFlags & EF_NODRAW )
		return;

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if ( body->s.eFlags & EF_TICKING ) {
		body->activator->splashDamage += mine->splashDamage;
		body->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	body->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	body->s.eFlags |= EF_TICKING;
	body->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = body;
	mine->think = ProximityMine_ExplodeOnBody;
	mine->nextthink = level.time + 10 * 1000;
}
#endif

static vec3_t	bodyMins = {-15, -15, -24};
static vec3_t	bodyMaxs = {15, 15, 32};

static gentity_t *CopyToBody( gentity_t *ent, qboolean teamkill ) {
	gentity_t	*body;
	int i;

	body = G_Spawn();
	body->classname = "freezebody";
	body->s = ent->s;
	body->s.eFlags = 0;
/*
	if ( ent->s.eFlags & EF_KAMIKAZE ) {
		body->s.eFlags |= EF_KAMIKAZE;
	}
*/
	body->s.powerups = 1 << PW_BATTLESUIT;
	body->s.number = body - g_entities;
	body->s.time = level.time;
	if ( !teamkill ) {
		body->timestamp = level.time;
	}
	else {
		body->timestamp = level.time - (g_autoThawTime.integer - g_friendlyAutoThawTime.integer) * 1000;
	}
	body->physicsObject = qtrue;
	body->physicsBounce = 0.25f;

	G_SetOrigin( body, ent->r.currentOrigin );
	body->s.pos.trType = TR_LINEAR;
	body->s.pos.trTime = level.time;
	VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	body->s.event = 0;

//unlagged - backward reconciliation
	for ( i = 0; i < NUM_CLIENT_HISTORY; i++ ) {
		body->history[i] = ent->history[i];
	}
	body->historyHead = ent->historyHead;
	body->saved = ent->saved;
//unlagged - backward reconciliation

	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case LEGS_WALKCR:
	case LEGS_WALK:
	case LEGS_RUN:
	case LEGS_BACK:
	case LEGS_SWIM:
	case LEGS_IDLE:
	case LEGS_IDLECR:
	case LEGS_TURN:
	case LEGS_BACKCR:
	case LEGS_BACKWALK:
		switch ( rand() % 4 ) {
		case 0:
			body->s.legsAnim = LEGS_JUMP;
			break;
		case 1:
			body->s.legsAnim = LEGS_LAND;
			break;
		case 2:
			body->s.legsAnim = LEGS_JUMPB;
			break;
		case 3:
			body->s.legsAnim = LEGS_LANDB;
			break;
		}
	}

	body->r.svFlags = ent->r.svFlags & SVF_BOT;
	VectorCopy( bodyMins, body->r.mins );
	VectorCopy( bodyMaxs, body->r.maxs );
	VectorCopy( ent->r.absmin, body->r.absmin );
	VectorCopy( ent->r.absmax, body->r.absmax );

	body->clipmask = MASK_PLAYERSOLID;
	body->r.contents = CONTENTS_BODY;

	body->think = Body_think;
	body->nextthink = level.time + 50;

	body->groundFriction = 4.0f;

	body->die = Body_die;
	body->takedamage = qtrue;

	body->target_ent = ent;
	ent->target_ent = body;
	body->s.otherEntityNum = ent->s.number;
	body->noise_index = G_SoundIndex( "sound/player/tankjr/jump1.wav" );
	body->freezeState = qtrue;
	body->spawnflags = ent->client->sess.sessionTeam;
	body->waterlevel = ent->waterlevel;
	body->count = 0;

	G_LinkEntity( body ); //ssdemo

	return body;
}

static qboolean NearbyBody( gentity_t *targ ) {
	gentity_t	*spot;
	vec3_t	delta;

	if ( g_gametype.integer == GT_CTF ) {
		return qfalse;
	}

	spot = NULL;
	while ( ( spot = G_Find( spot, FOFS( classname ), "freezebody" ) ) != NULL ) {
		if ( !spot->freezeState ) continue;
		if ( spot->spawnflags != targ->client->sess.sessionTeam ) continue;
		VectorSubtract( spot->s.pos.trBase, targ->s.pos.trBase, delta );
		if ( VectorLength( delta ) > 100 ) continue;
		if ( level.time - spot->timestamp > 400 ) {
			return qtrue;
		}
	}
	return qfalse;
}

void player_freeze( gentity_t *self, gentity_t *attacker, int mod ) {
	gentity_t *body;
	qboolean teamkill;

	if ( level.warmupTime ) {
		return;
	}
	if ( g_gametype.integer != GT_TEAM && g_gametype.integer != GT_CTF ) {
		return;
	}

	if ( self != attacker && OnSameTeam( self, attacker ) ) {
		teamkill = qtrue;
		if ( !g_friendlyAutoThawTime.integer ) {
			return;
		}
	}
	else {
		teamkill = qfalse;
		if ( !g_autoThawTime.integer ) {
			return;
		}
	}

	if ( self != attacker && g_gametype.integer == GT_CTF && redflag && blueflag ) {
		vec3_t	dist1, dist2;

		VectorSubtract( redflag, self->s.pos.trBase, dist1 );
		VectorSubtract( blueflag, self->s.pos.trBase, dist2 );

		if ( self->client->sess.sessionTeam == TEAM_RED ) {
			if ( VectorLength( dist1 ) < VectorLength( dist2 ) ) {
				return;
			}
		} else if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			if ( VectorLength( dist2 ) < VectorLength( dist1 ) ) {
				return;
			}
		}
	}

	if ( g_hardcore.integer ) {
		switch ( mod ) {
		case MOD_UNKNOWN:
		case MOD_TELEFRAG:
		case MOD_SUICIDE:
		case MOD_TARGET_LASER:
#ifdef MISSIONPACK
		case MOD_JUICED:
#endif
		case MOD_GRAPPLE:
			return;
		}
	}
	else {
		switch ( mod ) {
		case MOD_UNKNOWN:
		case MOD_WATER:
		case MOD_SLIME:
		case MOD_CRUSH:
		case MOD_TELEFRAG:
		case MOD_FALLING:
		case MOD_SUICIDE:
		case MOD_TARGET_LASER:
		case MOD_TRIGGER_HURT:
#ifdef MISSIONPACK
		case MOD_JUICED:
#endif
		case MOD_GRAPPLE:
			return;
		}
	}

	switch ( mod ) {
	case MOD_SHOTGUN:
	case MOD_GAUNTLET:
	case MOD_MACHINEGUN:
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
	case MOD_RAILGUN:
	case MOD_LIGHTNING:
	case MOD_BFG:
	case MOD_BFG_SPLASH:
#ifdef MISSIONPACK
	case MOD_NAIL:
	case MOD_CHAINGUN:
	case MOD_PROXIMITY_MINE:
	case MOD_KAMIKAZE:
#endif
		if ( self != attacker && level.time - self->client->respawnTime < g_noFreezeTime.value * 1000 ) {
			return;
		}

		break;
	}

	self->client->sess.gamestats.numFrozen++;
	self->client->pers.teamstats[self->client->sess.sessionTeam].numFrozen++;
	if ( self != attacker && attacker->client && !OnSameTeam(self, attacker) ) {
		attacker->client->sess.gamestats.numFreezes++;
		attacker->client->pers.teamstats[attacker->client->sess.sessionTeam].numFreezes++;

		level.lastFreezer = attacker->s.number;
		level.lastFreezerTeam = attacker->client->sess.sessionTeam;
		level.lastFreezeTime = level.time;
		VectorCopy( self->r.currentOrigin, level.lastFreezeOrigin );
	}

	body = CopyToBody( self, teamkill );
	self->r.maxs[ 2 ] = -8;
	self->freezeState = qtrue;
	check_time = ( level.time - 3000 ) + 200;

	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
	self->health = GIB_HEALTH;

	if ( attacker->client && self != attacker && NearbyBody( self ) ) {
		attacker->client->ps.persistant[ PERS_DEFEND_COUNT ]++;
		attacker->client->ps.eFlags &= ~( EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	}
}

qboolean readyCheck( void ) {
	int	i;
	int total, ready;
	gentity_t	*e;

	if ( level.warmupTime == 0 ) return qfalse;
	if ( !g_doReady.integer ) return qfalse;

	total = 0;
	ready = 0;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( e->r.svFlags & SVF_BOT ) continue;
		if ( e->client->pers.connected == CON_DISCONNECTED ) continue;
		if ( e->client->sess.sessionTeam == TEAM_SPECTATOR ) continue;
		if ( e->client->sess.sessionTeam == TEAM_REDCOACH ) continue;
		if ( e->client->sess.sessionTeam == TEAM_BLUECOACH ) continue;

		total++;
		if ( e->readyBegin ) ready++;
	}

	if ( ((float)ready / total) >= (g_readyPercent.value / 100) ) {
		return qfalse;
	}

	return qtrue;
}

gentity_t *SelectRandomDeathmatchSpawnPoint( void );

void Team_ForceGesture( int team );

void team_wins( int team ) {
	int	i;
	gentity_t	*e;
	char	*teamstr;
	gentity_t	*spawnPoint;
	int	j;
	int	flight, sight, invis;
	gclient_t	*cl;
	gentity_t	*te;
	char *color;

	spawnPoint = SelectRandomDeathmatchSpawnPoint();
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		cl = e->client;
		if ( !e->inuse ) continue;

		// toggle the new round bit
		e->client->ps.stats[STAT_FLAGS] ^= SF_NEWROUND;

		// show the scoreboard to autofollowing clients
		if ( e->client->sess.autoFollow ) {
			e->client->forceScoreboardTime = level.time + 3000;
		}

		if ( e->freezeState ) {
			if ( !( g_dmflags.integer & 128 ) || cl->sess.sessionTeam != team ) {
				player_free( e );
			}
			continue;
		}
		if ( e->health < 1 ) continue;
		if ( is_spectator( cl ) ) continue;
		if ( g_dmflags.integer & 64 ) continue;

		if ( e->health < cl->ps.stats[ STAT_MAX_HEALTH ] ) {
			e->health = cl->ps.stats[ STAT_MAX_HEALTH ];
		}

		memset( cl->ps.ammo, 0, sizeof ( cl->ps.ammo ) );

		cl->ps.stats[ STAT_WEAPONS ] = 1 << WP_MACHINEGUN;
		cl->ps.ammo[ WP_MACHINEGUN ] = ammo_mg.integer;

		cl->ps.stats[ STAT_WEAPONS ] |= 1 << WP_GAUNTLET;
		cl->ps.ammo[ WP_GAUNTLET ] = -1;
//		cl->ps.ammo[ WP_GRAPPLING_HOOK ] = -1;

		cl->ps.weapon = WP_MACHINEGUN;
		cl->ps.weaponstate = WEAPON_READY;

		SpawnWeapon( cl );

		flight = cl->ps.powerups[ PW_FLIGHT ];
		sight = cl->ps.powerups[ PW_SIGHT ];
		invis = cl->ps.powerups[ PW_INVIS ];
		if ( cl->ps.powerups[ PW_REDFLAG ] ) {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
			cl->ps.powerups[ PW_REDFLAG ] = INT_MAX;
		}
		else if ( cl->ps.powerups[ PW_BLUEFLAG ] ) {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
			cl->ps.powerups[ PW_BLUEFLAG ] = INT_MAX;
		}
		else {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
		}
		cl->ps.powerups[ PW_FLIGHT ] = flight;

		if ( g_huntMode.integer ) {
			cl->ps.powerups[ PW_SIGHT ] = sight;
			cl->ps.powerups[ PW_INVIS ] = invis;
		}

		cl->ps.stats[ STAT_ARMOR ] = 0;

		if ( !( g_dmflags.integer & 1024 ) ) G_UseTargets( spawnPoint, e );
		cl->ps.weapon = 1;
		for ( j = WP_NUM_WEAPONS - 1; j > 0; j-- ) {
			if ( cl->ps.stats[ STAT_WEAPONS ] & ( 1 << j ) ) {
				cl->ps.weapon = j;
				break;
			}
		}
		if ( cl->ps.stats[ STAT_WEAPONS ] & ( 1 << WP_ROCKET_LAUNCHER ) ) {
			cl->ps.weapon = WP_ROCKET_LAUNCHER;
		}

		if ( g_startArmor.integer > 0 ) {
			cl->ps.stats[ STAT_ARMOR ] += g_startArmor.integer;
			if ( cl->ps.stats[ STAT_ARMOR ] > cl->ps.stats[ STAT_MAX_HEALTH ] * 2 ) {
				cl->ps.stats[ STAT_ARMOR ] = cl->ps.stats[ STAT_MAX_HEALTH ] * 2;
			}
		}
	}

	if ( level.numPlayingClients < 2 || g_gametype.integer == GT_CTF ) {
		return;
	}

	te = G_TempEntity( vec3_origin, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	if ( team == TEAM_RED ) {
		teamstr = "Red";
		te->s.eventParm = GTS_BLUE_CAPTURE;
	} else {
		teamstr = "Blue";
		te->s.eventParm = GTS_RED_CAPTURE;
	}

	if ( team == TEAM_RED ) {
		color = S_COLOR_RED;
	}
	else if ( team == TEAM_BLUE ) {
		color = S_COLOR_BLUE;
	}
	else {
		color = S_COLOR_MAGENTA;
	}

	G_SendServerCommand( -1, va( "cp \"%s%s " S_COLOR_WHITE "team scores!\n\"", color, teamstr ) ); //ssdemo
	G_SendServerCommand( -1, va( "print \"%s team scores!\n\"", teamstr ) ); //ssdemo

	AddTeamScore( vec3_origin, team, 1 );
	Team_ForceGesture( team );

	level.roundStartTime = level.time;

	CalculateRanks();
}

qboolean IsClientFrozen( gentity_t *ent ) {
	return ent->freezeState;
}

qboolean IsClientOOC( gentity_t *ent ) {
	if ( !ent->client || !ent->inuse ) {
		return qtrue;
	}

	if ( ent->client->pers.connected == CON_CONNECTING ) {
		return qtrue;
	}

	if ( ( ent->health < 1 || is_spectator( ent->client ) ) && level.time > ent->client->respawnTime ) {
		return qtrue;
	}

	return qfalse;
}

static qboolean CalculateScores( int team ) {
	int	i;
	gentity_t	*e, *last;
	int numfrozen = 0, numout = 0, numleft = 0, numonteam = 0;
	static int lastnumleft[TEAM_NUM_TEAMS] = { 0, 0, 0, 0 };

	last = NULL;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse || !e->client ) continue;
		if ( e->client->sess.sessionTeam != team ) continue;

		if ( IsClientFrozen( e ) ) {
			numfrozen++;
			continue;
		}

		if ( IsClientOOC( e ) ) {
			numout++;
			continue;
		}

		last = e;

		numleft++;
	}

//	Com_Printf("team: %d, numleft: %d, numfrozen: %d, numout: %d\n", team, numleft, numfrozen, numout);

	if ( numleft == 0 && numfrozen != 0 ) {
		team_wins( OtherTeam( team ) );
		lastnumleft[team] = 0;
		return qtrue;
	}

	if ( numleft == 1 && numfrozen + numout != 0 && lastnumleft[team] != 1 ) {
		SendLastPlayerEvent( last, team );
	}

	lastnumleft[team] = numleft;

	return qfalse;
}

void CheckDelay( void ) {
	int	i;
	gentity_t	*e;
	int	readyMask[4];
	static qboolean redfirst = qtrue;

	readyMask[0] = readyMask[1] = readyMask[2] = readyMask[3] = 0;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse || !e->client ) continue;
		if ( e->client->sess.sessionTeam == TEAM_SPECTATOR ) continue;

		if ( level.warmupTime ) {
			if ( !e->readyBegin ) continue;
		}
		else {
			if ( !IsClientFrozen( e ) && !IsClientOOC( e ) ) continue;
		}

		readyMask[i / 16] |= 1 << (i % 16);
	}

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		e->client->ps.stats[ STAT_CLIENTS_READY0 ] = readyMask[0];
		e->client->ps.stats[ STAT_CLIENTS_READY1 ] = readyMask[1];
		e->client->ps.stats[ STAT_CLIENTS_READY2 ] = readyMask[2];
		e->client->ps.stats[ STAT_CLIENTS_READY3 ] = readyMask[3];
	}

	// give players five seconds to get in the game
	if ( level.roundStartTime > level.time - 5000 ) {
		return;
	}

	if ( check_time > level.time - 3000 ) {
		return;
	}
	check_time = level.time;

	if ( redfirst ) {
		redfirst = qfalse;

		if ( !CalculateScores( TEAM_RED ) ) {
			CalculateScores( TEAM_BLUE );
		}
	}
	else {
		redfirst = qtrue;

		if ( !CalculateScores( TEAM_BLUE ) ) {
			CalculateScores( TEAM_RED );
		}
	}
}

void SP_target_location( gentity_t *self );

void locationSpawn( gentity_t *ent, gitem_t *item ) {
	gentity_t	*e;

	switch ( item->giType ) {
	case IT_AMMO:
		return;
	case IT_ARMOR:
		if ( Q_stricmp( item->classname, "item_armor_shard" ) ) {
			break;
		}
		return;
	case IT_HEALTH:
		if ( !Q_stricmp( item->classname, "item_health_mega" ) ) {
			break;
		}
		return;
	case IT_PERSISTANT_POWERUP:
		return;
	case IT_TEAM:
		if ( item->giTag == PW_BLUEFLAG ) {
			VectorCopy( ent->r.currentOrigin, blueflag );
		} else if ( item->giTag == PW_REDFLAG ) {
			VectorCopy( ent->r.currentOrigin, redflag );
		}
	case IT_INVISIBLE:
		return;
	}

	e = G_Spawn();
	e->classname = "target_location";
	e->message = item->pickup_name;
	e->count = 255;
	VectorCopy( ent->r.currentOrigin, e->s.origin );

	SP_target_location( e );
}

char *ConcatArgs( int start );

void Cmd_Drop_f( gentity_t *ent ) {
	char	*name;
	gitem_t	*it;
	gentity_t	*drop;
	int	quantity;
	int	j;

	if ( is_spectator( ent->client ) ) {
		return;
	}
	if ( ent->health <= 0 ) {
		return;
	}
	if ( level.timein > level.time ) {
		trap_SendServerCommand( ent-g_entities, "print \"You are not allowed to drop items during timeout.\n\"" );
		return;
	}
	name = ConcatArgs( 1 );
	it = BG_FindItem( name );
	if ( !Registered( it ) ) {
		return;
	}

	j = it->giTag;
	switch ( it->giType ) {
	case IT_WEAPON:
		if ( g_dmflags.integer & 256 ) {
			return;
		}
		if ( !( ent->client->ps.stats[ STAT_WEAPONS ] & ( 1 << j ) ) ) {
			return;
		}
		if ( ent->client->ps.weaponstate != WEAPON_READY ) {
			return;
		}
		if ( j == ent->s.weapon ) {
			return;
		}
		if ( j > WP_MACHINEGUN && /*j != WP_GRAPPLING_HOOK && */ ent->client->ps.ammo[ j ] ) {
			drop = Drop_Item( ent, it, 0 );
			drop->count = 1;
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.stats[ STAT_WEAPONS ] &= ~( 1 << j );
			ent->client->ps.ammo[ j ] -= 1;
		}
		break;
	case IT_AMMO:
		quantity = ent->client->ps.ammo[ j ];
		if ( !quantity ) {
			return;
		}
		if ( quantity > it->quantity ) {
			quantity = it->quantity;
		}
		drop = Drop_Item( ent, it, 0 );
		drop->count = quantity;
		drop->s.otherEntityNum = ent->s.clientNum + 1;
		ent->client->ps.ammo[ j ] -= quantity;
		break;
	case IT_POWERUP:
		if ( ent->client->ps.powerups[ j ] > level.time ) {
			drop = Drop_Item( ent, it, 0 );
			drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
			if ( drop->count < 1 ) {
				drop->count = 1;
			}
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.powerups[ j ] = 0;
		}
		break;
	case IT_HOLDABLE:
		if ( j == HI_KAMIKAZE ) {
			return;
		}
		if ( bg_itemlist[ ent->client->ps.stats[ STAT_HOLDABLE_ITEM ] ].giTag == j ) {
			drop = Drop_Item( ent, it, 0 );
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.stats[ STAT_HOLDABLE_ITEM ] = 0;
		}
		break;
	}
}

void Cmd_Autofollow_f( gentity_t *ent ) {
	char	*msg;

	ent->client->sess.autoFollow = !ent->client->sess.autoFollow;

	if ( ent->client->sess.autoFollow )
		msg = "autofollow ON\n";
	else
		msg = "autofollow OFF\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}

static void FloatToStr5( char *out, float in ) {
	char tmp[6];
	char *spaces = "     ";

	Com_sprintf( tmp, sizeof(tmp), "%.1f", in );
	Com_sprintf( out, 6, "%s%s", &spaces[strlen(tmp)], tmp );
}

static void G_PrintAccuracyStats( gentity_t *ent, gclient_t *client, accStats_t *accstats, gameStats_t *gamestats, qboolean accum ) {
	int i, numshown;
	float totalShots, totalEnemyHits, totalTeamHits, totalCorpseHits;
	int totalKills, totalDeaths, damageGiven, damageReceived;
	char str[MAX_STRING_CHARS];
	float acc, totalacc;
	stat_t stats[WP_NUM_WEAPONS];
	int numstats = 0, len;

	// if the target is invisible and the requestor is not a referee, forget it
	if ( client->sess.invisible && !ent->client->sess.referee ) {
		return;
	}

	totalShots = totalEnemyHits = totalTeamHits = totalCorpseHits = 0.0f;
	totalKills = totalDeaths = damageGiven = damageReceived = 0;

	totalacc = 0.0f;
	numshown = 0;

	for ( i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++ ) /*if ( i != WP_GRAPPLING_HOOK )*/ {
		totalShots += accstats->totalShots[i];
		totalEnemyHits += accstats->totalEnemyHits[i];
		totalTeamHits += accstats->totalTeamHits[i];
		totalCorpseHits += accstats->totalCorpseHits[i];

		totalKills += accstats->totalKills[i];
		totalDeaths += accstats->totalDeaths[i];
		damageGiven += accstats->damageGiven[i];
		damageReceived += accstats->damageReceived[i];

		if ( i == WP_GAUNTLET ) {
			continue;
		}

		if ( accstats->totalShots[i] == 0 && accstats->damageReceived[i] == 0 ) {
			continue;
		}

		if ( accstats->totalShots[i] > 0 ) {
			acc = (float)accstats->totalEnemyHits[i] / accstats->totalShots[i];
		}
		else {
			acc = 0.0f;
		}

		if ( accstats->totalShots[i] > 0 ) {
			totalacc += acc;
			numshown++;
		}

		stats[numstats].weapon = i;
		stats[numstats].acc = acc;
		stats[numstats].enemyhits = accstats->totalEnemyHits[i];
		stats[numstats].shots = accstats->totalShots[i];
		stats[numstats].teamhits = accstats->totalTeamHits[i];
		stats[numstats].corpsehits = accstats->totalCorpseHits[i];
		stats[numstats].kills = accstats->totalKills[i];
		stats[numstats].deaths = accstats->totalDeaths[i];
		stats[numstats].dg = accstats->damageGiven[i];
		stats[numstats].dr = accstats->damageReceived[i];

		numstats++;
	}

	if ( numshown > 0 ) {
		acc = totalacc / numshown;
	}
	else {
		acc = 0.0f;
	}

	stats[numstats].weapon = WP_NONE;
	stats[numstats].acc = acc;
	stats[numstats].enemyhits = totalEnemyHits;
	stats[numstats].shots = totalShots;
	stats[numstats].teamhits = totalTeamHits;
	stats[numstats].corpsehits = totalCorpseHits;
	stats[numstats].kills = totalKills;
	stats[numstats].deaths = totalDeaths;
	stats[numstats].dg = damageGiven;
	stats[numstats].dr = damageReceived;

	numstats++;

	// marshal the stats array
	Com_sprintf( str, sizeof(str), "accstats %d %d %d ", accum, client->ps.clientNum, numstats );
	
	for ( i = 0; i < numstats; i++ ) {
		len = strlen(str);
		Com_sprintf( str + len, sizeof(str) - len, "%d %.4f %d %d %d %d %d %d %d %d ",
			stats[i].weapon, stats[i].acc, stats[i].enemyhits, stats[i].shots,
			stats[i].teamhits, stats[i].corpsehits, stats[i].kills, stats[i].deaths,
			stats[i].dg, stats[i].dr );
	}

	len = strlen(str);
	Com_sprintf( str + len, sizeof(str) - len, "%d %d %d",
		gamestats->numFreezes, gamestats->numThaws, gamestats->numFrozen );

	trap_SendServerCommand( ent-g_entities, str );
}

qboolean CheckClientNum( gentity_t *ent, int clientNum ) {
	if ( clientNum < 0 || clientNum >= level.maxclients ) {
		if ( ent == NULL ) {
			G_Printf( "Client number must be between 0 and %d.\n", level.maxclients );
		}
		else {
			trap_SendServerCommand( ent-g_entities, va("print \"Client number must be between 0 and %d.\n\"", level.maxclients) );
		}

		return qfalse;
	}

	if ( !g_entities[clientNum].inuse || !g_entities[clientNum].client || (ent && !ent->client->sess.referee && g_entities[clientNum].client->sess.invisible) ) {
		if ( ent == NULL ) {
			G_Printf( "Client %d is not active.\n", clientNum );
		}
		else {
			trap_SendServerCommand( ent-g_entities, va("print \"Client %d is not active.\n\"", clientNum) );
		}

		return qfalse;
	}

	return qtrue;
}

static void PrintIgnoreList( gentity_t *ent, qboolean ignore ) {
	gentity_t *iter;
	char *spaces = "                                    ";

	trap_SendServerCommand( ent-g_entities, va("print \"^2You are ignoring the following players:\n\n\"") );
	trap_SendServerCommand( ent-g_entities, va("print \"^7Name                                | Client\n\"") );
	trap_SendServerCommand( ent-g_entities, va("print \"------------------------------------+-------\n\"") );

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( iter->inuse && !iter->client->sess.invisible && ent->client->pers.ignoring[iter->s.number] ) {
			trap_SendServerCommand( ent-g_entities, va("print \"^7%s%s^7|   %2d\n\"", iter->client->pers.netname, &spaces[strlencolor(iter->client->pers.netname)], iter->s.number) );
		}
	}

	trap_SendServerCommand( ent-g_entities, va("print \"\n^2Use %s <clientnum> to %s\n\"", ignore ? "ignore" : "unignore", ignore ? "add another player" : "remove a player") );
}

static void PrintMutedList( gentity_t *ent ) {
	gentity_t *iter;
	char *spaces = "                                    ";

	trap_SendServerCommand( ent-g_entities, va("print \"^2The following players are muted:\n\n\"") );
	trap_SendServerCommand( ent-g_entities, va("print \"^7Name                                | Client\n\"") );
	trap_SendServerCommand( ent-g_entities, va("print \"------------------------------------+-------\n\"") );

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( iter->inuse && iter->client && iter->client->pers.muted ) {
			trap_SendServerCommand( ent-g_entities, va("print \"^7%s%s^7|   %2d\n\"", iter->client->pers.netname, &spaces[strlencolor(iter->client->pers.netname)], iter->s.number) );
		}
	}

	trap_SendServerCommand( ent-g_entities, va("print \"\n^2Use ref_mute <clientnum> to mute and unmute players.\n") );
}

void Cmd_Ignore_f( gentity_t *ent, qboolean ignore ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;

	if ( trap_Argc() != 2 ) {
		PrintIgnoreList( ent, ignore );
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	if ( ignore ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You are now ignoring %s^7.\n\"", g_entities[clientnum].client->pers.netname) );
		trap_SendServerCommand( clientnum, va("print \"%s^7 is ignoring you.\n\"", ent->client->pers.netname) );
	}
	else {
		trap_SendServerCommand( ent-g_entities, va("print \"You are no longer ignoring %s^7.\n\"", g_entities[clientnum].client->pers.netname) );
		trap_SendServerCommand( clientnum, va("print \"%s^7 has stopped ignoring you.\n\"", ent->client->pers.netname) );
	}

	ent->client->pers.ignoring[clientnum] = ignore;
}

void Cmd_Stats_f( gentity_t *ent ) {
	gclient_t *client;
	gameStats_t gamestats;
	int i;

	if ( trap_Argc() == 1 ) {
		client = ent->client;
		trap_SendServerCommand( ent-g_entities, va("print \"^1Use /stats <clientnum> to get stats on another player.\n\n\"") );
	}
	else {
		int clientNum;
		char buffer[MAX_STRING_CHARS];

		trap_Argv( 1, buffer, sizeof(buffer) );
		clientNum = atoi( buffer );

		if ( !CheckClientNum( ent, clientNum ) ) return;

		client = g_entities[clientNum].client;
	}

	memset( &gamestats, 0, sizeof(gameStats_t) );

	for ( i = TEAM_FREE; i < TEAM_NUM_TEAMS; i++ ) {
		gamestats.damageGiven += client->pers.teamstats[i].damageGiven;
		gamestats.damageReceived += client->pers.teamstats[i].damageReceived;
		gamestats.numFreezes += client->pers.teamstats[i].numFreezes;
		gamestats.numFrozen += client->pers.teamstats[i].numFrozen;
		gamestats.numThaws += client->pers.teamstats[i].numThaws;
	}

	G_PrintAccuracyStats( ent, client, &client->pers.accstats, &gamestats, qfalse );
}

void Cmd_StatsAcc_f( gentity_t *ent ) {
	gclient_t *client;

	if ( trap_Argc() == 1 ) {
		client = ent->client;
		trap_SendServerCommand( ent-g_entities, va("print \"^1Use /stats <clientnum> to get stats on another player.\n\n\"") );
	}
	else {
		int clientNum;
		char buffer[MAX_STRING_CHARS];

		trap_Argv( 1, buffer, sizeof(buffer) );
		clientNum = atoi( buffer );

		if ( !CheckClientNum( ent, clientNum ) ) return;

		client = g_entities[clientNum].client;
	}

	G_PrintAccuracyStats( ent, client, &client->sess.accstats, &client->sess.gamestats, qtrue );
}

static void G_PrintStatsForTeam( gentity_t *ent, int team ) {
	gentity_t *iter;
	gameStats_t gamestats;
	int i;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || (iter->client->sess.invisible && !ent->client->sess.referee) ) {
			continue;
		}

		if ( iter->client->sess.sessionTeam != team ) continue;

		memset( &gamestats, 0, sizeof(gameStats_t) );

		for ( i = TEAM_FREE; i < TEAM_NUM_TEAMS; i++ ) {
			gamestats.damageGiven += iter->client->pers.teamstats[i].damageGiven;
			gamestats.damageReceived+= iter->client->pers.teamstats[i].damageReceived;
			gamestats.numFreezes += iter->client->pers.teamstats[i].numFreezes;
			gamestats.numFrozen += iter->client->pers.teamstats[i].numFrozen;
			gamestats.numThaws += iter->client->pers.teamstats[i].numThaws;
		}

		G_PrintAccuracyStats( ent, iter->client, &iter->client->pers.accstats, &gamestats, qfalse );
	}
}

void Cmd_StatsBlue_f( gentity_t *ent ) {
	G_PrintStatsForTeam( ent, TEAM_BLUE );
}

void Cmd_StatsRed_f( gentity_t *ent ) {
	G_PrintStatsForTeam( ent, TEAM_RED );
}

void Cmd_StatsAll_f( gentity_t *ent ) {
	G_PrintStatsForTeam( ent, TEAM_RED );
	G_PrintStatsForTeam( ent, TEAM_BLUE );
}

static int g_shotsNeeded[WP_NUM_WEAPONS] = {
	0,
	0,
	50,
	110,
	10,
	10,
	100,
	10,
	50,
	25,
//	0,
#ifdef MISSIONPACK
	75,
	10,
	100,
#endif
};

static void G_PrintWeaponAccuracies( gentity_t *ent, int weaponnum, qboolean top, qboolean match ) {
	gentity_t *iter;
	char str[MAX_STRING_CHARS];
	gentity_t *best = NULL;
	float bestacc;
	accStats_t *accstats;
	topshot_t topshots[MAX_CLIENTS];
	int numlines, i, len;

	if ( top ) {
		bestacc = -1.0f;
	}
	else {
		bestacc = 500.0f;
	}

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || (iter->client->sess.invisible && !ent->client->sess.referee) ) continue;

		if ( match ) {
			accstats = &iter->client->pers.accstats;
		}
		else {
			accstats = &iter->client->sess.accstats;
		}

		if ( accstats->totalShots[weaponnum] >= g_shotsNeeded[weaponnum] ) {
			float acc = (float)accstats->totalEnemyHits[weaponnum] / accstats->totalShots[weaponnum];
			if ( (top && acc > bestacc) || (!top && acc < bestacc) ) {
				bestacc = acc;
				best = iter;
			}				
		}
	}

	numlines = 0;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || (iter->client->sess.invisible && !ent->client->sess.referee) ) continue;

		if ( match ) {
			accstats = &iter->client->pers.accstats;
		}
		else {
			accstats = &iter->client->sess.accstats;
		}

		if ( accstats->totalShots[weaponnum] < g_shotsNeeded[weaponnum] ) continue;

		// weapon will hold whether or not this is the best accuracy
		topshots[numlines].weapon = (iter == best);
		topshots[numlines].acc = (float)accstats->totalEnemyHits[weaponnum] / accstats->totalShots[weaponnum];
		topshots[numlines].hits = accstats->totalEnemyHits[weaponnum];
		topshots[numlines].attacks = accstats->totalShots[weaponnum];
		topshots[numlines].kills = accstats->totalKills[weaponnum];
		topshots[numlines].deaths = accstats->totalDeaths[weaponnum];
		topshots[numlines].clientNum = iter->s.clientNum;

		numlines++;
	}

	// marshal the topshots array
	Com_sprintf( str, sizeof(str), "weaponacc %d %d ", numlines, weaponnum );
	
	for ( i = 0; i < numlines; i++ ) {
		len = strlen(str);
		Com_sprintf( str + len, sizeof(str) - len, "%d %.4f %d %d %d %d %d ",
			topshots[i].weapon, topshots[i].acc, topshots[i].hits, topshots[i].attacks,
			topshots[i].kills, topshots[i].deaths, topshots[i].clientNum );
	}

	// send it
	trap_SendServerCommand( ent - g_entities, str );
}

static char *g_shortNames[WP_NUM_WEAPONS] = {
	"None",
	"G",
	"MG",
	"SG",
	"GL",
	"RL",
	"LG",
	"RG",
	"PG",
	"BFG",
//	"GH",
#ifdef MISSIONPACK
	"NG",
	"PL",
	"CG",
#endif
};

void Cmd_Topshots_f( gentity_t *ent, qboolean top ) {
	int weapon, weaponnum, i, len;
	gentity_t *iter;
	char str[MAX_STRING_CHARS];
	char gun[MAX_STRING_CHARS];
	accStats_t *accstats;
	topshot_t topshots[MAX_CLIENTS];
	int numlines;

	weaponnum = WP_NONE;
	if ( trap_Argc() >= 2 ) {
		trap_Argv( 1, gun, sizeof(gun) );

		for ( i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++ ) {
			if ( Q_stricmp( g_shortNames[i], gun ) == 0 ) {
				weaponnum = i;
				break;
			}
		}
	}

	if ( weaponnum != WP_NONE /*&& weaponnum != WP_GRAPPLING_HOOK*/ ) {
		G_PrintWeaponAccuracies( ent, weaponnum, top, qtrue );
		return;
	}

	numlines = 0;

	// get the info on best weapon accuracies for every weapon
	for ( weapon = WP_MACHINEGUN; weapon < WP_NUM_WEAPONS; weapon++ ) {
		gentity_t *best = NULL;
		float bestacc;
		if ( top ) {
			bestacc = -1.0f;
		}
		else {
			bestacc = 500.0f;
		}

		//if ( weapon == WP_GRAPPLING_HOOK ) continue;

		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			if ( !iter->inuse || !iter->client || (iter->client->sess.invisible && !ent->client->sess.referee) ) continue;

			accstats = &iter->client->pers.accstats;

			if ( accstats->totalShots[weapon] >= g_shotsNeeded[weapon] ) {
				float acc = (float)accstats->totalEnemyHits[weapon] / accstats->totalShots[weapon];
				if ( (top && acc > bestacc) || (!top && acc < bestacc) ) {
					bestacc = acc;
					best = iter;
				}				
			}
		}

		if ( best != NULL ) {
			accstats = &best->client->pers.accstats;

			topshots[numlines].weapon = weapon;
			topshots[numlines].acc = bestacc;
			topshots[numlines].hits = accstats->totalEnemyHits[weapon];
			topshots[numlines].attacks = accstats->totalShots[weapon];
			topshots[numlines].kills = accstats->totalKills[weapon];
			topshots[numlines].deaths = accstats->totalDeaths[weapon];
			topshots[numlines].clientNum = best->s.clientNum;

			numlines++;
		}
	}

	// marshal the topshots array
	if ( top ) {
		Com_sprintf( str, sizeof(str), "topshots %d ", numlines );
	}
	else {
		Com_sprintf( str, sizeof(str), "bottomshots %d ", numlines );
	}
	
	for ( i = 0; i < numlines; i++ ) {
		len = strlen(str);
		Com_sprintf( str + len, sizeof(str) - len, "%d %.4f %d %d %d %d %d ",
			topshots[i].weapon, topshots[i].acc, topshots[i].hits, topshots[i].attacks,
			topshots[i].kills, topshots[i].deaths, topshots[i].clientNum );
	}

	// send it
	trap_SendServerCommand( ent - g_entities, str );
}

static void G_PrintTeamStats( gentity_t *ent, int team ) {
	gentity_t *iter;
	char str[MAX_STRING_CHARS];
	int numFreezes, numThaws, numFrozen, damageGiven, damageReceived;
	gameStats_t *gamestats;
	teamstat_t stats[MAX_CLIENTS + 1];
	int numstats = 0, i, len;

	numFreezes = numThaws = numFrozen = damageGiven = damageReceived = 0;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client || (iter->client->sess.invisible && !ent->client->sess.referee) ) continue;

		gamestats = &iter->client->pers.teamstats[team];

		if ( iter->client->sess.sessionTeam != team && 
				gamestats->numFreezes == 0 &&
				gamestats->numThaws == 0 &&
				gamestats->numFrozen == 0 &&
				gamestats->damageGiven == 0 &&
				gamestats->damageReceived == 0 ) {
			continue;
		}

		numFreezes += gamestats->numFreezes;
		numThaws += gamestats->numThaws;
		numFrozen += gamestats->numFrozen;
		damageGiven += gamestats->damageGiven;
		damageReceived += gamestats->damageReceived;

		stats[numstats].clientNum = iter->s.number;
		stats[numstats].freezes = gamestats->numFreezes;
		stats[numstats].thaws = gamestats->numThaws;
		stats[numstats].frozen = gamestats->numFrozen;
		stats[numstats].dg = gamestats->damageGiven;
		stats[numstats].dr = gamestats->damageReceived;
		numstats++;
	}

	stats[numstats].clientNum = -1;
	stats[numstats].freezes = numFreezes;
	stats[numstats].thaws = numThaws;
	stats[numstats].frozen = numFrozen;
	stats[numstats].dg = damageGiven;
	stats[numstats].dr = damageReceived;
	numstats++;

	// marshal the team stats array
	Com_sprintf( str, sizeof(str), "teamstats %d %d ", team, numstats );
	
	for ( i = 0; i < numstats; i++ ) {
		len = strlen(str);
		Com_sprintf( str + len, sizeof(str) - len, "%d %d %d %d %d %d ",
			stats[i].clientNum, stats[i].freezes, stats[i].thaws, stats[i].frozen, 
			stats[i].dg, stats[i].dr );
	}

	trap_SendServerCommand( ent-g_entities, str );
}

void Cmd_StatsTeam_f( gentity_t *ent ) {

	if ( ent->client->sess.sessionTeam == TEAM_RED ||
			ent->client->sess.sessionTeam == TEAM_BLUE ) {
		G_PrintTeamStats( ent, ent->client->sess.sessionTeam );
	}
	else {
		trap_SendServerCommand( ent-g_entities, "print \"You must be on a team to get team stats.\n\"" );
	}
}

void Cmd_StatsTeamRed_f( gentity_t *ent ) {
	G_PrintTeamStats( ent, TEAM_RED );
}

void Cmd_StatsTeamBlue_f( gentity_t *ent ) {
	G_PrintTeamStats( ent, TEAM_BLUE );
}

void Cmd_StatsTeamAll_f( gentity_t *ent ) {
	G_PrintTeamStats( ent, TEAM_RED );
	G_PrintTeamStats( ent, TEAM_BLUE );
}

void G_SendAllClientStats( void ) {
	gentity_t *ent;
	gameStats_t gamestats;
	int i;

	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( !ent->inuse || !ent->client ) {
			continue;
		}

		if ( ent->client->pers.noStats ) {
			continue;
		}

		if ( ent->client->sess.sessionTeam == TEAM_BLUE ) {
			G_PrintTeamStats( ent, TEAM_RED );
			G_PrintTeamStats( ent, TEAM_BLUE );
		}
		else {
			G_PrintTeamStats( ent, TEAM_BLUE );
			G_PrintTeamStats( ent, TEAM_RED );
		}

		memset( &gamestats, 0, sizeof(gameStats_t) );

		for ( i = TEAM_FREE; i < TEAM_NUM_TEAMS; i++ ) {
			gamestats.damageGiven += ent->client->pers.teamstats[i].damageGiven;
			gamestats.damageReceived+= ent->client->pers.teamstats[i].damageReceived;
			gamestats.numFreezes += ent->client->pers.teamstats[i].numFreezes;
			gamestats.numFrozen += ent->client->pers.teamstats[i].numFrozen;
			gamestats.numThaws += ent->client->pers.teamstats[i].numThaws;
		}

		G_PrintAccuracyStats( ent, ent->client, &ent->client->pers.accstats, &gamestats, qfalse );
	}
}

void Cmd_Clients_f( gentity_t *ent ) {
	gclient_t *cl;
	gentity_t *e;
	int i, numSorted;
	char *spaces = "                                    ";

	numSorted = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities,
		"print \"Num |Score|Ping|                Name                |Team|Ready|Delag\n\"" );

	trap_SendServerCommand( ent-g_entities,
		"print \"---------------------------------------------------------------------\n\"" );

	for ( i = 0; i < numSorted; i++) {
		int ping;
		char *ready;
		char *delag;
		int namelen;
		char str[MAX_STRING_CHARS];

		cl = &level.clients[level.sortedClients[i]];
		e = &g_entities[level.sortedClients[i]];

		if ( e->client->sess.invisible && !ent->client->sess.referee ) {
			continue;
		}

		namelen = strlencolor( cl->pers.netname );
		if ( namelen > 36 ) {
			namelen = 36;
		}

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if ( level.warmupTime ) {
			ready = e->readyBegin ? " Yes " : " No ";
		}
		else {
			ready = " N/A ";
		}

		if ( g_delagHitscan.integer ) {
			if ( cl->pers.delag & 1 ) {
				delag = " Yes ";
			}
			else if ( cl->pers.delag & 30 ) {
				delag = "Part ";
			}
			else {
				delag = " No  ";
			}
		}
		else {
			delag = " N/A ";
		}

		Com_sprintf( str, sizeof(str),
			"print \"%4d %5d %4d %s" S_COLOR_WHITE "%s %-6s^7 %-5s %-5s\n\"",
			level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],
			ping,
			cl->pers.netname,
			&spaces[namelen],
			TeamNameColor(cl->sess.sessionTeam),
			ready, delag );

		trap_SendServerCommand( ent-g_entities, str );
	}
}

void Cmd_Ready_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_REDCOACH || ent->client->sess.sessionTeam == TEAM_BLUECOACH ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Spectators and Coaches cannot be ready\n\""));
		return;
	}

	if ( ent->readyBegin ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You're already ready\n\""));
		return;
	}

	if ( !g_doReady.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Ready is not required on this server\n\""));
		return;
	}

	if ( level.warmupTime == 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Ready is not required after warmup\n\""));
		return;
	}

	ent->readyBegin = qtrue;
	G_SendServerCommand( -1, va("print \"%s " S_COLOR_WHITE "^3is ready!\n\"", ent->client->pers.netname ) ); //ssdemo
}

void Cmd_NotReady_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_REDCOACH || ent->client->sess.sessionTeam == TEAM_BLUECOACH ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Spectators cannot be ready\n\""));
		return;
	}

	if ( !ent->readyBegin ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You're already not ready\n\""));
		return;
	}

	ent->readyBegin = qfalse;
	G_SendServerCommand( -1, va("print \"%s " S_COLOR_WHITE "is not ready\n\"", ent->client->pers.netname ) ); //ssdemo
}

void Cmd_TimeOut_f( gentity_t *ent ) {

	if ( !g_doTimeouts.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Team timeouts are not enabled on this server\n\""));
		return;
	}

	if ( level.warmupTime != 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Timeouts are not allowed during warmup\n\""));
		return;
	}

	if ( level.timein > level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"There is already a timeout in progress\n\""));
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_RED ) {
		if ( level.time - level.timein < 10000 && level.timeoutteam == TEAM_RED ) {
			trap_SendServerCommand( ent-g_entities, va("print \"It has been less than 10 seconds since the last red timeout\n\""));
			return;
		}

		level.redtimeouts++;
		if ( level.redtimeouts > g_numTimeouts.integer ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Red team has no timeouts left\n\""));
			return;
		}

		G_SendServerCommand( -1, va("cp \"Red team (%s" S_COLOR_WHITE ")\ncalled a timeout (%d of %d)\n\"", ent->client->pers.netname, level.redtimeouts, g_numTimeouts.integer)); //ssdemo
		level.timeoutteam = TEAM_RED;
	}
	else if ( ent->client->sess.sessionTeam == TEAM_BLUE ) {
		if ( level.time - level.timein < 10000 && level.timeoutteam == TEAM_BLUE ) {
			trap_SendServerCommand( ent-g_entities, va("print \"It has been less than 10 seconds since the last blue timeout\n\""));
			return;
		}

		level.bluetimeouts++;
		if ( level.bluetimeouts > g_numTimeouts.integer ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Blue team has no timeouts left\n\""));
			return;
		}

		G_SendServerCommand( -1, va("cp \"Blue team (%s" S_COLOR_WHITE ")\ncalled a timeout (%d of %d)\n\"", ent->client->pers.netname, level.bluetimeouts, g_numTimeouts.integer)); //ssdemo
		level.timeoutteam = TEAM_BLUE;
	}
	else {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be on a team to call a timeout\n\""));
		return;
	}

	level.timein = level.time + g_maxTimeout.integer * 1000;
	level.timeout = level.time;
	SendTimeoutEvent();
}

void Cmd_TimeIn_f( gentity_t *ent ) {

	if ( level.timein <= level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"There is no timeout in progress\n\""));
		return;
	}

	if ( level.timein - level.time < 3000 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"There are less than three seconds left in the current timeout\n\""));
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_RED ) {
		if ( level.timeoutteam != TEAM_RED ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Red team did not call the timeout\n\""));
			return;
		}

		G_SendServerCommand( -1, va("cp \"Red team (%s" S_COLOR_WHITE ")\nended the timeout\n\"", ent->client->pers.netname)); //ssdemo
	}
	else if ( ent->client->sess.sessionTeam == TEAM_BLUE ) {
		if ( level.timeoutteam != TEAM_BLUE ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Blue team did not call the timeout\n\""));
			return;
		}

		G_SendServerCommand( -1, va("cp \"Blue team (%s" S_COLOR_WHITE ")\nended the timeout\n\"", ent->client->pers.netname)); //ssdemo
	}
	else {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be on a team to end a timeout\n\""));
		return;
	}

	level.timein = level.time + 5000;
	SendTimeoutEvent();
}

void Cmd_Lock_f( gentity_t *ent ) {
	if ( !g_doTeamLocks.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Teams cannot be locked by team members on this server\n\""));
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_RED ) {
		if ( level.redlocked ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Red team is already locked\n\""));
		}
		else {
			level.redlocked = qtrue;
			G_SendServerCommand( -1, va("print \"Red team locked by %s\n\"", ent->client->pers.netname)); //ssdemo
		}
	}
	else if ( ent->client->sess.sessionTeam == TEAM_BLUE ) {
		if ( level.bluelocked ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already locked\n\""));
		}
		else {
			level.bluelocked = qtrue;
			G_SendServerCommand( -1, va("print \"Blue team locked by %s\n\"", ent->client->pers.netname)); //ssdemo
		}
	}
	else {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be on a team to lock a team\n\""));
	}
}

void Cmd_Unlock_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_RED ) {
		if ( !level.redlocked ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Red team is already unlocked\n\""));
		}
		else {
			level.redlocked = qfalse;
			G_SendServerCommand( -1, va("print \"Red team unlocked by %s\n\"", ent->client->pers.netname)); //ssdemo
		}
	}
	else if ( ent->client->sess.sessionTeam == TEAM_BLUE ) {
		if ( !level.bluelocked ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already unlocked\n\""));
		}
		else {
			level.bluelocked = qfalse;
			G_SendServerCommand( -1, va("print \"Blue team unlocked by \n\"", ent->client->pers.netname)); //ssdemo
		}
	}
	else {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be on a team to unlock a team\n\""));
	}
}

void Cmd_Ref_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	qboolean goodpass = qfalse;

	if ( g_refPassword.string[0] == '\0' || !Q_stricmp( g_refPassword.string, "none" ) ) {
		trap_SendServerCommand( ent-g_entities, va("print \"This server is not configured for referee access\n\""));
		return;
	}

	if ( (trap_Argc() < 2 && !ent->client->sess.referee) || trap_Argc() > 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref <password>\n\""));
		return;
	}

	if ( trap_Argc() == 2 ) {
		trap_Argv( 1, buffer, sizeof( buffer ) );
		if ( strcmp( buffer, g_refPassword.string ) == 0 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"Referee login OK\n\""));
			ent->client->sess.referee = qtrue;
			ent->client->sess.guestref = qfalse;
		}
		else {
			trap_SendServerCommand( ent-g_entities, va("print \"Referee login failed\n\""));
			return;
		}
	}

	{
		char temp[256];
		char *refCmds[] = {
			"unref",
			"ref_clients",
			"ref_kick",
			"ref_ban",
			"ref_mute",
			"ref_remove",
			"ref_putred",
			"ref_putblue",
			"ref_putredcoach",
			"ref_putbluecoach",
			"ref_swap",
			"ref_makeref",
			"ref_restart",
			"ref_map",
			"ref_nextmap",
			"ref_allready",
			"ref_allnotready",
			"ref_lock",
			"ref_lockblue",
			"ref_lockred",
			"ref_unlock",
			"ref_unlockblue",
			"ref_unlockred",
			"ref_speclock",
			"ref_speclockblue",
			"ref_speclockred",
			"ref_specunlock",
			"ref_specunlockblue",
			"ref_specunlockred",
			"ref_timeout/ref_pause",
			"ref_timein/ref_unpause",
			"ref_addip",
			"ref_removeip",
			"ref_listip",
			"ref_invisible",
			"ref_wallhack",
			"ref_vote",
			"say_ref",
			"say_blue",
			"say_red",
		};
		const int nCmds = sizeof(refCmds) / sizeof(char *);
		const char *spaces = "                          ";
		const int nSpaces = strlen( spaces );
		int i, j;
		const int	nCols = 3;

		trap_SendServerCommand( ent-g_entities, va("print \"" S_COLOR_CYAN "These are your commands:\n\""));

		for ( i = 0; i < nCmds; i++ ) {
			j = strlen( refCmds[i] );
			if ( j > nSpaces )
				j = nSpaces;

			if ( i % nCols == nCols - 1 || i == nCmds - 1 )
				Com_sprintf( temp, sizeof(temp), "print \"" S_COLOR_YELLOW "%s\n\"", refCmds[i] );
			else
				Com_sprintf( temp, sizeof(temp), "print \"" S_COLOR_YELLOW "%s%s\"", refCmds[i], &spaces[j] );

			trap_SendServerCommand( ent-g_entities, temp );
		}
	}
}

static qboolean CheckRef( gentity_t *ent, qboolean noguest ) {
	if ( !ent->client || !ent->client->sess.referee ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be a referee to use this command\n\""));
		return qfalse;
	}

	if ( ent->client->sess.guestref && noguest ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You are not allowed to do this as a guest referee\n\""));
		return qfalse;
	}

	return qtrue;
}

void Cmd_Unref_f( gentity_t *ent ) {
	if ( trap_Argc() == 1 ) {
		if ( !CheckRef( ent, qfalse ) ) return;

		ent->client->sess.referee = qfalse;
		ent->client->sess.guestref = qfalse;
		ent->client->sess.invisible = qfalse;
		ent->client->sess.wallhack = qfalse;

		ClientUserinfoChanged( ent->s.clientNum );
	}
	else {
		char buffer[MAX_TOKEN_CHARS];
		int clientnum;
		gentity_t *client;

		if ( !CheckRef( ent, qtrue ) ) return;

		if ( trap_Argc() != 2 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"usage: unref [clientnum] (clientnum is optional)\n\""));
			return;
		}

		trap_Argv( 1, buffer, sizeof( buffer ) );
		clientnum = atoi( buffer );

		if ( !CheckClientNum( ent, clientnum ) ) return;

		client = &g_entities[clientnum];
		client->client->sess.referee = qfalse;
		client->client->sess.guestref = qfalse;
		client->client->sess.invisible = qfalse;
		client->client->sess.wallhack = qfalse;
		
		ClientUserinfoChanged( client->s.clientNum );
	}
}

void Cmd_RefKick_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_kick <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	trap_DropClient( clientnum, "was kicked" );
}

void Cmd_RefRemove_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_remove <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "spectator", qtrue );
}

void Cmd_RefPutRed_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_putred <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "red", qtrue );
}

void Cmd_RefPutBlue_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_putblue <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "blue", qtrue );
}

void Cmd_RefPutBlueCoach_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_putbluecoach <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeamCoach( client, "blue", qtrue );
}

void Cmd_RefPutRedCoach_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_putredcoach <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeamCoach( client, "red", qtrue );
}

void Cmd_RefSwap_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum1, clientnum2;
	gentity_t *client1, *client2;
	char *t1, *t2;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( trap_Argc() != 3 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_swap <clientnum> <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum1 = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum1 ) ) return;

	trap_Argv( 2, buffer, sizeof( buffer ) );
	clientnum2 = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum2 ) ) return;

	client1 = &g_entities[clientnum1];
	client2 = &g_entities[clientnum2];

	if ( client1->client->sess.sessionTeam == client2->client->sess.sessionTeam ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You may not swap players on the same team.\n\""));
		return;
	}
	if ( client1->client->sess.sessionTeam == TEAM_REDCOACH || client1->client->sess.sessionTeam == TEAM_BLUECOACH ||
		client2->client->sess.sessionTeam == TEAM_REDCOACH || client2->client->sess.sessionTeam == TEAM_BLUECOACH){
		trap_SendServerCommand( ent-g_entities, va("print \"You may not swap coaches.\n\""));
		return;
	}

	switch ( client1->client->sess.sessionTeam ) {
	case TEAM_RED:
		t1 = "r";
		break;

	case TEAM_BLUE:
		t1 = "b";
		break;

	case TEAM_SPECTATOR:
		t1 = "s";
		break;

	default:
		t1 = "f";
		break;
	}

	switch ( client2->client->sess.sessionTeam ) {
	case TEAM_RED:
		t2 = "r";
		break;

	case TEAM_BLUE:
		t2 = "b";
		break;

	case TEAM_SPECTATOR:
		t2 = "s";
		break;

	default:
		t2 = "f";
		break;
	}

	client1->freezeState = qfalse;
	client2->freezeState = qfalse;

	SetTeam( client1, t2, qtrue );
	SetTeam( client2, t1, qtrue );
}

void Cmd_RefMakeRef_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;

	if ( !CheckRef( ent, qtrue ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_makeref <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	g_entities[clientnum].client->sess.referee = qtrue;
	g_entities[clientnum].client->sess.guestref = qtrue;
}

void Cmd_RefBan_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;

	if ( !CheckRef( ent, qtrue ) ) return;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: ref_ban <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	trap_DropClient( clientnum, "was banned" );
	trap_SendConsoleCommand( EXEC_APPEND, va("addip %s\n", g_entities[clientnum].client->sess.ip ) );
}

void Cmd_RefMute_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gclient_t *client;

	if ( !CheckRef( ent, qtrue ) ) return;

	if ( trap_Argc() != 2 ) {
		PrintMutedList( ent );
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = g_entities[clientnum].client;

	client->pers.muted = !client->pers.muted;

	if ( client->pers.muted ) {
		G_SendServerCommand( -1, va("print \"%s ^7has muted %s^7.\n\"", ent->client->pers.netname, client->pers.netname)); //ssdemo
	}
	else {
		G_SendServerCommand( -1, va("print \"%s ^7has unmuted %s^7.\n\"", ent->client->pers.netname, client->pers.netname)); //ssdemo
	}
}

void Cmd_RefClients_f( gentity_t *ent ) {
	gclient_t *cl;
	gentity_t *e;
	int i, numSorted;
	char *spaces = "                            ";

	if ( !CheckRef( ent, qfalse ) ) return;

	numSorted = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities,
		"print \"Num |Score|Ping|            Name            |    Address    |Team|Ready|Delag\n\"" );
	trap_SendServerCommand( ent-g_entities,
		"print \"-----------------------------------------------------------------------------\n\"" );

	for ( i = 0; i < numSorted; i++) {
		int ping;
		char *ready;
		char *delag;
		int namelen;

		cl = &level.clients[level.sortedClients[i]];
		e = &g_entities[level.sortedClients[i]];

		namelen = strlencolor( cl->pers.netname );
		if ( namelen > 28 ) {
			namelen = 28;
		}

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if ( level.warmupTime ) {
			ready = e->readyBegin ? " Yes " : " No ";
		}
		else {
			ready = " N/A ";
		}

		if ( g_delagHitscan.integer ) {
			if ( cl->pers.delag & 1 ) {
				delag = " Yes ";
			}
			else if ( cl->pers.delag & 30 ) {
				delag = "Part ";
			}
			else {
				delag = " No  ";
			}
		}
		else {
			delag = " N/A ";
		}

		trap_SendServerCommand( ent-g_entities, 
			va( "print \"%4d %5d %4d %s" S_COLOR_WHITE "%s %-15s %-6s^7 %-5s %-5s\n\"",
				level.sortedClients[i],
				cl->ps.persistant[PERS_SCORE],
				ping,
				cl->pers.netname,
				&spaces[namelen],
				cl->sess.ip,
				TeamNameColor(cl->sess.sessionTeam),
				ready, delag )
			);
	}
}

void Cmd_RefRestart_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	SaveGameState();
	trap_SendConsoleCommand( EXEC_APPEND, va("map_restart 0\n") );
}

void Cmd_RefAllReady_f( gentity_t *ent ) {
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( !g_doReady.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Ready is not required on this server\n\""));
		return;
	}

	for ( client = g_entities; client < &g_entities[level.maxclients]; client++ ) {
		if ( !client->inuse || !client->client ) {
			continue;
		}

		client->readyBegin = qtrue;
	}

	G_SendServerCommand( -1, va("print \"^3Everyone is ready!\n\"")); //ssdemo
}

void Cmd_RefAllNotReady_f( gentity_t *ent ) {
	gentity_t *client;

	if ( !CheckRef( ent, qfalse ) ) return;

	for ( client = g_entities; client < &g_entities[level.maxclients]; client++ ) {
		if ( !client->inuse || !client->client ) {
			continue;
		}

		client->readyBegin = qfalse;
	}

	G_SendServerCommand( -1, va("print \"Everyone is not ready\n\"")); //ssdemo
}

void Cmd_RefLock_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	Cmd_RefLockBlue_f( ent );
	Cmd_RefLockRed_f( ent );
}

void Cmd_RefLockBlue_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.bluelocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already locked\n\""));
	}
	else {
		level.bluelocked = qtrue;
		G_SendServerCommand( -1, va("print \"Blue team locked by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefLockRed_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.redlocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Red team is already locked\n\""));
	}
	else {
		level.redlocked = qtrue;
		G_SendServerCommand( -1, va("print \"Red team locked by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefUnlock_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	Cmd_RefUnlockBlue_f( ent );
	Cmd_RefUnlockRed_f( ent );
}

void Cmd_RefUnlockBlue_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( !level.bluelocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already unlocked\n\""));
	}
	else {
		level.bluelocked = qfalse;
		G_SendServerCommand( -1, va("print \"Blue team unlocked by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefUnlockRed_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( !level.redlocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Red team is already unlocked\n\""));
	}
	else {
		level.redlocked = qfalse;
		G_SendServerCommand( -1, va("print \"Red team unlocked by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefSpecLock_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	Cmd_RefSpecLockBlue_f( ent );
	Cmd_RefSpecLockRed_f( ent );
}

void Cmd_RefSpecLockBlue_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.bluespeclocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already locked against spectating\n\""));
	}
	else {
		level.bluespeclocked = qtrue;
		G_SendServerCommand( -1, va("print \"Blue team locked against spectating by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefSpecLockRed_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.redspeclocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Red team is already locked against spectating\n\""));
	}
	else {
		level.redspeclocked = qtrue;
		G_SendServerCommand( -1, va("print \"Red team locked against spectating by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefSpecUnlock_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	Cmd_RefSpecUnlockBlue_f( ent );
	Cmd_RefSpecUnlockRed_f( ent );
}

void Cmd_RefSpecUnlockBlue_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( !level.bluespeclocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Blue team is already unlocked for spectating\n\""));
	}
	else {
		level.bluespeclocked = qfalse;
		G_SendServerCommand( -1, va("print \"Blue team unlocked for spectating by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefSpecUnlockRed_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( !level.redspeclocked ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Red team is already unlocked for spectating\n\""));
	}
	else {
		level.redspeclocked = qfalse;
		G_SendServerCommand( -1, va("print \"Red team unlocked for spectating by referee (%s" S_COLOR_WHITE ")\n\"", ent->client->pers.netname)); //ssdemo
	}
}

void Cmd_RefTimeout_f( gentity_t *ent ) {

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.timein > level.time ) {
		G_SendServerCommand( -1, va("cp \"A referee (%s" S_COLOR_WHITE ")\nextended the timeout\n\"", ent->client->pers.netname)); //ssdemo
		level.timein = level.timein + 300 * 1000;
		if ( level.timein > level.time + 600 * 1000 ) {
			level.timein = level.time + 600 * 1000;
		}
	}
	else {
		G_SendServerCommand( -1, va("cp \"A referee (%s" S_COLOR_WHITE ")\ncalled a timeout\n\"", ent->client->pers.netname)); //ssdemo
		level.timeoutteam = TEAM_FREE;
		level.timein = level.time + 600 * 1000;
		level.timeout = level.time;
	}

	SendTimeoutEvent();
}

void Cmd_RefTimein_f( gentity_t *ent ) {

	if ( !CheckRef( ent, qfalse ) ) return;

	if ( level.timein <= level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"There is no timeout in progress\n\""));
		return;
	}

	if ( level.timein - level.time < 3000 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"There are less than three seconds left in the current timeout\n\""));
		return;
	}

	G_SendServerCommand( -1, va("cp \"A referee (%s" S_COLOR_WHITE ")\nended the timeout\n\"", ent->client->pers.netname)); //ssdemo

	level.timein = level.time + 5000;
	SendTimeoutEvent();
}

void Cmd_RefMap_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	char s[MAX_STRING_CHARS];
	char cmd[MAX_STRING_CHARS];

	if ( !CheckRef( ent, qfalse ) ) return;

	trap_Argv( 1, buffer, sizeof( buffer ) );

	trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
	if (*s) {
		Com_sprintf( cmd, sizeof(cmd), "map %s; set nextmap \"%s\"", buffer, s );
	} else {
		Com_sprintf( cmd, sizeof(cmd), "map %s", buffer );
	}

	SaveGameState();
	trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", cmd) );
}

void Cmd_RefNextmap_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	SaveGameState();
	trap_SendConsoleCommand( EXEC_APPEND, va("vstr nextmap\n") );
}

void Cmd_RefAddIP_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];	

	if ( !CheckRef( ent, qtrue ) ) return;

	trap_Argv( 1, buffer, sizeof( buffer ) );
	trap_SendConsoleCommand( EXEC_APPEND, va("addip %s\n", buffer) );
}

void Cmd_RefRemoveIP_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];	

	if ( !CheckRef( ent, qtrue ) ) return;

	trap_Argv( 1, buffer, sizeof( buffer ) );
	trap_SendConsoleCommand( EXEC_APPEND, va("removeip %s\n", buffer) );
}

void Cmd_RefListIP_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qtrue ) ) return;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", g_banIPs.string) );
}

void Cmd_RefVote_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qtrue ) ) return;

	Cmd_Vote_f( ent, qtrue );
}

static void FakeClientDisconnect( gentity_t *ent ) {
	gentity_t	*tent;
	int			i;

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( is_spectator( &level.clients[ i ] ) &&
				( level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW ||
				level.clients[i].sess.spectatorState == SPECTATOR_FROZEN ) &&
				level.clients[i].sess.spectatorClient == ent->s.clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect
	if ( !is_spectator( ent->client ) ) {
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

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == ent->s.clientNum ) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );
	}

	StopFollowing( ent );
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	ent->client->sess.spectatorTime = level.time;
	ent->client->sess.teamLeader = qfalse;

	ent->freezeState = qfalse;

	CalculateRanks();

	ClientUserinfoChanged( ent->s.clientNum );
}

void Cmd_RefInvisible_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( ent->client->sess.invisible ) {
		G_SendServerCommand( -1, va("print \"%s ^7connected\n\"", ent->client->pers.netname) ); //ssdemo

		ent->client->sess.invisible = qfalse;

		G_LogPrintf( "Ref_Visible: %i\n", ent->s.clientNum );
	}
	else {
		G_SendServerCommand( -1, va("print \"%s ^7disconnected\n\"", ent->client->pers.netname) ); //ssdemo

		FakeClientDisconnect( ent );

		ent->client->sess.invisible = qtrue;

		G_LogPrintf( "Ref_Invisible: %i\n", ent->s.clientNum );
	}

	ClientUserinfoChanged( ent->s.clientNum );
}

void Cmd_RefWallhack_f( gentity_t *ent ) {
	if ( !CheckRef( ent, qfalse ) ) return;

	if ( ent->client->sess.wallhack ) {
		ent->client->sess.wallhack = qfalse;
		trap_SendServerCommand( ent-g_entities, va("print \"Referee wall hack is disabled\n\""));

		G_LogPrintf( "Ref_UnWallhack: %i\n", ent->s.clientNum );
	}
	else {
		ent->client->sess.wallhack = qtrue;
		trap_SendServerCommand( ent-g_entities, va("print \"Referee wall hack is enabled (only when you spectate another player)\n\""));

		G_LogPrintf( "Ref_Wallhack: %i\n", ent->s.clientNum );
	}
}

void Cmd_CoachInvite_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( trap_Argc() != 2 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: coachinvite <clientnum>\n\""));
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( ent, clientnum ) ) return;

	client = &g_entities[clientnum];
	
	if(ent->client->sess.sessionTeam == TEAM_RED){
		client->client->coachInvitedTeam = TEAM_RED;
		trap_SendServerCommand( client-g_entities, va("print \"You have been invited to coach the red team\n\""));
		return;

	}else if(ent->client->sess.sessionTeam == TEAM_BLUE){
		client->client->coachInvitedTeam = TEAM_BLUE;
		trap_SendServerCommand( client-g_entities, va("print \"You have been invited to coach the blue team\n\""));
		return;
	}else{
		trap_SendServerCommand( ent-g_entities, va("print \"You must be on either the red or blue team to invite a coach\n\""));
		return;
	}
}

void Cmd_Coach_f( gentity_t *ent ) {

	if(ent->client->coachInvitedTeam == TEAM_RED){
		SetTeamCoach(ent,"red",qtrue);
		return;
	}else if(ent->client->coachInvitedTeam == TEAM_BLUE){
		SetTeamCoach(ent,"blue",qtrue);
		return;
	}else{
		trap_SendServerCommand( ent-g_entities, va("print \"You have not been invited to coach any teams\n\""));
		return;
	}
}

#define MAX_PRINT 900
#define MAX_FILE 14000
#define MAX_LINES 40

const char *g_szIllegalChars = "\\\";";

void Cmd_Help_f( gentity_t *ent ) {
	char path[MAX_QPATH], buff[MAX_QPATH], filechars[MAX_FILE + 1], line[MAX_PRINT + 1];
	fileHandle_t f;
	int len, i, j, lines;
	qboolean eol;
	char lastchar;

	if ( trap_Argc() != 2 ) {
		strcpy( buff, "help" );
	}
	else {
		trap_Argv( 1, buff, sizeof(buff) );
	}

	Com_sprintf( path, sizeof(path), "help/%s.txt", buff );

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f || !len ) {
		trap_SendServerCommand( ent-g_entities, va("print \"unable to open help file %s\n\"", buff) );
		return;
	}

	len = Q_min(len, sizeof(filechars) - 1);

	trap_FS_Read( filechars, len, f );
	filechars[len] = '\0';

	i = 0;
	j = 0;
	lines = 0;
	eol = qfalse;
	while ( i < len ) {
		// skip illegal characters
		if ( strchr(g_szIllegalChars, filechars[i]) ) {
			i++;
			continue;
		}

		line[j] = filechars[i];

		lastchar = line[j];

		if ( line[j] == '\r' || line[j] == '\n' ) {
			eol = qtrue;
		}
		else {
			if ( eol ) {
				line[j] = '\0';
				trap_SendServerCommand( ent-g_entities, va("print \"%s\"", line) );
				eol = qfalse;

				lines++;
				if ( lines >= MAX_LINES ) {
					break;
				}

				line[0] = filechars[i];
				j = 0;
			}
		}

		i++;
		j++;

		if ( j >= MAX_PRINT ) {
			line[j] = '\0';
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", line) );

			lines++;
			if ( lines >= MAX_LINES ) {
				break;
			}

			j = 0;
		}
	}

	if ( lines < MAX_LINES ) {
		line[j] = '\0';	
		trap_SendServerCommand( ent-g_entities, va("print \"%s\"", line) );
	}

	if ( lastchar != '\r' && lastchar != '\n' ) {
		trap_SendServerCommand( ent-g_entities, va("print \"\n\"") );
	}

/*
	for ( i = 0; i < len; i += MAX_PRINT ) {
		Q_strncpyz( line, &filechars[i], sizeof(line) );
		trap_SendServerCommand( ent-g_entities, va("print \"%s\"", line) );
	}
*/
	trap_FS_FCloseFile( f );
}

void Svcmd_Remove_f( void ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( trap_Argc() != 2 ) {
		G_Printf( "usage: remove <clientnum>\n" );
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( NULL, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "spectator", qtrue );
}

void Svcmd_PutRed_f( void ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( trap_Argc() != 2 ) {
		G_Printf( "usage: putred <clientnum>\n" );
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( NULL, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "red", qtrue );
}

void Svcmd_PutBlue_f( void ) {
	char buffer[MAX_TOKEN_CHARS];
	int clientnum;
	gentity_t *client;

	if ( trap_Argc() != 2 ) {
		G_Printf( "usage: putblue <clientnum>\n" );
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	clientnum = atoi( buffer );

	if ( !CheckClientNum( NULL, clientnum ) ) return;

	client = &g_entities[clientnum];
	client->freezeState = qfalse;
	SetTeam( client, "blue", qtrue );
}

void Svcmd_HardRestart_f( void ) {
	char mapcmd[MAX_STRING_CHARS];
	char nextmap[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );

	if ( nextmap[0] != '\0' ) {
		if ( sv_cheats.integer ) {
			Com_sprintf( mapcmd, sizeof(mapcmd), "devmap %s; set nextmap \"%s\"", sv_mapname.string, nextmap );
		}
		else {
			Com_sprintf( mapcmd, sizeof(mapcmd), "map %s; set nextmap \"%s\"", sv_mapname.string, nextmap );
		}
	}
	else {
		if ( sv_cheats.integer ) {
			Com_sprintf( mapcmd, sizeof(mapcmd), "devmap %s", sv_mapname.string );
		}
		else {
			Com_sprintf( mapcmd, sizeof(mapcmd), "map %s", sv_mapname.string );
		}
	}

	trap_SendConsoleCommand( EXEC_APPEND, mapcmd );
}

int G_ItemDisabled( gitem_t *item );

qboolean WeaponDisabled( gitem_t *item ) {
	if ( g_weaponlimit.integer & 1 ) {
		if ( !Q_stricmp( item->classname, "ammo_bullets" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 2 ) {
		if ( !Q_stricmp( item->classname, "weapon_shotgun" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_shells" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 4 ) {
		if ( !Q_stricmp( item->classname, "weapon_grenadelauncher" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_grenades" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 8 ) {
		if ( !Q_stricmp( item->classname, "weapon_rocketlauncher" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_rockets" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 16 ) {
		if ( !Q_stricmp( item->classname, "weapon_lightning" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_lightning" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 32 ) {
		if ( !Q_stricmp( item->classname, "weapon_railgun" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_slugs" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 64 ) {
		if ( !Q_stricmp( item->classname, "weapon_plasmagun" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_cells" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 128 ) {
		if ( !Q_stricmp( item->classname, "weapon_bfg" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_bfg" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 256 ) {
		if ( !Q_stricmp( item->classname, "weapon_nailgun" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_nails" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 512 ) {
		if ( !Q_stricmp( item->classname, "weapon_prox_launcher" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_mines" ) ) {
			return qtrue;
		}
	}
	if ( g_weaponlimit.integer & 1024 ) {
		if ( !Q_stricmp( item->classname, "weapon_chaingun" ) ) {
			return qtrue;
		}
		if ( !Q_stricmp( item->classname, "ammo_belt" ) ) {
			return qtrue;
		}
	}

	if ( G_ItemDisabled( item ) ) {
		return qtrue;
	}
	return qfalse;
}

void RegisterWeapon( void ) {
	if ( g_wpflags.integer & 2 ) {
		RegisterItem( BG_FindItemForWeapon( WP_SHOTGUN ) );
	}
	if ( g_wpflags.integer & 4 ) {
		RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );
	}
	if ( g_wpflags.integer & 8 ) {
		RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );
	}
	if ( g_wpflags.integer & 16 ) {
		RegisterItem( BG_FindItemForWeapon( WP_LIGHTNING ) );
	}
	if ( g_wpflags.integer & 32 ) {
		RegisterItem( BG_FindItemForWeapon( WP_RAILGUN ) );
	}
	if ( g_wpflags.integer & 64 ) {
		RegisterItem( BG_FindItemForWeapon( WP_PLASMAGUN ) );
	}
	if ( g_wpflags.integer & 128 ) {
		RegisterItem( BG_FindItemForWeapon( WP_BFG ) );
	}
#ifdef MISSIONPACK
	if ( g_wpflags.integer & 256 ) {
		RegisterItem( BG_FindItemForWeapon( WP_NAILGUN ) );
	}
	if ( g_wpflags.integer & 512 ) {
		RegisterItem( BG_FindItemForWeapon( WP_PROX_LAUNCHER ) );
	}
	if ( g_wpflags.integer & 1024 ) {
		RegisterItem( BG_FindItemForWeapon( WP_CHAINGUN ) );
	}
#endif

	//RegisterItem( BG_FindItemForWeapon( WP_GRAPPLING_HOOK ) );

	VectorClear( redflag );
	VectorClear( blueflag );
}

void SpawnWeapon( gclient_t *client ) {
	int	i;

	if ( g_weaponlimit.integer & 1 && !( g_wpflags.integer & 1 ) ) {
		client->ps.stats[ STAT_WEAPONS ] &= ~( 1 << WP_MACHINEGUN );
		client->ps.ammo[ WP_MACHINEGUN ] = 0;
	}
	if ( g_wpflags.integer & 2 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_SHOTGUN;
		client->ps.ammo[ WP_SHOTGUN ] = ammo_sg.integer;
	}
	if ( g_wpflags.integer & 4 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_GRENADE_LAUNCHER;
		client->ps.ammo[ WP_GRENADE_LAUNCHER ] = ammo_gl.integer;
	}
	if ( g_wpflags.integer & 8 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_ROCKET_LAUNCHER;
		client->ps.ammo[ WP_ROCKET_LAUNCHER ] = ammo_rl.integer;
	}
	if ( g_wpflags.integer & 16 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_LIGHTNING;
		client->ps.ammo[ WP_LIGHTNING ] = ammo_lg.integer;
	}
	if ( g_wpflags.integer & 32 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_RAILGUN;
		client->ps.ammo[ WP_RAILGUN ] = ammo_rg.integer;
	}
	if ( g_wpflags.integer & 64 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_PLASMAGUN;
		client->ps.ammo[ WP_PLASMAGUN ] = ammo_pg.integer;
	}
	if ( g_wpflags.integer & 128 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_BFG;
		client->ps.ammo[ WP_BFG ] = ammo_bfg.integer;
	}
#ifdef MISSIONPACK
	if ( g_wpflags.integer & 256 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_NAILGUN;
		client->ps.ammo[ WP_NAILGUN ] = 20;
	}
	if ( g_wpflags.integer & 512 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_PROX_LAUNCHER;
		client->ps.ammo[ WP_PROX_LAUNCHER ] = 10;
	}
	if ( g_wpflags.integer & 1024 ) {
		client->ps.stats[ STAT_WEAPONS ] |= 1 << WP_CHAINGUN;
		client->ps.ammo[ WP_CHAINGUN ] = 100;
	}
#endif

	if ( g_dmflags.integer & 1024 ) {
		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			client->ps.ammo[ i ] = 200;
		}
	}
}

int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[ 2 ] );

int InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	vec3_t	intersections[ 2 ], vec;
	int	n;

	if ( !targ->freezeState ) {
		return qfalse;
	}
	VectorCopy( dir, vec );
	VectorInverse( vec );
	n = RaySphereIntersections( targ->r.currentOrigin, 16, point, vec, intersections );
	if ( n > 0 ) {
		VectorSubtract( intersections[ 0 ], targ->r.currentOrigin, vec );
		if ( impactpoint ) {
			VectorCopy( intersections[ 0 ], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	return qfalse;
}

void BG_RemoveColors( char *str ) {
	int		i, j;

	i = 0; j = 0;
	for ( ; str[i] != '\0'; i++, j++ ) {
		if ( str[i] == '^' ) {
			j -= 1;
			if ( str[i + 1] )
				i += 1;
		} else {
			str[j] = str[i];
		}
	}

	str[j] = '\0';
}

/*
==================
G_ClipVelocity

Slide or bounce off of the impacting surface
==================
*/
#define	OVERCLIP		1.001f

void G_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float time_left, float friction ) {
	float	backoff, frac;

	// find the magnitude of the vector "in" along "normal"
	backoff = DotProduct (in, normal);

	// is it big?
	if ( backoff < -400.0f ) {
		float len;

		// yeah, so bounce
		VectorMA( in, -2.0f * backoff, normal, out );

		// totally elastic collisions suck (boing boing boing)
		frac = 1.0f / friction * 1.5f;
		if ( frac > 0.9f ) {
			frac = 0.9f;
		}
		VectorScale( out, frac, out );

		// clamp the vector to under 320 length
		// practical only: keeps from bouncing off space maps
		len = VectorLength( out );
		if ( len > 320.0f ) {
			VectorScale( out, 320.0f / len, out );
		}
	}
	else {
		// tilt the plane a bit to avoid floating-point error issues
		if ( backoff < 0 ) {
			backoff *= OVERCLIP;
		} else {
			backoff /= OVERCLIP;
		}

		// slide along
		VectorMA( in, -backoff, normal, out );

		// friction due to gravity only (don't care otherwise)
		if ( normal[2] > 0.0f ) {
			frac = 1.0f - friction * time_left * normal[2];
			if ( frac < 0.0f ) {
				// shouldn't happen
				frac = 0.0f;
			}
			VectorScale( out, frac, out );
		}
	}
}

#define	MAX_CLIP_PLANES	5

qboolean G_BodySlideMove( gentity_t *ent ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, velocity, origin;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	float gravity;
	vec3_t		worldUp = { 0.0f, 0.0f, 1.0f };
	
	numbumps = 4;

	VectorCopy( ent->s.pos.trDelta, primal_velocity );
	VectorCopy( primal_velocity, velocity );
	VectorCopy( ent->r.currentOrigin, origin );

	time_left = (float)(level.time - level.previousTime) / 1000.0f;

	// adjust gravity if in water
	if ( ent->waterlevel ) {
		// sink slower than players because of the ice
		gravity = g_gravity.value / 4;
	}
	else {
		gravity = g_gravity.value;
	}

	VectorCopy( velocity, endVelocity );
	endVelocity[2] -= gravity * time_left;
	velocity[2] = ( velocity[2] + endVelocity[2] ) * 0.5;
	primal_velocity[2] = endVelocity[2];

	numplanes = 0;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( origin, time_left, velocity, end );

		// see if we can make it there
		trap_Trace( &trace, origin, ent->r.mins, ent->r.maxs, end, ent->s.number, ent->clipmask );

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			VectorClear( velocity );
			VectorCopy( origin, ent->r.currentOrigin );
			VectorCopy( origin, ent->s.pos.trBase );
			VectorCopy( velocity, ent->s.pos.trDelta );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trType = TR_LINEAR;
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy( trace.endpos, origin );
		}

		if (trace.fraction == 1) {
			break;		// moved the entire distance
		}

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			VectorClear( velocity );
			VectorCopy( origin, ent->r.currentOrigin );
			VectorCopy( origin, ent->s.pos.trBase );
			VectorCopy( velocity, ent->s.pos.trDelta );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trType = TR_LINEAR;
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 ) {
				VectorAdd( trace.plane.normal, velocity, velocity );
				break;
			}
		}

		if ( i < numplanes ) {
			continue;
		}

		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = DotProduct( velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			G_ClipVelocity( velocity, planes[i], clipVelocity, time_left, ent->groundFriction );

			// slide along the plane
			G_ClipVelocity( endVelocity, planes[i], endClipVelocity, time_left, ent->groundFriction );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}

				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				G_ClipVelocity( clipVelocity, planes[j], clipVelocity, time_left, ent->groundFriction );
				G_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, time_left, ent->groundFriction );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}

					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( velocity );
					VectorCopy( origin, ent->r.currentOrigin );
					VectorCopy( origin, ent->s.pos.trBase );
					VectorCopy( velocity, ent->s.pos.trDelta );
					ent->s.pos.trTime = level.time;
					ent->s.pos.trType = TR_LINEAR;
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	VectorCopy( endVelocity, velocity );

	VectorCopy( origin, ent->r.currentOrigin );
	VectorCopy( origin, ent->s.pos.trBase );
	VectorCopy( velocity, ent->s.pos.trDelta );
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR;

	for ( i = 0; i < numplanes; i++ ) {
		if ( planes[i][2] <= 0.0f ) {
			return qtrue;
		}
	}

	return qfalse;
}

#define	BODYSTEPSIZE 18

void G_BodyStepSlideMove( gentity_t *ent ) {
	vec3_t start_o, start_v, down_o, down_v;
	vec3_t down, up;
	trace_t trace;
	float stepSize;
	float frametime;

	frametime = (float)(level.time - level.previousTime) / 1000.0f;

	// grab the accumulated new velocity (accumulates during client frames)
	VectorAdd( ent->s.pos.trDelta, ent->damageVelocity, ent->s.pos.trDelta );
	VectorClear( ent->damageVelocity );

	VectorCopy (ent->r.currentOrigin, start_o);
	VectorCopy (ent->s.pos.trDelta, start_v);

	if ( !G_BodySlideMove( ent ) ) {
		// not clipped, so forget stepping
		return;
	}

	// require an appreciable velocity
	stepSize = Com_Clamp( 5, BODYSTEPSIZE, VectorLength(start_v) / 32.0f );

	VectorCopy( ent->r.currentOrigin, down_o);
	VectorCopy( ent->s.pos.trDelta, down_v);

	VectorCopy (start_o, up);
	up[2] += stepSize;

	// test the player position if they were a stepheight higher
	trap_Trace( &trace, start_o, ent->r.mins, ent->r.maxs, up, ent->s.number, ent->clipmask );
	if ( trace.allsolid ) {
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];

	// try slidemove from this position
	VectorCopy( trace.endpos, ent->r.currentOrigin );
	VectorCopy( trace.endpos, ent->s.pos.trBase );
	VectorCopy( start_v, ent->s.pos.trDelta );

	G_BodySlideMove( ent );

	// push down the final amount
	VectorCopy( ent->r.currentOrigin, down );
	down[2] -= stepSize;
	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, down, ent->s.number, ent->clipmask );
	if ( !trace.allsolid ) {
		VectorCopy( trace.endpos, ent->r.currentOrigin );
		VectorCopy( trace.endpos, ent->s.pos.trBase );
	}
	if ( trace.fraction < 1.0 ) {
		G_ClipVelocity( ent->s.pos.trDelta, trace.plane.normal, ent->s.pos.trDelta, frametime, ent->groundFriction );
	}
}

void G_BodyMove( gentity_t *ent ) {
	float frametime;

	frametime = (float)(level.time - level.previousTime) / 1000.0f;

	G_BodyStepSlideMove( ent );

	// slow down if in water
	if ( ent->waterlevel ) {
		float frac = 1.0f - 5.0f * frametime;
		if ( frac < 0 ) {
			frac = 0;
		}
		VectorScale( ent->s.pos.trDelta, frac, ent->s.pos.trDelta );
	}

	if ( VectorLength( ent->s.pos.trDelta ) < 1.0f ) {
		// way too slow, so cancel what velocity we've got
		VectorClear( ent->s.pos.trDelta );
		return;
	}
}

int G_NumPlayersConnecting( void ) {
	gentity_t *iter;
	int count = 0;

	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || !iter->client ) continue;

		if ( iter->client->pers.connected == CON_CONNECTING ) {
			count++;
		}
	}

	return count;
}

/*
===========
SelectBestFreezeSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
#define MAX_SPAWN_POINTS 512

gentity_t *SelectBestFreezeSpawnPoint( gentity_t *ent, vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t *spot, *iter;
	gentity_t *list_spot[MAX_SPAWN_POINTS];
	vec3_t list_origin[MAX_SPAWN_POINTS];
	vec3_t spotforward, iterforward, delta;
	float dist, threshhold, distweight, dirweight, playerweight;
	int numSpots, rnd, i, shift;
	const float strata[] = { 1.00f, 0.70f, 0.45f, 0.25f, 0.10f, 0.00f };
	const float numStrata = sizeof(strata) / sizeof(float);
	team_t team = ent->client->sess.sessionTeam;

	spot = NULL;

	// assign every spawn point a weight
	while ( ( spot = G_Find(spot, FOFS(classname), "info_player_deathmatch") ) != NULL ) {
		spot->spawnWeight = 0.0f;

		// find the distance between you and the 
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );

		// calculate the distance weight
		if ( dist > 640.0f ) {
			distweight = 1.0f;
		}
		else {
			distweight = dist / 640.0f;
		}

		// set these up for multiplication
		playerweight = 1.0f;
		dirweight = 1.0f;

		// get the spot's forward vector
		AngleVectors( spot->s.angles, spotforward, NULL, NULL );

		// loop through the other clients
		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			float threshdist;

			// if not a player, ignore
			if ( !iter->inuse || !iter->client ) continue;

			// ignore if a spectator
			if ( iter->client->sess.sessionTeam == TEAM_SPECTATOR ||  iter->client->sess.sessionTeam == TEAM_BLUECOACH ||  iter->client->sess.sessionTeam == TEAM_REDCOACH ) continue;
					
			// players shouldn't start too close to team members
			if ( iter->client->sess.sessionTeam == team ) {
				threshdist = 320.0f;
			}
			// or close at ALL to enemies
			else {
				threshdist = 960.0f;
			}

			// get the distance to the enemy
			VectorSubtract( spot->s.origin, iter->r.currentOrigin, delta );
			dist = VectorLength( delta );

			// calculate the enemy distance weight
			if ( dist <= threshdist ) {
				playerweight *= dist / threshdist;

				// take into account the direction the enemy is facing if he's too close
				if ( dist <= 320.0f && iter->client->sess.sessionTeam != team ) {
					AngleVectors( iter->r.currentAngles, iterforward, NULL, NULL );
					dirweight *= (1.0f - DotProduct( iterforward, spotforward )) / 2.0f;
				}
			}
		}

		// set the spawn point's total weight
		spot->spawnWeight = distweight * playerweight * dirweight;
	}

	// loop downward through a few strata
	for ( i = 0; i < numStrata; i++ ) {
		threshhold = strata[i];
		numSpots = 0;
		spot = NULL;

		// loop through the spawn points
		while ( numSpots < MAX_SPAWN_POINTS &&
				( spot = G_Find(spot, FOFS(classname), "info_player_deathmatch") ) != NULL ) {

			// can we use it?
			if ( spot->spawnWeight < threshhold ) {
				continue;
			}

			if ( (spot->flags & FL_NO_BOTS) && (ent->r.svFlags & SVF_BOT) ) {
				continue;
			}

			if ( (spot->flags & FL_NO_HUMANS) && !(ent->r.svFlags & SVF_BOT) ) {
				continue;
			}

			if ( SpotWouldTelefrag( spot ) ) {
				continue;
			}

			// yep!
			list_spot[numSpots] = spot;
			numSpots++;
		}

		// if we have one or more spawn points, stop now
		if ( numSpots >= 1 ) {
			break;
		}
	}

	if ( numSpots ) {
		// select a random spot
		rnd = random() * numSpots;

		// on the off-chance that we get a 1.0 from random()
		if ( rnd >= numSpots ) rnd = numSpots - 1;

		// return the origin and angles
		VectorCopy( list_spot[rnd]->s.origin, origin );
		origin[2] += 9;
		VectorCopy( list_spot[rnd]->s.angles, angles );

		return list_spot[rnd];
	}

	// when all spawn points would telefrag

	// loop downward through a few strata
	for ( i = 0; i < numStrata; i++ ) {
		threshhold = strata[i];
		numSpots = 0;
		spot = NULL;

		// loop through the spawn points
		while ( numSpots < MAX_SPAWN_POINTS &&
				(spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL ) {

			// can we use it?
			if ( spot->spawnWeight < threshhold ) {
				continue;
			}

			if ( (spot->flags & FL_NO_BOTS) && (ent->r.svFlags & SVF_BOT) ) {
				continue;
			}

			if ( (spot->flags & FL_NO_HUMANS) && !(ent->r.svFlags & SVF_BOT) ) {
				continue;
			}

			// loop through some shifts
			for ( shift = 0; shift < 4; shift++ ) {
				vec3_t actualorigin, start;
				trace_t trace;
				vec3_t forward, right, up;

				VectorCopy( spot->s.origin, actualorigin );
				AngleVectors( spot->s.angles, forward, right, up );

				if ( shift == 0 ) {
					VectorMA( actualorigin, 48.0f, forward, actualorigin );
				}
				else if ( shift == 1 ) {
					VectorMA( actualorigin, 64.0f, up, actualorigin );
				}
				else if ( shift == 2 ) {
					VectorMA( actualorigin, 48.0f, right, actualorigin );
				}
				else if ( shift == 3 ) {
					VectorMA( actualorigin, -48.0f, right, actualorigin );
				}

				actualorigin[2] += 9;
				
				if ( PointWouldTelefrag(actualorigin) ) {
					continue;
				}

				VectorCopy( spot->s.origin, start );
				start[2] += 9;

				// is it safe?
				trap_Trace( &trace, start, playerMins, playerMaxs, actualorigin, ENTITYNUM_NONE, MASK_SOLID );

				if ( trace.fraction < 1.0f ) {
					continue;
				}

				// safe enough! note that this will sometimes (rarely) drop players off of cliffs and such,
				// but that's much preferable to a zillion telefrags at the beginning of a match

				list_spot[numSpots] = spot;
				VectorCopy( actualorigin, list_origin[numSpots] );
				numSpots++;
				break;
			}
		}

		// if we have two or more spawn points, stop now
		if ( numSpots >= 1 ) {
			break;
		}
	}

	if ( numSpots ) {
		// select a random spot
		rnd = random() * numSpots;

		// on the off-chance that we get a 1.0 from random()
		if ( rnd >= numSpots ) rnd = numSpots - 1;

		// return the origin and angles
		VectorCopy( list_origin[rnd], origin );
		VectorCopy( list_spot[rnd]->s.angles, angles );

		return list_spot[rnd];
	}

	// we couldn't find one *sniff*

	spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");

	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy( spot->s.origin, origin );
	origin[2] += 9;
	VectorCopy( spot->s.angles, angles );

	return spot;
}

/*
G_CheckFPCmd

This goes at the top of flood-protected commands, like so:

void Cmd_Foo_f( gentity_t *ent ) {
	if ( !G_CheckFPCmd(ent) ) return;

	// actual processing
}
*/
qboolean G_CheckFPCmd( gentity_t *ent ) {
	if ( !ent->inuse || !ent->client ) {
		return qfalse;
	}

	if ( !g_floodProtect.integer || level.time - ent->client->lastFPCmdTime > 1000 ) {
		ent->client->lastFPCmdTime = level.time;
		return qtrue;
	}

	return qfalse;
}

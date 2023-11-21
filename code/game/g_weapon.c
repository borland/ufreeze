// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#define NUM_NAILSHOTS 15

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


/*
======================================================================

GAUNTLET

======================================================================
*/

void Weapon_Gauntlet( gentity_t *ent ) {

}

/*
===============
CheckGauntletAttack
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	VectorMA (muzzle, 32, forward, end);

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}
//freeze
	if ( is_body( traceEnt ) ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;

		if ( is_body_freeze( traceEnt ) ) {
			tent->s.generic1 = 5;
		}
	}
//freeze

	if ( !traceEnt->takedamage) {
		return qfalse;
	}

	if (ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
#ifdef MISSIONPACK
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif
//freeze
	if ( g_dmflags.integer & 1024 && !( g_weaponlimit.integer & 2048 ) ) {
		s_quadFactor = 8;
	}
//freeze

	damage = 50 * s_quadFactor;
	G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		damage, 0, MOD_GAUNTLET );

	return qtrue;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

#ifdef MISSIONPACK
#define CHAINGUN_SPREAD		600
#endif
#define MACHINEGUN_SPREAD	200
#define	MACHINEGUN_DAMAGE	7
#define	MACHINEGUN_TEAM_DAMAGE	5		// wimpier MG in teamplay

void Bullet_Fire (gentity_t *ent, float spread, int damage, int weapon ) {
	trace_t		tr;
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t		impactpoint, bouncedir;
#endif
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

//unlagged - attack prediction #2
	// we have to use something now that the client knows in advance
	int			seed = ent->client->attackTime % 256;
//unlagged - attack prediction #2

	damage *= s_quadFactor;

//unlagged - attack prediction #2
	// this has to match what's on the client
/*
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
*/
	r = Q_random(&seed) * M_PI * 2.0f;
	u = sin(r) * Q_crandom(&seed) * spread * 16;
	r = cos(r) * Q_crandom(&seed) * spread * 16;
//unlagged - attack prediction #2
	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {

//unlagged - backward reconciliation #2
		// backward-reconcile the other clients
		G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

//unlagged - backward reconciliation #2
		// put them back
		G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// send bullet impact
		if ( ( traceEnt->takedamage && traceEnt->client ) || is_body_freeze( traceEnt ) ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;
//unlagged - attack prediction #2
			// we need the client number to determine whether or not to
			// suppress this event
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
			if( LogAccuracyHit( traceEnt, ent, weapon ) ) {
				ent->client->accuracy_hits++;
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
//unlagged - attack prediction #2
			// we need the client number to determine whether or not to
			// suppress this event
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
#endif
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_MACHINEGUN);
#ifdef MISSIONPACK
			}
#endif
		}
		break;
	}
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire ( gentity_t *ent ) {
	gentity_t	*m;

	m = fire_bfg (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

SHOTGUN

======================================================================
*/

//freeze
struct {
	qboolean struck, unique;
	gentity_t *targ;
	vec3_t origin;
} g_shotgunHits[DEFAULT_SHOTGUN_COUNT];
//freeze

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define	DEFAULT_SHOTGUN_DAMAGE	10

qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent, int pelletnum ) {
	trace_t		tr;
	int			i, passent;
	gentity_t	*traceEnt;
	vec3_t		tr_start, tr_end;
	qboolean	uniqueHit;
	int damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );

	trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
	traceEnt = &g_entities[ tr.entityNum ];

	uniqueHit = qfalse;

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( traceEnt->takedamage ) {
		if ( traceEnt->s.eType == ET_PLAYER ) {
			uniqueHit = qtrue;
			for ( i = 0; i < pelletnum; i++ ) {
				if ( !g_shotgunHits[i].struck ) continue;
				
				if ( g_shotgunHits[i].targ == traceEnt ) {
					uniqueHit = qfalse;
					break;
				}
			}

			g_shotgunHits[pelletnum].struck = qtrue;
			g_shotgunHits[pelletnum].unique = uniqueHit;
			g_shotgunHits[pelletnum].targ = traceEnt;
			VectorCopy( tr.endpos, g_shotgunHits[pelletnum].origin );
		}
		else {
			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN );
		}

		if ( LogAccuracyHit( traceEnt, ent, WP_SHOTGUN ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int			i,j;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	vec3_t		angles;
	int			oldScore;
	qboolean	hitClient = qfalse;
	int damage;

//unlagged - attack prediction #2
	// use this for testing
	//Com_Printf( "Server seed: %d\n", seed );
//unlagged - attack prediction #2

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	vectoangles( forward, angles );
	AngleVectors( angles, forward, right, up );

	oldScore = ent->client->ps.persistant[PERS_SCORE];

//unlagged - backward reconciliation #2
	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

//freeze
	for ( i = 0; i < DEFAULT_SHOTGUN_COUNT; i++ ) {
		g_shotgunHits[i].struck = qfalse;
	}
//freeze

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {

	
		if(i < DEFAULT_SHOTGUN_COUNT/2){	//bullet is in the 'middle' ring
			//distance of 8 out, rotate by i
			switch(i){
			case 0:
                r = -1 * DEFAULT_SHOTGUN_SPREAD * 9;
				u = 0;
				break;
			case 1:
                r = -0.3f * DEFAULT_SHOTGUN_SPREAD * 9;
				u = 0.8f * DEFAULT_SHOTGUN_SPREAD * 9;
				break;
			case 2:
                r = 0.5f * DEFAULT_SHOTGUN_SPREAD * 9;
				u = 0.5f * DEFAULT_SHOTGUN_SPREAD * 9;
				break;
			case 3:
                r = 0.4f * DEFAULT_SHOTGUN_SPREAD * 9;
				u = -0.6f * DEFAULT_SHOTGUN_SPREAD * 9;
				break;
			case 4:
                r = -0.3f * DEFAULT_SHOTGUN_SPREAD * 9;
				u = -0.7f * DEFAULT_SHOTGUN_SPREAD * 9;
				break;
			}
		}else{	//'outer' ring
			//distance of 16 out, rotate by j = i/2
			j= i-DEFAULT_SHOTGUN_COUNT/2;
			switch(j){
			case 0:
                r = -0.7f * DEFAULT_SHOTGUN_SPREAD * 17;
				u = 0.3f * DEFAULT_SHOTGUN_SPREAD * 17;
				break;
			case 1:
                r = 0.2f * DEFAULT_SHOTGUN_SPREAD * 17;
				u = 0.8f * DEFAULT_SHOTGUN_SPREAD * 17;
				break;
			case 2:
                r = 0.9f * DEFAULT_SHOTGUN_SPREAD * 17;
				u = 0.0f * DEFAULT_SHOTGUN_SPREAD * 17;
				break;
			case 3:
                r = 0.2f * DEFAULT_SHOTGUN_SPREAD * 17;
				u = -0.7f * DEFAULT_SHOTGUN_SPREAD * 17;
				break;
			case 4:
                r = -0.6f * DEFAULT_SHOTGUN_SPREAD * 17;
				u = -0.25f * DEFAULT_SHOTGUN_SPREAD * 17;
				break;
			case 5:
				r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
				u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
			}
		}

		r += Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD *2;
		u += Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD *2;

		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);
		if ( ShotgunPellet( origin, end, ent, i ) && !hitClient ) {
			hitClient = qtrue;
			ent->client->accuracy_hits++;
		}
	}

	damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;

	for ( i = 0; i < DEFAULT_SHOTGUN_COUNT; i++ ) {
		if ( !g_shotgunHits[i].struck ) continue;

		G_Damage( g_shotgunHits[i].targ, ent, ent, forward, g_shotgunHits[i].origin,
				damage, g_shotgunHits[i].unique ? 0 : DAMAGE_NO_PERS_HIT, MOD_SHOTGUN );
	}

//unlagged - backward reconciliation #2
	// put them back
	G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2
}


void weapon_supershotgun_fire (gentity_t *ent) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
//unlagged - attack prediction #2
	// this has to be something the client can predict now
	//tent->s.eventParm = rand() & 255;		// seed for spread pattern
	tent->s.eventParm = ent->client->attackTime % 256; // seed for spread pattern
//unlagged - attack prediction #2
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PLASMA GUN

======================================================================
*/

void Weapon_Plasmagun_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define	MAX_RAIL_HITS	64
void weapon_railgun_fire (gentity_t *ent) {
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
//freeze
#else
//	vec3_t	impactpoint, bouncedir;
//freeze
#endif
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];
//freeze
	qboolean ownerUnlinked = qfalse, isCorpsicle = qfalse;
//freeze

	damage = 100 * s_quadFactor;

	VectorMA (muzzle, 8192, forward, end);

//unlagged - backward reconciliation #2
	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT );

		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			break;
		}

		traceEnt = &g_entities[ trace.entityNum ];

//freeze
		if ( is_body_freeze( traceEnt ) ) {
			isCorpsicle = qtrue;
			ownerUnlinked = qfalse;

			// check to see if the owner has been unlinked (just been railed, in other words)
			for ( i = 0; i < unlinked; i++ ) {
				if ( unlinkedEntities[i]->s.number == traceEnt->s.otherEntityNum ) {
					break;
				}
			}

			if ( i < unlinked ) {
				ownerUnlinked = qtrue;
			}
		}
		else {
			isCorpsicle = qfalse;
			ownerUnlinked = qfalse;
		}
//freeze

		if ( traceEnt->takedamage ) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if ( G_InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					SnapVectorTowards( trace.endpos, muzzle );
					// send railgun beam effect
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					// set player number for custom colors on the railtrail
					tent->s.clientNum = ent->s.clientNum;
					VectorCopy( muzzle, tent->s.origin2 );
					// move origin a bit to come closer to the drawn gun muzzle
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;	// don't make the explosion at the end
					//
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			}
			else {
				if( !ownerUnlinked && LogAccuracyHit( traceEnt, ent, WP_RAILGUN ) ) {
					hits++;
				}
				G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
			}
#else
				if( !ownerUnlinked && LogAccuracyHit( traceEnt, ent, WP_RAILGUN ) ) {
					hits++;
				}
				G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
#endif
/*freeze
			if ( is_body_freeze( traceEnt ) && level.time - traceEnt->timestamp > 400 ) {
				if ( InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					SnapVectorTowards( trace.endpos, muzzle );
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					tent->s.clientNum = ent->s.clientNum;
					VectorCopy( muzzle, tent->s.origin2 );
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;

					VectorCopy( impactpoint, muzzle );
					passent = ENTITYNUM_NONE;
				}
			}
freeze*/
		}
		if ( trace.contents & CONTENTS_SOLID ) {
			break;		// we hit something solid enough to stop the beam
		}

//freeze
		if ( isCorpsicle && !ownerUnlinked ) {
			// do some ice chips
			vec3_t otherside;
			VectorMA( traceEnt->r.currentOrigin, traceEnt->r.maxs[0], forward, otherside );
			tent = G_TempEntity( otherside, EV_ICE_CHIPS );
			tent->s.eventParm = 10;
			tent->s.otherEntityNum = traceEnt->s.number;
		}
//freeze

		// unlink this entity, so the next trace will go past it
		G_UnlinkEntity( traceEnt ); //ssdemo
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

//unlagged - backward reconciliation #2
	// put them back
	G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		G_LinkEntity( unlinkedEntities[i] ); //ssdemo
	}

	// the final trace endpos will be the terminal point of the rail trail

//freeze
	if ( g_railJumpKnock.integer ) {
		vec3_t v, dir;
		float dist, points;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( trace.endpos[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - trace.endpos[i];
			} else if ( trace.endpos[i] > ent->r.absmax[i] ) {
				v[i] = trace.endpos[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist < 100.0f ) {
			points = g_railJumpKnock.integer * ( 1.0 - dist / 100.0f );

			if( CanDamage (ent, trace.endpos) ) {
				VectorSubtract (ent->r.currentOrigin, trace.endpos, dir);
				// push the center of mass higher than the origin
				dir[2] += 24;
				G_Damage (ent, NULL, ent, dir, trace.endpos, (int)points, DAMAGE_NO_DAMAGE, MOD_RAILGUN);
			}
		}
	}
//freeze

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
	VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if ( hits == 0 ) {
		// complete miss
		ent->client->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->client->accurateCount += hits;
		if ( ent->client->accurateCount >= 2 ) {
			ent->client->accurateCount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
			// add the sprite over the player's head
			ent->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
		ent->client->accuracy_hits++;
	}

}


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/
/*
void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
//freeze
	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( ent, forward, right, up, muzzle );
//freeze
	if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);

	ent->client->fireHeld = qtrue;
}

void Weapon_HookFree (gentity_t *ent)
{
//freeze
	ent->parent->timestamp = level.time;
//freeze
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}

void Weapon_HookThink (gentity_t *ent) {
	if (!ent->parent || !ent->parent->inuse || !ent->parent->client ||
			ent->parent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->parent->health <= 0 ||
			ent->parent->client->ps.stats[STAT_HEALTH] < 0 || !ent->parent->client->hook) {
		Weapon_HookFree(ent);
		return;
	}

	if (ent->enemy) {
		if ((ent->enemy->client && ent->enemy->client->sess.sessionTeam < TEAM_SPECTATOR &&
				ent->enemy->health > 0 && ent->enemy->client->ps.stats[STAT_HEALTH] > 0) || 
				is_body_freeze(ent->enemy)) {
			vec3_t v, oldorigin;

			VectorCopy(ent->r.currentOrigin, oldorigin);
			v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
			v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
			v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
			SnapVectorTowards( v, oldorigin );	// save net bandwidth

			G_SetOrigin( ent, v );
		}
		else {
			Weapon_HookFree(ent);
			return;
		}
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

	ent->think = Weapon_HookThink;
	ent->nextthink = level.time + 50;
}
*/
/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void Weapon_LightningFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
#ifdef MISSIONPACK
	vec3_t impactpoint, bouncedir;
#endif
	gentity_t	*traceEnt, *tent;
	int			damage, i, passent;

//unlagged - server options
	// this is configurable now
//	damage = 8 * s_quadFactor;
	damage = g_lightningDamage.integer * s_quadFactor;
//unlagged - server options

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {
		VectorMA( muzzle, LIGHTNING_RANGE, forward, end );

//unlagged - backward reconciliation #2
		// backward-reconcile the other clients
		G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		trap_Trace( &tr, muzzle, NULL, NULL, end, passent, MASK_SHOT );

//unlagged - backward reconciliation #2
		// put them back
		G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

#ifdef MISSIONPACK
		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if (i) {
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			//
			tent = G_TempEntity( muzzle, EV_LIGHTNINGBOLT );
			VectorCopy( tr.endpos, end );
			SnapVector( end );
			VectorCopy( end, tent->s.origin2 );
		}
#endif
		if ( tr.entityNum == ENTITYNUM_NONE ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if ( traceEnt->takedamage) {
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					VectorSubtract( end, impactpoint, forward );
					VectorNormalize(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_LIGHTNING);
			}
#else
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_LIGHTNING);
#endif
		}

		if ( ( traceEnt->takedamage && traceEnt->client ) || is_body_freeze( traceEnt ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
//unlagged - attack prediction #2
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
//unlagged - LG bandwidth
			tent->r.svFlags |= SVF_SINGLECLIENT;
			tent->r.singleClient = ent->s.number;
//unlagged - LG bandwidth
			if( LogAccuracyHit( traceEnt, ent, WP_LIGHTNING ) ) {
				ent->client->accuracy_hits++;
			}
		}
		else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
//unlagged - attack prediction #2
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
//unlagged - LG bandwidth
			tent->r.svFlags |= SVF_SINGLECLIENT;
			tent->r.singleClient = ent->s.number;
//unlagged - LG bandwidth
		}

		break;
	}
}

#ifdef MISSIONPACK
/*
======================================================================

NAILGUN

======================================================================
*/

void Weapon_Nailgun_Fire (gentity_t *ent) {
	gentity_t	*m;
	int			count;

	for( count = 0; count < NUM_NAILSHOTS; count++ ) {
		m = fire_nail (ent, muzzle, forward, right, up );
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
	}

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void weapon_proxlauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_prox (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

#endif

//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker, int weapon ) {

	if( !attacker->client ) {
		return qfalse;
	}

	if ( is_body_freeze( target ) ) {
		if ( !level.warmupTime ) {
			attacker->client->sess.accstats.totalCorpseHits[weapon]++;
			attacker->client->pers.accstats.totalCorpseHits[weapon]++;
		}
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !target->takedamage ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		if ( !level.warmupTime ) {
			attacker->client->sess.accstats.totalTeamHits[weapon]++;
			attacker->client->pers.accstats.totalTeamHits[weapon]++;
		}
		return qfalse;
	}

	if ( !level.warmupTime ) {
		attacker->client->sess.accstats.totalEnemyHits[weapon]++;
		attacker->client->pers.accstats.totalEnemyHits[weapon]++;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}



/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	if (ent->client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}

#ifdef MISSIONPACK
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif
//freeze
	if ( g_dmflags.integer & 1024 ) {
		s_quadFactor = 8;
	}
//freeze

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( /*ent->s.weapon != WP_GRAPPLING_HOOK && */ ent->s.weapon != WP_GAUNTLET ) {
#ifdef MISSIONPACK
		if( ent->s.weapon == WP_NAILGUN ) {
			ent->client->accuracy_shots += NUM_NAILSHOTS;
		} else {
			ent->client->accuracy_shots++;
		}
#else
		ent->client->accuracy_shots++;
#endif
	}

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );

//freeze
	ent->lastFiringTime = level.time;
//freeze

	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_LIGHTNING]++;
			ent->client->pers.accstats.totalShots[WP_LIGHTNING]++;
		}
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_SHOTGUN] += DEFAULT_SHOTGUN_COUNT;
			ent->client->pers.accstats.totalShots[WP_SHOTGUN] += DEFAULT_SHOTGUN_COUNT;
		}
		break;
	case WP_MACHINEGUN:
		if ( g_gametype.integer != GT_TEAM ) {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, WP_MACHINEGUN );
		} else {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE, WP_MACHINEGUN );
		}
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_MACHINEGUN]++;
			ent->client->pers.accstats.totalShots[WP_MACHINEGUN]++;
		}
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_GRENADE_LAUNCHER]++;
			ent->client->pers.accstats.totalShots[WP_GRENADE_LAUNCHER]++;
		}
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_ROCKET_LAUNCHER]++;
			ent->client->pers.accstats.totalShots[WP_ROCKET_LAUNCHER]++;
		}
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_PLASMAGUN]++;
			ent->client->pers.accstats.totalShots[WP_PLASMAGUN]++;
		}
		break;
	case WP_RAILGUN:
		weapon_railgun_fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_RAILGUN]++;
			ent->client->pers.accstats.totalShots[WP_RAILGUN]++;
		}
		break;
	case WP_BFG:
		BFG_Fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_BFG]++;
			ent->client->pers.accstats.totalShots[WP_BFG]++;
		}
		break;
/*
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
*/
#ifdef MISSIONPACK
	case WP_NAILGUN:
		Weapon_Nailgun_Fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_NAILGUN] += NUM_NAILSHOTS;
			ent->client->pers.accstats.totalShots[WP_NAILGUN] += NUM_NAILSHOTS;
		}
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire( ent );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_PROX_LAUNCHER]++;
			ent->client->pers.accstats.totalShots[WP_PROX_LAUNCHER]++;
		}
		break;
	case WP_CHAINGUN:
		Bullet_Fire( ent, CHAINGUN_SPREAD, MACHINEGUN_DAMAGE, WP_CHAINGUN );
		if ( !level.warmupTime ) {
			ent->client->sess.accstats.totalShots[WP_ChAINGUN]++;
			ent->client->pers.accstats.totalShots[WP_ChAINGUN]++;
		}
		break;
#endif
	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
}


#ifdef MISSIONPACK

/*
===============
KamikazeRadiusDamage
===============
*/
static void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (!ent->takedamage) {
			continue;
		}

		// dont hit things we have already hit
		if( ent->kamikazeTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		// dont hit things we have already hit
		if( ent->kamikazeShockTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			//
			dir[2] = 0;
			VectorNormalize(dir);
			if ( ent->client ) {
				ent->client->ps.velocity[0] = dir[0] * push;
				ent->client->ps.velocity[1] = dir[1] * push;
				ent->client->ps.velocity[2] = 100;
			}
			ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void KamikazeDamage( gentity_t *self ) {
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if (self->count >= KAMI_SHOCKWAVE_STARTTIME) {
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400,	(int) (float) t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME) );
	}
	//
	if (self->count >= KAMI_EXPLODE_STARTTIME) {
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage( self->s.pos.trBase, self->activator, 400,	(int) (float) t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME) );
	}

	// either cycle or kill self
	if( self->count >= KAMI_SHOCKWAVE_ENDTIME ) {
		G_FreeEntity( self );
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE) {
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void G_StartKamikaze( gentity_t *ent ) {
	gentity_t	*explosion;
	gentity_t	*te;
	vec3_t		snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if ( ent->client ) {
		VectorCopy( ent->s.pos.trBase, snapped );
	}
	else {
		VectorCopy( ent->activator->s.pos.trBase, snapped );
	}
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( explosion, snapped );

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	G_LinkEntity( explosion ); //ssdemo

	if (ent->client) {
		//
		explosion->activator = ent;
		//
		ent->s.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		G_Damage( ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE );
	}
	else {
		if ( !strcmp(ent->activator->classname, "bodyque") ) {
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		}
		else {
			explosion->activator = ent->activator;
		}
	}

	// play global sound at all clients
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}
#endif

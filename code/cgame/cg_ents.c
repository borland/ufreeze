// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"


/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}

//anticheat - origin hashing
void CG_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
/*
	centity_t *cent = &cg_entities[entityNum];

	if ( entityNum != cg.predictedPlayerState.clientNum && cent->currentState.eType != ET_SPEAKER && cent->currentState.eType != ET_ITEM ) {
		if ( origin != NULL ) {
			CG_Printf("sound: %.2f %.2f %.2f  %d  %d  %d\n", origin[0], origin[1], origin[2], 
					entityNum, entchannel, sfx);
		}
		else {
			CG_Printf("sound: %d  %d  %d  %d\n", cent->currentState.eType, entityNum, entchannel, sfx);
		}
	}
*/

	// if it's a player
	if ( entityNum >= 0 && entityNum < MAX_CLIENTS ) {
		// if it's the predicted client
		if ( entityNum == cg.predictedPlayerState.clientNum ) {
			// play it locally
			trap_S_StartLocalSound( sfx, entchannel );
		}
		else {
			// play at just the origin - disregard the entity number
			trap_S_StartSound( origin, ENTITYNUM_NONE, entchannel, sfx );
			// we NEED an origin - the one sent to trap_S_UpdateEntityPosition is very wrong
		}
	}
	// if it's a sound that requires an origin
	else if ( entityNum >= ENTITYNUM_MAX_NORMAL || entityNum < 0 ) {
		// play a sound at that origin
		trap_S_StartSound( origin, entityNum, entchannel, sfx );
	}
	else {
		// otherwise, assume CG_SetEntitySoundPosition() was called for this entity,
		// and play the sound on it
		trap_S_StartSound( NULL, entityNum, entchannel, sfx );
	}
}
//anticheat - origin hashing

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/


#define HASH_DISTANCE 512.0f

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
//anticheat - origin hashing
		vec3_t origin;

		VectorCopy( cent->lerpOrigin, origin );

		if ( cent->currentState.clientNum != cg.predictedPlayerState.clientNum &&
				cent->currentState.number < MAX_CLIENTS ) {
			// For testing with OGC (since it hooks trap_S_UpdateEntityPosition)
			//BG_HashOrigin( origin, cgs.sv_mapname, cent->currentState.clientNum, 1 );

			origin[0] += crandom() * HASH_DISTANCE;
			origin[1] += crandom() * HASH_DISTANCE;
			origin[2] += crandom() * HASH_DISTANCE;
		}
//anticheat - origin hashing
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if ( cent->currentState.constantLight ) {
		int		cl;
		int		i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;
		trap_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
	}
}


/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	CG_AddRefEntityToScene (&ent, 0, REF_ALWAYS, qtrue); //multiview
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	CG_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t		ent;
	entityState_t	*es;
	gitem_t			*item;
	int				msec;
	float			frac;
	float			scale;
	weaponInfo_t	*wi;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) {
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	item = &bg_itemlist[ es->modelindex ];
	if ( cg_simpleItems.integer && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		CG_AddRefEntityToScene(&ent, 0, REF_ALWAYS, qtrue); //multiview
		return;
	}

	// items bob up and down continuously
	scale = 0.005 + cent->currentState.number * 0.00001;
	cent->lerpOrigin[2] += 4 + cos( ( cg.time + 1000 ) *  scale ) * 4;

	memset (&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if ( item->giType == IT_HEALTH ) {
		VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		VectorCopy( cg.autoAngles, cent->lerpAngles );
		AxisCopy( cg.autoAxis, ent.axis );
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -= 
			wi->weaponMidpoint[0] * ent.axis[0][0] +
			wi->weaponMidpoint[1] * ent.axis[1][0] +
			wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -= 
			wi->weaponMidpoint[0] * ent.axis[0][1] +
			wi->weaponMidpoint[1] * ent.axis[1][1] +
			wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -= 
			wi->weaponMidpoint[0] * ent.axis[0][2] +
			wi->weaponMidpoint[1] * ent.axis[1][2] +
			wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;	// an extra height boost
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if ( msec >= 0 && msec < ITEM_SCALEUP_TIME ) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale( ent.axis[0], frac, ent.axis[0] );
		VectorScale( ent.axis[1], frac, ent.axis[1] );
		VectorScale( ent.axis[2], frac, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ( ( item->giType == IT_WEAPON ) ||
		 ( item->giType == IT_ARMOR ) ) {
		ent.renderfx |= RF_MINLIGHT;
	}

	// increase the size of the weapons when they are presented as items
	if ( item->giType == IT_WEAPON ) {
		VectorScale( ent.axis[0], 1.5, ent.axis[0] );
		VectorScale( ent.axis[1], 1.5, ent.axis[1] );
		VectorScale( ent.axis[2], 1.5, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
#ifdef MISSIONPACK
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.weaponHoverSound );
#endif
	}

#ifdef MISSIONPACK
	if ( item->giType == IT_HOLDABLE && item->giTag == HI_KAMIKAZE ) {
		VectorScale( ent.axis[0], 2, ent.axis[0] );
		VectorScale( ent.axis[1], 2, ent.axis[1] );
		VectorScale( ent.axis[2], 2, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}
#endif

	// add to refresh list
	CG_AddRefEntityToScene(&ent, 0, REF_ALWAYS, qtrue); //multiview

#ifdef MISSIONPACK
	if ( item->giType == IT_WEAPON && wi->barrelModel ) {
		refEntity_t	barrel;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( ent.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		AxisCopy( ent.axis, barrel.axis );
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		CG_AddRefEntityToScene( &barrel, 0, REF_ALWAYS, qtrue ); //multiview
	}
#endif

	// accompanying rings / spheres for powerups
	if ( !cg_simpleItems.integer ) 
	{
		vec3_t spinAngles;

		VectorClear( spinAngles );

		if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
		{
			if ( ( ent.hModel = cg_items[es->modelindex].models[1] ) != 0 )
			{
				if ( item->giType == IT_POWERUP )
				{
					ent.origin[2] += 12;
					spinAngles[1] = ( cg.time & 1023 ) * 360 / -1024.0f;
				}
				AnglesToAxis( spinAngles, ent.axis );
				
				// scale up if respawning
				if ( frac != 1.0 ) {
					VectorScale( ent.axis[0], frac, ent.axis[0] );
					VectorScale( ent.axis[1], frac, ent.axis[1] );
					VectorScale( ent.axis[2], frac, ent.axis[2] );
					ent.nonNormalizedAxes = qtrue;
				}
				CG_AddRefEntityToScene( &ent, 0, REF_ALWAYS, qtrue ); //multiview
			}
		}
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;
//	int	col;

	s1 = &cent->currentState;
	if ( s1->weapon > WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

	// add trails
	if ( weapon->missileTrailFunc ) 
	{
		weapon->missileTrailFunc( cent, weapon );
	}
/*
	if ( cent->currentState.modelindex == TEAM_RED ) {
		col = 1;
	}
	else if ( cent->currentState.modelindex == TEAM_BLUE ) {
		col = 2;
	}
	else {
		col = 0;
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	}
*/
	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
	}

	// add missile sound
	if ( weapon->missileSound && cg.timein <= cg.time ) {
		vec3_t	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	if ( cent->currentState.weapon == WP_PLASMAGUN ) {
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		CG_AddRefEntityToScene( &ent, 0, REF_ALWAYS, qtrue ); //multiview
		return;
	}

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

#ifdef MISSIONPACK
	if ( cent->currentState.weapon == WP_PROX_LAUNCHER ) {
		if (s1->generic1 == TEAM_BLUE) {
			ent.hModel = cgs.media.blueProxMine;
		}
	}
#endif

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY ) {
		RotateAroundDirection( ent.axis, cg.time / 4 );
	} else {
#ifdef MISSIONPACK
		if ( s1->weapon == WP_PROX_LAUNCHER ) {
			AnglesToAxis( cent->lerpAngles, ent.axis );
		}
		else
#endif
		{
			RotateAroundDirection( ent.axis, s1->time );
		}
	}

	// add to refresh list
	CG_AddRefEntityToScene( &ent, 0, REF_ALWAYS, qtrue ); //multiview
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
/*
static void CG_Grapple( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;

	s1 = &cent->currentState;
	if ( s1->weapon > WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

#if 0 // FIXME add grapple pull sound here..?
	// add missile sound
	if ( weapon->missileSound ) {
		trap_S_AddLoopingSound( cent->currentState.number, origin, vec3_origin, weapon->missileSound );
	}
#endif

	// Will draw cable if needed
	CG_GrappleTrail ( cent, weapon );

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	trap_R_AddRefEntityToScene( &ent );
}
*/

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	CG_AddRefEntityToScene(&ent, 0, REF_ALWAYS, qfalse); //multiview

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		CG_AddRefEntityToScene(&ent, 0, REF_ALWAYS, qfalse); //multiview
	}

}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	CG_AddRefEntityToScene(&ent, 0, REF_ALWAYS, qfalse); //multiview
}

/*
===============
CG_Portal
===============
*/
//multiview - moved creation of refEntity_t to CG_PortalEffect()
static void CG_Portal( centity_t *cent ) {
	entityState_t		*s1;
	vec3_t				axis[3];
	vec3_t				src, dest;

	s1 = &cent->currentState;

	VectorCopy( cent->lerpOrigin, src );
	VectorCopy( s1->origin2, dest );
	ByteToDir( s1->eventParm, axis[0] );
	PerpendicularVector( axis[1], axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorNegate( axis[1], axis[1] );

	// calculate the last axis
	CrossProduct( axis[0], axis[1], axis[2] );

	CG_PortalEffect( src, dest, axis, s1->powerups, s1->frame, s1->clientNum / 256.0f * 360 );
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

//freeze
//ssdemo
	//if ( cg.timein > cg.time ) {
	if ( cg.timein > cg.time || cg.ssDemoPaused ) {
//ssdemo
		VectorCopy( in, out );
		return;
	}
//freeze

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );

	// FIXME: origin change when on a rotating object
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

//freeze
//ssdemo
	//if ( cg.timein > cg.time ) {
	if ( cg.timein > cg.time || cg.ssDemoPaused ) {
//ssdemo
		f = 0;
	}
	else {
//freeze
		f = cg.frameInterpolation;
	}

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

//freeze
	if ( cent->currentState.eType == ET_PLAYER ) {
		if ( (cent->currentState.eFlags & EF_GRAPPLE) && (cent->nextState.eFlags & EF_GRAPPLE) ) {
			cent->grappleLerpOrigin[0] = cent->currentState.origin2[0] + f * ( cent->nextState.origin2[0] - cent->currentState.origin2[0] );
			cent->grappleLerpOrigin[1] = cent->currentState.origin2[1] + f * ( cent->nextState.origin2[1] - cent->currentState.origin2[1] );
			cent->grappleLerpOrigin[2] = cent->currentState.origin2[2] + f * ( cent->nextState.origin2[2] - cent->currentState.origin2[2] );
			cent->grappleValid = qtrue;
		}
		else {
			cent->grappleValid = qfalse;
		}
	}
//freeze
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {
//freeze
	int time;
//freeze
//unlagged - projectile nudge
	// this will be set to how far forward projectiles will be extrapolated
	int timeshift = 0;
//unlagged - projectile nudge

//anticheat
	if ( cent != &cg.predictedPlayerEntity && cent->currentState.eType == ET_PLAYER ) {
		if ( cent->currentState.eFlags & EF_HASHED ) {
			BG_HashOrigin( cent->currentState.pos.trBase, cgs.sv_mapname, cent->currentState.clientNum, -1 );
			BG_UnhashDeadFlag( &cent->currentState.eFlags, cgs.sv_mapname, cent->currentState.number );
			cent->currentState.eFlags &= ~EF_HASHED;
		}

		if ( cg.nextSnap && (cent->nextState.eFlags & EF_HASHED) ) {
			BG_HashOrigin( cent->nextState.pos.trBase, cgs.sv_mapname, cent->nextState.clientNum, -1 );
			BG_UnhashDeadFlag( &cent->nextState.eFlags, cgs.sv_mapname, cent->nextState.number );
			cent->nextState.eFlags &= ~EF_HASHED;
		}
	}
//anticheat

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
			cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

//unlagged - timenudge extrapolation
	// interpolating failed (probably no nextSnap), so extrapolate
	// this can also happen if the teleport bit is flipped, but that
	// won't be noticeable
	if ( cent->currentState.number < MAX_CLIENTS &&
			cent->currentState.clientNum != cg.predictedPlayerState.clientNum ) {
		cent->currentState.pos.trType = TR_LINEAR_STOP;
		cent->currentState.pos.trTime = cg.snap->serverTime;
		cent->currentState.pos.trDuration = 1000 / sv_fps.integer;
	}
//unlagged - timenudge extrapolation

//freeze
	if ( cent->currentState.eType == ET_PLAYER ) {
		if ( cent->currentState.eFlags & EF_GRAPPLE ) {
			VectorCopy( cent->currentState.origin2, cent->grappleLerpOrigin );
			cent->grappleValid = qtrue;
		}
		else {
			cent->grappleValid = qfalse;
		}
	}

	if ( cg.timein > cg.time ) {
		time = cg.snap->serverTime;
	}
	else {
		time = cg.time;
	}
//freeze

//ssdemo
	if ( cg.ssDemoPaused ) {
		time = cg.snap->serverTime;
	}
//ssdemo

//unlagged - projectile nudge
	// if it's a missile but not a grappling hook
	if ( cent->currentState.eType == ET_MISSILE /*&& cent->currentState.weapon != WP_GRAPPLING_HOOK */) {
		// if it's one of ours
		if ( cent->currentState.otherEntityNum == cg.clientNum ) {
			// extrapolate one server frame's worth - this will correct for tiny
			// visual inconsistencies introduced by backward-reconciling all players
			// one server frame before running projectiles
			timeshift = 1000 / sv_fps.integer;
		}
		// if it's not, and it's not a grenade launcher
		else if ( cent->currentState.weapon != WP_GRENADE_LAUNCHER ) {
			// extrapolate based on cg_projectileNudge
			timeshift = cg_projectileNudge.integer + 1000 / sv_fps.integer;
		}
	}

	// just use the current frame and evaluate as best we can
//	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
//	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );
	BG_EvaluateTrajectory( &cent->currentState.pos, time + timeshift, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, time + timeshift, cent->lerpAngles );

	// if there's a time shift
	if ( timeshift != 0 ) {
		trace_t tr;
		vec3_t lastOrigin;

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, lastOrigin );

		CG_Trace( &tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin, cent->currentState.number, MASK_SHOT );

		// don't let the projectile go through the floor
		if ( tr.fraction < 1.0f ) {
			cent->lerpOrigin[0] = lastOrigin[0] + tr.fraction * ( cent->lerpOrigin[0] - lastOrigin[0] );
			cent->lerpOrigin[1] = lastOrigin[1] + tr.fraction * ( cent->lerpOrigin[1] - lastOrigin[1] );
			cent->lerpOrigin[2] = lastOrigin[2] + tr.fraction * ( cent->lerpOrigin[2] - lastOrigin[2] );
		}
	}
//unlagged - projectile nudge

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, 
		cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase( centity_t *cent ) {
	refEntity_t model;
#ifdef MISSIONPACK
	vec3_t angles;
	int t, h;
	float c;

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
#else
	if ( cgs.gametype == GT_CTF) {
#endif
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );
		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
	}
#ifdef MISSIONPACK
	else if ( cgs.gametype == GT_OBELISK ) {
		// show the obelisk
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		model.hModel = cgs.media.overloadBaseModel;
		CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
		// if hit
		if ( cent->currentState.frame == 1) {
			// show hit model
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			//
			model.hModel = cgs.media.overloadEnergyModel;
			CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
		}
		// if respawning
		if ( cent->currentState.frame == 2) {
			if ( !cent->miscTime ) {
				cent->miscTime = cg.time;
			}
			t = cg.time - cent->miscTime;
			h = (cg_obeliskRespawnDelay.integer - 5) * 1000;
			//
			if (t > h) {
				c = (float) (t - h) / h;
				if (c > 1)
					c = 1;
			}
			else {
				c = 0;
			}
			// show the lights
			AnglesToAxis( cent->currentState.angles, model.axis );
			//
			model.shaderRGBA[0] = c * 0xff;
			model.shaderRGBA[1] = c * 0xff;
			model.shaderRGBA[2] = c * 0xff;
			model.shaderRGBA[3] = c * 0xff;

			model.hModel = cgs.media.overloadLightsModel;
			CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
			// show the target
			if (t > h) {
				if ( !cent->muzzleFlashTime ) {
					CG_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.obeliskRespawnSound );
					cent->muzzleFlashTime = 1;
				}
				VectorCopy(cent->currentState.angles, angles);
				angles[YAW] += (float) 16 * acos(1-c) * 180 / M_PI;
				AnglesToAxis( angles, model.axis );

				VectorScale( model.axis[0], c, model.axis[0]);
				VectorScale( model.axis[1], c, model.axis[1]);
				VectorScale( model.axis[2], c, model.axis[2]);

				model.shaderRGBA[0] = 0xff;
				model.shaderRGBA[1] = 0xff;
				model.shaderRGBA[2] = 0xff;
				model.shaderRGBA[3] = 0xff;
				//
				model.origin[2] += 56;
				model.hModel = cgs.media.overloadTargetModel;
				CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
			}
			else {
				//FIXME: show animated smoke
			}
		}
		else {
			cent->miscTime = 0;
			cent->muzzleFlashTime = 0;
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			// show the lights
			model.hModel = cgs.media.overloadLightsModel;
			CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
			// show the target
			model.origin[2] += 56;
			model.hModel = cgs.media.overloadTargetModel;
			CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue ); //multiview
		}
	}
	else if ( cgs.gametype == GT_HARVESTER ) {
		// show harvester model
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterRedSkin;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterBlueSkin;
		}
		else {
			model.hModel = cgs.media.harvesterNeutralModel;
			model.customSkin = 0;
		}
		CG_AddRefEntityToScene( &model, 0, REF_ALWAYS, qtrue );
	}
#endif
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
/*
	case ET_GRAPPLE:
		CG_Grapple( cent );
		break;
*/
	case ET_TEAM:
		CG_TeamBase( cent );
		break;
	case ET_TIMEOUT:
		cg.timeout = cent->currentState.time;
		cg.timein = cent->currentState.time2;
		cg.totaltimeout = cent->currentState.powerups + cent->currentState.generic1 * 32768;
		cg.timeoutteam = cent->currentState.eventParm;
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

//unlagged - early transitioning
//ssdemo
	//if ( cg.nextSnap && cg.time >= cg.timein ) {
	if ( cg.nextSnap && cg.time >= cg.timein && !cg.ssDemoPaused ) {
//ssdemo
		// pre-add some of the entities sent over by the server
		// we have data for them and they don't need to interpolate
		for ( num = 0 ; num < cg.nextSnap->numEntities ; num++ ) {
			cent = &cg_entities[ cg.nextSnap->entities[ num ].number ];
			if ( cent->nextState.eType == ET_MISSILE || cent->nextState.eType == ET_GENERAL ) {
				// transition it immediately and add it
				CG_TransitionEntity( cent );
				cent->interpolate = qtrue;
				CG_AddCEntity( cent );
			}
		}
	}
//unlagged - early transitioning

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
//unlagged - early transitioning
//ssdemo
		//if ( !cg.nextSnap || cg.timein > cg.time || cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL ) {
		if ( !cg.nextSnap || cg.timein > cg.time || cg.ssDemoPaused || cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL ) {
//ssdemo
//unlagged - early transitioning
			CG_AddCEntity( cent );
		}
	}
}


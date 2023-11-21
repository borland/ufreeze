#include "cg_local.h"

void CG_AddInvisibleEntity( refEntity_t *ent, centity_t *cent, int team, int refFlags ) {
	entityState_t *state = &cent->currentState;
	float speed, wishvisval, diff, firingval;
	int lastFiringTime;
	vec3_t red = { 1.0f, 0.4f, 0.3f }, blue = { 0.4f, 0.5f, 1.0f }, neutral = { 0.8f, 0.9f, 1.0f };
	vec_t *color = neutral;

	// this should be fine as long as the speed is always the same
	// for every player (which it is for now)
	float topspeed = cg.predictedPlayerState.speed;

	if ( cgs.huntMode == 2 ) {
		if ( (team == TEAM_RED && !cg_swapSkins.integer) ||
				(team == TEAM_BLUE && cg_swapSkins.integer) ) {
			color = red;
		}
		else if ( (team == TEAM_BLUE && !cg_swapSkins.integer) ||
				(team == TEAM_RED && cg_swapSkins.integer) ) {
			color = blue;
		}
	}
 
	speed = VectorLength( state->pos.trDelta );
	wishvisval = Com_Clamp(0.0f, 1.0f, (speed - (topspeed / 2.0f)) / topspeed);
	//wishvisval = Com_Clamp(1.0f, 1.0f, (speed - 160.0f) / 480.0f);
	diff = Com_Clamp(0.0f, 5.0f, Q_fabs(wishvisval - cent->lastVisVal) * (float)cg.frametime / 1000.0f);

	if ( wishvisval < cent->lastVisVal ) {
		wishvisval = Com_Clamp(0.0f, 1.0f, cent->lastVisVal - diff);
	}
	else {
		wishvisval = Com_Clamp(0.0f, 1.0f, cent->lastVisVal + diff);
	}

	//CG_Printf("wishval: %.3f, vis: %d\n", wishvisval, (int)(wishvisval * 1.0f * 255));

	ent->customShader = cgs.media.invisShader;
	ent->shaderRGBA[0] = wishvisval * color[0] * 255;
	ent->shaderRGBA[1] = wishvisval * color[1] * 255;
	ent->shaderRGBA[2] = wishvisval * color[2] * 255;
	ent->shaderRGBA[3] = 255;
	CG_AddRefEntityToScene( ent, cent->currentState.clientNum, refFlags, qfalse );

	cent->lastVisVal = wishvisval;

	lastFiringTime = cg_entities[state->clientNum].lastFiringTime;

	firingval = 1.0f - Com_Clamp(0.0f, 1.0f, (float)(cg.time - lastFiringTime) / 1000.0f);

	if ( firingval > 0.0f ) {
		weaponInfo_t *weaponInfo = &cg_weapons[state->weapon];

		ent->customShader = cgs.media.invisFiringShader;
		ent->shaderRGBA[0] = firingval * weaponInfo->flashDlightColor[0] * 255;
		ent->shaderRGBA[1] = firingval * weaponInfo->flashDlightColor[1] * 255;
		ent->shaderRGBA[2] = firingval * weaponInfo->flashDlightColor[2] * 255;
		ent->shaderRGBA[3] = 255;
		ent->shaderTime = (float)lastFiringTime / 1000.0f;
		CG_AddRefEntityToScene( ent, cent->currentState.clientNum, refFlags, qfalse );
	}
}

void CG_Draw3DModelInvis( float x, float y, float w, float h, qhandle_t model, vec3_t origin, vec3_t angles, centity_t *cent ) {
	refdef_t		refdef;
	refEntity_t		ent;

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	CG_AddInvisibleEntity( &ent, cent, cgs.clientinfo[cent->currentState.clientNum].team, REF_ALWAYS );
	trap_R_RenderScene( &refdef );
}

#define SNOW_X 512
#define SNOW_Y 512
#define SNOW_EXT_X 8192
#define SNOW_EXT_Y 8192

#define SNOW_EXT_Z 8192
#define SNOW_STEP_Z 4096

#define SNOWFLAKE_RADIUS 2.0f

// uncomment this to get snowflake stats
//#define SNOWDEBUG

#define SNOW_ENVELOPE 512
#define SNOW_NEAR 16
#define SNOW_ENVELOPE_X (SNOW_ENVELOPE / (SNOW_EXT_X * 2 / SNOW_X))
#define SNOW_ENVELOPE_Y (SNOW_ENVELOPE / (SNOW_EXT_Y * 2 / SNOW_Y))

#define SNOWINDEX_X(x) ((x + SNOW_EXT_X) * SNOW_X / (SNOW_EXT_X * 2))
#define SNOWPOS_X(x) (x * SNOW_EXT_X * 2 / SNOW_X - SNOW_EXT_X)

#define SNOWINDEX_Y(y) ((y + SNOW_EXT_Y) * SNOW_Y / (SNOW_EXT_Y * 2))
#define SNOWPOS_Y(y) (y * SNOW_EXT_Y * 2 / SNOW_Y - SNOW_EXT_Y)

static short snowtop[SNOW_X][SNOW_Y];
static short snowbot[SNOW_X][SNOW_Y];
static short snowspd[SNOW_X][SNOW_Y];

static qboolean initsnow = qfalse;

void CG_InitWeather( void ) {
	int x, y;
	vec3_t start, end;
	trace_t tr;
	vec3_t newstart;
	static int seed = 0x4f;

	int starttime, endtime;

	if ( initsnow ) {
		return;
	}

	initsnow = qtrue;

	starttime = trap_Milliseconds();

	// these will never change
	start[2] = SNOW_EXT_Z;
	end[2] = -SNOW_EXT_Z;

	// step through all the X snow coordinates
	for ( x = 0; x < SNOW_X; x++ ) {
		start[0] = end[0] = SNOWPOS_X(x);

		// step through all the Y snow coordinates
		for ( y = 0; y < SNOW_Y; y++ ) {
			start[1] = end[1] = SNOWPOS_Y(y);

			// trace down
			CG_Trace( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID|MASK_WATER );

			// forget about this coordinate if hit nothing
			if ( tr.fraction >= 1.0f ) {
				snowtop[x][y] = 0.0f;
				snowbot[x][y] = 0.0f;
				continue;
			}

			// hit sky on the way down?
			if ( tr.surfaceFlags & (SURF_SKY|SURF_NOIMPACT) ) {
				// retrace, starting a little below the impact point
				VectorCopy( tr.endpos, newstart);
				newstart[2] -= 1.0f;
				CG_Trace( &tr, newstart, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID|MASK_WATER );
			}

			// forget about this coordinate if hit nothing
			if ( tr.fraction >= 1.0f ) {
				snowtop[x][y] = 0.0f;
				snowbot[x][y] = 0.0f;
				continue;
			}

			VectorCopy( tr.endpos, newstart );

			// we know the floor is here now
			snowbot[x][y] = newstart[2];

			// now trace up
			CG_Trace( &tr, newstart, NULL, NULL, start, ENTITYNUM_NONE, MASK_SOLID|MASK_WATER );

			// if we hit nothing or didn't hit sky, forget it
			if ( tr.fraction >= 1.0f || !(tr.surfaceFlags & (SURF_SKY|SURF_NOIMPACT))) {
				snowtop[x][y] = 0.0f;
				snowbot[x][y] = 0.0f;
				continue;
			}

			// we know the sky is here now
			snowtop[x][y] = tr.endpos[2];

			// save the random speed factor converted to 16-bit
			snowspd[x][y] = Q_random( &seed ) * 32768;
		}
	}

	endtime = trap_Milliseconds();
	CG_Printf( "Snow space calculated in %dms\n", endtime - starttime );
}

void CG_DrawSnow( void ) {
	float px, py, pz, ix, iy, iz, rz, speedfact;
	int i, x, y, plx, ply, plz, x1, x2, y1, y2, dist2;
	vec3_t origin;
	int time;
	int stepz;
	vec3_t forward, right, up, vieworigin, delta;
	float cosfov;
	polyVert_t	verts[4];
	vec3_t corners[4];

#ifdef SNOWDEBUG
	int total = 0;
	int starttime, endtime;
	starttime = trap_Milliseconds();
#endif

	if ( !initsnow ) {
		return;
	}

	// don't want snow?
	if ( !cg_weather.integer || cg_weather.integer > 16 ) {
		return;
	}

	if ( !cgs.media.snowflakeShader ) {
		return;
	}
	
	time = cg.time + 60000;

	// set the polygon's texture coordinates
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	verts[1].st[0] = 1;
	verts[1].st[1] = 1;
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;

	// set the polygon's vertex colors
	for ( i = 0; i < 4; i++ ) {
		verts[i].modulate[0] = 255;
		verts[i].modulate[1] = 255;
		verts[i].modulate[2] = 255;
		verts[i].modulate[3] = 128;
	}

	stepz = SNOW_STEP_Z / cg_weather.integer;

	// get a local copy for speed	
	VectorCopy( cg.refdef.vieworg, vieworigin );

	// grab our player's eye position as integers for fast access later
	plx = vieworigin[0];
	ply = vieworigin[1];
	plz = vieworigin[2];

	// get the forward vector (used later for culling), and right and up
	// vectors (used later for making these camera-facing)
	AngleVectors( cg.refdefViewAngles, forward, right, up );

	// calculate the relative vertices of a camera-facing polygon
	VectorAdd( right, up, corners[0] );
	VectorScale( corners[0], 0.5f, corners[0] );
	VectorScale( corners[0], -1, corners[2] );
	VectorSubtract( corners[0], up, corners[1] );
	VectorScale( corners[1], -1, corners[3] );

	// scale 'em
	for ( i = 0; i < 4; i++ ) {
		VectorScale( corners[i], SNOWFLAKE_RADIUS, corners[i] );
	}

	// grab the cosine of half the field of view (used later for culling)
	// add a bit more angle for a little optimization below
	cosfov = cos(DEG2RAD(cg_fov.value / 2 + 5.0f));

	// grab the closest "snow index" to our player
	x1 = SNOWINDEX_X(plx) - SNOW_ENVELOPE_X;
	x2 = SNOWINDEX_X(plx) + SNOW_ENVELOPE_X;
	y1 = SNOWINDEX_Y(ply) - SNOW_ENVELOPE_Y;
	y2 = SNOWINDEX_Y(ply) + SNOW_ENVELOPE_Y;

	// clamp it down to do less loop work
	x1 = CLAMP(x1, 0, SNOW_X - 1);
	y1 = CLAMP(y1, 0, SNOW_Y - 1);

	// step through the possible X coordinates of our snow envelope
	for ( x = x1; x <= x2; x++ ) {
		// get X world coordinate
		px = SNOWPOS_X(x);
		// and where it is in relation to the player's eye origin
		ix = px - plx;

		// step through the possible Y coordinates of our snow envelope
		for ( y = y1; y <= y2; y++ ) {
			// if there's no snow space at this snow index, stop here
			if ( snowtop[x][y] == 0.0f && snowbot[x][y] == 0.0f ) {
				continue;
			}

			// get Y world coordinate
			py = SNOWPOS_Y(y);
			// and where it is in relation to the player's eye origin
			iy = py - ply;

			// if outside an infinitely tall cylinder surrounding the player
			// (and definitely outside the snow envelope), stop here
			if ( ( dist2 = ix * ix + iy * iy ) > SNOW_ENVELOPE * SNOW_ENVELOPE ) {
				continue;
			}

			// grab the speed factor and convert back to 32-bit floating-point
			speedfact = (float)snowspd[x][y] / 32768 * 0.5 + 0.25;

			// find the "floor" - the closest "stepz" units
			pz = (plz > 0) ? plz - (plz % stepz) : plz - stepz - (plz % stepz);

			// figure the snowflake's position above the floor
			rz = stepz - ((int)((float)time * speedfact * 0.5)) % stepz;
			pz += rz;

			// too far below view origin?
			if ( pz - plz > stepz / 2 ) {
				pz -= stepz;
			}
			// too far above view origin?
			else if ( plz - pz > stepz / 2 ) {
				pz += stepz;
			}

			// if it's not within snow space, stop here
			if ( pz > snowtop[x][y] || pz < snowbot[x][y] ) {
				continue;
			}

			iz = pz - plz;

			// if it's outside the envelope (finally comparing against a sphere), stop here
			if ( (dist2 = dist2 + iz * iz) > SNOW_ENVELOPE * SNOW_ENVELOPE ) {
				continue;
			}

			if ( dist2 < SNOW_NEAR * SNOW_NEAR ) {
				continue;
			}

			// store the coordinates
			origin[0] = px;
			origin[1] = py;
			origin[2] = pz;

			// if it's probably out of the field of view, stop here
			// (note this is before the final position - that's why we added
			// 5 degrees above)
			// (also note that this is not frustum culling, but it's
			// nice and cheap, and culls out at least 1/2 of the snowflakes,
			// usually many more)
			VectorSubtract( origin, vieworigin, delta );
			if ( DotProduct( delta, forward ) / VectorLength( delta ) < cosfov ) {
				continue;
			}

			// calculate a little spiral
			origin[0] += cos((float)time * speedfact / 256.0f) * 16.0f;
			origin[1] += sin((float)time * speedfact / 256.0f) * 16.0f;

			// add the origin to the corners (basically translate the "corner" vertices by "origin")
			VectorAdd( origin, corners[0], verts[0].xyz );
			VectorAdd( origin, corners[1], verts[1].xyz );
			VectorAdd( origin, corners[2], verts[2].xyz );
			VectorAdd( origin, corners[3], verts[3].xyz );

			// link it up
			trap_R_AddPolyToScene( cgs.media.snowflakeShader, 4, verts );

#ifdef SNOWDEBUG
			total++;
#endif
		}
	}

#ifdef SNOWDEBUG
	endtime = trap_Milliseconds();
	CG_Printf("%d snowflakes in %d milliseconds\n", total, endtime - starttime);
#endif
}

#define NUM_POSITIONS 60

typedef struct {
	vec3_t origin;
	vec3_t angles;
	int time;
} savedpos_t;

static int g_head = 0, g_tail = 0;
static savedpos_t g_positions[NUM_POSITIONS];

#define RAIN_STEP_Z 16384
#define RAIN_ENVELOPE 640
#define RAIN_ENVELOPE_X (RAIN_ENVELOPE / (SNOW_EXT_X * 2 / SNOW_X))
#define RAIN_ENVELOPE_Y (RAIN_ENVELOPE / (SNOW_EXT_Y * 2 / SNOW_Y))

void CG_DrawRain( void ) {
	float px, py, pz, ix, iy, iz, rz, speedfact;
	int i, x, y, x1, x2, y1, y2;
	float plx, ply, plz, dist2;
	vec3_t origin, oldorigin;
	int time, frametime, oldtime, curr, timedelta;
	int stepz;
	vec3_t forward, vieworigin, delta, orgdelta, anglesdelta, oldvieworg, oldviewangles, perp;
	float cosfov, cosyaw, sinyaw;
	polyVert_t	verts[3];

#ifdef SNOWDEBUG
	int total = 0;
	int starttime, endtime;
	starttime = trap_Milliseconds();
#endif

	if ( !initsnow ) {
		return;
	}

	// don't want rain?
	if ( !cg_weather.integer || cg_weather.integer > 16 ) {
		return;
	}

	if ( !cgs.media.rainShader ) {
		return;
	}

	// set the polygon's texture coordinates
	verts[0].st[0] = 0.5;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;

	// set the polygon's vertex colors
	for ( i = 0; i < 3; i++ ) {
		verts[i].modulate[0] = 96;
		verts[i].modulate[1] = 96;
		verts[i].modulate[2] = 96;
		verts[i].modulate[3] = 255;
	}

	stepz = RAIN_STEP_Z / cg_weather.integer;

	// get a local copy for speed
	VectorCopy( cg.refdef.vieworg, vieworigin );

	// grab our player's eye position as integers for fast access later
	plx = vieworigin[0];
	ply = vieworigin[1];
	plz = vieworigin[2];

	time = cg.time + 60000;
	frametime = cg.frametime;
	if ( frametime < 33 ) {
		frametime = 33;
	}
	oldtime = time - frametime;

	// pop off the oldest position if full
	if ( (g_tail + 1) % NUM_POSITIONS == g_head ) {
		g_head = (g_head + 1) % NUM_POSITIONS;
	}

	// push the current position onto the queue
	VectorCopy( vieworigin, g_positions[g_tail].origin );
	VectorCopy( cg.refdefViewAngles, g_positions[g_tail].angles );
	g_positions[g_tail].time = time;
	g_tail = (g_tail + 1) % NUM_POSITIONS;

	curr = (g_tail + NUM_POSITIONS - 1 ) % NUM_POSITIONS;
	while ( curr != g_head ) {
		if ( g_positions[curr].time <= oldtime ) {
			break;
		}

		curr = (curr + NUM_POSITIONS - 1) % NUM_POSITIONS;
	}

	// not enough info to streak yet
	if ( curr == g_head ) {
		VectorCopy( vieworigin, oldvieworg );
		VectorClear( anglesdelta );
	}
	else {
		timedelta = time - g_positions[curr].time;
		VectorSubtract( vieworigin, g_positions[curr].origin, orgdelta );
		VectorMA( vieworigin, -(float)frametime / timedelta, orgdelta, oldvieworg );

		oldviewangles[YAW] = LerpAngle( cg.refdefViewAngles[YAW], g_positions[curr].angles[YAW], (float)frametime / timedelta );
		oldviewangles[PITCH] = LerpAngle( cg.refdefViewAngles[PITCH], g_positions[curr].angles[PITCH], (float)frametime / timedelta );
		oldviewangles[ROLL] = LerpAngle( cg.refdefViewAngles[ROLL], g_positions[curr].angles[ROLL], (float)frametime / timedelta );

		AnglesSubtract( cg.refdefViewAngles, oldviewangles, anglesdelta );
	}

	// get the forward vector (used later for culling)
	AngleVectors( cg.refdefViewAngles, forward, NULL, NULL );

	cosyaw = cos( DEG2RAD(anglesdelta[YAW]) );
	sinyaw = sin( DEG2RAD(anglesdelta[YAW]) );

	// grab the cosine of half the field of view (used later for culling)
	// add a bit more angle for a little optimization below
	cosfov = cos(DEG2RAD(cg_fov.value / 2 + 5.0f));

	// grab the closest "snow index" to our player
	x1 = SNOWINDEX_X(plx) - RAIN_ENVELOPE_X;
	x2 = SNOWINDEX_X(plx) + RAIN_ENVELOPE_X;
	y1 = SNOWINDEX_Y(ply) - RAIN_ENVELOPE_Y;
	y2 = SNOWINDEX_Y(ply) + RAIN_ENVELOPE_Y;

	// clamp it down to do less loop work
	x1 = CLAMP(x1, 0, SNOW_X - 1);
	y1 = CLAMP(y1, 0, SNOW_Y - 1);

	// step through the possible X coordinates of our snow envelope
	for ( x = x1; x <= x2; x++ ) {
		// get X world coordinate
		px = SNOWPOS_X(x);
		// and where it is in relation to the player's eye origin
		ix = px - plx;

		// step through the possible Y coordinates of our snow envelope
		for ( y = y1; y <= y2; y++ ) {
			// if there's no snow space at this snow index, stop here
			if ( snowtop[x][y] == 0.0f && snowbot[x][y] == 0.0f ) {
				continue;
			}

			// get Y world coordinate
			py = SNOWPOS_Y(y);
			// and where it is in relation to the player's eye origin
			iy = py - ply;

			// if outside an infinitely tall cylinder surrounding the player
			// (and definitely outside the snow envelope), stop here
			if ( ( dist2 = ix * ix + iy * iy ) > RAIN_ENVELOPE * RAIN_ENVELOPE ) {
				continue;
			}

			// grab the speed factor and convert back to 32-bit floating-point
			speedfact = (float)snowspd[x][y] / 32768 * 0.5 + 2.25;

			// find the "floor" - the closest "stepz" units
			pz = (plz > 0) ? plz - ((int)plz % stepz) : plz - stepz - ((int)plz % stepz);

			// figure the snowflake's position above the floor
			rz = stepz - ((int)((float)time * speedfact * 0.5)) % stepz;
			pz += rz;

			// too far below view origin?
			if ( pz - plz > stepz / 2 ) {
				pz -= stepz;
			}
			// too far above view origin?
			else if ( plz - pz > stepz / 2 ) {
				pz += stepz;
			}

			// if it's not within snow space, stop here
			if ( pz > snowtop[x][y] || pz < snowbot[x][y] ) {
				continue;
			}

			iz = pz - plz;

			// if it's outside the envelope (finally comparing against a sphere), stop here
			if ( (dist2 = dist2 + iz * iz) > RAIN_ENVELOPE * RAIN_ENVELOPE ) {
				continue;
			}

			if ( dist2 < SNOW_NEAR * SNOW_NEAR ) {
				continue;
			}

			// store the coordinates
			origin[0] = px;
			origin[1] = py;
			origin[2] = pz;

			// if it's probably out of the field of view, stop here
			// (note this is before the final position - that's why we added
			// 5 degrees above)
			// (also note that this is not frustum culling, but it's
			// nice and cheap, and culls out at least 1/2 of the snowflakes,
			// usually many more)
			VectorSubtract( origin, vieworigin, delta );
			if ( DotProduct( delta, forward ) / VectorLength( delta ) < cosfov ) {
				continue;
			}

			oldorigin[0] = px + cos((float)oldtime * speedfact / 1024.0f) * 16.0f;
			oldorigin[1] = py + sin((float)oldtime * speedfact / 1024.0f) * 16.0f;
			oldorigin[2] = pz + ((float)(frametime) * speedfact * 0.5);

			VectorSubtract( oldorigin, oldvieworg, delta );

			oldorigin[0] = cosyaw * delta[0] - sinyaw * delta[1];
			oldorigin[1] = sinyaw * delta[0] + cosyaw * delta[1];
			oldorigin[2] = delta[2];

			VectorAdd( oldorigin, vieworigin, verts[0].xyz );

			// calculate a little spiral
			origin[0] += cos((float)time * speedfact / 1024.0f) * 16.0f;
			origin[1] += sin((float)time * speedfact / 1024.0f) * 16.0f;

			VectorSubtract( oldorigin, origin, delta );
			CrossProduct( delta, forward, perp );
			VectorNormalize( perp );

			VectorSubtract( origin, perp, verts[1].xyz );
			VectorAdd( origin, perp, verts[2].xyz );
	
			// link it up
			trap_R_AddPolyToScene( cgs.media.rainShader, 3, verts );

#ifdef SNOWDEBUG
			total++;
#endif
		}
	}

#ifdef SNOWDEBUG
	endtime = trap_Milliseconds();
	CG_Printf("%d snowflakes in %d milliseconds\n", total, endtime - starttime);
#endif
}

void CG_DrawWeather() {
	if ( cg.predictedPlayerState.stats[STAT_FLAGS] & SF_RAIN ) {
		CG_DrawRain();
	}
	else {
		CG_DrawSnow();
	}
}

void CG_IceChip( vec3_t origin, int entityNum ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t		velocity;
	vec3_t		angles;

	if ( !cg_iceChips.integer ) {
		return;
	}

	VectorSubtract( origin, cg_entities[entityNum].lerpOrigin, velocity );
	VectorNormalize( velocity );
	VectorScale( velocity, 64.0f + random() * 192.0f, velocity );
	velocity[2] *= 0.5f;
	velocity[0] += random() * 16.0f - 8.0f;
	velocity[1] += random() * 16.0f - 8.0f;
	velocity[2] += random() * 16.0f - 8.0f;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_ICEFRAGMENT;
	le->floatFactor = -0.5f;
	le->startTime = cg.time;
	le->endTime = le->startTime + 2000 + random() * 1000;

	VectorCopy( origin, re->origin );
	re->hModel = cgs.media.gibFist;

	angles[0] = random() * 360;
	angles[1] = random() * 360;
	angles[2] = random() * 360;
	AnglesToAxis( angles, re->axis );
	VectorScale( re->axis[0], random() * 0.375f + 0.125f, re->axis[0] );
	VectorScale( re->axis[1], random() * 0.375f + 0.125f, re->axis[1] );
	VectorScale( re->axis[2], random() * 0.375f + 0.125f, re->axis[2] );
	re->nonNormalizedAxes = qtrue;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.3f;

	re->customShader = cgs.media.freezeShader2;
}

#define	ICE_VELOCITY	500
#define	ICE_JUMP		250

void CG_IceChips( vec3_t origin, int amount ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t		velocity;
	vec3_t		angles;
	int			i;

	if ( !cg_iceChips.integer ) {
		return;
	}
	
	for ( i = 0; i < amount; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_ICEFRAGMENT;
		le->floatFactor = -0.5f;
		le->startTime = cg.time;
		le->endTime = le->startTime + 5000 + random() * 3000;

		VectorCopy( origin, re->origin );
		re->hModel = cgs.media.gibFist;

		angles[0] = random() * 360;
		angles[1] = random() * 360;
		angles[2] = random() * 360;
		AnglesToAxis( angles, re->axis );
		VectorScale( re->axis[0], random() * 1.5f + 0.5f, re->axis[0] );
		VectorScale( re->axis[1], random() * 1.5f + 0.5f, re->axis[1] );
		VectorScale( re->axis[2], random() * 1.5f + 0.5f, re->axis[2] );
		re->nonNormalizedAxes = qtrue;

		le->pos.trType = TR_GRAVITY;
		VectorCopy( origin, le->pos.trBase );
		velocity[0] = crandom() * ICE_VELOCITY;
		velocity[1] = crandom() * ICE_VELOCITY;
		velocity[2] = ICE_JUMP + crandom() * ICE_VELOCITY;
		VectorCopy( velocity, le->pos.trDelta );

		le->pos.trTime = cg.time;

		le->bounceFactor = 0.3f;

		re->customShader = cgs.media.freezeShader2;
	}
}

void CG_ShowInfo_f( void ) {
	cg.warningTime = 999999;
}

void CG_Drop_f( void ) {
	char	command[ 128 ];
	char	message[ 128 ];
	gitem_t	*item;
	int	j;

	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR || cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_REDCOACH || cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_BLUECOACH ) {
		return;
	}
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 ) {
		CG_Printf( "You must be alive to use this command.\n" );
		return;
	}
	trap_Args( message, 128 );
	item = BG_FindItem( message );
	if ( !item ) {
		CG_Printf( "unknown item: %s\n", message );
		return;
	}

	if ( !cg_items[ item->giTag ].registered ) {
		return;
	}

	j = item->giTag;
	switch ( item->giType ) {
	case IT_WEAPON:
		if ( cgs.dmflags & 256 ) {
			return;
		}
		if ( !( cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << j ) ) ) {
			CG_Printf( "Out of item: %s\n", message );
			return;
		}
		if ( cg.snap->ps.weaponstate != WEAPON_READY ) {
			return;
		}
		if ( j == cg.snap->ps.weapon ) {
			return;
		}
		if ( j <= WP_MACHINEGUN /*|| j == WP_GRAPPLING_HOOK */) {
			CG_Printf( "Item is not dropable.\n" );
			return;
		}
	case IT_AMMO:
		if ( cg.snap->ps.ammo[ j ] < 1 ) {
			CG_Printf( "Out of item: %s\n", message );
			return;
		}
		break;
	case IT_POWERUP:
		if ( cg.snap->ps.powerups[ j ] <= cg.time ) {
			CG_Printf( "Out of item: %s\n", message );
			return;
		}
		break;
	case IT_HOLDABLE:
		if ( j == HI_KAMIKAZE ) {
			CG_Printf( "Item is not dropable.\n" );
			return;
		}
		if ( bg_itemlist[ cg.snap->ps.stats[ STAT_HOLDABLE_ITEM ] ].giTag != j ) {
			CG_Printf( "Out of item: %s\n", message );
			return;
		}
		break;
	default:
		CG_Printf( "Item is not dropable.\n" );
		return;
	}

	Com_sprintf( command, 128, "drop %s", message );
	trap_SendClientCommand( command );
}

void CG_BodyObituary( entityState_t *ent, char *targetName ) {
	int	target, attacker;
	char	*message;
	char	*message2;
	const char	*attackerInfo;
	char	attackerName[38];
	gender_t	gender;
	char	*s;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;

	attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
	if ( !attackerInfo ) return;
	Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof ( attackerName ) - 2 );
	strcat( attackerName, S_COLOR_WHITE );

	if ( rand() & 1 ) {
		message = "thawed";
		message2 = "like a package of frozen peas";
	} else {
		gender = cgs.clientinfo[ target ].gender;
		message = "evicted";
		if ( gender == GENDER_FEMALE ) {
			message2 = "from her igloo";
		} else if ( gender == GENDER_NEUTER ) {
			message2 = "from its igloo";
		} else {
			message2 = "from his igloo";
		}
	}

	if ( attacker == cg.snap->ps.clientNum && cg.snap->ps.clientNum == cg.clientNum ) {
		s = va( "^1You thawed ^7%s", targetName );
		CG_CenterPrint( s, SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
	}
	if ( target == cg.snap->ps.clientNum && cg.snap->ps.clientNum == cg.clientNum ) {
		s = va( "%s ^1thawed you", attackerName );
		CG_CenterPrint( s, SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
	}
	CG_Printf( "%s %s %s %s.\n", attackerName, message, targetName, message2 );
}

qboolean Q_Isfreeze( int clientNum ) {
	int readyMask[4];

	readyMask[0] = cg.snap->ps.stats[ STAT_CLIENTS_READY0 ];
	readyMask[1] = cg.snap->ps.stats[ STAT_CLIENTS_READY1 ];
	readyMask[2] = cg.snap->ps.stats[ STAT_CLIENTS_READY2 ];
	readyMask[3] = cg.snap->ps.stats[ STAT_CLIENTS_READY3 ];

	if ( readyMask[clientNum / 16] & ( 1 << (clientNum % 16) ) ) {
		return ( !( cg.warmup || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) );
	}
	return qfalse;
}

qboolean Q_IsFrozenBody( int entityNum ) {
	return (!cg_entities[entityNum].currentState.weapon &&
			cg_entities[entityNum].currentState.otherEntityNum != ENTITYNUM_NONE);
}

void CG_AddGib( localEntity_t *le ) {
	const qhandle_t	hShader = cgs.media.freezeShader;

	if ( le->refEntity.customShader == hShader ) {
		le->refEntity.customShader = 0;
		CG_AddRefEntityToScene( &le->refEntity, 0, REF_ALWAYS, qtrue ); //multiview
		le->refEntity.customShader = hShader;
	}
}

int CG_Cvar_VariableIntegerValue( const char *name ) {
	char val[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( name, val, sizeof(val) );

	return atoi(val);
}

void CG_AddOffhandGrapple( centity_t *cent ) {
	vec3_t start, end, up, delta, angles, temp, dir;
	vec_t dist;
	refEntity_t beam;

	if ( cent == &cg.predictedPlayerEntity ) {
		// if no grapple
		if ( cg.predictedPlayerState.pm_flags & PMF_GRAPPLE_FLY ) {
			VectorCopy( cg.predictedPlayerState.origin, start );
			VectorMA( cg.predictedPlayerState.grapplePoint, (float)cg.predictedPlayerState.grappleTime / 1000.0f, 
					cg.predictedPlayerState.grappleVelocity, end );
		}
		else if ( cg.predictedPlayerState.pm_flags & PMF_GRAPPLE_PULL ) {
			VectorCopy( cg.predictedPlayerState.origin, start );
			VectorCopy( cg.predictedPlayerState.grapplePoint, end );
		}
		else {
			return;
		}

		start[2] += cg.predictedPlayerState.viewheight;
		AngleVectors( cg.predictedPlayerState.viewangles, NULL, NULL, up );
		VectorMA( start, -8, up, start );
	}
	else {
		if ( !cent->grappleValid ) {
			return;
		}

		VectorCopy( cent->lerpOrigin, start );
		VectorCopy( cent->grappleLerpOrigin, end );

		start[2] += DEFAULT_VIEWHEIGHT;
		AngleVectors( cent->lerpAngles, NULL, NULL, up );
		VectorMA( start, -8, up, start );
	}

	VectorSubtract( end, start, delta );
	VectorCopy( delta, dir );
	dist = VectorNormalize( dir );

	VectorMA( end, -8, dir, end );

	if ( dist < 64.0f ) {
		return; // Don't draw if close
	}

	memset( &beam, 0, sizeof(beam) );

	beam.reType = RT_MODEL;
	beam.hModel = cgs.media.grappleCylinder;
	beam.customShader = cgs.media.quadShader_nocull;

	VectorMA( start, 0.5f, delta, beam.origin );

	vectoangles( delta, angles );
	AnglesToAxis( angles, beam.axis );

	VectorScale( beam.axis[0], dist * 1.03f, beam.axis[0] ); // forward
	VectorScale( beam.axis[1], 1.25f, beam.axis[1] ); // right
	VectorScale( beam.axis[2], 1.25f, beam.axis[2] ); // up

	// swap forward and up vectors
	VectorCopy( beam.axis[0], temp );
	VectorCopy( beam.axis[2], beam.axis[0] );
	VectorCopy( temp, beam.axis[2] );

	beam.nonNormalizedAxes = qtrue;

	beam.shaderRGBA[0] = 32;
	beam.shaderRGBA[1] = 32;
	beam.shaderRGBA[2] = 32;
	beam.shaderRGBA[3] = 255;

	CG_AddRefEntityToScene( &beam, 0, REF_ALWAYS, qfalse );


	memset( &beam, 0, sizeof(beam) );

	VectorCopy ( start, beam.origin );
	VectorCopy( end, beam.oldorigin );

	beam.reType = RT_RAIL_CORE;
	beam.customShader = cgs.media.railCoreShader;

	beam.shaderRGBA[0] = 128;
	beam.shaderRGBA[1] = 128;
	beam.shaderRGBA[2] = 160;
	beam.shaderRGBA[3] = 255;

	CG_AddRefEntityToScene( &beam, 0, REF_ALWAYS, qfalse );


	memset( &beam, 0, sizeof(beam) );

	VectorCopy ( end, beam.origin );

	beam.reType = RT_SPRITE;
	beam.customShader = cgs.media.grappleFlareShader;
	beam.radius = 24.0f;

	CG_AddRefEntityToScene( &beam, 0, REF_ALWAYS, qfalse );

	trap_R_AddLightToScene( beam.origin, 100 + (rand()&7), 0.5f, 0.45f, 0.45f );
}

static void FloatToStr5( char *out, float in ) {
	char tmp[6];
	char *spaces = "     ";

	Com_sprintf( tmp, sizeof(tmp), "%.1f", in );
	Com_sprintf( out, 6, "%s%s", &spaces[strlen(tmp)], tmp );
}

void CG_ShowTopshots( qboolean top ) {
	int num, i, arg;
	char *name = top ? "topshots" : "bottomshots";
	char *desc = top ? "Best" : "Worst";

	num = atoi( CG_Argv(1) );

	if ( num == 0 ) {
		CG_Printf( "^3Not enough accuracy information is available.\n\n" );
		return;
	}

#ifndef MISSIONPACK
	CG_Printf( "^1Use /%s {MG|SG|GL|RL|LG|RG|PG|BFG} to get accuracies on a specific weapon.\n\n", name );
#else
	CG_Printf( "^1Use /%s {MG|SG|GL|RL|LG|RG|PG|BFG|NG|PL|CG} to get accuracies on a specific weapon.\n\n", name );
#endif
	CG_Printf( "^3%s match accuracies:\n\n", desc );

	CG_Printf( "^7Weapon        |  Acc  Hits/Atts | Kls Dths | Num/Player\n" );
	CG_Printf( "^7--------------+-----------------+----------+---------------------------------\n" );

	// unmarshal the topshots
	for ( i = 0, arg = 2; i < num; i++, arg += 7 ) {
		topshot_t topshot;
		clientInfo_t *ci;
		char stracc[6];

		topshot.weapon = atoi( CG_Argv(arg + 0) );
		topshot.acc = atof( CG_Argv(arg + 1) );
		topshot.hits = atoi( CG_Argv(arg + 2) );
		topshot.attacks = atoi( CG_Argv(arg + 3) );
		topshot.kills = atoi( CG_Argv(arg + 4) );
		topshot.deaths = atoi( CG_Argv(arg + 5) );
		topshot.clientNum = atoi( CG_Argv(arg + 6) );

		ci = &cgs.clientinfo[topshot.clientNum];
		if ( !ci->infoValid ) {
			continue;
		}

		FloatToStr5( stracc, topshot.acc * 100.0f );

		CG_Printf( "^3%-13s^7 |%s ^5%5d/%-4d ^7|^2%4d ^1%4d ^7|  %2d/%s\n",
			BG_WeaponName( topshot.weapon ),
			stracc,
			topshot.hits,
			topshot.attacks,
			topshot.kills,
			topshot.deaths,
			topshot.clientNum,
			ci->name );
	}
}

void CG_ShowWeaponAcc( void ) {
	int num, i, arg;
	int weapon;

	num = atoi( CG_Argv(1) );

	if ( num == 0 ) {
		CG_Printf( "^3Not enough accuracy information is available.\n\n" );
		return;
	}

	weapon = atoi( CG_Argv(2) );

	CG_Printf( "^3Match accuracies for %s:\n\n", BG_WeaponName(weapon) );

	CG_Printf( "^7  Acc  Hits/Atts | Kls Dths | Num/Player\n" );
	CG_Printf( "^7-----------------+----------+------------------------------------------------\n" );

	// unmarshal the topshots
	for ( i = 0, arg = 3; i < num; i++, arg += 7 ) {
		topshot_t topshot;
		clientInfo_t *ci;
		char stracc[6];

		topshot.weapon = atoi( CG_Argv(arg + 0) );
		topshot.acc = atof( CG_Argv(arg + 1) );
		topshot.hits = atoi( CG_Argv(arg + 2) );
		topshot.attacks = atoi( CG_Argv(arg + 3) );
		topshot.kills = atoi( CG_Argv(arg + 4) );
		topshot.deaths = atoi( CG_Argv(arg + 5) );
		topshot.clientNum = atoi( CG_Argv(arg + 6) );

		ci = &cgs.clientinfo[topshot.clientNum];
		if ( !ci->infoValid ) {
			continue;
		}

		FloatToStr5( stracc, topshot.acc * 100.0f );

		CG_Printf( "%s%s ^5%5d/%-4d ^7|^2%4d ^1%4d ^7|  %2d/%s\n",
			topshot.weapon != 0 ? "^7" : "^5",
			stracc,
			topshot.hits,
			topshot.attacks,
			topshot.kills,
			topshot.deaths,
			topshot.clientNum,
			ci->name );
	}
}

void CG_ShowAccuracyStats( void ) {
	qboolean accum;
	int num, i, arg, freezes, thaws, frozen;
	int clientnum;
	clientInfo_t *ci;

	accum = atoi( CG_Argv(1) );
	clientnum = atoi( CG_Argv(2) );

	ci = &cgs.clientinfo[clientnum];
	if ( !ci->infoValid ) {
		return;
	}

	num = atoi( CG_Argv(3) );

	if ( num <= 1 ) {
		if ( !cg.showWStats && !cg.wstatsShowing ) { //wstats - don't dump to the console if doing +wstats
			CG_Printf( "^3No accuracy information is available.\n\n" );

			freezes = atoi( CG_Argv(4) );
			thaws = atoi( CG_Argv(5) );
			frozen = atoi( CG_Argv(6) );

			CG_Printf("\n^3Freezes: ^7%d  ^3Thaws: ^7%d  ^3Times Frozen: ^7%d\n\n",
				freezes, thaws, frozen );
		}

		return;
	}

//wstats
	if ( !accum && clientnum == cg.clientNum ) {
		cgs.numWeaponStats = 0;
		memset( cgs.validWeaponData, 0, sizeof(cgs.validWeaponData) );
		memset( cgs.weaponAccuracy, 0, sizeof(cgs.weaponAccuracy) );
		memset( cgs.totalShots, 0, sizeof(cgs.totalShots) );
		memset( cgs.totalEnemyHits, 0, sizeof(cgs.totalEnemyHits) );
		memset( cgs.totalTeamHits, 0, sizeof(cgs.totalTeamHits) );
		memset( cgs.totalCorpseHits, 0, sizeof(cgs.totalCorpseHits) );
		memset( cgs.totalKills, 0, sizeof(cgs.totalKills) );
		memset( cgs.totalDeaths, 0, sizeof(cgs.totalDeaths) );
		memset( cgs.damageGiven, 0, sizeof(cgs.damageGiven) );
		memset( cgs.damageReceived, 0, sizeof(cgs.damageReceived) );
		cgs.freezes = cgs.thaws = cgs.frozen = 0;
	}
//wstats

	if ( !cg.showWStats && !cg.wstatsShowing ) { //wstats - don't dump to the console if doing +wstats
		if ( accum ) {
			CG_Printf( "^3Accumulated accuracy stats for ^7%s:\n\n", ci->name );
		}
		else {
			CG_Printf( "^3Accuracy stats for ^7%s:\n\n", ci->name );
		}

		CG_Printf( "^7Weapon        |  Acc  Hits/Atts Team Corpse | Kills Deaths |    DG    DR \n" );
		CG_Printf( "^7--------------+-----------------------------+--------------+-------------\n" );
	}

	for ( i = 0, arg = 4; i < num; i++, arg += 10 ) {
		stat_t stat;
		char stracc[6];

		stat.weapon = atoi( CG_Argv(arg + 0) );
		stat.acc = atof( CG_Argv(arg + 1) );
		stat.enemyhits = atoi( CG_Argv(arg + 2) );
		stat.shots = atoi( CG_Argv(arg + 3) );
		stat.teamhits = atoi( CG_Argv(arg + 4) );
		stat.corpsehits = atoi( CG_Argv(arg + 5) );
		stat.kills = atoi( CG_Argv(arg + 6) );
		stat.deaths = atoi( CG_Argv(arg + 7) );
		stat.dg = atoi( CG_Argv(arg + 8) );
		stat.dr = atoi( CG_Argv(arg + 9) );

//wstats
		if ( !accum && clientnum == cg.clientNum ) {
			// store the stats
			cgs.validWeaponData[stat.weapon] = qtrue;
			cgs.weaponAccuracy[stat.weapon] = stat.acc;
			cgs.totalShots[stat.weapon] = stat.shots;
			cgs.totalEnemyHits[stat.weapon] = stat.enemyhits;
			cgs.totalTeamHits[stat.weapon] = stat.teamhits;
			cgs.totalCorpseHits[stat.weapon] = stat.corpsehits;
			cgs.totalKills[stat.weapon] = stat.kills;
			cgs.totalDeaths[stat.weapon] = stat.deaths;
			cgs.damageGiven[stat.weapon] = stat.dg;
			cgs.damageReceived[stat.weapon] = stat.dr;
			cgs.numWeaponStats++;
		}
//wstats

		if ( !cg.showWStats && !cg.wstatsShowing ) { //wstats - don't dump to the console if doing +wstats
			FloatToStr5( stracc, stat.acc * 100.0f );

			if ( stat.weapon != WP_NONE ) {
				CG_Printf( "^3%-13s^7 |%s ^5%5d/%-4d %4d   %4d ^7| ^2%5d  ^1%5d ^7| ^2%5d ^1%5d\n",
					BG_WeaponName( stat.weapon ),
					stracc,
					stat.enemyhits,
					stat.shots,
					stat.teamhits,
					stat.corpsehits,
					stat.kills,
					stat.deaths,
					stat.dg,
					stat.dr );
			}
			else {
				CG_Printf( "^7--------------+-----------------------------+--------------+-------------\n" );
				CG_Printf( "^7Totals:       |^7%s ^5%5d/%-4d %4d   %4d ^7| ^2%5d  ^1%5d ^7| ^2%5d ^1%5d\n",
					stracc,
					stat.enemyhits,
					stat.shots,
					stat.teamhits,
					stat.corpsehits,
					stat.kills,
					stat.deaths,
					stat.dg,
					stat.dr );
			}
		}
	}

	freezes = atoi( CG_Argv(arg + 0) );
	thaws = atoi( CG_Argv(arg + 1) );
	frozen = atoi( CG_Argv(arg + 2) );

//wstats
	if ( !accum && clientnum == cg.clientNum ) {
		cgs.freezes = freezes;
		cgs.thaws = thaws;
		cgs.frozen = frozen;
	}
//wstats

	if ( !cg.showWStats && !cg.wstatsShowing ) { //wstats - don't dump to the console if doing +wstats
		CG_Printf("\n^3Freezes: ^7%d  ^3Thaws: ^7%d  ^3Times Frozen: ^7%d\n\n",
			freezes, thaws, frozen );
	}
}

//wstats
#define WSTATS_WIDTH ((int)(SMALLCHAR_WIDTH * 1.0f))
#define WSTATS_HEIGHT ((int)(SMALLCHAR_HEIGHT * 0.625f))
#define WSTATS_HEIGHT_FUDGE ((int)(SMALLCHAR_HEIGHT * 0.625f))

static void CG_DrawWStatsString( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, WSTATS_WIDTH, WSTATS_HEIGHT, 0 );
}

void CG_DrawBorder( int x, int y, int width, int height, int thickness, vec4_t color ) {
	// top line
	CG_FillRect( x, y, width, thickness, color );
	// bottom line
	CG_FillRect( x, y + height - thickness, width, thickness, color );
	// left line
	CG_FillRect( x, y + thickness, thickness, height - thickness * 2, color );
	// right line
	CG_FillRect( x + width - thickness, y + thickness, thickness, height - thickness * 2, color );
}

qboolean CG_DrawWStats( void ) {
	float fade;
	vec4_t hcolor, scolor, lcolor[2], ocolor, bcolor;
	int startx, starty, x, y, height, width, numlines, x1, y1, x2, y2;
	int i, weapon;

	if ( cg.showWStats ) {
		fade = 1.0f;
	} else {
		float *fadeColor = CG_FadeColor( cg.wstatsFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			return qfalse;
		}

		fade = fadeColor[3];
	}

	fade *= 0.75f;

	ocolor[0] = 0.5f;
	ocolor[1] = 0.3f;
	ocolor[2] = 0.2f;
	ocolor[3] = fade * 0.5f;

	bcolor[0] = 0.8f;
	bcolor[1] = 0.1f;
	bcolor[2] = 0.0f;
	bcolor[3] = fade * 0.75f;

	hcolor[0] = 1.0f;
	hcolor[1] = 0.8f;
	hcolor[2] = 1.0f;
	hcolor[3] = fade;

	scolor[0] = scolor[1] = scolor[2] = 1.0f;
	scolor[3] = fade * 0.75f;

	lcolor[0][0] = 1.0f;
	lcolor[0][1] = 0.5f;
	lcolor[0][2] = 0.4f;
	lcolor[0][3] = fade;

	lcolor[1][0] = 1.0f;
	lcolor[1][1] = 0.8f;
	lcolor[1][2] = 0.7f;
	lcolor[1][3] = fade;

	startx = 8;
	width = 58 * WSTATS_WIDTH;

	height = (cgs.numWeaponStats + 5) * WSTATS_HEIGHT_FUDGE;
	if ( cg.intermissionStarted ) {
		starty = 472 - height;
	}
	else {
		starty = 360 - height;
	}

	x1 = startx - 2;
	y1 = starty - 2;
	x2 = width + 5;
	y2 = height + 3;

	// background
	trap_R_SetColor( ocolor );
	CG_DrawPic( x1, y1, x2, y2, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	// border
	CG_DrawBorder( x1 - 1, y1 - 1, x2 + 2, y2 + 2, 1, bcolor );

	x = startx;
	y = starty;
	numlines = 0;

	if ( cgs.numWeaponStats == 0 ) {
		y += WSTATS_HEIGHT_FUDGE;
		CG_DrawWStatsString( x, y, "No weapon accuracy stats are available.", hcolor );
		y += WSTATS_HEIGHT_FUDGE;
	}
	else {
		for ( i = 1; i <= WP_NUM_WEAPONS; i++ ) {
			char stracc[6];
			char line[MAX_STRING_CHARS];
			vec_t *color;

			if ( i == 1 ) {
				CG_DrawWStatsString( x, y, "Weapon          Acc  Hits/Atts Team  Kll  Dth    DG    DR ", hcolor );
				y += WSTATS_HEIGHT_FUDGE;
			}
			
			if ( i == 1 || i == WP_NUM_WEAPONS ) {
				CG_FillRect( x, y + WSTATS_HEIGHT / 2, 58 * WSTATS_WIDTH, 1, scolor );
				y += WSTATS_HEIGHT_FUDGE;
			}

			if ( i == WP_NUM_WEAPONS ) {
				weapon = WP_NONE;
			}
			else {
				weapon = i;
			}

			if ( !cgs.validWeaponData[weapon] ) {
				continue;
			}

			FloatToStr5( stracc, cgs.weaponAccuracy[weapon] * 100.0f );

			if ( weapon != WP_NONE ) {
				Com_sprintf( line, sizeof(line), "%-13s %s %5d/%-4d %4d %4d %4d %5d %5d",
					BG_WeaponName( weapon ),
					stracc,
					cgs.totalEnemyHits[weapon],
					cgs.totalShots[weapon],
					cgs.totalTeamHits[weapon],
					cgs.totalKills[weapon],
					cgs.totalDeaths[weapon],
					cgs.damageGiven[weapon],
					cgs.damageReceived[weapon] );

				color = lcolor[numlines % 2];
			}
			else {
				Com_sprintf( line, sizeof(line), "Totals:       %s %5d/%-4d %4d %4d %4d %5d %5d",
					stracc,
					cgs.totalEnemyHits[weapon],
					cgs.totalShots[weapon],
					cgs.totalTeamHits[weapon],
					cgs.totalKills[weapon],
					cgs.totalDeaths[weapon],
					cgs.damageGiven[weapon],
					cgs.damageReceived[weapon] );

				color = hcolor;
			}

			CG_DrawWStatsString( x, y, line, color );

			y += WSTATS_HEIGHT_FUDGE;
			numlines++;
		}

		CG_FillRect( x + 14 * WSTATS_WIDTH, starty, 1, (numlines + 3) * WSTATS_HEIGHT_FUDGE, scolor );
		CG_FillRect( x + 36 * WSTATS_WIDTH, starty, 1, (numlines + 3) * WSTATS_HEIGHT_FUDGE, scolor );
	}

	y += WSTATS_HEIGHT_FUDGE;
	CG_DrawWStatsString( x, y, va("Freezes: %d  Thaws: %d  Times Frozen: %d", cgs.freezes, cgs.thaws, cgs.frozen), hcolor );

	return qtrue;
}
//wstats

void CG_Record( ){
	char buf[256];
	Com_sprintf(buf,256,"set g_synchronousclients 1 ; record %s ; set g_synchronousclients 0;\n",CG_Argv(1));
	//Com_Printf(buf);
	trap_SendConsoleCommand(buf);
}

void CG_AutoRecord( void ){
	char buf[256];
	int i;

	//if we are a spec then don't record, coaches however will record
	if(cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR) return;

	if(cg_autodemo.integer == 1 || cg_autodemo.integer == 2){
		if(cg.doReadyUsed == 1 || cg_autodemo.integer == 2){ //game had a doready or we record the lot
			for(i=0;i<10000;i++){
				Com_sprintf(buf,256,"demos/ufreeze_autodemo_%d.dm_68",i);
				if(CG_FileExists(buf)) continue;
				else break;
			}
			
			Com_sprintf(buf,256,"ufreeze_autodemo_%d",i);

			Com_sprintf(buf,256,"set g_synchronousclients 1 ; record %s ; set g_synchronousclients 0;\n",buf);
			trap_SendConsoleCommand(buf);
		}
	}
}

void CG_ShowTeamStats( void ) {
	int num, i, arg, team, namelen;
	char *spaces = "                                    ";

	team = atoi( CG_Argv(1) );
	num = atoi( CG_Argv(2) );

	if ( num <= 1 ) {
		CG_Printf( "^3No team information is available.\n\n" );
		return;
	}

	CG_Printf( "^3Player stats for %s ^3(^7*^3 = not still on team)^7:\n\n", TeamNameColor(team) );
	CG_Printf( "^7Player                              | Frez Thaw Froz  Net |    DG    DR \n" );
	CG_Printf( "^7------------------------------------+---------------------+-------------\n" );

	for ( i = 0, arg = 3; i < num; i++, arg += 6 ) {
		teamstat_t stat;
		clientInfo_t *ci;

		stat.clientNum = atoi( CG_Argv(arg + 0) );
		stat.freezes = atoi( CG_Argv(arg + 1) );
		stat.thaws = atoi( CG_Argv(arg + 2) );
		stat.frozen = atoi( CG_Argv(arg + 3) );
		stat.dg = atoi( CG_Argv(arg + 4) );
		stat.dr = atoi( CG_Argv(arg + 5) );

		if ( stat.clientNum >= 0 ) {
			ci = &cgs.clientinfo[stat.clientNum];
			if ( !ci->infoValid	) {
				continue;
			}

			namelen = strlencolor( ci->name );
			if ( ci->team != team ) {
				namelen++;
			}

			if ( namelen > 36 ) {
				namelen = 36;
			}

			CG_Printf( "^7%s%s%s^7| ^3%4d %4d %4d ^7%4d | ^2%5d ^1%5d\n",
				ci->team != team ? "*" : "",
				ci->name,
				&spaces[namelen],
				stat.freezes,
				stat.thaws,
				stat.frozen,
				stat.freezes + stat.thaws - stat.frozen,
				stat.dg,
				stat.dr );
		}
		else {
			CG_Printf( "^7------------------------------------+---------------------+-------------\n" );
			CG_Printf( "^7Totals:                             | ^3%4d %4d %4d ^7%4d | ^2%5d ^1%5d \n\n", 
				stat.freezes,
				stat.thaws,
				stat.frozen,
				stat.freezes + stat.thaws - stat.frozen,
				stat.dg,
				stat.dr );
		}
	}
}

// this has to be global because lcc complains about
// locals being > 32K otherwise
static snapshot_t g_activeSnapshots[2];

/*
CG_ClearCG

Clears most of cg_t - everything but the data we need
to keep the game running smoothly, or to keep it from
crashing
*/
static void CG_ClearCG( void ) {
	refdef_t refdef;
	vec3_t refdefViewAngles;
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	qboolean validPPS;
	int clientFrame;
	int clientNum;
	int deferredPlayerLoading;
	int frametime;
	int time;
	int oldTime;
	int latestSnapshotTime;
	int latestSnapshotNum;
	snapshot_t *snap;
	snapshot_t *nextSnap;

	// save the stuff we want to keep
	refdef = cg.refdef;
	VectorCopy( cg.refdefViewAngles, refdefViewAngles );
	predictedPlayerState = cg.predictedPlayerState;
	predictedPlayerEntity = cg.predictedPlayerEntity;
	validPPS = cg.validPPS;
	snap = cg.snap;
	nextSnap = cg.nextSnap;
	memcpy( &g_activeSnapshots, cg.activeSnapshots, sizeof(cg.activeSnapshots) );
	clientFrame = cg.clientFrame;
	clientNum = cg.clientNum;
	deferredPlayerLoading = cg.deferredPlayerLoading;
	frametime = cg.frametime;
	time = cg.time;
	oldTime = cg.oldTime;
	latestSnapshotTime = cg.latestSnapshotTime;
	latestSnapshotNum = cg.latestSnapshotNum;

	// clear it out
	memset( &cg, 0, sizeof(cg_t) );

	// restore the saved stuff
	cg.refdef = refdef;
	VectorCopy( refdefViewAngles, cg.refdefViewAngles );
	cg.predictedPlayerState = predictedPlayerState;
	cg.predictedPlayerEntity = predictedPlayerEntity;
	cg.validPPS = validPPS;
	cg.snap = snap;
	cg.nextSnap = nextSnap;
	memcpy( &cg.activeSnapshots, g_activeSnapshots, sizeof(cg.activeSnapshots) );
	cg.clientFrame = clientFrame;
	cg.clientNum = clientNum;
	cg.deferredPlayerLoading = deferredPlayerLoading;
	cg.frametime = frametime;
	cg.time = time;
	cg.oldTime = oldTime;
	cg.latestSnapshotTime = latestSnapshotTime;
	cg.latestSnapshotNum = latestSnapshotNum;

	CG_InitializeWindowRequests();
}

/*
CG_DemoStateChanged

Called when the client needs to read and react to the
CS_SSDEMOSTATE configstring.
*/
void CG_DemoStateChanged( void ) {
	const char *state = CG_ConfigString( CS_SSDEMOSTATE );

	if ( *state != '\0' ) {
		int ssDemoPaused = cg.ssDemoPaused;

		// if the demo has just started
		if ( !cg.ssDemoPlayback ) {
			// clear most of cg_t
			CG_ClearCG();
		}

		// flag that we're playing a demo
		cg.ssDemoPlayback = qtrue;

		// set the demo time difference, skip-to frame, and pause state
		cg.ssDemoTimeDiff = atoi( Info_ValueForKey(state, "td") );
		cg.ssSkipToDemoTime = atoi( Info_ValueForKey(state, "sk") );
		cg.ssDemoPaused = atoi( Info_ValueForKey(state, "ps") );

		if ( !ssDemoPaused && cg.ssDemoPaused ) {
			cg.ssDemoPauseTime = cg.snap->serverTime;
		}

		//CG_Printf("demo time difference: %d\n", ssDemoTimeDiff);
	}
	else {
		// if there was a demo playing
		if ( cg.ssDemoPlayback ) {
			int i;

			// clear most of cg_t
			CG_ClearCG();

			// clear recorded client info
			for ( i = HALF_CLIENTS; i < MAX_CLIENTS; i++ ) {
				memset( &cgs.clientinfo[i], 0, sizeof(clientInfo_t) );
			}
		}

		cg.ssDemoPlayback = qfalse;
		cg.ssDemoTimeDiff = 0;
		cg.ssSkipToDemoTime = 0;
		cg.ssDemoPaused = qfalse;
	}
}

static char *g_szCantExec[] = {
	"+wstats", "-wstats", NULL
};

/*
CG_CantExecDuringDemo
*/
qboolean CG_CantExecDuringDemo( const char *cmd ) {
	int i;
	
	for ( i = 0; g_szCantExec[i] != NULL; i++ ) {
		if ( !Q_stricmp(cmd, g_szCantExec[i]) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
CG_IsSpectator

Returns qtrue if the game client is a spectator of some kind
*/
qboolean CG_IsSpectator( void ) {
	// if the player is frozen AND is acting like a spectator
	if ( cg.snap->ps.stats[STAT_FLAGS] & SF_FROZEN &&
			(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qtrue;
	}
	// OR the player is a bona fide spectator
	else if ( cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR || cg.snap->ps.stats[STAT_TEAM] == TEAM_REDCOACH || cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUECOACH ) {
		return qtrue;
	}

	return qfalse;
}

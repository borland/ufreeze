#include "cg_local.h"
#include "cg_multiview.h"

void CG_InitializeWindowRequests( void ) {
	int i;

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		cg.requestedPipWindows[i] = -1;
	}

	trap_SendConsoleCommand( "pipreq\n" );
}

void CG_SendPipWindowCommand( void ) {
	char wins[MAX_STRING_CHARS];
	int i;

	wins[0] = '\0';
	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		if ( cg.requestedPipWindows[i] != -1 ) {
			Q_strcat( wins, sizeof(wins), va("\\%d\\%d", i, cg.requestedPipWindows[i]) );
		}
	}

	trap_SendConsoleCommand( va("pipreq %s\n", wins) );
}

void CG_RequestPipWindowUp( int clientNum ) {
	int i;

	if ( clientNum == cg.predictedPlayerState.clientNum ) {
		return;
	}

	// look for an opening in the requested windows
	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		if ( cg.requestedPipWindows[i] == clientNum ) {
			return;
		}

		if ( cg.requestedPipWindows[i] == -1 ) {
			break;
		}
	}

	if ( i == MAX_PIP_WINDOWS ) {
		return;
	}

	cg.requestedPipWindows[i] = clientNum;

	CG_SendPipWindowCommand();
	CG_InsertPipWindow( MAX_PIP_WINDOWS, 8, 300, 160, 120, clientNum );
}

void CG_RequestPipWindowDown( int clientNum ) {
	int i;

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		if ( cg.requestedPipWindows[i] == clientNum ) {
			break;
		}
	}

	if ( i == MAX_PIP_WINDOWS ) {
		return;
	}

	cg.requestedPipWindows[i] = -1;

	CG_SendPipWindowCommand();
	CG_RemovePipWindow( clientNum );
}

void CG_RequestPipWindowMaximize( int clientNum ) {
	pipWindow_t *pip;
	int i;

	for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
		pip = &cg.pipWindows[i];
		if ( pip->valid && pip->clientNum == clientNum ) {
			break;
		}
	}

	if ( i == MAX_PIP_WINDOWS ) {
		return;
	}

	if ( cg.predictedPlayerState.clientNum == cg.clientNum || cg.predictedPlayerState.clientNum == clientNum ) {
		CG_RequestPipWindowDown( clientNum );
	}
	else {
		pip->clientNum = cg.predictedPlayerState.clientNum;

		for ( i = 0; i < MAX_PIP_WINDOWS; i++ ) {
			if ( cg.requestedPipWindows[i] == clientNum ) {
				cg.requestedPipWindows[i] = cg.predictedPlayerState.clientNum;
			}
		}

		CG_SendPipWindowCommand();
	}

	trap_SendConsoleCommand( va("follow %d\n", clientNum) );
}

qboolean CG_PipAllowed( void ) {
	if ( !CG_IsSpectator() ) {
		return qfalse;
	}

	return qtrue;
}

void CG_RefreshPipWindows( void ) {
	int i;

	for ( i = 0; i < cg.numPipWindows; i++ ) {
		pipWindow_t *pip = &cg.pipWindows[cg.pipZOrder[i]];
		clientInfo_t *ci;
		centity_t *iter, *body;
		vec3_t angles;
		int health, armor;

		ci = &cgs.clientinfo[pip->clientNum];
		if ( !ci->infoValid ) {
			CG_RequestPipWindowDown( pip->clientNum );
			i--;
			continue;
		}

		// look for a frozen body
		body = NULL;
		for ( iter = &cg_entities[MAX_CLIENTS]; iter < &cg_entities[ENTITYNUM_MAX_NORMAL]; iter++ ) {
			if ( !iter->currentValid ) continue;

			if ( iter->currentState.eType == ET_PLAYER && iter->currentState.clientNum == pip->clientNum &&
					Q_IsFrozenBody(iter - cg_entities) ) {
				body = iter;
				break;
			}
		}

		// get the origin and angles

		// if no frozen body
		if ( body == NULL ) {
			pip->frozen = qfalse;

			// if the client number is the same as the predicted client's
			if ( cg.predictedPlayerState.clientNum == pip->clientNum ) {
				// just grab the origin and angles from the player state
				VectorCopy( cg.predictedPlayerState.origin, pip->origin );
				VectorCopy( cg.predictedPlayerState.viewangles, angles );
			}
			else {
				// get the interpolated origin and angles from the client entity
				centity_t *cent = &cg_entities[pip->clientNum];
				if ( !cent->currentValid ) {
					// don't go on if there's no data
					pip->haveData = qfalse;
					continue;
				}

				VectorCopy( cent->lerpOrigin, pip->origin );
				VectorCopy( cent->lerpAngles, angles );
			}
		}
		else {
			pip->frozen = qtrue;

			// grab the body's origin and angles, and force the pitch
			VectorCopy( body->lerpOrigin, pip->origin );
			VectorCopy( body->lerpAngles, angles );
			angles[PITCH] = 25;
		}

		// if the client number is the same as the predicted client's
		if ( cg.predictedPlayerState.clientNum == pip->clientNum ) {
			// grab the data from the player state
			pip->viewheight = cg.predictedPlayerState.viewheight;
			pip->deadyaw = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
			health = cg.predictedPlayerState.stats[STAT_HEALTH];
			armor = cg.predictedPlayerState.stats[STAT_ARMOR];
			pip->weapon = cg.predictedPlayerState.weapon;
			pip->ammo = cg.predictedPlayerState.ammo[pip->weapon];
			memcpy( pip->powerups, cg.predictedPlayerState.powerups, sizeof(pip->powerups) );
		}
		else {
			// otherwise, grab it from the data we shoved across the wire
			pipPlayerState_t *pps;

			pps = &cgs.pipPlayerStates[pip->clientNum];
			// if there's nothing yet
			if ( !pps->valid ) {
				// don't go on
				pip->haveData = qfalse;
				continue;			
			}

			// get it
			pip->viewheight = pps->viewheight;
			pip->deadyaw = pps->deadyaw;
			health = pps->health;
			armor = pps->armor;
			pip->weapon = pps->weapon;
			pip->ammo = pps->ammo;
			memcpy( pip->powerups, pps->powerups, sizeof(pip->powerups) );
		}

		// force a view height if frozen
		if ( pip->frozen ) {
			pip->viewheight = DEFAULT_VIEWHEIGHT;
		}

		// mark this window as having data
		pip->haveData = qtrue;

		// get the forward vector from the angles
		AngleVectors( angles, pip->forward, NULL, NULL );

		// fix up the health and armor
		if ( health < 0 ) health = 0;
		if ( armor < 0 ) armor = 0;
		pip->health = health;
		pip->armor = armor;

		// copy the name
		Q_strncpyz( pip->name, ci->name, MAX_QPATH );
		pip->team = ci->team;
	}
}

#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonPipView( refdef_t *refdef, vec3_t angles, pipWindow_t *pip ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -8, -8, -8 };
	static vec3_t	maxs = { 8, 8, 8 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;
	float height, range;

	if ( pip->frozen ) {
		height = 25;
		range = 40;
	}
	else {
		height = cg_thirdPersonHeight.value;
		range = cg_thirdPersonRange.value;
	}

	refdef->vieworg[2] += pip->viewheight;

	VectorCopy( angles, focusAngles );

	// if dead, look at killer
	if ( pip->health <= 0 && !pip->frozen ) {
		focusAngles[YAW] = pip->deadyaw;
		angles[YAW] = pip->deadyaw;
	}

	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( refdef->vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( refdef->vieworg, view );

	view[2] += 8;

	angles[PITCH] *= 0.5;

	AngleVectors( angles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -range * forwardScale, forward, view );
	VectorMA( view, -range * sideScale, right, view );

	view[2] += height;

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
	CG_Trace( &trace, refdef->vieworg, mins, maxs, view, pip->clientNum, MASK_SOLID );

	if ( trace.fraction != 1.0 ) {
		VectorCopy( trace.endpos, view );
		view[2] += (1.0 - trace.fraction) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace( &trace, refdef->vieworg, mins, maxs, view, pip->clientNum, MASK_SOLID );
		VectorCopy( trace.endpos, view );
	}


	VectorCopy( view, refdef->vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, refdef->vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	angles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	angles[YAW] -= cg_thirdPersonAngle.value;
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonPipView( refdef_t *refdef, vec3_t angles, pipWindow_t *pip ) {
	float *origin;
	
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = refdef->vieworg;

	// if dead, fix the angle and don't add any kick
	if ( pip->health <= 0 && !pip->frozen ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = pip->deadyaw;
		origin[2] += pip->viewheight;
		return;
	}

	// add view height
	origin[2] += pip->viewheight;
}

static int CG_CalcPipViewValues( refdef_t *refdef, pipWindow_t *pip ) {
	vec3_t angles;

	// intermission view
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		VectorCopy( pip->origin, refdef->vieworg );
		vectoangles( pip->forward, angles );
		AnglesToAxis( angles, refdef->viewaxis );
		return CG_CalcFov( refdef, qfalse );
	}

	// set the initial view origin
	VectorCopy( pip->origin, refdef->vieworg );
	vectoangles( pip->forward, angles );
	if ( angles[PITCH] < -180 ) {
		angles[PITCH] += 360;
	}

	pip->renderingThirdPerson = (cg_thirdPerson.integer || pip->health <= 0 || pip->frozen);

	if ( pip->renderingThirdPerson ) {
		// back away from character
		CG_OffsetThirdPersonPipView( refdef, angles, pip );
	} else {
		CG_OffsetFirstPersonPipView( refdef, angles, pip );
	}

	AnglesToAxis( angles, refdef->viewaxis );

	// field of view
	return CG_CalcFov( refdef, qfalse );
}

void CG_DrawBorderRaw( int x, int y, int width, int height, int thickness, vec4_t color ) {
	trap_R_SetColor( color );

	// top line
	trap_R_DrawStretchPic( x, y, width, thickness, 0, 0, 0, 0, cgs.media.whiteShader );
	// bottom line
	trap_R_DrawStretchPic( x, y + height - thickness, width, thickness, 0, 0, 0, 0, cgs.media.whiteShader );
	// left line
	trap_R_DrawStretchPic( x, y + thickness, thickness, height - thickness * 2, 0, 0, 0, 0, cgs.media.whiteShader );
	// right line
	trap_R_DrawStretchPic( x + width - thickness, y + thickness, thickness, height - thickness * 2, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

static void CG_AdjustPipFrom640( pipWindow_t *pip, float *x1, float *y1, float *x2, float *y2 ) {
	// scale for screen sizes
	*x1 = floor( pip->x1 * cgs.screenXScale );
	*y1 = floor( pip->y1 * cgs.screenYScale );
	*x2 = floor( pip->x2 * cgs.screenXScale );
	*y2 = floor( pip->y2 * cgs.screenYScale );
}

static void CG_DrawPipString( pipWindow_t *pip, int x, int y, int width, int height, const char *s, vec_t *color ) {
	float w, h;
	w = pip->x2 - pip->x1;
	h = pip->y2 - pip->y1;
	
	CG_DrawStringExt( pip->x1 + x * w / 640, pip->y1 + y * h / 480, s, color, qtrue, qfalse, width * w / 640, height * h / 480, 0 );
}

#define PIPCONTROL_SIZE 16
#define PIPPADDING_SIZE 4
#define PIPBORDER_SIZE 4

static void CG_DrawPipWindowElements( pipWindow_t *pip, int zorder ) {
	vec4_t wincolor = { 0.7f, 0.8f, 1.0f, 1.0f };
	vec4_t highlight = { 1.0f, 1.0f, 1.0f, 1.0f };
	float x1, y1, x2, y2, w, h;

	// adjust the window corners for the current screen
	CG_AdjustPipFrom640( pip, &x1, &y1, &x2, &y2 );
	w = x2 - x1;
	h = y2 - y1;

	//
	// window border
	//

	// if it's the current window
	if ( cg.curPipWindow == cg.pipZOrder[zorder] ) {
		// give it a thick, highlighted border
		CG_DrawBorderRaw( x1 - 1, y1 - 1, w + 2, h + 2, 2, highlight );
	}
	else {
		// otherwise, give it a thin, dark border
		CG_DrawBorderRaw( x1 - 1, y1 - 1, w + 2, h + 2, 1, wincolor );
	}

	//
	// close button
	//

	// if it's the current window and the manipulation is a CLOSE type
	if ( cg.curPipWindow == cg.pipZOrder[zorder] && (cg.pipManipType == MANIP_HOVERCLOSE || cg.pipManipType == MANIP_CLOSE) ) {
		// give it a highlighted color
		trap_R_SetColor( highlight );
	}
	else {
		// otherwise, give it a dark color
		trap_R_SetColor( wincolor );
	}

	// draw it
	trap_R_DrawStretchPic( x2 - PIPCONTROL_SIZE - PIPPADDING_SIZE, y1 + PIPPADDING_SIZE, PIPCONTROL_SIZE, PIPCONTROL_SIZE, 0, 0, 1, 1, cgs.media.pipWindowXShader );

	//
	// maximize button
	//

	// if it's the current window and the manipulation is a MAX type
	if ( cg.curPipWindow == cg.pipZOrder[zorder] && (cg.pipManipType == MANIP_HOVERMAX || cg.pipManipType == MANIP_MAX) ) {
		// give it a highlighted color
		trap_R_SetColor( highlight );
	}
	else {
		// otherwise, give it a dark color
		trap_R_SetColor( wincolor );
	}

	// draw it
	trap_R_DrawStretchPic( x2 - (PIPCONTROL_SIZE + PIPPADDING_SIZE) * 2, y1 + PIPPADDING_SIZE, PIPCONTROL_SIZE, PIPCONTROL_SIZE, 0, 0, 1, 1, cgs.media.pipWindowMaxShader );
}

static void CG_DrawPipName( pipWindow_t *pip ) {
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

	if ( pip->name[0] != '\0' ) {
		int x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(pip->name));
		CG_DrawPipString( pip, x, 24, GIANT_WIDTH, GIANT_HEIGHT, pip->name, color );
	}
}

static void CG_DrawPipField( pipWindow_t *pip, int x, int y, int width, int value ) {
	char	num[16], *ptr;
	int		l;
	int		frame;
	float	w, h;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	w = pip->x2 - pip->x1;
	h = pip->y2 - pip->y1;

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH * (width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( pip->x1 + x * w / 640, pip->y1 + y * h / 480, CHAR_WIDTH * w / 640, CHAR_HEIGHT * h / 480, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

static void CG_DrawPip3DModelRGBA( pipWindow_t *pip, float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, byte shaderRGBA[4] ) {
	float pw, ph;
	pw = pip->x2 - pip->x1;
	ph = pip->y2 - pip->y1;

	CG_Draw3DModelRGBA( pip->x1 + x * pw / 640, pip->y1 + y * ph / 480, w * pw / 640, h * ph / 480, model, skin, origin, angles, shaderRGBA );
}

static void CG_DrawPip3DModel( pipWindow_t *pip, float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	byte shaderRGBA[4] = { 255, 255, 255, 255 };
	CG_DrawPip3DModelRGBA( pip, x, y, w, h, model, skin, origin, angles, shaderRGBA );
}

static void CG_DrawPipPic( pipWindow_t *pip, float x, float y, float width, float height, qhandle_t hShader ) {
	float pw, ph;
	pw = pip->x2 - pip->x1;
	ph = pip->y2 - pip->y1;

	CG_DrawPic( pip->x1 + x * pw / 640, pip->y1 + y * ph / 480, width * pw / 640, height * ph / 480, hShader );
}

static void CG_DrawPipHead( pipWindow_t *pip, float x, float y, float w, float h, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;
	byte			shaderRGBA[4];

	ci = &cgs.clientinfo[pip->clientNum];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

//pmskins
		if ( cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_SPECTATOR ) {
			if ( (ci->team == TEAM_BLUE && !cg_swapSkins.integer) ||
					(ci->team == TEAM_RED && cg_swapSkins.integer) ) {
				shaderRGBA[0] = 0;
				shaderRGBA[1] = 0;
				shaderRGBA[2] = 255;
				shaderRGBA[3] = 255;
			}
			else {
				shaderRGBA[0] = 255;
				shaderRGBA[1] = 0;
				shaderRGBA[2] = 0;
				shaderRGBA[3] = 255;
			}
		}
		else if ( ci->team == TEAM_FREE || ci->team != cg.predictedPlayerState.stats[STAT_TEAM] ) {
			shaderRGBA[0] = cgs.enemyHeadColor[0] * 255;
			shaderRGBA[1] = cgs.enemyHeadColor[1] * 255;
			shaderRGBA[2] = cgs.enemyHeadColor[2] * 255;
			shaderRGBA[3] = 255;
		}
		else {
			shaderRGBA[0] = cgs.headColor[0] * 255;
			shaderRGBA[1] = cgs.headColor[1] * 255;
			shaderRGBA[2] = cgs.headColor[2] * 255;
			shaderRGBA[3] = 255;
		}
//pmskins
		CG_DrawPip3DModelRGBA( pip, x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, shaderRGBA );
	}
	else if ( cg_drawIcons.integer ) {
		CG_DrawPipPic( pip, x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPipPic( pip, x, y, w, h, cgs.media.deferShader );
	} else if ( Q_Isfreeze(pip->clientNum) ) {
		CG_DrawPipPic( pip, x, y, w, h, cgs.media.noammoShader );
	}
}

static void CG_DrawPipFlagModel( pipWindow_t *pip, float x, float y, float w, float h, int team ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( cg_draw3dIcons.integer ) {
		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_DrawPip3DModel( pip, x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		}
		else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		}
		else {
			return;
		}

		if (item) {
		  CG_DrawPipPic( pip, x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

static void CG_DrawPipStatusBarFlag( pipWindow_t *pip, float x, int team ) {
	CG_DrawPipFlagModel( pip, x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team );
}

static void CG_DrawPipTeamBackground( pipWindow_t *pip ) {
	vec4_t hcolor;
	float x1, y1, x2, y2, w, h;

	// adjust the window corners for the current screen
	CG_AdjustPipFrom640( pip, &x1, &y1, &x2, &y2 );
	w = x2 - x1;
	h = y2 - y1;

	hcolor[3] = 0.33f;
	if ( pip->team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( pip->team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}

	trap_R_SetColor( hcolor );
	trap_R_DrawStretchPic( x1 + 1, y1 + 420 * h / 480, w - 2, 60 * h / 480, 0, 0, 1, 1, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

static void CG_DrawPipStatusBar( pipWindow_t *pip ) {
	int color;
	int value;
	vec3_t origin;
	vec3_t angles;
	vec4_t hcolor;
	float size;
	static float colors[4][4] = { 
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( !cg_drawStatus.integer ) {
		return;
	}

	if ( pip->health <= 0 || pip->frozen ) {
		return;
	}

	CG_DrawPipTeamBackground( pip );

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( pip->weapon && cg_weapons[pip->weapon].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
		CG_DrawPip3DModel( pip, CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[pip->weapon].ammoModel, 0, origin, angles );
	}

	VectorClear( angles );
	angles[YAW] = 180 + 20 * sin(cg.time / 500.0f);
	angles[PITCH] = 0 + 10 * sin(cg.time / 973.0f);

	size = ICON_SIZE * 1.25;
	CG_DrawPipHead( pip, 185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 480 - size, size, size, angles );

	if ( pip->powerups[PW_REDFLAG] ) {
		CG_DrawPipStatusBarFlag( pip, 185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	}
	else if ( pip->powerups[PW_BLUEFLAG] ) {
		CG_DrawPipStatusBarFlag( pip, 185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	}

	if ( pip->armor ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_DrawPip3DModel( pip, 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}

	//
	// ammo
	//
	if ( pip->weapon ) {
		value = pip->ammo;
		if ( value > -1 ) {
			if ( value >= 0 ) {
				color = 0;	// green
			} else {
				color = 1;	// red
			}

			trap_R_SetColor( colors[color] );
			CG_DrawPipField (pip, 0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[pip->weapon].ammoIcon;
				if ( icon ) {
					CG_DrawPipPic( pip, CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = pip->health;
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawPipField ( pip, 185, 432, 3, value);
	CG_GetColorForHealth( pip->health, pip->armor, hcolor );
	trap_R_SetColor( hcolor );

	//
	// armor
	//
	value = pip->armor;
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawPipField (pip, 370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPipPic( pip, 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
}

static void CG_DrawPipPowerups( pipWindow_t *pip ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
		{ 0.2f, 1.0f, 0.2f, 1.0f },
		{ 1.0f, 0.2f, 0.2f, 1.0f } 
	};
	float y = 480 - ICON_SIZE;

	if ( pip->health <= 0 ) {
		return;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < PW_NUM_POWERUPS ; i++ ) {
		if ( !pip->powerups[i] ) {
			continue;
		}
		t = pip->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if (item) {
			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor( colors[color] );
			CG_DrawPipField( pip, x, y, 2, sortedTime[ i ] / 1000 );

			t = pip->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap_R_SetColor( NULL );
			} else {
				vec4_t	modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor( modulate );
			}

			if ( cg.powerupActive == sorted[i] && 
					cg.time - cg.powerupTime < PULSE_TIME ) {
				f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
				size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
			} else {
				size = ICON_SIZE;
			}

			CG_DrawPipPic( pip, 640 - size, y + ICON_SIZE / 2 - size / 2, 
					size, size, trap_R_RegisterShader( item->icon ) );
		}
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawPip2D( pipWindow_t *pip, int zorder ) {
	// draw the border and buttons
	CG_DrawPipWindowElements( pip, zorder );

	if ( !cg_draw2D.integer ) {
		return;
	}

	// don't draw any other 2D elements if we have no data
	if ( !pip->haveData ) {
		return;
	}

	// draw the name of the player in PIP
	CG_DrawPipName( pip );
	// draw the status bar (health, armor, ammo)
	CG_DrawPipStatusBar( pip );
	// draw powerups (+ counters)
	CG_DrawPipPowerups( pip );
	// just in case
	trap_R_SetColor( NULL );
}

static void CG_DrawPip3D( pipWindow_t *pip ) {
	vec4_t nodatacolor = { 0.1f, 0.2f, 0.5f, 1.0f };
	refdef_t refdef;
	float x1, y1, x2, y2, w, h;

	// adjust the window corners for the current screen
	CG_AdjustPipFrom640( pip, &x1, &y1, &x2, &y2 );
	w = x2 - x1;
	h = y2 - y1;

	memset( &refdef, 0, sizeof(refdef_t) );

	// calculate size of 3D view
	refdef.x = x1;
	refdef.y = y1;
	refdef.width = w;
	refdef.height = h;

	// do third-person stuff, etc.
	CG_CalcPipViewValues( &refdef, pip );

	// set time for shaders, etc.
	refdef.time = cg.time;
	// set which areas should be drawn
	memcpy( refdef.areamask, cg.snap->areamask, sizeof(refdef.areamask) );

	// if there's current data
	if ( pip->haveData ) {
		// clear the scene
		trap_R_ClearScene();
		// re-add all the ref entities for this view
		CG_AddAllRefEntitiesFor( pip->clientNum, &refdef, qfalse, pip->renderingThirdPerson );
		// render it
		trap_R_RenderScene( &refdef );		
	}
	else {
		// otherwise, draw a blank screen
		trap_R_SetColor( nodatacolor );
		trap_R_DrawStretchPic( x1, y1, w, h, 0, 0, 0, 0, cgs.media.whiteShader );
	}
}

void CG_DrawPipWindows( void ) {
	int i;

	if ( !CG_PipAllowed() ) {
		return;
	}

	for ( i = 0; i < cg.numPipWindows; i++ ) {
		pipWindow_t *pip;

		pip = &cg.pipWindows[cg.pipZOrder[i]];

		if ( !pip->valid ) {
			continue;
		}

		CG_DrawPip3D( pip );
		CG_DrawPip2D( pip, i );
	}
}

void CG_PipMouseEvent( float x, float y ) {
	float deltax, deltay;

	cgs.cursorX = Com_Clamp( 0, 640, cgs.cursorX + x );
	cgs.cursorY = Com_Clamp( 0, 480, cgs.cursorY + y );
	deltax = cgs.cursorX - cg.pipDownX;
	deltay = cgs.cursorY - cg.pipDownY;

	if ( cg.curPipWindow >= 0 ) {
		pipWindow_t *pip = &cg.pipWindows[cg.curPipWindow];

		if ( cg.pipManipType == MANIP_MOVE ) {
			pip->x1 = pip->downx1 + deltax;
			pip->y1 = pip->downy1 + deltay;
			pip->x2 = pip->downx2 + deltax;
			pip->y2 = pip->downy2 + deltay;
		}

		if ( cg.pipManipType == MANIP_RESIZEW ||
				cg.pipManipType == MANIP_RESIZESW ||
				cg.pipManipType == MANIP_RESIZENW ) {
			pip->x1 = pip->downx1 + deltax;
			if ( pip->x2 - pip->x1 < PIPCONTROL_SIZE * 3 ) {
				pip->x1 = pip->x2 - PIPCONTROL_SIZE * 3;
			}
		}

		if ( cg.pipManipType == MANIP_RESIZEN ||
				cg.pipManipType == MANIP_RESIZENW ||
				cg.pipManipType == MANIP_RESIZENE ) {
			pip->y1 = pip->downy1 + deltay;
			if ( pip->y2 - pip->y1 < PIPCONTROL_SIZE * 2 ) {
				pip->y1 = pip->y2 - PIPCONTROL_SIZE * 2;
			}
		}

		if ( cg.pipManipType == MANIP_RESIZEE ||
				cg.pipManipType == MANIP_RESIZESE ||
				cg.pipManipType == MANIP_RESIZENE ) {
			pip->x2 = pip->downx2 + deltax;
			if ( pip->x2 - pip->x1 < PIPCONTROL_SIZE * 3 ) {
				pip->x2 = pip->x1 + PIPCONTROL_SIZE * 3;
			}
		}

		if ( cg.pipManipType == MANIP_RESIZES ||
				cg.pipManipType == MANIP_RESIZESW ||
				cg.pipManipType == MANIP_RESIZESE ) {
			pip->y2 = pip->downy2 + deltay;
			if ( pip->y2 - pip->y1 < PIPCONTROL_SIZE * 2 ) {
				pip->y2 = pip->y1 + PIPCONTROL_SIZE * 2;
			}
		}
	}
}

void CG_DrawPipCrosshair( void ) {
	float		w, h;
	float		x, y;
	qhandle_t	cursor;

	trap_R_SetColor( NULL );

	w = h = 32;
	x = cgs.cursorX - 320;
	y = cgs.cursorY - 240;
	CG_AdjustFrom640( &x, &y, &w, &h );

	if ( cg.curPipWindow >= 0 ) {
		switch ( cg.pipManipType ) {
			case MANIP_HOVERMOVE:
			case MANIP_MOVE:
				cursor = cgs.media.spectatorMoveShader;
				break;
			case MANIP_HOVERRESIZEN:
			case MANIP_RESIZEN:
			case MANIP_HOVERRESIZES:
			case MANIP_RESIZES:
				cursor = cgs.media.spectatorResizeNSShader;
				break;
			case MANIP_HOVERRESIZEE:
			case MANIP_RESIZEE:
			case MANIP_HOVERRESIZEW:
			case MANIP_RESIZEW:
				cursor = cgs.media.spectatorResizeEWShader;
				break;
			case MANIP_HOVERRESIZENW:
			case MANIP_RESIZENW:
			case MANIP_HOVERRESIZESE:
			case MANIP_RESIZESE:
				cursor = cgs.media.spectatorResizeNWSEShader;
				break;
			case MANIP_HOVERRESIZENE:
			case MANIP_RESIZENE:
			case MANIP_HOVERRESIZESW:
			case MANIP_RESIZESW:
				cursor = cgs.media.spectatorResizeNESWShader;
				break;
			case MANIP_HOVERCLOSE:
			case MANIP_CLOSE:
				cursor = cgs.media.spectatorPointerXShader;
				break;
			case MANIP_HOVERMAX:
			case MANIP_MAX:
				cursor = cgs.media.spectatorPointerMaxShader;
				break;
			default:
				cursor = cgs.media.spectatorPointerShader;
				break;
		}
	}
	else {
		cursor = cgs.media.spectatorPointerShader;
	}

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, cursor );
}

qboolean CG_InsertPipWindow( int zorder, int x, int y, int w, int h, int clientNum ) {
	int i, window;
	pipWindow_t *pip;

	if ( cg.numPipWindows >= MAX_PIP_WINDOWS ) {
		return qfalse;
	}

	if ( zorder > cg.numPipWindows ) {
		zorder = cg.numPipWindows;
	}

	if ( zorder < 0 ) {
		zorder = 0;
	}

	// find a free window slot
	for ( window = 0; window < MAX_PIP_WINDOWS; window++ ) {
		if ( !cg.pipWindows[window].valid ) break;
	}

	if ( window == MAX_PIP_WINDOWS ) {
		// shouldn't happen
		return qfalse;
	}

	// shift the windows up
	for ( i = MAX_PIP_WINDOWS - 1; i > zorder; i-- ) {
		cg.pipZOrder[i] = cg.pipZOrder[i - 1];
	}

	pip = &cg.pipWindows[window];
	pip->valid = qtrue;
	pip->x1 = x;
	pip->y1 = y;
	pip->x2 = x + w;
	pip->y2 = y + h;
	pip->clientNum = clientNum;

	cg.pipZOrder[zorder] = window;
	cg.numPipWindows++;

	return qtrue;
}

qboolean CG_RemovePipWindow( int clientNum ) {
	pipWindow_t *pip;
	int i;

	if ( cg.numPipWindows <= 0 ) {
		return qfalse;
	}

	for ( i = 0; i < cg.numPipWindows; i++ ) {
		pip = &cg.pipWindows[cg.pipZOrder[i]];

		if ( pip->clientNum == clientNum ) break;
	}

	if ( i == cg.numPipWindows ) {
		return qfalse;
	}

	if ( cg.curPipWindow == cg.pipZOrder[i] ) {
		cg.curPipWindow = -1;
	}

	pip->valid = qfalse;

	for ( ; i < MAX_PIP_WINDOWS - 1; i++ ) {
		cg.pipZOrder[i] = cg.pipZOrder[i + 1];
	}

	cg.numPipWindows--;

	return qtrue;
}

void CG_RemoveAllPipWindows( void ) {
	memset( cg.pipWindows, 0, sizeof(cg.pipWindows) );
	memset( cg.pipZOrder, 0, sizeof(cg.pipZOrder) );
	cg.numPipWindows = 0;
	cg.pipManageMode = qfalse;
	cg.curPipWindow = 0;
}

void CG_BringPipToFront( int zorder ) {
	int i;
	int temp = cg.pipZOrder[zorder];

	for ( i = zorder; i < cg.numPipWindows - 1; i++ ) {
		cg.pipZOrder[i] = cg.pipZOrder[i + 1];
	}

	cg.pipZOrder[i] = temp;
}

void CG_ManagePipWindows( void ) {
	usercmd_t latestCmd, lastCmd;
	int current, i;
	qboolean latestValid, lastValid, buttonDown, lastButtonDown;
	float mx, my, mw, mh;
	pipWindow_t *pip = NULL;
	int curPipWindow, pipManipType;

	if ( !CG_PipAllowed() ) {
		cg.pipManageMode = qfalse;
		return;
	}

	if ( !cg.pipManageMode ) {
		return;
	}

	if ( cg.pointClickScores ) {
		return;
	}

	current = trap_GetCurrentCmdNumber();
	latestValid = trap_GetUserCmd( current, &latestCmd );
	lastValid = trap_GetUserCmd( current - 1, &lastCmd );

	buttonDown = latestValid && (latestCmd.buttons & BUTTON_ATTACK) != 0;
	lastButtonDown = lastValid && (lastCmd.buttons & BUTTON_ATTACK) != 0;

	// if the button is held, make no changes to the state
	if ( lastButtonDown && buttonDown ) {
		return;
	}

	curPipWindow = cg.curPipWindow;
	pipManipType = cg.pipManipType;

	// if a button isn't currently held down, reset
	if ( !buttonDown ) {
		cg.curPipWindow = -1;
		cg.pipManipType = MANIP_NONE;
	}

	mx = cgs.cursorX;
	my = cgs.cursorY;
	mw = mh = 0;
	CG_AdjustFrom640( &mx, &my, &mw, &mh );
	mx = floor( mx );
	my = floor( my );

	if ( buttonDown ) {
		cg.pipDownX = cgs.cursorX;
		cg.pipDownY = cgs.cursorY;
	}
	
	for ( i = cg.numPipWindows - 1; i >= 0; i-- ) {
		float x1, y1, x2, y2;

		pip = &cg.pipWindows[cg.pipZOrder[i]];

		x1 = pip->x1;
		y1 = pip->y1;
		x2 = pip->x2;
		y2 = pip->y2;
		CG_AdjustFrom640( &x1, &y1, &x2, &y2 );
		x1 = floor( x1 );
		y1 = floor( y1 );
		x2 = floor( x2 );
		y2 = floor( y2 );

		if ( mx >= x1 && mx < x2 && my >= y1 && my < y2 ) {
			cg.curPipWindow = cg.pipZOrder[i];
		}
		else {
			continue;
		}

		if ( mx >= x2 - PIPCONTROL_SIZE - PIPPADDING_SIZE &&
				mx < x2 - PIPPADDING_SIZE &&
				my >= y1 + PIPPADDING_SIZE &&
				my < y1 + PIPCONTROL_SIZE + PIPPADDING_SIZE ) {
			if ( buttonDown ) {
				cg.pipManipType = MANIP_CLOSE;
				CG_RequestPipWindowDown( pip->clientNum );
				trap_S_StartLocalSound( cgs.media.menu_out_sound, CHAN_AUTO );
			}
			else {
				cg.pipManipType = MANIP_HOVERCLOSE;
			}

			break;
		}

		if ( mx >= x2 - (PIPCONTROL_SIZE + PIPPADDING_SIZE) * 2 &&
				mx < x2 - PIPCONTROL_SIZE - PIPPADDING_SIZE * 2 &&
				my >= y1 + PIPPADDING_SIZE &&
				my < y1 + PIPCONTROL_SIZE + PIPPADDING_SIZE ) {
			if ( buttonDown ) {
				cg.pipManipType = MANIP_MAX;
				CG_RequestPipWindowMaximize( pip->clientNum );
				trap_S_StartLocalSound( cgs.media.menu_out_sound, CHAN_AUTO );
			}
			else {
				cg.pipManipType = MANIP_HOVERMAX;
			}

			break;
		}

		if ( (mx < x1 + PIPCONTROL_SIZE && my < y1 + PIPBORDER_SIZE) ||
				(mx < x1 + PIPBORDER_SIZE && my < y1 + PIPCONTROL_SIZE) ) {
			// northwest corner
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZENW;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZENW;
			}

			break;
		}

		if ( (mx >= x2 - PIPCONTROL_SIZE && my < y1 + PIPBORDER_SIZE) ||
				(mx >= x2 - PIPBORDER_SIZE && my < y1 + PIPCONTROL_SIZE) ) {
			// northeast corner
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZENE;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZENE;
			}

			break;
		}

		if ( (mx < x1 + PIPCONTROL_SIZE && my >= y2 - PIPBORDER_SIZE) ||
				(mx < x1 + PIPBORDER_SIZE && my >= y2 - PIPCONTROL_SIZE) ) {
			// southwest corner
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZESW;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZESW;
			}

			break;
		}

		if ( (mx >= x2 - PIPCONTROL_SIZE && my >= y2 - PIPBORDER_SIZE) ||
				(mx >= x2 - PIPBORDER_SIZE && my >= y2 - PIPCONTROL_SIZE) ) {
			// southeast corner
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZESE;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZESE;
			}

			break;
		}

		if ( my < y1 + PIPBORDER_SIZE ) {
			// north edge
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZEN;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZEN;
			}

			break;
		}

		if ( my >= y2 - PIPBORDER_SIZE ) {
			// south edge
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZES;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZES;
			}

			break;
		}

		if ( mx < x1 + PIPBORDER_SIZE ) {
			// west edge
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZEW;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZEW;
			}

			break;
		}

		if ( mx >= x2 - PIPBORDER_SIZE ) {
			// east edge
			if ( buttonDown ) {
				cg.pipManipType = MANIP_RESIZEE;
			}
			else {
				cg.pipManipType = MANIP_HOVERRESIZEE;
			}

			break;
		}

		if ( buttonDown ) {
			cg.pipManipType = MANIP_MOVE;
			CG_BringPipToFront( i );
		}
		else {
			cg.pipManipType = MANIP_HOVERMOVE;
		}

		break;
	}

	if ( buttonDown ) {
		if ( cg.pipManipType == MANIP_NONE ) {
			trap_SendConsoleCommand( "pipstop\n" );
		}
		else {
			pip->downx1 = pip->x1;
			pip->downy1 = pip->y1;
			pip->downx2 = pip->x2;
			pip->downy2 = pip->y2;
		}
	}

	if ( cg.curPipWindow >= 0 && cg.curPipWindow != curPipWindow ) {
		trap_S_StartLocalSound( cgs.media.menu_move_sound, CHAN_AUTO );
	}
	else if ( cg.pipManipType != pipManipType && (cg.pipManipType == MANIP_HOVERCLOSE || cg.pipManipType == MANIP_HOVERMAX) ) {
		trap_S_StartLocalSound( cgs.media.menu_move_sound, CHAN_AUTO );
	}
}

static int cg_refEntityNum;
refEntitySave_t cg_refEntities[MAX_REFENTITIES];

void CG_AddRefEntityToScene( refEntity_t *re, int clientNum, int refFlags, qboolean testPVS ) {
	refEntitySave_t *rs;

	// if no more save slots left OR can't do PIP OR have no PIP windows up
	if ( cg_refEntityNum >= MAX_REFENTITIES || !CG_PipAllowed() || cg.numPipWindows == 0 ) {
		// if it's only for third-person rendering
		if ( (refFlags & REF_DRAW_TYPES) == REF_THIRD_PERSON ) {
			// if the client is the game client AND we're not rendering in third person
			if ( clientNum == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson ) {
				// add it with RF_THIRD_PERSON
				int oldfx = re->renderfx;
				re->renderfx |= RF_THIRD_PERSON;
				trap_R_AddRefEntityToScene( re );
				re->renderfx = oldfx;
			}
			else {
				// otherwise just add it straight
				trap_R_AddRefEntityToScene( re );
			}
		}
		// if it's only for first-person rendering
		else if ( (refFlags & REF_DRAW_TYPES) == REF_FIRST_PERSON ) {
			// if the client is the game client AND we're not rendering in third person
			if ( clientNum == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson ) {
				// add it with RF_FIRST_PERSON
				int oldfx = re->renderfx;
				re->renderfx |= RF_FIRST_PERSON;
				trap_R_AddRefEntityToScene( re );
				re->renderfx = oldfx;
			}
			else {
				// otherwise do nothing - it's superfluous in this mode
			}
		}
		else {
			// just add it straight
			trap_R_AddRefEntityToScene( re );
		}

		// don't add it to the list
		return;
	}

	// get a pointer to the next available slot
	rs = &cg_refEntities[cg_refEntityNum++];

	// set the data
	memcpy( &rs->re, re, sizeof(refEntity_t) );
	rs->clientNum = clientNum;
	rs->refFlags = refFlags;
	rs->testPVS = testPVS;
}

void CG_InitializeRefEntities( void ) {
	memset( cg_refEntities, 0, sizeof(cg_refEntities) );
	CG_ResetRefEntities();
}

void CG_ResetRefEntities( void ) {
	cg_refEntityNum = 0;
}

void CG_AddAllRefEntitiesFor( int clientNum, refdef_t *refdef, qboolean mainView, qboolean thirdPerson ) {
	int i;
	for ( i = 0; i < cg_refEntityNum; i++ ) {
		refEntitySave_t *rs = &cg_refEntities[i];

		if ( (rs->refFlags & REF_NOT_PIP) && !mainView ) {
			continue;
		}

		if ( (rs->refFlags & REF_NOT_MAIN) && mainView ) {
			continue;
		}

		if ( rs->testPVS && !trap_R_inPVS( refdef->vieworg, rs->re.origin ) ) {
			continue;
		}

		switch ( rs->refFlags & REF_DRAW_TYPES ) {
		case REF_ALWAYS:
			trap_R_AddRefEntityToScene( &rs->re );
			break;

		case REF_ONLY_CLIENT:
			if ( clientNum == rs->clientNum ) {
				trap_R_AddRefEntityToScene( &rs->re );
			}
			break;

		case REF_THIRD_PERSON:
			if ( clientNum == rs->clientNum && !thirdPerson ) {
				// set the RF_THIRD_PERSON flag
				int oldfx = rs->re.renderfx;
				rs->re.renderfx |= RF_THIRD_PERSON;
				trap_R_AddRefEntityToScene( &rs->re );
				rs->re.renderfx = oldfx;
			}
			else {
				trap_R_AddRefEntityToScene( &rs->re );
			}
			break;

		case REF_FIRST_PERSON:
			if ( clientNum == rs->clientNum && !thirdPerson ) {
				// set the RF_FIRST_PERSON flag
				int oldfx = rs->re.renderfx;
				rs->re.renderfx |= RF_FIRST_PERSON;
				trap_R_AddRefEntityToScene( &rs->re );
				rs->re.renderfx = oldfx;
			}
		}
	}
}

/*
// in case we ever do MVD
static void CG_UnserializePlayerData( playerData_t *pd, char *data ) {
	void *curr = data;
	int pack;

	// always change
	UNSERIALIZE_INT(pd->commandTime, curr);

	// high degree of change
	UNSERIALIZE_VEC3S(pd->origin, curr);
	UNSERIALIZE_VEC3S(pd->velocity, curr);
	UNSERIALIZE_VEC3S(pd->viewangles, curr);
	UNSERIALIZE_CHAR(pd->bobCycle, curr);
	UNSERIALIZE_SHORT(pd->weaponTime, curr);
	//UNSERIALIZE_CHAR(pd->pmove_framecount, curr); // not needed?

	// medium-high degree of change
	UNSERIALIZE_VEC3S(pd->grapplePoint, curr);
	UNSERIALIZE_INT(pd->eventSequence, curr);

	UNSERIALIZE_CHARARRAY((char *)&pack, 3, curr);
	pd->events[0] = pack & 1023;
	pd->events[1] = (pack >> 10) & 1023;

	UNSERIALIZE_CHAR(pd->eventParms[0], curr);
	UNSERIALIZE_CHAR(pd->eventParms[1], curr);
	UNSERIALIZE_INT(pd->entityEventSequence, curr);
	UNSERIALIZE_SHORTARRAY(pd->ammo, WP_NUM_WEAPONS - 1, curr);
	UNSERIALIZE_CHAR(pd->movementDir, curr);

	// medium degree of change
	UNSERIALIZE_CHAR(pd->legsAnim, curr);
	UNSERIALIZE_CHAR(pd->torsoAnim, curr);
	UNSERIALIZE_SHORT(pd->eFlags, curr);
	UNSERIALIZE_SHORT(pd->pm_time, curr);
	UNSERIALIZE_SHORT(pd->pm_flags, curr);
	UNSERIALIZE_SHORT(pd->grappleTime, curr);

	UNSERIALIZE_SHORT(pack, curr);
	pd->grappleEntity = pack & 1023;
	pd->weapon = (pack >> 10) & 15;
	pd->weaponstate = (pack >> 14) & 3;

	UNSERIALIZE_CHAR(pd->damageEvent, curr);
	UNSERIALIZE_CHAR(pd->damageYaw, curr);
	UNSERIALIZE_CHAR(pd->damagePitch, curr);
	UNSERIALIZE_CHAR(pd->damageCount, curr);
	UNSERIALIZE_SHORTARRAY(pd->stats, STAT_NUM_SSDEMO_STATS, curr);
	UNSERIALIZE_SHORTARRAY(pd->persistant, PERS_NUM_PERSISTANT, curr);
	UNSERIALIZE_VEC3S(pd->grappleVelocity, curr);

	// medium-low degree of change
	UNSERIALIZE_SHORT(pd->groundEntityNum, curr);
	UNSERIALIZE_SHORTARRAY(pd->delta_angles, 3, curr);
	UNSERIALIZE_SHORT(pd->legsTimer, curr);
	UNSERIALIZE_SHORT(pd->torsoTimer, curr);
	UNSERIALIZE_SHORT(pd->externalEvent, curr);
	UNSERIALIZE_CHAR(pd->externalEventParm, curr);
	UNSERIALIZE_INT(pd->externalEventTime, curr);
	//UNSERIALIZE_CHAR(pd->jumppad_frame, curr); // not needed?

	// low degree of change
	UNSERIALIZE_CHAR(pd->viewheight, curr);
	UNSERIALIZE_INTARRAY(pd->powerups, PW_NUM_POWERUPS - 1, curr);
	UNSERIALIZE_INT(pd->generic1, curr);
	UNSERIALIZE_SHORT(pd->loopSound, curr);

	UNSERIALIZE_SHORT(pack, curr);
	pd->jumppad_ent = pack & 1023;
	pd->pm_type = (pack >> 10) & 7;

	// almost never change
	UNSERIALIZE_CHAR(pd->clientNum, curr);
	UNSERIALIZE_SHORT(pd->gravity, curr);
	UNSERIALIZE_SHORT(pd->speed, curr);
}

static void CG_UnstuffPlayerData( playerState_t *ps, playerData_t *pd ) {
	int i;

	// always change
	ps->commandTime = pd->commandTime;

	// high degree of change
	ShortToVec3( pd->origin, ps->origin, 1.0f );
	ShortToVec3( pd->velocity, ps->velocity, 1.0f );
	ShortToVec3( pd->viewangles, ps->viewangles, 4.0f );
	ps->bobCycle = pd->bobCycle;
	ps->weaponTime = pd->weaponTime;

	// medium-high degree of change
	ps->movementDir = pd->movementDir;
	ShortToVec3( pd->grapplePoint, ps->grapplePoint, 1.0f );
	ps->eventSequence = pd->eventSequence;
	ps->events[0] = pd->events[0];
	ps->events[1] = pd->events[1];
	ps->eventParms[0] = pd->eventParms[0];
	ps->eventParms[1] = pd->eventParms[1];
	ps->entityEventSequence = pd->entityEventSequence;
	for ( i = 1; i < WP_NUM_WEAPONS; i++ ) {
		ps->ammo[i] = pd->ammo[i - 1];
	}
	
	// medium degree of change
	ps->legsAnim = pd->legsAnim;
	ps->torsoAnim = pd->torsoAnim;
	ps->eFlags = pd->eFlags;
	ps->pm_time = pd->pm_time;
	ps->pm_flags = pd->pm_flags;
	ps->weapon = pd->weapon;
	ps->weaponstate = pd->weaponstate;
	ps->damageEvent = pd->damageEvent;
	ps->damageYaw = pd->damageYaw;
	ps->damagePitch = pd->damagePitch;
	ps->damageCount = pd->damageCount;
	for ( i = 0; i < STAT_NUM_SSDEMO_STATS; i++ ) {
		ps->stats[i] = pd->stats[i];
	}
	for ( i = 0; i < PERS_NUM_PERSISTANT; i++ ) {
		ps->persistant[i] = pd->persistant[i];
	}
	ShortToVec3( pd->grappleVelocity, ps->grappleVelocity, 4.0f );
	ps->grappleTime = pd->grappleTime;
	ps->grappleEntity = pd->grappleEntity;

	// medium-low degree of change
	ps->groundEntityNum = pd->groundEntityNum;
	ps->delta_angles[0] = pd->delta_angles[0];
	ps->delta_angles[1] = pd->delta_angles[1];
	ps->delta_angles[2] = pd->delta_angles[2];
	ps->legsTimer = pd->legsTimer;
	ps->torsoTimer = pd->torsoTimer;
	ps->externalEvent = pd->externalEvent;
	ps->externalEventParm = pd->externalEventParm;
	ps->externalEventTime = pd->externalEventTime;

	// low degree of change
	ps->pm_type = pd->pm_type;
	ps->viewheight = pd->viewheight;
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		ps->powerups[i] = pd->powerups[i - 1];
	}
	ps->generic1 = pd->generic1;
	ps->loopSound = pd->loopSound;
	ps->jumppad_ent = pd->jumppad_ent;

	// almost never change
	ps->clientNum = pd->clientNum;
	ps->gravity = pd->gravity;
	ps->speed = pd->speed;
}

void CG_ParsePlayerSnapshot( const char *base64 ) {
	int base64Size = strlen(base64);
	char rle[2048];
	int rlesize = sizeof(rle);
	char data[2048];
	int datasize = sizeof(data);
	playerData_t pd;

	// base-64-decode the string
	if ( !BG_Base64_Decode( base64, base64Size, rle, &rlesize ) ) {
		CG_Error( "Base64 decoding error\n" );
	}

	// decompress it
	BG_RLE_Decompress( (unsigned char *)rle, rlesize, (unsigned char *)data, &datasize );
	if ( datasize == -1 ) {
		CG_Error( "RLE decompression error\n" );
	}

	// turn it into a playerData_t
	CG_UnserializePlayerData( &pd, data );

	// shift the old player state back
	memcpy( &cgs.oldPlayerStates[pd.clientNum], &cgs.playerStates[pd.clientNum], sizeof(playerState_t) );
	cgs.oldPlayerStatesValid[pd.clientNum] = cgs.playerStatesValid[pd.clientNum];

	// transform the playerData_t to playerState_t
	CG_UnstuffPlayerData( &cgs.playerStates[pd.clientNum], &pd );
	// mark it as valid
	cgs.playerStatesValid[pd.clientNum] = qtrue;
}
*/

static void CG_UnserializePipPlayerData( pipPlayerData_t *pd, int *clientNum, char *data, int *size ) {
	void *curr = data;
	int pack;

	// almost never change
	UNSERIALIZE_CHAR(*clientNum, curr);

	// medium-high degree of change
	UNSERIALIZE_SHORT(pd->ammo, curr);

	// medium degree of change
	UNSERIALIZE_CHARARRAY((char *)&pack, 3, curr);
	pd->weapon = pack & 15;
	pd->health = (pack >> 4) & 1023;
	pd->armor = (pack >> 14) & 1023;

	// low degree of change
	UNSERIALIZE_CHAR(pd->viewheight, curr);
	UNSERIALIZE_SHORT(pd->deadyaw, curr);
	UNSERIALIZE_INTARRAY(pd->powerups, PW_NUM_POWERUPS - 1, curr);

	*size = (char *)curr - data;
}

static void CG_UnstuffPipPlayerData( pipPlayerState_t *ps, pipPlayerData_t *pd ) {
	int i;

	// medium-high degree of change
	ps->ammo = pd->ammo;

	// medium degree of change
	ps->weapon = pd->weapon;
	ps->health = pd->health;
	ps->armor = pd->armor;

	// low degree of change
	ps->viewheight = pd->viewheight;
	ps->deadyaw = pd->deadyaw;
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		ps->powerups[i] = pd->powerups[i - 1];
	}

	ps->valid = qtrue;
}

static pipPlayerData_t g_pipPlayerData[MAX_CLIENTS];

void CG_ParsePipPlayerSnapshot( const char *base64 ) {
	int base64Size = strlen(base64);
	char rle[2048];
	int rlesize = sizeof(rle);
	char data[2048];
	int datasize = sizeof(data);
	pipPlayerData_t pd;
	int clientNum;
	int temp;

	// base-64-decode the string
	if ( !BG_Base64_Decode( base64, base64Size, rle, &rlesize ) ) {
		CG_Error( "Base64 decoding error\n" );
	}

	// decompress it
	BG_RLE_Decompress( (unsigned char *)rle, rlesize, (unsigned char *)data, &datasize );

	if ( datasize == -1 ) {
		CG_Error( "RLE decompression error\n" );
	}

	// turn it into a playerData_t
	CG_UnserializePipPlayerData( &pd, &clientNum, data, &temp );

	// copy for deltas later
	memcpy( &g_pipPlayerData[clientNum], &pd, sizeof(pipPlayerData_t) );

	// transform the playerData_t to playerState_t
	CG_UnstuffPipPlayerData( &cgs.pipPlayerStates[clientNum], &g_pipPlayerData[clientNum] );
}

/*
CG_ParsePipPlayerDelta

Parses a delta sent to the client with a 'pdd' command
*/
void CG_ParsePipPlayerDelta( const char *base64 ) {
	int base64Size = strlen(base64);
	char rle[2048];
	int rlesize = sizeof(rle);
	char data[2048];
	int datasize = sizeof(data);
	pipPlayerData_t pd;
	int clientNum;
	int size, totalsize = 0;

	//CG_Printf("size: %d\n", base64Size);

	// base-64-decode the string
	if ( !BG_Base64_Decode( base64, base64Size, rle, &rlesize ) ) {
		CG_Error( "Base64 decoding error\n" );
	}

	// decompress it
	BG_RLE_Decompress( (unsigned char *)rle, rlesize, (unsigned char *)data, &datasize );

	if ( datasize == -1 ) {
		CG_Error( "RLE decompression error\n" );
	}

	// while there's still stuff left on the stream
	while ( totalsize < datasize ) {
		// unserialize the next chunk into a playerData_t
		CG_UnserializePipPlayerData( &pd, &clientNum, &data[totalsize], &size );
		totalsize += size;

		// apply the delta
		BG_UndoXORDelta( (char *)&g_pipPlayerData[clientNum], (char *)&pd, sizeof(pipPlayerData_t) );

		// transform the playerData_t to playerState_t
		CG_UnstuffPipPlayerData( &cgs.pipPlayerStates[clientNum], &g_pipPlayerData[clientNum] );
	}
}

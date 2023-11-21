// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

//freeze
#define WARNING_TIME 15000
#define CHEATCHECK_TIME 5000
//freeze


#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
// FIXME: see ui_main.c, same problem
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
// TTimo: FIXME
//		const unsigned char *s = text;
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

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

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK

/*
================
CG_Draw3DModel, CG_Draw3DModelRGBA

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	byte shaderRGBA[4] = { 255, 255, 255, 255 };
	CG_Draw3DModelRGBA( x, y, w, h, model, skin, origin, angles, shaderRGBA );
}

void CG_Draw3DModelRGBA( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, byte shaderRGBA[4] ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	memcpy( ent.shaderRGBA, shaderRGBA, sizeof(byte) * 4 );

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
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles, qboolean scoreboard ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;
//freeze
	qboolean invis = (!cg_drawGun.integer && clientNum == cg.predictedPlayerState.clientNum &&
			cg.predictedPlayerState.powerups[PW_INVIS] && !cg.predictedPlayerState.powerups[PW_SIGHT] && !scoreboard);
//freeze

	ci = &cgs.clientinfo[ clientNum ];

//freeze
	//if ( cg_draw3dIcons.integer ) {
	if ( cg_draw3dIcons.integer || invis ) {
//freeze
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

//freeze
		if ( invis ) {
			CG_Draw3DModelInvis( x, y, w, h, ci->headModel, origin, headAngles, &cg.predictedPlayerEntity );
		}
		else {
//freeze
//pmskins
			byte shaderRGBA[4];

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
			CG_Draw3DModelRGBA( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, shaderRGBA );
		}
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
//freeze
	} else if ( Q_Isfreeze( clientNum ) ) {
		CG_DrawPic( x, y, w, h, cgs.media.noammoShader );
//freeze
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

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
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		}
		else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		}
//freeze
/*
		else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		}
*/
//freeze
		else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size, 
				cg.snap->ps.clientNum, angles, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawStatusBar

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
#ifdef MISSIONPACK
	qhandle_t	handle;
#endif
	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	}
	else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	}
//freeze
/*
	else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}
*/
//freeze

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
#ifdef MISSIONPACK
	if( cgs.gametype == GT_HARVESTER ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			handle = cgs.media.redCubeModel;
		} else {
			handle = cgs.media.blueCubeModel;
		}
		CG_Draw3DModel( 640 - (TEXT_ICON_SPACE + ICON_SIZE), 416, ICON_SIZE, ICON_SIZE, handle, 0, origin, angles );
	}
#endif
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
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
	CG_DrawField ( 185, 432, 3, value);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
#ifdef MISSIONPACK
	//
	// cubes
	//
	if( cgs.gametype == GT_HARVESTER ) {
		value = ps->generic1;
		if( value > 99 ) {
			value = 99;
		}
		trap_R_SetColor( colors[0] );
		CG_DrawField (640 - (CHAR_WIDTH*2 + TEXT_ICON_SPACE + ICON_SIZE), 432, 2, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeIcon;
			} else {
				handle = cgs.media.blueCubeIcon;
			}
			CG_DrawPic( 640 - (TEXT_ICON_SPACE + ICON_SIZE), 432, ICON_SIZE, ICON_SIZE, handle );
		}
	}
#endif
}
#endif

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size, y, size, size, clientNum, angles, qfalse );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPSs
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime - cg.totaltimeout;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

//freeze
/*
=================
CG_DrawSpeed
=================
*/
static float CG_DrawSpeed( float y ) {
	char		*s;
	int			w;

	if ( cg.predictedPlayerState.pm_flags & PMF_GRAPPLE_PULL ) {
		s = va( "%iups", (int)VectorLength(cg.predictedPlayerState.velocity));
	}
	else {
		s = va( "%iups", (int)cg.xyspeed );
	}

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawFrozen
=================
*/
static float CG_DrawFrozen( float y, clientInfo_t *ci) {
	char		*s;
	int			w;
	float			color[4];

	if ( ci->frozen >= 1000 ) {
//ssdemo
		//float tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
		//int thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);

		float tdiff;
		int thawed;

		if ( cg.ssDemoPaused ) {
			tdiff = (float)(cg.ssDemoPauseTime - ci->tinfolastvalid) / 1000.0f;
		}
		else {
			tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
		}

		thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);
//ssdemo

		
		if ( thawed > 99 ) {
			thawed = 99;
		}

		if ( cg.timein > cg.time ) {
			s = va( "%d%% Thawed", ci->frozen - 1000 );
		}
		else {
			s = va( "%d%% Thawed", thawed );
		}

		color[0] = 1.0f;
		color[1] = 0.25f;
		color[2] = 0.1f;
		color[3] = 1.0f;
	}
	else {
		s = va( "%d%% Frozen", ci->frozen );

		color[0] = 0.25f;
		color[1] = 0.5f;
		color[2] = 1.0f;
		color[3] = 1.0f;
	}

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 635 - w, y + 2, s, color);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimein
=================
*/
static void CG_DrawTimein( void ) {
	char		*s;
	int			w, cw;
	float		scale;
	int			diff = ( cg.timein + 999 - cg.time ) / 1000;

	if ( diff != cg.timeoutCount ) {
		cg.timeoutCount = diff;
		switch ( cg.timeoutCount ) {
		case 3:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 0:
			trap_S_StartLocalSound( cgs.media.timeoutSound, CHAN_ANNOUNCER );
		default:
			break;
		}
	}

	scale = 0.45f;
	cw = 16;

	switch ( cg.timeoutCount ) {
	case 0:
		s = "TIME IN!";
		cw = 24;
		scale = 0.51f;
		break;
	case 1:
		s = va( "Starts in: %d", diff );
		cw = 24;
		scale = 0.51f;
		break;
	case 2:
		s = va( "Starts in: %d", diff );
		cw = 20;
		scale = 0.48f;
		break;
	case 3:
		s = va( "Starts in: %d", diff );
		cw = 16;
		scale = 0.45f;
		break;
	default:
		s = va( "Time out: %d:%02d", diff / 60, diff % 60);
		cw = 16;
		scale = 0.45f;
		break;
	}

	w = CG_DrawStrlen( s );
	CG_DrawStringExt( 320 - w * cw/2, 320, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
}
//freeze

/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;
//freeze
	int team;
	int numleft = 0, lasty = 0, selfy;
//freeze

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

//freeze
//ssdemo
	if ( cg.ssDemoPlayback ) {
		// if in demo playback, use the team of the player
		// you're following
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}
	else {
//ssdemo
		team = cg.snap->ps.stats[STAT_TEAM];
	}
//freeze

	if ( team != TEAM_RED && team != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == team ) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
//freeze
		if ( ci->infoValid ) {
			if ( !ci->frozen ) {
				numleft++;
				lasty = i * TINYCHAR_HEIGHT;
			}

			if ( sortedTeamPlayers[i] == cg.snap->ps.clientNum ) {
				selfy = i * TINYCHAR_HEIGHT;
			}
		}

//freeze
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

//freeze
	selfy += y;
	lasty += y;
//freeze

	if ( team == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( team == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

//freeze
	hcolor[0] = 0.0f;
	hcolor[1] = 1.0f;
	hcolor[2] = 0.0f;
	hcolor[3] = 0.5f;
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, selfy, w, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	if ( numleft == 1 ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.5f;
		trap_R_SetColor( hcolor );
		CG_DrawPic( x, lasty, w, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );
	}
//freeze

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == team ) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
//freeze
				if ( ci->location == -1 ) {
					p = "waiting to play";
					hcolor[0] = 0.25f;
					hcolor[1] = 0.5f;
					hcolor[2] = 1.0f;
					hcolor[3] = 1.0f;
				}
				else {
//freeze
					p = CG_ConfigString(CS_LOCATIONS + ci->location);
				}
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

//freeze
			if ( ci->frozen ) {
				if ( ci->frozen < 1000 ) {
					Com_sprintf( st, sizeof(st), "  %2d%%", ci->frozen );
					hcolor[0] = 0.25f;
					hcolor[1] = 0.5f;
					hcolor[2] = 1.0f;
					hcolor[3] = 1.0f;
				}
				else {
//ssdemo
					//float tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
					//int thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);

					float tdiff;
					int thawed;

					if ( cg.ssDemoPaused ) {
						tdiff = (float)(cg.ssDemoPauseTime - ci->tinfolastvalid) / 1000.0f;
					}
					else {
						tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
					}

					thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);
//ssdemo
			
					if ( thawed > 99 ) {
						thawed = 99;
					}

					if ( cg.timein > cg.time ) {
						Com_sprintf( st, sizeof(st), "  %2d%%", ci->frozen - 1000 );
					}
					else {
						Com_sprintf( st, sizeof(st), "  %2d%%", thawed );
					}

					hcolor[0] = 1.0f;
					hcolor[1] = 0.25f;
					hcolor[2] = 0.1f;
					hcolor[3] = 1.0f;
				}
			}
			else {
//freeze
				CG_GetColorForHealth( ci->health, ci->armor, hcolor );
				Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
			}

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

//freeze
			if ( !ci->frozen ) {
//freeze
				if ( cg_weapons[ci->curWeapon].weaponIcon ) {
					CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						cg_weapons[ci->curWeapon].weaponIcon );
				} else {
					CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						cgs.media.deferShader );
				}
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
//freeze
				if ( cgs.huntMode == 2 && j == PW_INVIS ) {
					continue;
				}
//freeze

				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

//freeze
			if ( Q_Isfreeze( ci - cgs.clientinfo ) ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, cgs.media.noammoShader );
			}
//freeze

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;
//freeze
	clientInfo_t *ci;

//ssdemo
	// if playing a demo
	if ( cg.ssDemoPlayback ) { 
		// use the client info of the player being followed
		ci = &cgs.clientinfo[cg.predictedPlayerState.clientNum];
	}
	else {
//ssdemo
		ci = &cgs.clientinfo[cg.clientNum];
	}
//freeze

	y = 0;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
//freeze
	if ( cg_drawFrozen.integer && ci->frozen ) {
		y = CG_DrawFrozen( y, ci );
	}
//freeze
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
//freeze
	if ( cg_drawSpeed.integer ) {
		y = CG_DrawSpeed( y );
	}
//freeze
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			v;
	vec4_t		color;
	float		y1;
	gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
				}
			}
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.flagStatus >= 0 && cgs.flagStatus <= 3 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.flagShader[cgs.flagStatus] );
				}
			}
		}
#endif
/*freeze
		if ( cgs.gametype >= GT_CTF ) {
freeze*/
		if ( cgs.gametype >= GT_TEAM ) {
//freeze
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%2i", v );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	} else {
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}
#endif // MISSIONPACK

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < PW_NUM_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
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
		  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
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

		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2, 
			  size, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK


//freeze
/*
=====================
CG_DrawFractions

=====================
*/

static void CG_DrawFractionString( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

static float CG_DrawFractions( float y ) {
	int red, blue, redtotal, bluetotal, i;
	char s[MAX_STRING_CHARS];
	vec4_t cblue, cred;

	cblue[0] = 0.1f;
	cblue[1] = 0.2f;
	cblue[2] = 1.0f;
	cblue[3] = 0.5f;

	cred[0] = 1.0f;
	cred[1] = 0.2f;
	cred[2] = 0.1f;
	cred[3] = 0.5f;

	red = blue = redtotal = bluetotal = 0;

	for ( i = 0; i < cgs.maxclients; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[i];

		if ( !ci->infoValid ) {
			continue;
		}

		if ( ci->team == TEAM_RED ) {
			redtotal++;

			if ( !Q_Isfreeze(i) ) {
				red++;
			}
		}
		else if ( ci->team == TEAM_BLUE ) {
			bluetotal++;

			if ( !Q_Isfreeze(i) ) {
				blue++;
			}
		}
	}

	Com_sprintf( s, sizeof(s), "%2d/%2d", red, redtotal );
	CG_DrawFractionString( 640 - 10 * SMALLCHAR_WIDTH - 4, y - SMALLCHAR_HEIGHT, s, cred );

	Com_sprintf( s, sizeof(s), "%2d/%2d", blue, bluetotal );
	CG_DrawFractionString( 640 - 5 * SMALLCHAR_WIDTH, y - SMALLCHAR_HEIGHT, s, cblue );

	return y - SMALLCHAR_HEIGHT;
}
//freeze

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	y = CG_DrawScores( y );
//freeze
	y = CG_DrawFractions( y );
//freeze
	y = CG_DrawPowerups( y );
}
#endif // MISSIONPACK

/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 8, y, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			CG_DrawBigString( ICON_SIZE + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[ value ].pickup_name, fadeColor[0] );
			trap_R_SetColor( NULL );
		}
	}
	
	return y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerLeft

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	} 


	y = CG_DrawPickupItem( y );
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}

}
#endif // MISSIONPACK

#ifdef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#if 0 // sos001208 - DEAD
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK


/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

//unlagged - misc
	if ( !cg_lagometer.integer /* || cgs.localServer */) {
//unlagged - misc
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

/*
==============
CG_RefNotify

Called for important messages to the referee that should stay in the center of the screen
for a few moments
==============
*/
void CG_RefNotify( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.refNotify, str, sizeof(cg.refNotify) );

	cg.refNotifyTime = cg.time;
	cg.refNotifyY = y;
	cg.refNotifyCharWidth = charWidth;

	// count the number of lines for centering
	cg.refNotifyLines = 1;
	s = cg.refNotify;
	while( *s ) {
		if (*s == '\n')
			cg.refNotifyLines++;
		s++;
	}
}

static void CG_DrawHugeStringAt( char *start, int y, int charWidth, float *color ) {
	int		l;
	int		x, w;
#ifdef MISSIONPACK // bk010221 - unused else
  int h;
#endif

	trap_R_SetColor( color );

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = charWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
			charWidth, (int)(charWidth * 1.5), 0 );

		y += charWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		y;
	float	*color;
	int charWidth;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, cg_centerPrintTime.value * 1000 );

	if ( !color ) {
		return;
	}

	start = cg.centerPrint;
	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;
	charWidth = cg.centerPrintCharWidth;

	CG_DrawHugeStringAt( start, y, charWidth, color );
}

/*
===================
CG_DrawRefNotify
===================
*/
static void CG_DrawRefNotify( void ) {
	char	*start;
	int		y;
	float	*color;
	int charWidth;

	if ( !cg.refNotifyTime ) {
		return;
	}

	color = CG_FadeColor( cg.refNotifyTime, 8000 );

	if ( !color ) {
		return;
	}

	start = cg.refNotify;
	y = cg.refNotifyY - cg.refNotifyLines * BIGCHAR_HEIGHT / 2;
	charWidth = cg.refNotifyCharWidth;

	CG_DrawHugeStringAt( start, y, charWidth, color );
}


/*
================================================================================

CROSSHAIR

================================================================================
*/

//freeze
/*
=================
CG_DrawThirdPersonCrosshair
=================
*/
static void CG_DrawThirdPersonCrosshair(void) {
	trace_t trace;
	vec3_t muzzlePoint, forward, right, up, endPoint;
	refEntity_t re;
	int ca;
	qhandle_t hShader;
	vec3_t mins = { -1, -1, -1 }, maxs = {  1,  1,  1 };
	vec3_t delta;
	float f;
	int maxdist;
	vec4_t hcolor;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||  cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUECOACH ||  cg.snap->ps.persistant[PERS_TEAM] == TEAM_REDCOACH) {
		return;
	}

	if ( !cg.renderingThirdPerson ) {
		return;
	}

	if ( !cg_draw2D.integer ) {
		return;
	}

	VectorCopy( cg.predictedPlayerState.origin, muzzlePoint );
	muzzlePoint[2] += cg.predictedPlayerState.viewheight;
	AngleVectors( cg.predictedPlayerState.viewangles, forward, right, up );

	switch ( cg.predictedPlayerState.weapon ) {
		case WP_GAUNTLET:
			maxdist = 32.0f;
			break;
		case WP_LIGHTNING:
			maxdist = LIGHTNING_RANGE;
			break;
		default:
			maxdist = 8192.0f;
			break;
	}

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	VectorMA( muzzlePoint, maxdist, forward, endPoint );

	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cg.predictedPlayerState.clientNum, MASK_SHOT );

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader2[ ca % NUM_CROSSHAIRS ];

	memset (&re, 0, sizeof(re));

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
	}
	else {
		hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0f;
	}

	re.shaderRGBA[0] = 255 * hcolor[0];
	re.shaderRGBA[1] = 255 * hcolor[1];
	re.shaderRGBA[2] = 255 * hcolor[2];
	re.shaderRGBA[3] = 255 * hcolor[3];

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( cg_crosshairPulse.integer && f > 0 && f < ITEM_BLOB_TIME ) {
		f = f / ITEM_BLOB_TIME + 1;
	}
	else {
		f = 1;
	}

	VectorSubtract( cg.refdef.vieworg, trace.endpos, delta );

	VectorCopy( trace.endpos, re.origin);
	re.reType = RT_SPRITE;
	re.renderfx = RF_DEPTHHACK;
	re.radius = f * cg_crosshairSize.integer * VectorLength(delta) * cg.refdef.fov_x / 51200.0f;
	re.rotation = 0;
	re.customShader = hShader;
	CG_AddRefEntityToScene( &re, cg.predictedPlayerState.clientNum, REF_ONLY_CLIENT | REF_NOT_PIP, qfalse ); //multiview

	memset (&re, 0, sizeof(re));

	re.shaderTime = cg.time / 1000.0f;
	re.reType = RT_RAIL_CORE;
	re.customShader = cgs.media.railCoreShader;
 
	VectorCopy(muzzlePoint, re.origin);
	VectorCopy(trace.endpos, re.oldorigin);

	re.shaderRGBA[0] = 32 * hcolor[0];
	re.shaderRGBA[1] = 32 * hcolor[1];
	re.shaderRGBA[2] = 32 * hcolor[2];
	re.shaderRGBA[3] = 255 * hcolor[3];

	CG_AddRefEntityToScene( &re, cg.predictedPlayerState.clientNum, REF_ONLY_CLIENT | REF_NOT_PIP, qfalse ); //multiview
}

/*
=================
CG_DrawSpectatorCrosshair
=================
*/
static void CG_DrawSpectatorCrosshair(void) {
	float		w, h;
	float		x, y;
	int time = cg.time - cg.pointClickClientTime;
	clientInfo_t *ci;
	vec4_t hcolor;

	ci = &cgs.clientinfo[cg.pointClickClientNum];

	if ( time < 1000 && (cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR ||
			ci->team == cg.snap->ps.stats[STAT_TEAM] ||
			(ci->team == TEAM_BLUE && cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUECOACH) ||
			(ci->team == TEAM_RED && cg.snap->ps.stats[STAT_TEAM] == TEAM_REDCOACH)) ) {
		float		r, g, b;

		if ( (!cg_swapSkins.integer && ci->team == TEAM_RED) ||
				(cg_swapSkins.integer && ci->team == TEAM_BLUE) ) {
			r = 0.8f;
			g = 0.0f;
			b = 0.2f;
		}
		else if ( (!cg_swapSkins.integer && ci->team == TEAM_BLUE) ||
				(cg_swapSkins.integer && ci->team == TEAM_RED) ) {
			r = 0.2f;
			g = 0.0f;
			b = 0.8f;
		}
		else {
			r = 0.1f;
			g = 0.8f;
			b = 0.1f;
		}

		if ( time < 500 ) {
			hcolor[0] = r;
			hcolor[1] = g;
			hcolor[2] = b;

			w = h = 16 + 32;
		}
		else {
			float f = (500.0f - (time - 500)) / 500.0f;

			hcolor[0] = 1.0f - (1.0f - r) * f;
			hcolor[1] = 1.0f - (1.0f - g) * f;
			hcolor[2] = 1.0f - (1.0f - b) * f;

			w = h = f * 16 + 32;
		}
	}
	else {
		hcolor[0] = 1.0f;
		hcolor[1] = 1.0f;
		hcolor[2] = 1.0f;

		w = h = 32;
	}

	hcolor[3] = 0.5f;

	trap_R_SetColor( hcolor );

	x = y = 0;
	CG_AdjustFrom640( &x, &y, &w, &h );

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, cgs.media.spectatorCrosshairShader );

	trap_R_SetColor( NULL );
}
//freeze

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;
	vec4_t		hcolor;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUECOACH || cg.snap->ps.persistant[PERS_TEAM] == TEAM_REDCOACH ) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		String2RGB( cg_crosshairColor.string, hcolor );
		hcolor[3] = 1.0f;
		trap_R_SetColor( hcolor );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( cg_crosshairPulse.integer && f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, hShader );
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;
//freeze
	clientInfo_t *ci;
	vec3_t mins, maxs;

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUECOACH || cg.snap->ps.persistant[PERS_TEAM] == TEAM_REDCOACH  ) {
		mins[0] = mins[1] = mins[2] = -8;
		maxs[0] = maxs[1] = maxs[2] = 8;
	}
	else {
		VectorCopy( vec3_origin, mins );
		VectorCopy( vec3_origin, maxs );
	}
//freeze

//freeze
	if ( !cg.renderingThirdPerson ) {
//freeze
		VectorCopy( cg.refdef.vieworg, start );
		VectorMA( start, 131072, cg.refdef.viewaxis[0], end );
//freeze
	}
	else {
		vec3_t forward;
		AngleVectors( cg.predictedPlayerState.viewangles, forward, NULL, NULL );

		VectorCopy( cg.predictedPlayerState.origin, start );
		start[2] += cg.predictedPlayerState.viewheight;
		VectorMA( start, 131072, forward, end );
	}
//freeze

	CG_Trace( &trace, start, mins, maxs, end, 
		cg.predictedPlayerState.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
//freeze
		entityState_t	*es;
		
		es = &cg_entities[ trace.entityNum ].currentState;
		if ( es->powerups & ( 1 << PW_BATTLESUIT ) ) {
			cg.crosshairClientNum = es->otherEntityNum;
			cg.crosshairClientTime = cg.time;

			ci = &cgs.clientinfo[es->clientNum];

			if ( cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_SPECTATOR || 
					(ci->infoValid && cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_BLUECOACH && ci->team == TEAM_BLUE) ||
					(ci->infoValid && cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_REDCOACH && ci->team == TEAM_RED) ||
					(ci->infoValid && cg.predictedPlayerState.stats[STAT_TEAM] == ci->team) ) {
				cg.pointClickClientNum = es->otherEntityNum;
				cg.pointClickClientTime = cg.time;
			}
			return;
		}

		if ( cg.time - cg.crosshairClientTime >= 500 ) {
			for ( ci = cgs.clientinfo; ci < &cgs.clientinfo[cgs.maxclients]; ci++ ) {
				if ( !ci->infoValid ) {
					continue;
				}

				if ( ci->frozen > 1000 && ci->unfreezer == cg.predictedPlayerState.clientNum ) {
					cg.crosshairClientNum = ci - cgs.clientinfo;
					cg.crosshairClientTime = cg.time;
					return;
				}
			}
		}
//freeze
		return;
	}

	ci = &cgs.clientinfo[trace.entityNum];

	if ( cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
		cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_REDCOACH &&
		cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_BLUECOACH) {
		centity_t *cent;

		// if the player is in fog, don't show it
		content = trap_CM_PointContents( trace.endpos, 0 );
		if ( (content & CONTENTS_FOG) ) {
			return;
		}

		cent = &cg_entities[trace.entityNum];
		// if the player is invisible
		if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
//freeze
			// if he's on the other team AND he's going slow enough AND he hasn't fired lately
			if ( ci->team != cg.predictedPlayerState.stats[STAT_TEAM] &&
					VectorLength(cent->currentState.pos.trDelta) < 180.0f &&
					cg.time - cent->lastFiringTime > 500 ) {
				// don't show it
//freeze
				return;
			}
		}
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;

	if ( cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_SPECTATOR ||
			(ci->infoValid && cg.predictedPlayerState.stats[STAT_TEAM] == ci->team) || 
			(ci->infoValid && (cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_REDCOACH && ci->team == TEAM_RED)) ||
			(ci->infoValid && (cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_BLUECOACH && ci->team == TEAM_BLUE))) {
		cg.pointClickClientNum = trace.entityNum;
		cg.pointClickClientTime = cg.time;
	}
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;
//freeze
	clientInfo_t	*ci, *me;
	char str[MAX_QPATH];
//freeze

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
/*freeze
	if ( cg.renderingThirdPerson ) {
		return;
	}
freeze*/

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( MIN(cg.crosshairClientTime, cg.time), 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

//freeze
	ci = &cgs.clientinfo[ cg.crosshairClientNum ];
	name = ci->name;
//freeze

#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );

//freeze
//ssdemo
	// if doing a demo playback
	if ( cg.ssDemoPlayback ) {
		// assume "me" is the player followed
		me = &cgs.clientinfo[ cg.predictedPlayerState.clientNum ];
	}
	else {
//ssdemo
		me = &cgs.clientinfo[ cg.clientNum ];
	}

	if ( ( me->team == TEAM_RED || 
		me->team == TEAM_BLUE ) &&
		me->team == ci->team ) {

		if ( ci->frozen ) {
			if ( ci->frozen < 1000 ) {
				Com_sprintf( str, MAX_QPATH, "%d%% Frozen", ci->frozen );
				color[0] *= 0.25f;
				color[1] *= 0.5f;
			}
			else {
//ssdemo
				//float tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
				//int thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);

				float tdiff;
				int thawed;

				if ( cg.ssDemoPaused ) {
					tdiff = (float)(cg.ssDemoPauseTime - ci->tinfolastvalid) / 1000.0f;
				}
				else {
					tdiff = (float)(cg.time - ci->tinfolastvalid) / 1000.0f;
				}

				thawed = ci->frozen - 1000 + (int)(tdiff / cgs.thawTime * 100.0f);
//ssdemo
		
				if ( thawed > 99 ) {
					thawed = 99;
				}

				if ( cg.timein > cg.time ) {
					Com_sprintf( str, MAX_QPATH, "%d%% Thawed", ci->frozen - 1000 );
				}
				else {
					Com_sprintf( str, MAX_QPATH, "%d%% Thawed", thawed );
				}
				color[1] *= 0.25f;
				color[2] *= 0.1f;
			}
		}
		else if ( ci->health || ci->armor ) {
			Com_sprintf( str, MAX_QPATH, "%d, %d", ci->health, ci->armor );
			color[2] *= 0.25f;
		}
		else {
			str[0] = '\0';
		}

		if ( str[0] ) {
			w = CG_DrawStrlen( str ) * SMALLCHAR_WIDTH;

			color[3] *= 0.5f;
			CG_DrawStringExt( 320 - w / 2, 170 + BIGCHAR_HEIGHT, str, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
		}
	}
	else if ( me->team != ci->team && Q_Isfreeze( cg.crosshairClientNum ) ) {
		w = CG_DrawStrlen( "Frozen" ) * SMALLCHAR_WIDTH;

		color[0] *= 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] *= 0.5f;

		CG_DrawStringExt( 320 - w / 2, 170 + BIGCHAR_HEIGHT, "Frozen", color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
//freeze
#endif
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
//freeze
	char *msg;
//ssdemo
	if ( cg.ssDemoPlayback ) {
		if ( cg.ssDemoPaused ) {
			msg = "DEMO SPECTATOR - PAUSED";
		}
		else if ( cg.ssSkipToDemoTime != 0 ) {
			msg = "DEMO SPECTATOR - SKIPPING";
		}
		else {
			msg = "DEMO SPECTATOR";
		}
	}
//ssdemo
	else if ( cg.snap->ps.stats[STAT_TEAM] == TEAM_RED ) {
		msg = "FROZEN SPECTATOR - RED TEAM";
	}
	else if ( cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUE ) {
		msg = "FROZEN SPECTATOR - BLUE TEAM";
	}
	else {
		msg = "SPECTATOR";		
	}

	CG_DrawBigString(320 - strlen(msg) * 8, 440, msg, 1.0F);
//freeze
		
/*freeze
	if ( cgs.gametype == GT_TOURNAMENT ) {
freeze*/
	if ( cgs.gametype == GT_TOURNAMENT || Q_Isfreeze( cg.snap->ps.clientNum ) ) {
//freeze
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else if ( cgs.gametype >= GT_TEAM &&
//freeze
			cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR ) {
//freeze
//ssdemo
		if ( !cg.ssDemoPlayback )
//ssdemo
			CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
#endif
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo->team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo->team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}


static qboolean CG_DrawScoreboard() {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}																					  


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	CG_DrawRefNotify();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		CG_DrawRefNotify();
		return;
	}
#endif
	cg.pointClickPlayers = qfalse;
	cg.pointClickScores = qfalse;

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
//wstats
	if ( cg.predictedPlayerState.stats[STAT_TEAM] != TEAM_SPECTATOR && cg.predictedPlayerState.stats[STAT_TEAM] != TEAM_REDCOACH && cg.predictedPlayerState.stats[STAT_TEAM] != TEAM_BLUECOACH ) {
		cg.wstatsFadeTime = cg.time;
		cg.wstatsShowing = CG_DrawWStats();
	}
//wstats
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float		x;
	vec4_t		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;


//freeze
	if ( cg.snap->ps.pm_type == PM_FREEZE ) {
		CG_DrawBigString( 320 - 18 * 8, 24, "following ^5(frozen)^7", 1.0F );
	}
	else {
//freeze
		CG_DrawBigString( 320 - 9 * 8, 24, "following", 1.0F );
	}

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( name ) );

	CG_DrawStringExt( x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}


#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  static int proxCounter;
  static int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

  if (proxTime == 0) {
    proxTime = cg.time + 5000;
    proxCounter = 5;
    proxTick = 0;
  }

  if (cg.time > proxTime) {
    proxTick = proxCounter--;
    proxTime = cg.time + 1000;
  }

  if (proxTick != 0) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
//freeze
		if ( sec == -2 ) {
			s = "Type /ready to begin match";
		}
		else {
//freeze
			s = "Waiting for players";
		}
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
#ifdef MISSIONPACK
			w = CG_Text_Width(s, 0.6f, 0);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
			w = CG_DrawStrlen( s );
			if ( w > 640 / GIANT_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawStringExt( 320 - w * cw/2, 20,s, colorWhite, 
					qfalse, qtrue, cw, (int)(cw * 1.5f), 0 );
#endif
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
#ifdef MISSIONPACK
		} else if ( cgs.gametype == GT_1FCTF ) {
			s = "One Flag CTF";
		} else if ( cgs.gametype == GT_OBELISK ) {
			s = "Overload";
		} else if ( cgs.gametype == GT_HARVESTER ) {
			s = "Harvester";
#endif
		} else {
			s = "";
		}
#ifdef MISSIONPACK
		w = CG_Text_Width(s, 0.6f, 0);
		CG_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt( 320 - w * cw/2, 25,s, colorWhite, 
				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
#endif
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
//ssdemo
		if ( cg.timein <= cg.time && !cg.ssDemoPaused ) {
//ssdemo
			switch ( sec ) {
			case 0:
				trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
				break;
			case 1:
				trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
				break;
			case 2:
				trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		}
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		scale = 0.54f;
		break;
	case 1:
		cw = 24;
		scale = 0.51f;
		break;
	case 2:
		cw = 20;
		scale = 0.48f;
		break;
	default:
		cw = 16;
		scale = 0.45f;
		break;
	}

#ifdef MISSIONPACK
		w = CG_Text_Width(s, scale, 0);
		CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	w = CG_DrawStrlen( s );
	CG_DrawStringExt( 320 - w * cw/2, 70, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
#endif
}

//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif

//freeze
static void CG_DrawWarningString( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT * 0.6f, 0 );
}


/*
=================
CG_ShowWarning
=================
*/
void CG_ShowWarning() {
	vec4_t hcolor[2], tcolor[2], ocolor, bcolor;
	vec_t alpha;
	int startx, starty, y, height, numlines, x1, y1, x2, y2;
	const int column1 = 1, column2 = 28;
	int timenudge, friendlyfire, pure, hardcore, dotimeouts, doteamlocks, numtimeouts; //, forcerespawn, forcenofloat;
	char *tmp1, *tmp2;

	timenudge = CG_Cvar_VariableIntegerValue( "cl_timenudge" );
//ssdemo
	// if playing a demo, use recorded systeminfo instead
	if ( cg.ssDemoPlayback ) {	
		const char *info = CG_ConfigString( CS_SSDEMOSYSTEMINFO );

		friendlyfire = atoi( Info_ValueForKey(info, "g_friendlyFire") );
		pure = atoi( Info_ValueForKey(info, "sv_pure") );
		hardcore = atoi( Info_ValueForKey(info, "g_hardcore") );
		dotimeouts = atoi( Info_ValueForKey(info, "g_doTimeouts") );
		numtimeouts = atoi( Info_ValueForKey(info, "g_numTimeouts") );
		doteamlocks = atoi( Info_ValueForKey(info, "g_doTeamLocks") );
		//forcerespawn = atoi( Info_ValueForKey(info, "g_forceRespawn") );
		//forcenofloat = atoi( Info_ValueForKey(info, "g_forceNoFloat") );
	}
	else {
//ssdemo
		friendlyfire = CG_Cvar_VariableIntegerValue( "g_friendlyFire" );
		pure = CG_Cvar_VariableIntegerValue( "sv_pure" );
		hardcore = CG_Cvar_VariableIntegerValue( "g_hardcore" );
		dotimeouts = CG_Cvar_VariableIntegerValue( "g_doTimeouts" );
		numtimeouts = CG_Cvar_VariableIntegerValue( "g_numTimeouts" );
		doteamlocks = CG_Cvar_VariableIntegerValue( "g_doTeamLocks" );
		//forcerespawn = CG_Cvar_VariableIntegerValue( "g_forceRespawn" );
		//forcenofloat = CG_Cvar_VariableIntegerValue( "g_forceNoFloat" );
	}

	alpha = 0.4f;

	if ( cg.warningTime < 250 ) {
		alpha = (float)cg.warningTime / 250 * alpha;
	}

	ocolor[0] = 0.2f;
	ocolor[1] = 0.2f;
	ocolor[2] = 0.2f;
	ocolor[3] = alpha;

	bcolor[0] = 0.4f;
	bcolor[1] = 0.5f;
	bcolor[2] = 0.7f;
	bcolor[3] = alpha;

	numlines = 5;

//ssdemo
	//if ( cgs.delagHitscan ) numlines++;
	if ( cgs.delagHitscan && !cg.ssDemoPlayback ) numlines++;
//ssdemo
	if ( hardcore ) numlines++;
	if ( friendlyfire ) numlines++;
	if ( pmove_fixed.integer && cg_pmove_fixed.integer ) numlines++;
	if ( cgs.grapple ) numlines++;
	if ( cgs.railJumpKnock ) numlines++;
	if ( dotimeouts ) numlines += 2;
	if ( doteamlocks ) numlines++;
	//if ( forcerespawn ) numlines++;
	//if ( forcenofloat ) numlines++;

	startx = column1 * SMALLCHAR_WIDTH;

	if ( cg.warningTime > WARNING_TIME - 500 ) {
		startx += (cg.warningTime - (WARNING_TIME - 500)) * 1280 / 500;
	}

	height = numlines * (SMALLCHAR_HEIGHT * 0.65);
	starty = 360 - height;

	x1 = startx - 2;
	y1 = starty - 2;
	x2 = (column2 + 13) * SMALLCHAR_WIDTH + 4;
	y2 = height + 2;

	trap_R_SetColor( ocolor );
	CG_DrawPic( x1, y1, x2, y2, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	CG_DrawBorder( x1 - 1, y1 - 1, x2 + 2, y2 + 2, 1, bcolor );

	tcolor[0][0] = 1.0f;
	tcolor[0][1] = 0.5f;
	tcolor[0][2] = 0.4f;
	tcolor[0][3] = alpha;

	tcolor[1][0] = 1.0f;
	tcolor[1][1] = 0.8f;
	tcolor[1][2] = 0.7f;
	tcolor[1][3] = alpha;

	hcolor[0][0] = 1.0f;
	hcolor[0][1] = 0.2f;
	hcolor[0][2] = 1.0f;
	hcolor[0][3] = alpha;

	hcolor[1][0] = 1.0f;
	hcolor[1][1] = 0.8f;
	hcolor[1][2] = 1.0f;
	hcolor[1][3] = alpha;

	numlines = 0;

	y = starty;
	CG_DrawWarningString( startx, y, "Pure Server................", tcolor[numlines % 2] );
	CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, (pure ? "Yes" : "No"), hcolor[numlines % 2] );
	numlines++;

	y += SMALLCHAR_HEIGHT * 0.65;
	CG_DrawWarningString( startx, y, "Round / Time Limit.........", tcolor[numlines % 2] );
	tmp1 = cgs.capturelimit == 0 ? "INF" : va("%d", cgs.capturelimit);
	tmp2 = cgs.timelimit == 0 ? "INF" : va("%d", cgs.timelimit);
	CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%s / %s", tmp1, tmp2), hcolor[numlines % 2] );
	numlines++;

	y += SMALLCHAR_HEIGHT * 0.65;
	CG_DrawWarningString( startx, y, "Lag Compensation (Server)..", tcolor[numlines % 2] );
	CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, (cgs.delagHitscan ? "On" : "Off"), hcolor[numlines % 2] );
	numlines++;

//ssdemo
	//if ( cgs.delagHitscan ) {
	if ( cgs.delagHitscan && !cg.ssDemoPlayback ) {
//ssdemo
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Lag Compensation (Client)..", tcolor[numlines % 2] );
		if ( cg_delag.integer == 0 ) {
			CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "Off", hcolor[numlines % 2] );
		}
		else if ( cg_delag.integer & 1 ) {
			CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "On", hcolor[numlines % 2] );
		}
		else {
			CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "Partial", hcolor[numlines % 2] );
		}
		numlines++;
	}

	if ( hardcore ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Suicide Policy.............", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "Hardcore", hcolor[numlines % 2] );
		numlines++;
	}

	if ( friendlyfire ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Friendly Fire..............", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "On", hcolor[numlines % 2] );
		numlines++;
	}

	y += SMALLCHAR_HEIGHT * 0.65;
	CG_DrawWarningString( startx, y, "Thaw Time..................", tcolor[numlines % 2] );
	CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%01.1f seconds", cgs.thawTime), hcolor[numlines % 2] );
	numlines++;

	y += SMALLCHAR_HEIGHT * 0.65;
	CG_DrawWarningString( startx, y, "Autothaw Time..............", tcolor[numlines % 2] );
	CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%d:%02d", cgs.autoThawTime / 60, cgs.autoThawTime % 60), hcolor[numlines % 2] );
	numlines++;

	if ( pmove_fixed.integer && cg_pmove_fixed.integer ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Fixed Movement Framerate...", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%d FPS", 1000 / pmove_msec.integer), hcolor[numlines % 2] );
		numlines++;
	}

	if ( cgs.grapple ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Offhand Grappling Hook.....", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "On", hcolor[numlines % 2] );
		numlines++;
	}

	if ( cgs.railJumpKnock ) {
		char *strength;

		if ( cgs.railJumpKnock < 100 ) {
			strength = "Weak";
		}
		else if ( cgs.railJumpKnock >= 200 ) {
			strength = "Strong";
		}
		else { 
			strength = "Normal";
		}

		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Rail Jump Strength.........", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, strength, hcolor[numlines % 2] );
		numlines++;
	}

	if ( dotimeouts ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Team Timeouts..............", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "On", hcolor[numlines % 2] );
		numlines++;

		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Number of Timeouts.........", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%d", numtimeouts), hcolor[numlines % 2] );
		numlines++;
	}

	if ( doteamlocks ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Team Locking...............", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, "On", hcolor[numlines % 2] );
		numlines++;
	}

// they'll find out soon enough
/*
	if ( forcerespawn ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Force Respawn Time.........", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%d seconds", forcerespawn ), hcolor[numlines % 2] );
		numlines++;
	}

	if ( forcenofloat ) {
		y += SMALLCHAR_HEIGHT * 0.65;
		CG_DrawWarningString( startx, y, "Force Spectate Time........", tcolor[numlines % 2] );
		CG_DrawWarningString( startx + column2 * SMALLCHAR_WIDTH, y, va("%d seconds", forcenofloat), hcolor[numlines % 2] );
		numlines++;
	}
*/
}

static void CG_FollowCrosshairName(void) {
	if ( cg.time - cg.pointClickClientTime < 1000 && (cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR ||
			cgs.clientinfo[cg.pointClickClientNum].team == cg.snap->ps.stats[STAT_TEAM] || 
			(cg.snap->ps.stats[STAT_TEAM] == TEAM_REDCOACH && cgs.clientinfo[cg.pointClickClientNum].team == TEAM_RED) ||  
			(cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUECOACH && cgs.clientinfo[cg.pointClickClientNum].team == TEAM_BLUE))) {
		trap_SendConsoleCommand( va("follow %d\n", cg.pointClickClientNum) );
		trap_S_StartLocalSound( cgs.media.menu_out_sound, CHAN_AUTO );
	}
	else {
		// if really a spectator
		if ( cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR || cg.snap->ps.stats[STAT_TEAM] == TEAM_REDCOACH || cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUE ) {
			// do the follow cycle
			trap_SendConsoleCommand( va("follownext\n") );
		}
		else {
			// if not really a spectator, follow self
			trap_SendConsoleCommand( va("follow %d\n", cg.clientNum) );
		}
	}
}
//freeze

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
	int current;
	usercmd_t latestCmd, lastCmd;
	qboolean latestValid, lastValid, isspectator, lastFreezingAngles;

#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif

	lastFreezingAngles = cg.freezingAngles;
	cg.freezingAngles = qfalse;

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot || cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

//freeze
	cg.pointClickPlayers = qfalse;
	cg.pointClickScores = qfalse;
	isspectator = qfalse;

	if ( !cg.demoPlayback ) {
		// see if the player is worthy of point-and-click mode
		isspectator = CG_IsSpectator();

		// if showing the scoreboard
		if ( cg.showScores && (cg_pointClickSpec.integer || cg.pipManageMode) ) {
			cg.pointClickScores = isspectator;
			if ( cg.pointClickScores ) {
				cg.freezingAngles = qtrue;
			}
		}
		else {
			// if NOT in follow mode
			if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
				// you can point-and-click players
				cg.pointClickPlayers = isspectator;
			}
		}
	}

	current = trap_GetCurrentCmdNumber();
	latestValid = trap_GetUserCmd(current, &latestCmd);
	lastValid = trap_GetUserCmd(current - 1, &lastCmd);

	// if we're in point-and-click mode for the scoreboard or in PIP management mode
	if ( cg.pointClickScores || cg.pipManageMode ) {
		if ( latestValid && lastValid ) {
			// get the angle delta between this frame and last one
			float dx = AngleSubtract(SHORT2ANGLE(lastCmd.angles[YAW]), SHORT2ANGLE(latestCmd.angles[YAW]));
			float dy = AngleSubtract(SHORT2ANGLE(latestCmd.angles[PITCH]), SHORT2ANGLE(lastCmd.angles[PITCH]));

			// adjust it for mouse settings

			dx /= cg_sensitivity.value;
			dx /= m_yaw.value;
			
			dy /= cg_sensitivity.value;
			dy /= m_pitch.value;

			// move one of the pointers
			if ( cg.pointClickScores ) {
				CG_ScoreboardMouseEvent( dx, dy );

				// check for a button press
				if ( latestCmd.buttons & BUTTON_ATTACK && !(lastCmd.buttons & BUTTON_ATTACK) ) {
					CG_FollowScoreboardName();
				}
			}
			else if ( cg.pipManageMode ) {
				CG_PipMouseEvent( dx, dy );
				CG_ManagePipWindows();
			}
		}
	}
//freeze

/*
	if (cg.cameraMode) {
		return;
	}
*/

//freeze
	if ( cg.timein > cg.time ) {
		int time = cg.time - cg.timeout;
		float alpha;
		vec4_t ocolor;

		if ( time > 3000 ) {
			alpha = 0.5f;
		}
		else {
			alpha = ((float)time / 3000) * 0.5f;
		}

		ocolor[0] = 0.2f;
		ocolor[1] = 0.2f;
		ocolor[2] = 0.2f;
		ocolor[3] = alpha;

		trap_R_SetColor( ocolor );
		CG_DrawPic( 0, 0, 640, 240, cgs.media.teamStatusBar );
		CG_DrawPic( 0, 240, 640, 480, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );
	}

	if ( cg.warningTime == 999999 )
		cg.warningTime = WARNING_TIME;

	if ( cg.warningTime > 0 ) {
		CG_ShowWarning();

		// don't change if the frame takes more than 1/5 second
		if ( cg.frametime < 200 ) {
			cg.warningTime -= cg.frametime;
		}
	}
//freeze

//anticheat
	if ( cg.randomcrash == 0 ) {
		int key = cg.snap->ps.stats[STAT_KEY1] + cg.snap->ps.stats[STAT_KEY2] * 65536;
		char *charkey = BG_IntToCheatKey( key, cg.snap->ps.stats[STAT_FLAGS] );

		if ( cg.numAimChecks < 3 ) {
			// have we scheduled a check for +aim?
			if ( cg.nextAimCheck == 0 ) {
				// no, so schedule one
				cg.nextAimCheck = cg.time + 10000 + random() * 10000;
			}

			// if it's time to check
			if ( cg.time >= cg.nextAimCheck ) {
				// go ahead and check +aim
				trap_SendConsoleCommand( "+aim\n" );
				// store the last time we sent a +aim - our handler for it will set it to 0
				cg.lastAimCheck = cg.time;
				// increment the number of checks we've done
				cg.numAimChecks++;
				// schedule another check
				cg.nextAimCheck = cg.time + 10000 + random() * 10000;
			}
		}

		// if our handler didn't catch it within 1000ms
		if ( cg.lastAimCheck != 0 && cg.lastAimCheck < cg.time - 1000 ) {
			// mark it so we don't do this twice
			cg.lastAimCheck = 0;
			// increment the number of failures
			cg.failedAimChecks++;

			if ( cg.failedAimChecks == 3 ) {
				// alert the server admin and referees
				trap_SendConsoleCommand( va("%s using \"a client hook\"\n", charkey) );
				// schedule a random crash
				cg.randomcrash = cg.time + random() * 30000;
			}
		}
	}
//anticheat

//freeze
	// restrict requesting new user info to every five seconds at the most
	if ( cg.resendUinfo && cg.time - cg.uinfoTime > 5000 ) {
		trap_SendConsoleCommand( "uinfo\n" );
		cg.resendUinfo = qfalse;
		cg.uinfoTime = cg.time;
	}
//freeze

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 && !cg.noMainView ) {
			CG_DrawStatusBar();
			CG_DrawAmmoWarning();
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();
			CG_DrawHoldableItem();
			CG_DrawReward();
		}
    
		if ( cgs.gametype >= GT_TEAM ) {
			CG_DrawTeamInfo();
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();
	CG_DrawPipWindows(); //multiview
	if ( !cg.noMainView ) {
		CG_DrawLagometer();
	}
	CG_DrawUpperRight();
	CG_DrawLowerRight();
	CG_DrawLowerLeft();

//freeze
	if ( cg.timein != 0 && cg.timein + 1000 > cg.time ) {
		CG_DrawTimein();
	}
//freeze

	if ( !cg.pointClickScores && !cg.pipManageMode ) {
		// if we're in point-and-click mode for players
		if ( cg.pointClickPlayers ) {
			CG_DrawCrosshairNames();
			CG_DrawSpectatorCrosshair();

			if ( latestValid && lastValid &&
					latestCmd.buttons & BUTTON_ATTACK && !(lastCmd.buttons & BUTTON_ATTACK) ) {
				CG_FollowCrosshairName();
			}
		}
		else {
			CG_DrawCrosshairNames();

			if ( isspectator && latestValid && lastValid &&
					latestCmd.buttons & BUTTON_ATTACK && !(lastCmd.buttons & BUTTON_ATTACK) ) {
				// if following a player already
				if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
					// do the follow cycle
					trap_SendConsoleCommand( va("follownext\n") );
				}
				else {
					// if not already following, follow self
					trap_SendConsoleCommand( va("follow %d\n", cg.snap->ps.clientNum) );
				}
			}
		}
	}
//freeze

//multiview
	if ( !cg.pointClickScores && cg.pipManageMode ) {
		CG_DrawPipCrosshair();
		cg.freezingAngles = qtrue;
	}
//multiview

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing ) {
		CG_DrawCenterString();
		CG_DrawRefNotify();

		if ( !CG_DrawFollow() ) {
			CG_DrawWarmup();
		}
	}

//wstats
	cg.wstatsShowing = CG_DrawWStats();
//wstats

//freeze
	if ( cg.freezingAngles && !lastFreezingAngles ) {
		trap_SendConsoleCommand( "freezeangles\n" );
		cg.freezeAnglesTime = cg.time;
	}
	else if ( !cg.freezingAngles && lastFreezingAngles ) {
		trap_SendConsoleCommand( "unfreezeangles\n" );
		cg.unfreezeAnglesTime = cg.time;
	}
//freeze
}


static void CG_DrawTourneyScoreboard() {
#ifdef MISSIONPACK
#else
	CG_DrawOldTourneyScoreboard();
#endif
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	if ( !cg.noMainView ) {
//freeze
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
			CG_DrawThirdPersonCrosshair();
		}

		CG_DrawWeather();
//freeze

		CG_AddAllRefEntitiesFor( cg.predictedPlayerState.clientNum, &cg.refdef, qtrue, cg.renderingThirdPerson ); //multiview

		// draw 3D view
		trap_R_RenderScene( &cg.refdef );
	}

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	CG_RefreshPipWindows(); //multiview

	// draw status bar and other floating elements
 	CG_Draw2D();
}




// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+0)
#define SB_HEAD_X			(SCOREBOARD_X+32)

#define SB_SCORELINE_X		80

#define SCORECHAR_WIDTH		12
#define SCORECHAR_HEIGHT	12

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH) // width 6
#define SB_SCORE_X			(SB_SCORELINE_X ) // width 6
#define SB_RATING_X			(SB_SCORELINE_X + 5 * SCORECHAR_WIDTH) // width 6
#define SB_PING_X			(SB_SCORELINE_X + 13 * SCORECHAR_WIDTH + 6) // width 5
#define SB_TIME_X			(SB_SCORELINE_X + 18 * SCORECHAR_WIDTH + 6) // width 5
#define SB_FROZEN_X		(SB_SCORELINE_X + 23 * SCORECHAR_WIDTH + 6) // width 5
#define SB_NAME_X			(SB_SCORELINE_X + 29 * SCORECHAR_WIDTH) // width 15

#define SB_MAX_SCROLL (MAX_CLIENTS * SB_INTER_HEIGHT + BIGCHAR_HEIGHT * 3 + SB_INTER_HEIGHT / 2)

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed

void CG_DrawScoreboardString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y + 2, s, color, qfalse, qtrue, SCORECHAR_WIDTH-1, SCORECHAR_HEIGHT, 0 );
}

void CG_DrawScoreboardString2( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y + 2, s, color, qfalse, qtrue, SCORECHAR_WIDTH-3, SCORECHAR_HEIGHT, 0 );
}

static void CG_DrawScoreboardSmallString( int x, int y, const char *s, float color[4] ) {
//	float	color[4];

	CG_DrawStringExt( x, y + 2, s, color, qfalse, qfalse, SCORECHAR_WIDTH-5, SCORECHAR_HEIGHT-5, 0 );
}

static void CG_DrawScoreboardSmallerString( int x, int y, const char *s, float color[4] ) {
//	float	color[4];

	CG_DrawStringExt( x, y + 2, s, color, qfalse, qfalse, SCORECHAR_WIDTH-7, SCORECHAR_HEIGHT-5, 0 );
}

void CG_FollowScoreboardName(void) {
	score_t *score;
	int i, cursorY;

	if ( !cg.scoreBoardShowing ) {
		return;
	}

	cursorY = cgs.cursorY;
	if ( cursorY < SB_TOP + SB_INTER_HEIGHT ) {
		cursorY = SB_TOP + SB_INTER_HEIGHT;
	}
	else if (cursorY >= SB_STATUSBAR) {
		cursorY = SB_STATUSBAR - 1;
	}

	for ( i = 0; i < cg.numScores; i++ ) {
		score = &cg.scores[i];

		if ( cursorY >= score->topLine && cursorY < score->bottomLine ) {
			clientInfo_t *ci = &cgs.clientinfo[score->client];

			if ( cg.pipManageMode ) {
				if ( ci->infoValid && ci->team != TEAM_SPECTATOR && ci->team != TEAM_REDCOACH && ci->team != TEAM_BLUECOACH ) {
					CG_RequestPipWindowUp( score->client );
					trap_S_StartLocalSound( cgs.media.menu_out_sound, CHAN_AUTO );
					return;
				}
			}
			else {
				if ( ci->infoValid && ci->team != TEAM_SPECTATOR && ci->team != TEAM_REDCOACH && ci->team != TEAM_BLUECOACH &&
					(ci->team == cg.snap->ps.stats[STAT_TEAM] || cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR|| cg.snap->ps.stats[STAT_TEAM] == TEAM_BLUECOACH || cg.snap->ps.stats[STAT_TEAM] == TEAM_REDCOACH) ) {
						trap_SendConsoleCommand( va("follow %d\n", score->client) );
						trap_S_StartLocalSound( cgs.media.menu_out_sound, CHAN_AUTO );
					return;
				}
			}
		}
	}
}

void CG_ScoreboardMouseEvent(float x, float y) {
	int maxYScroll = cg.scoreboardHeight - (SB_STATUSBAR - SB_TOP);
	if ( maxYScroll < 0 ) {
		maxYScroll = 0;
	}

	cgs.cursorX += x;
	if (cgs.cursorX < 0) {
		cgs.cursorX = 0;
	}
	else if (cgs.cursorX > 640) {
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;
	if (cgs.cursorY < SB_TOP + SB_INTER_HEIGHT) {
		cgs.scrollY -= SB_TOP + SB_INTER_HEIGHT - cgs.cursorY;
		cgs.cursorY = SB_TOP + SB_INTER_HEIGHT;
	}
	else if (cgs.cursorY >= SB_STATUSBAR) {
		cgs.scrollY += cgs.cursorY - SB_STATUSBAR;
		cgs.cursorY = SB_STATUSBAR - 1;
	}

	if ( cgs.scrollY < 0 ) {
		cgs.scrollY = 0;
	}
	else if ( cgs.scrollY > maxYScroll ) {
		cgs.scrollY = maxYScroll;
	}
}

void CG_DrawScoreboardCrosshair(void) {
	float		w, h;
	float		x, y;
	qhandle_t	shader;

	trap_R_SetColor( NULL );

	w = h = 32;
	x = cgs.cursorX - 320;
	y = cgs.cursorY - 240;
	CG_AdjustFrom640( &x, &y, &w, &h );

	if ( cg.pipManageMode ) {
		shader = cgs.media.spectatorPointerShader;
	}
	else {
		shader = cgs.media.spectatorCrosshairShader;
	}

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, shader );
}


/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawCoachScore( int x, int y, score_t *score, float *color, float fade, qboolean largeFormat ) {
	char	string[1024];
	vec3_t	headAngles;
	float	tColor[4];
	clientInfo_t	*ci;
	int iconx, headx;
	//freeze
	int readyMask[4];
	qboolean ref = ((cg.snap->ps.stats[STAT_FLAGS] & SF_REF) != 0);
	int cursorY;
	//freeze
	//unlagged - true ping
	int ping;
	//unlagged - true ping
	//ssdemo
	char pingstr[10];
	//ssdemo

	if ( y < SB_TOP ) {
		score->topLine = 0;
		score->bottomLine = 0;
		return;
	}

	if ( y >= SB_STATUSBAR ) {
		score->topLine = 0;
		score->bottomLine = 0;
		return;
	}

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	if ( largeFormat ) {
		int margin = (SB_NORMAL_HEIGHT - BIGCHAR_HEIGHT) / 2;
		score->topLine = y - margin;
		score->bottomLine = y + BIGCHAR_HEIGHT + margin;
	}
	else {
		score->topLine = y;
		score->bottomLine = y + SB_INTER_HEIGHT;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);


	//unlagged - true ping
	if ( cg_truePing.integer ) {
		ping = score->realPing;
	}
	else {
		ping = score->ping;
	}
	//unlagged - true ping

	//ssdemo
	if ( cg.ssDemoPlayback && score->client >= HALF_CLIENTS ) {
		strcpy( pingstr, " ^5REC" );
	}
	else {
		if(ping < 65)
			Com_sprintf( pingstr, sizeof(pingstr), "^2%4i", ping );
		else if(ping > 64 && ping < 150)
			Com_sprintf( pingstr, sizeof(pingstr), "^3%4i", ping );
		else if(ping > 149)
			Com_sprintf( pingstr, sizeof(pingstr), "^1%4i", ping );
	}
	//ssdemo - also changed Com_sprintf() calls below to use pingstr instead of ping

	//freeze

	//completely different scoreboard for teamplay

	// draw the score line
	if ( ping == -1 ) {
		Com_sprintf(string, sizeof(string),
			" connecting          %s", ci->name);
	} else if ( ci->team == TEAM_SPECTATOR ) {
		//freeze
		char *num = ref ? va("^3%2d^7/", score->client) : "";

		if ( score->referee ) {
			Com_sprintf(string, sizeof(string),
				" REF  %s %3i %4i %s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
		}
		else {
			//freeze
			Com_sprintf(string, sizeof(string),
				" SPEC %s %3i ^5%4i ^7%s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
		}
		CG_DrawScoreboardString( x , y, string, fade );
		return;
	} else {
		char *num = ref ? va("^3%2d^7/", score->client) : "";

		if ( score->referee ) {
			Com_sprintf(string, sizeof(string),
				"R%3i", score->score);
		}
		else {
			Com_sprintf(string, sizeof(string),
				" %3i", score->score);
		}
		CG_DrawScoreboardString( x , y, string, fade );

		tColor[3] = fade;

		tColor[0] = 0.67; tColor[1] = 0.67; tColor[2] = 0.67;  //dk.gray

		CG_DrawScoreboardString( x + 50 , y, "^3Coach", fade );

		Com_sprintf(string, sizeof(string),
			"%s", ci->name);

		CG_DrawScoreboardString( x + 116 , y, string, fade );


		//CG_DrawPic( x, y-1, 320 , 20, cgs.media.freezeShader2 );

	}

	return;

}

static void CG_DrawClientScore( int x, int y, score_t *score, float *color, float fade, qboolean largeFormat ) {
	char	string[1024];
	vec3_t	headAngles;
	float	tColor[4];
	clientInfo_t	*ci;
	int iconx, headx;
	//freeze
	int readyMask[4];
	qboolean ref = ((cg.snap->ps.stats[STAT_FLAGS] & SF_REF) != 0);
	int cursorY;
	//freeze
	//unlagged - true ping
	int ping;
	//unlagged - true ping
	//ssdemo
	char pingstr[10];
	//ssdemo

	if ( y < SB_TOP ) {
		score->topLine = 0;
		score->bottomLine = 0;
		return;
	}

	if ( y >= SB_STATUSBAR ) {
		score->topLine = 0;
		score->bottomLine = 0;
		return;
	}

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	if ( largeFormat ) {
		int margin = (SB_NORMAL_HEIGHT - BIGCHAR_HEIGHT) / 2;
		score->topLine = y - margin;
		score->bottomLine = y + BIGCHAR_HEIGHT + margin;
	}
	else {
		score->topLine = y;
		score->bottomLine = y + SB_INTER_HEIGHT;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	
	//unlagged - true ping
	if ( cg_truePing.integer ) {
		ping = score->realPing;
	}
	else {
		ping = score->ping;
	}
	//unlagged - true ping

	//ssdemo
	if ( cg.ssDemoPlayback && score->client >= HALF_CLIENTS ) {
		strcpy( pingstr, " ^5REC" );
	}
	else {
		if(ping < 65)
			Com_sprintf( pingstr, sizeof(pingstr), "^2%4i", ping );
		else if(ping > 64 && ping < 150)
			Com_sprintf( pingstr, sizeof(pingstr), "^3%4i", ping );
		else if(ping > 149)
			Com_sprintf( pingstr, sizeof(pingstr), "^1%4i", ping );
	}
	//ssdemo - also changed Com_sprintf() calls below to use pingstr instead of ping

	//freeze
	
	//completely different scoreboard for teamplay
	if(cgs.gametype == GT_TEAM){
	
		// draw the score line
		if ( ping == -1 ) {
			Com_sprintf(string, sizeof(string),
				" connecting          %s", ci->name);
		} else if ( ci->team == TEAM_SPECTATOR ) {
			//freeze
			char *num = ref ? va("^3%2d^7/", score->client) : "";

			if ( score->referee ) {
				Com_sprintf(string, sizeof(string),
					" REF  %s %3i %4i %s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
			}
			else {
				//freeze
				Com_sprintf(string, sizeof(string),
					" SPEC %s %3i ^5%4i ^7%s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
			}
			CG_DrawScoreboardString( x , y, string, fade );
			return;
		} else {
			char *num = ref ? va("^3%2d^7/", score->client) : "";

			if ( score->referee ) {
				Com_sprintf(string, sizeof(string),
					"R%3i", score->score);
			}
			else {
				Com_sprintf(string, sizeof(string),
					" %3i", score->score);
			}
				CG_DrawScoreboardString( x , y, string, fade );
	
				tColor[3] = fade;
				
				tColor[0] = 0.0; tColor[1] = 1.0; tColor[2] = 1.0;  //ltblue				

				Com_sprintf(string,sizeof(string),"%3i",score->scoreFlags);
				CG_DrawScoreboardSmallString(x + 40, y, string, tColor );

				tColor[0] = 0.67; tColor[1] = 0.67; tColor[2] = 0.67;  //dk.gray

				Com_sprintf(string,sizeof(string),"%3i",score->deaths);
				CG_DrawScoreboardSmallString(x + 40, y+7, string, tColor );

				tColor[0] = 1.0; tColor[1] = 0.65; tColor[2] = 0;  //orange

				//time / froz
				Com_sprintf(string,sizeof(string),"%3i",score->time);
				CG_DrawScoreboardSmallString(x + 60, y, string, tColor );

				tColor[0] = 0.0; tColor[1] = 1.0; tColor[2] = 1.0;  //blue

				Com_sprintf(string,sizeof(string),"%3i",score->timeFrozen);
				CG_DrawScoreboardSmallString(x + 60, y+7, string, tColor );

				tColor[0] = 1.0; tColor[1] = 1.0; tColor[2] = 1.0;  //white

				CG_DrawScoreboardString2(x + 77, y, pingstr, fade );

				Com_sprintf(string, sizeof(string),
					"%s", ci->name);

				CG_DrawScoreboardString( x + 116 , y, string, fade );

				if ( Q_Isfreeze( score->client ) ){
					//they are frozen
					CG_DrawPic( x, y-1, 320 , 20, cgs.media.freezeShader2 );

				}
		}

		return;
	}

	//freeze
/*	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
		}
	} else*/
	//freeze	

		if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
			if( largeFormat ) {
				CG_DrawFlagModel( iconx+x, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_RED, qfalse );
			}
			else {
				CG_DrawFlagModel( iconx+x, y, 16, 16, TEAM_RED, qfalse );
			}
		} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
			if( largeFormat ) {
				CG_DrawFlagModel( iconx+x, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_BLUE, qfalse );
			}
			else {
				CG_DrawFlagModel( iconx+x, y, 16, 16, TEAM_BLUE, qfalse );
			}
		} else {
			if ( ci->botSkill > 0 && ci->botSkill <= 5 ) {
				if ( cg_drawIcons.integer ) {
					if( largeFormat ) {
						CG_DrawPic( iconx+x, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
					}
					else {
						CG_DrawPic( iconx+x, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
					}
				}
			} else if ( ci->handicap < 100 ) {
				Com_sprintf( string, sizeof( string ), "%i", ci->handicap );
				/*freeze
				if ( cgs.gametype == GT_TOURNAMENT )
				freeze*/
				if ( cgs.gametype == GT_TOURNAMENT || cgs.gametype == GT_TEAM )
					//freeze
					CG_DrawSmallStringColor( iconx+x, y - SMALLCHAR_HEIGHT/2, string, color );
				else
					CG_DrawSmallStringColor( iconx+x, y, string, color );
			}

			//freeze
			if ( cgs.gametype == GT_TOURNAMENT || cgs.gametype == GT_TEAM ) {
				//freeze
				// draw the wins / losses
				if ( cgs.gametype == GT_TOURNAMENT ) {
					Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
					//freeze
				} else {
					Com_sprintf( string, sizeof( string ), "%i %s", score->scoreFlags, ci->teamLeader ? "L" : "" );
				}
				//freeze
				if( ci->handicap < 100 && !ci->botSkill ) {
					CG_DrawSmallStringColor( iconx+x, y + SMALLCHAR_HEIGHT/2, string, color );
				}
				else {
					CG_DrawSmallStringColor( iconx+x, y, string, color );
				}
			}
		}


		// draw the face
		VectorClear( headAngles );
		headAngles[YAW] = 180;
		if( largeFormat ) {
			CG_DrawHead( headx, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE, 
				score->client, headAngles, qtrue );
		}
		else {
			CG_DrawHead( headx, y, 16, 16, score->client, headAngles, qtrue );
		}

#ifdef MISSIONPACK
		// draw the team task
		if ( ci->teamTask != TEAMTASK_NONE ) {
			if ( ci->teamTask == TEAMTASK_OFFENSE ) {
				CG_DrawPic( headx + 48, y, 16, 16, cgs.media.assaultShader );
			}
			else if ( ci->teamTask == TEAMTASK_DEFENSE ) {
				CG_DrawPic( headx + 48, y, 16, 16, cgs.media.defendShader );
			}
		}
#endif


		// draw the score line
		if ( ping == -1 ) {
			Com_sprintf(string, sizeof(string),
				" connecting          %s", ci->name);
		} else if ( ci->team == TEAM_SPECTATOR ) {
			//freeze
			char *num = ref ? va("^3%2d^7/", score->client) : "";

			if ( score->referee ) {
				Com_sprintf(string, sizeof(string),
					" REF  %s %4i %4i %s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
			}
			else {
				//freeze
				Com_sprintf(string, sizeof(string),
					" SPEC %s %4i %4i %s%s", pingstr, score->time, score->timeFrozen, num, ci->name);
			}
		} else {
			//freeze
			char *num = ref ? va("^3%2d^7/", score->client) : "";

			if ( score->referee ) {
				Com_sprintf(string, sizeof(string),
					"R%4i %s %4i %4i %s%s", score->score, pingstr, score->time, score->timeFrozen, num, ci->name);
			}
			else {
				//freeze
				Com_sprintf(string, sizeof(string),
					"%5i %s %4i %4i %s%s", score->score, pingstr, score->time, score->timeFrozen, num, ci->name);
			}
		}

		// highlight your position
		if ( score->client == cg.snap->ps.clientNum ) {
			float	hcolor[4];
			int		rank;

			localClient = qtrue;

			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR 
				|| cgs.gametype >= GT_TEAM ) {
					rank = -1;
				} else {
					rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
				}
				if ( rank == 0 ) {
					hcolor[0] = 0;
					hcolor[1] = 0;
					hcolor[2] = 0.7f;
				} else if ( rank == 1 ) {
					hcolor[0] = 0.7f;
					hcolor[1] = 0;
					hcolor[2] = 0;
				} else if ( rank == 2 ) {
					hcolor[0] = 0.7f;
					hcolor[1] = 0.7f;
					hcolor[2] = 0;
				} else {
					hcolor[0] = 0.7f;
					hcolor[1] = 0.7f;
					hcolor[2] = 0.7f;
				}

				hcolor[3] = fade * 0.7;
				CG_FillRect( x + SCORECHAR_WIDTH , y, 
					320 - SCORECHAR_WIDTH, BIGCHAR_HEIGHT, hcolor );
		}

		cursorY = cgs.cursorY;
		if ( cursorY < SB_TOP + SB_INTER_HEIGHT ) {
			cursorY = SB_TOP + SB_INTER_HEIGHT;
		}
		else if (cursorY >= SB_STATUSBAR) {
			cursorY = SB_STATUSBAR - 1;
		}

		//freeze
		if ( cg.pointClickScores && cursorY >= score->topLine && cursorY < score->bottomLine &&
			ci->infoValid && ci->team != TEAM_SPECTATOR &&
			(ci->team == cg.snap->ps.stats[STAT_TEAM] || cg.snap->ps.stats[STAT_TEAM] == TEAM_SPECTATOR) ) {
				float	hcolor[4];

				hcolor[0] = 0.5f;
				hcolor[1] = 0.0f;
				hcolor[2] = 0.5f;
				hcolor[3] = fade * 0.5;

				CG_FillRect( x + SCORECHAR_WIDTH , y, 
					320 - x - SCORECHAR_WIDTH, BIGCHAR_HEIGHT, hcolor );

				if ( score->client != cg.lastClientHighlighted ) {
					trap_S_StartLocalSound( cgs.media.menu_move_sound, CHAN_AUTO );
					cg.lastClientHighlighted = score->client;
				}

				cg.highlightedClient = qtrue;
			}
			//freeze

			CG_DrawScoreboardString( x , y, string, fade );

			//freeze
			readyMask[0] = cg.snap->ps.stats[ STAT_CLIENTS_READY0 ];
			readyMask[1] = cg.snap->ps.stats[ STAT_CLIENTS_READY1 ];
			readyMask[2] = cg.snap->ps.stats[ STAT_CLIENTS_READY2 ];
			readyMask[3] = cg.snap->ps.stats[ STAT_CLIENTS_READY3 ];

			// add the "ready" marker for intermission exiting
			if ( readyMask[score->client / 16] & ( 1 << (score->client % 16) ) ) {
				if ( cg.warmup || cg.predictedPlayerState.pm_type == PM_INTERMISSION )
					CG_DrawScoreboardString( iconx-10+x, y, "READY", fade );
			}
			//freeze
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight ) {
	int		i,x;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;

	x = 0;
	if ( team == TEAM_BLUE || team == TEAM_BLUECOACH) x = 320;

	if ( team == TEAM_BLUE || team == TEAM_RED ){
		//draw the 'headers'
		color[0] = 1.0; color[1] = 0.65; color[2] = 0.0;
		CG_DrawScoreboardSmallString( x+10, y, "Score", color);

		color[0] = 0.0; color[1] = 1.0; color[2] = 1.0;  //ltblue		
		CG_DrawScoreboardSmallerString(x + 50, y-7, "Thw", color );

		color[0] = 0.67; color[1] = 0.67; color[2] = 0.67;  //dk.grey
		CG_DrawScoreboardSmallerString(x + 50, y, "Dth", color );

		color[0] = 1; color[1] = 0.65; color[2] = 0;  //orange
		CG_DrawScoreboardSmallerString(x + 70, y-7, "Tme", color );

		color[0] = 0; color[1] = 1; color[2] = 1;  //lt blue
		CG_DrawScoreboardSmallerString(x + 70, y, "Frz", color );

		color[0] = 1; color[1] = 1; color[2] = 1;  //white
		CG_DrawScoreboardSmallerString(x + 90, y, "Ping", color );

		color[0] = 1.0; color[1] = 1.0; color[2] = 1.0;
		CG_DrawScoreboardSmallString( x+116, y, "Name", color);

	}

	y += lineHeight;


	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}
		if(team == TEAM_RED || team == TEAM_BLUE)
			CG_DrawClientScore(x, y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );
		else
			CG_DrawCoachScore(x, y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );

		count++;
	}

	return count;
}

static void CG_DrawTeamScoreboardBackground( int x, int y, int w, int h, float alpha, int team ) {
	if ( y < SB_TOP ) {
		h -= SB_TOP - y;
		if ( h < 0 ) {
			return;
		}
		y = SB_TOP;
	}

	if ( y + h >= SB_STATUSBAR + SB_INTER_HEIGHT ) {
		h = SB_STATUSBAR + SB_INTER_HEIGHT - y;
		if ( h < 0 ) {
			return;
		}
	}

	CG_DrawTeamBackground( x, y, w, h, alpha, team );
}

/*sigh, q3 doesn't have a max function*/
static int _max(const int a,const int b)
{
	if(a > b) return a;
	return b;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2, n1a, n2a;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	int startY;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	//if ( c0g.warmup && !cg.showScores ) {
	//	return qfalse;
	//}

	if ( cg.showScores || cg.snap->ps.stats[STAT_FLAGS] & SF_FORCESCOREBOARD ||
		(cg.predictedPlayerState.pm_type == PM_DEAD && !cg.warmup) ||
		cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
			fade = 1.0;
			fadeColor = colorWhite;
		} else {
			fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

			if ( !fadeColor ) {
				// next time scoreboard comes up, don't print killer
				cg.deferredPlayerLoading = 0;
				cg.killerName[0] = 0;
				return qfalse;
			}
			fade = fadeColor[3]; //bugfix
		}

		// fragged by ... line
		if ( cg.killerName[0] ) {
			s = va("Fragged by %s", cg.killerName );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			y = 40;
			CG_DrawBigString( x, y, s, fade );
		}

		// current rank
		if ( cgs.gametype < GT_TEAM) {
			if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
				s = va("%s place with %i",
					CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
					cg.snap->ps.persistant[PERS_SCORE] );
				w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
				x = ( SCREEN_WIDTH - w ) / 2;
				y = 60;
				CG_DrawBigString( x, y, s, fade );
			}
		} else {
			if ( cg.teamScores[0] == cg.teamScores[1] ) {
				s = va("Teams are tied at %i", cg.teamScores[0] );
			} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
				s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
			} else {
				s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
			}

			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			y = 60;
			CG_DrawBigString( x, y, s, fade );
		}

		// scoreboard
		y = SB_HEADER;

		//	CG_DrawPic( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore );
		//	CG_DrawPic( SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing );
		//	CG_DrawPic( SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime );
		//freeze
		//	CG_DrawPic( SB_FROZEN_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardFrozen );
		//freeze
		//	CG_DrawPic( SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName );

		y = SB_HEADER+16;

		// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores

		maxClients = MAX_CLIENTS; //SB_MAXCLIENTS_INTER;
		lineHeight = 20;
		topBorderSize = 8;
		bottomBorderSize = 16;

		localClient = qfalse;
		cg.highlightedClient = qfalse;

		if ( cg.pointClickScores ) {
			y -= cgs.scrollY;
		}

		startY = y;

		if ( cgs.gametype >= GT_TEAM ) {
			//
			// teamplay scoreboard
			//
			//y += lineHeight/2;

			//if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			maxClients -= n1;
			n1a = CG_TeamScoreboard( y+(n1*lineHeight), TEAM_REDCOACH, fade, maxClients, lineHeight );
			maxClients -= n1a;
			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			maxClients -= n2;
			n2a = CG_TeamScoreboard( y+(n2*lineHeight), TEAM_BLUECOACH, fade, maxClients, lineHeight );
			maxClients -= n2a;

			CG_DrawTeamScoreboardBackground( 0, y - topBorderSize, 320, (_max(n2+n2a,n1+n1a)+1) * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_DrawTeamScoreboardBackground( 320, y - topBorderSize, 320, (_max(n2+n2a,n1+n1a)+1) * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );

			y += ((_max(n2+n2a,n1+n1a)+1) * lineHeight) + BIGCHAR_HEIGHT;

			//} else {
			//n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			//CG_DrawTeamScoreboardBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			//y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			//maxClients -= n1;
			//n2 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			//CG_DrawTeamScoreboardBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			//y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			//maxClients -= n2;
			//}
			n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

		} else {
			//
			// free for all scoreboard
			//
			n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
		}

		/*if (!localClient) {
			// draw local client at the bottom
			for ( i = 0 ; i < cg.numScores ; i++ ) {
				if ( cg.scores[i].client == cg.snap->ps.clientNum && cgs.clientinfo[cg.scores[i].client].infoValid ) {
					CG_DrawClientScore(80, y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
					break;
				}
			}
		}*/

		cg.scoreboardHeight = y - startY - BIGCHAR_HEIGHT - lineHeight / 2;

		if ( !cg.highlightedClient ) {
			cg.lastClientHighlighted = -1;
		}

		//freeze
		// if doing point-and-click scoreboarding
		if ( cg.pointClickScores ) {
			// draw the scoreboard crosshair
			CG_DrawScoreboardCrosshair();
		}
		//freeze

		// load any models that have been deferred
		if ( ++cg.deferredPlayerLoading > 10 ) {
			CG_LoadDeferredPlayers();
		}

		return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float		x;
	vec4_t		color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard( void ) {
	const char		*s;
	vec4_t			color;
	int				min, tens, ones;
	clientInfo_t	*ci;
	int				y;
	int				i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
	} else {
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			s = va("%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			y += 64;
		}
	}


}


#include "g_local.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"

#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "inv.h"

#include "chars.h"

int BotTeamFindCorpsicle( bot_state_t *bs ) {
	int	i;
	//float	vis, f;
	gentity_t	*ent, *corpse;
	float	alertness;
	float	squaredist;
	vec3_t	dir;
	int	areanum, t;
	int best = -1, bestt = 0;

	alertness = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_ALERTNESS, 0, 1 );

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( i == bs->client ) continue;
		ent = &g_entities[ i ];
		// if not valid, no corpse, or not frozen, continue
		if ( !ent->inuse || !ent->target_ent || !ent->freezeState ) continue;

		// if not on the bot's team, skip
		if ( !BotSameTeam( bs, i ) ) continue;

		corpse = ent->target_ent;

		// if invalid or being thawed by someone else
		if ( !corpse->inuse || (corpse->count && corpse->unfreezer->s.number != bs->client) ) continue;

		// this should be a frozen corpse on the bot's team by now

		VectorSubtract( corpse->r.currentOrigin, bs->origin, dir );

		// check his distance against his alertness
		squaredist = VectorLengthSquared( dir );
		if ( squaredist > Square( 900.0 + alertness * 4000.0 ) ) continue;

/*
		// check visibility
		f = 90 + 90 - ( 90 - ( squaredist > Square( 810 ) ? Square( 810 ) : squaredist ) / ( 810 * 9 ) );
		vis = BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, f, corpse->s.number );
		if ( vis <= 0 ) continue;
*/
		// check potentially visible set instead
		// (it's a little like ESP this way, but more realistic)
		if ( !trap_InPVS(bs->eye, corpse->r.currentOrigin) ) continue;

		// find out if the bot can actually get there
		areanum = BotPointAreaNum( corpse->r.currentOrigin );
		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl );
		if ( t != 0 ) {
			if ( t < bestt || bestt == 0 ) {
				bestt = t;
				best = corpse->s.number;
			}
		}
	}

	return best;
}

void BotRefuseOrder( bot_state_t *bs );

void BotTeamSeekGoals( bot_state_t *bs ) {
	aas_entityinfo_t	entinfo;
	int	c;
	int	areanum;
	gentity_t *ent;

	// if accompanying without orders (in other words, thawing a teammate)
	if ( bs->ltgtype == LTG_TEAMACCOMPANY && !bs->ordered ) {
		BotEntityInfo( bs->teammate, &entinfo );
		// if the thawed entity is invalid or is no longer frozen
		if ( !entinfo.valid || !( entinfo.powerups & ( 1 << PW_BATTLESUIT ) ) ) {
			// stop accompanying
			bs->ltgtype = 0;
			return;
		}

		ent = &g_entities[entinfo.number];
		// if there's no associated corpsicle or the corpsicle is being thawed by someone else
		if ( !ent->target_ent || (ent->count && ent->unfreezer->s.number != bs->client) ) {
			// stop accompanying
			bs->ltgtype = 0;
			return;
		}

		areanum = BotPointAreaNum( entinfo.origin );
		// if the target is not reachable
		if ( !trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl ) ) {
			// stop accompanying
			bs->ltgtype = 0;
		}
		return;
	}

	// if it's time for the bot to make its own decision
	if ( 1 /*bs->owndecision_time < FloatTime()*/ ) {
		// see if there's a frozen teammate nearby
		c = BotTeamFindCorpsicle( bs );
		// if there is and this bot is not already trying to thaw him
		if ( c >= 0 && ( bs->ltgtype != LTG_TEAMACCOMPANY || bs->teammate != c || bs->ordered ) ) {
			// stop taking orders, and start accompanying the frozen player
			BotRefuseOrder( bs );
			bs->decisionmaker = bs->client;
			bs->ordered = qfalse;
			bs->teammate = c;
			bs->teammatevisible_time = FloatTime();
			bs->teammessage_time = 0;
			bs->arrive_time = 1;
			bs->teamgoal_time = FloatTime() + 120;
			bs->ltgtype = LTG_TEAMACCOMPANY;
			bs->formation_dist = 70;
			BotSetTeamStatus( bs );
			// decide again in 5 seconds
			bs->owndecision_time = FloatTime() + 5;
			return;
		}
	}
#if 0
	if ( BotTeamLeader( bs ) ) {
		return;
	}
	if ( bs->lastgoal_ltgtype ) {
		bs->teamgoal_time += 60;
	}
	if ( !bs->ordered && bs->lastgoal_ltgtype ) {
		bs->ltgtype = 0;
	}
	if ( bs->ltgtype == LTG_TEAMHELP ||
		bs->ltgtype == LTG_TEAMACCOMPANY ||
		bs->ltgtype == LTG_DEFENDKEYAREA ||
		bs->ltgtype == LTG_CAMPORDER ||
		bs->ltgtype == LTG_PATROL ||
		bs->ltgtype == LTG_GETITEM ) {
		return;
	}
	if ( BotSetLastOrderedTask( bs ) ) {
		return;
	}
	if ( bs->owndecision_time > FloatTime() ) {
		return;
	}
	if ( bs->ctfroam_time > FloatTime() ) {
		return;
	}
	if ( BotAggression( bs ) < 50 ) {
		return;
	}
	bs->teammessage_time = FloatTime() + 2 * random();

	bs->ltgtype = 0;
	bs->ctfroam_time = FloatTime() + CTF_ROAM_TIME;
	BotSetTeamStatus( bs );
	bs->owndecision_time = FloatTime() + 5;
#endif
}

int BotNumTeamFrozen(bot_state_t *bs) {
	gentity_t *bot;
	gentity_t *iter;
	int numFrozen;

	bot = &g_entities[bs->client];

	if (bot->client->sess.sessionTeam != TEAM_RED &&
			bot->client->sess.sessionTeam != TEAM_BLUE) {
		return qfalse;
	}

	numFrozen = 0;
	for (iter = g_entities; iter < &g_entities[level.maxclients]; iter++) {
		if (!iter->inuse || !iter->client || !BotSameTeam(bs, iter->s.number) || !IsClientFrozen(iter)) {
			continue;
		}

		numFrozen++;
	}

	return numFrozen;
}

qboolean BotHalfTeamIsOOC(bot_state_t *bs) {
	gentity_t *bot;
	gentity_t *iter;
	int numOOC, numTeam;

	bot = &g_entities[bs->client];

	if (bot->client->sess.sessionTeam != TEAM_RED &&
			bot->client->sess.sessionTeam != TEAM_BLUE) {
		return qfalse;
	}

	// count the number of players on this bot's team, and how many are
	// out of commission (frozen, connecting, or not respawned yet)
	numOOC = numTeam = 0;
	for (iter = g_entities; iter < &g_entities[level.maxclients]; iter++) {
		if (!iter->inuse || !iter->client || !BotSameTeam(bs, iter->s.number)) {
			continue;
		}

		numTeam++;

		if (IsClientFrozen(iter) || IsClientOOC(iter)) {
			numOOC++;
		}
	}

	if (numOOC >= numTeam / 2) {
		return qtrue;
	}

	return qfalse;
}

/*
===================
BotBattleUseGrapple
===================
*/
void BotBattleUseGrapple(bot_state_t *bs) {
	gentity_t *ent = &g_entities[bs->entitynum];
	gentity_t *traceent;
	vec3_t delta, start, end;
	vec3_t mins = { -8.0f, -8.0f, -8.0f };
	vec3_t maxs = { 8.0f, 8.0f, 8.0f };
	trace_t trace;
	float grappler, attack_skill, grapples, rnd;

	if ( !g_grapple.integer ) {
		bs->grapple = qfalse;
		return;
	}

	if ( ent->client->ps.pm_flags & PMF_GRAPPLE_FLY ) {
		VectorCopy( ent->client->ps.grappleVelocity, delta );
		VectorNormalize( delta );
		VectorMA( ent->client->ps.grapplePoint, 8192.0f, delta, end );

		trap_Trace( &trace, ent->client->ps.grapplePoint, NULL, NULL, end, ent->s.number, MASK_SHOT );

		if ( trace.fraction == 1.0f || (trace.surfaceFlags & SURF_NOIMPACT) ) {
			//G_Printf("^1already grappling: didn't hit geo\n");
			bs->grapple = qfalse;
			return;
		}

		VectorCopy( trace.endpos, start );
		VectorMA( start, 15.0f, trace.plane.normal, start );
		VectorCopy( start, end );
		end[2] -= 8192.0f;
		
		trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER );
		traceent = &g_entities[trace.entityNum];		

		if ( trace.fraction == 1.0f || (traceent->r.contents & CONTENTS_TRIGGER &&
				traceent->touch != trigger_push_touch && traceent->touch != trigger_teleporter_touch) ) {
			//G_Printf("^3already grappling: bot would fall into void\n");
			bs->grapple = qfalse;
			return;
		}

		bs->grapple = qtrue;
		bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
		return;
	}
	else if ( ent->client->ps.pm_flags & PMF_GRAPPLE_PULL ) {
		VectorSubtract( ent->client->ps.grapplePoint, ent->client->ps.origin, delta );
		if ( VectorLength(delta) > 64.0f && VectorLength(ent->client->ps.velocity) > g_speed.value ) {
			bs->grapple = qtrue;
			bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
		}
		else {
			bs->grapple = qfalse;
		}

		return;
	}

	// if the bot has no enemy, forget it
	if ( bs->enemy < 0 ) {
		bs->grapple = qfalse;
		return;
	}

	// if the bot wants to be sneaky, forget it
	if ( bs->sneaky ) {
		bs->grapple = qfalse;
		return;
	}

	grappler = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_GRAPPLE_USER, 0, 1);
	attack_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
	grapples = Q_max(grappler, attack_skill) * 10 * bot_grapplemonkey.value;
	rnd = random() * 100;

	if ( grapples > rnd ) {
		//G_Printf("going to grapple: ");

		VectorCopy( ent->client->ps.velocity, delta );
		VectorNormalize( delta );	
		VectorMA( ent->client->ps.origin, 8192.0f, delta, end );

		trap_Trace( &trace, ent->client->ps.origin, mins, maxs, end, ent->s.number, MASK_SHOT );

		if ( trace.fraction == 1.0f || (trace.surfaceFlags & SURF_NOIMPACT) || trace.fraction * 8192.0f < 256.0f ) {
			//G_Printf("^1didn't hit geo\n");
			bs->grapple = qfalse;
			return;
		}

		VectorCopy( trace.endpos, start );
		VectorMA( start, 15.0f, trace.plane.normal, start );
		VectorCopy( start, end );
		end[2] -= 8192.0f;
		
		trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER );
		traceent = &g_entities[trace.entityNum];		

		if ( trace.fraction == 1.0f || (traceent->r.contents & CONTENTS_TRIGGER &&
				traceent->touch != trigger_push_touch && traceent->touch != trigger_teleporter_touch) ) {
			//G_Printf("^3bot would fall into void\n");
			bs->grapple = qfalse;
			return;
		}

		bs->grapple = qtrue;
		bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
	}
}

/*
=================
BotGoalUseGrapple
=================
*/
void BotGoalUseGrapple(bot_state_t *bs) {
	gentity_t *ent = &g_entities[bs->entitynum];
	gentity_t *traceent;
	vec3_t delta, start, end;
	vec3_t mins = { -8.0f, -8.0f, -8.0f };
	vec3_t maxs = { 8.0f, 8.0f, 8.0f };
	bot_goal_t goal;
	trace_t trace;
	float grappler, attack_skill, grapples, rnd, goaldist, intdist;

	if ( ent->client->ps.pm_flags & PMF_GRAPPLE_FLY ) {
		VectorCopy( ent->client->ps.grappleVelocity, delta );
		VectorNormalize( delta );
		VectorMA( ent->client->ps.grapplePoint, 8192.0f, delta, end );

		trap_Trace( &trace, ent->client->ps.grapplePoint, NULL, NULL, end, ent->s.number, MASK_SHOT );

		if ( trace.fraction == 1.0f || (trace.surfaceFlags & SURF_NOIMPACT) ) {
			//G_Printf("^1already grappling: didn't hit geo\n");
			bs->grapple = qfalse;
			return;
		}

		VectorCopy( trace.endpos, start );
		VectorMA( start, 15.0f, trace.plane.normal, start );
		VectorCopy( start, end );
		end[2] -= 8192.0f;
		
		trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER );
		traceent = &g_entities[trace.entityNum];		

		if ( trace.fraction == 1.0f || (traceent->r.contents & CONTENTS_TRIGGER &&
				traceent->touch != trigger_push_touch && traceent->touch != trigger_teleporter_touch) ) {
			//G_Printf("^4already grappling: bot would fall into void\n");
			bs->grapple = qfalse;
			return;
		}

		bs->grapple = qtrue;
		bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
		return;
	}
	else if ( ent->client->ps.pm_flags & PMF_GRAPPLE_PULL ) {
		VectorSubtract( ent->client->ps.grapplePoint, ent->client->ps.origin, delta );
		if ( VectorLength(delta) > 64.0f && VectorLength(ent->client->ps.velocity) > g_speed.value ) {
			bs->grapple = qtrue;
			bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
		}
		else {
			bs->grapple = qfalse;
		}

		return;
	}

	// if the bot wants to be sneaky, forget it
	if (bs->sneaky) {
		bs->grapple = qfalse;
		return;
	}

	if (!trap_BotGetTopGoal(bs->gs, &goal)) {
		// if the bot has no goal, forget it
		bs->grapple = qfalse;
		return;
	}

	VectorSubtract( goal.origin, ent->client->ps.origin, delta );
	goaldist = VectorLength(delta);
	if ( goaldist < 160.f ) {
		// if the bot could just as easily walk, forget it
		bs->grapple = qfalse;
		return;
	}

	grappler = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_GRAPPLE_USER, 0, 1);
	attack_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
	grapples = Q_max(grappler, attack_skill) * 50 * bot_grapplemonkey.value;
	rnd = random() * 100;

	if ( grapples > rnd ) {
		//G_Printf("going to grapple: ");

		VectorCopy( ent->client->ps.velocity, delta );
		VectorNormalize( delta );	
		VectorMA( ent->client->ps.origin, 8192.0f, delta, end );

		trap_Trace( &trace, ent->client->ps.origin, mins, maxs, end, ent->s.number, MASK_SHOT );

		if ( trace.fraction == 1.0f || (trace.surfaceFlags & SURF_NOIMPACT) || trace.fraction * 8192.0f < 80.0f ) {
			//G_Printf("^1didn't hit geo\n");
			bs->grapple = qfalse;
			return;
		}

		VectorSubtract( trace.endpos, goal.origin, delta );
		intdist = VectorLength(delta);
		if ( intdist > goaldist || intdist > 160.0f ) {
			// if the endpoint would move the bot further from the goal, forget it
			//G_Printf("^2takes bot further\n");
			bs->grapple = qfalse;
			return;
		}

		VectorCopy( trace.endpos, start );
		VectorMA( start, 15.0f, trace.plane.normal, start );
		VectorCopy( goal.origin, end );
		end[2] += 15.0f;
		
		trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SHOT );

		if ( trace.fraction < 1.0f ) {
			//G_Printf("^3geo in the way from endpoint to goal\n");
			bs->grapple = qfalse;
			return;
		}

		VectorCopy( start, end );
		end[2] -= 8192.0f;

		trap_Trace( &trace, start, NULL, NULL, end, ent->s.number, MASK_SHOT | CONTENTS_TRIGGER );
		traceent = &g_entities[trace.entityNum];		

		if ( trace.fraction == 1.0f || (traceent->r.contents & CONTENTS_TRIGGER &&
				traceent->touch != trigger_push_touch && traceent->touch != trigger_teleporter_touch) ) {
			//G_Printf("^4bot would fall into void\n");
			bs->grapple = qfalse;
			return;
		}

		//G_Printf("^5grappling\n");
		bs->grapple = qtrue;
		bs->tfl &= ~(TFL_ROCKETJUMP|TFL_BFGJUMP);
	}
}

qboolean BotVisibleEnemySeer(bot_state_t *bs) {
	float vis;
	int i;
	aas_entityinfo_t entinfo;

	for (i = 0; i < MAX_CLIENTS; i++) {
		vec3_t delta, dir, deltadir;

		if (i == bs->client) continue;
		//
		BotEntityInfo(i, &entinfo);
		//
		if (!entinfo.valid) continue;
		//if the enemy isn't dead and the enemy isn't the bot self
		if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) continue;
		//if the enemy is not a seer
		if (!(entinfo.powerups & (1 << PW_SIGHT))) {
			continue;
		}
		//if on the same team
		if (BotSameTeam(bs, i)) continue;

		//check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
		if (vis <= 0) continue;

		AngleVectors(entinfo.angles, dir, NULL, NULL);

		VectorSubtract(entinfo.origin, bs->origin, delta);
		VectorNormalize2(delta, deltadir);

		// if the enemy is facing away
		if (DotProduct(deltadir, dir) > 0.0f) {
			// don't count him as visible
			continue;				
		}

		return qtrue;
	}
	return qfalse;
}

qboolean BotVisibleEnemyFlagCarrier(bot_state_t *bs) {
	float vis;
	int i;
	aas_entityinfo_t entinfo;

	for (i = 0; i < MAX_CLIENTS; i++) {
		vec3_t delta, dir, deltadir;

		if (i == bs->client) continue;
		//
		BotEntityInfo(i, &entinfo);
		//
		if (!entinfo.valid) continue;
		//if the enemy isn't dead and the enemy isn't the bot self
		if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) continue;
		//if the enemy is not a seer
		if (!(entinfo.powerups & (1 << PW_REDFLAG)) && !(entinfo.powerups & (1 << PW_BLUEFLAG))) {
			continue;
		}
		//if on the same team
		if (BotSameTeam(bs, i)) continue;

		//check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
		if (vis <= 0) continue;

		AngleVectors(entinfo.angles, dir, NULL, NULL);

		VectorSubtract(entinfo.origin, bs->origin, delta);
		VectorNormalize2(delta, deltadir);

		// if the enemy is facing away
		if (DotProduct(deltadir, dir) > 0.0f) {
			// don't count him as visible
			continue;				
		}

		return qtrue;
	}
	return qfalse;
}

void BotBattleSneak(bot_state_t *bs) {
	// if the bot has no enemy, do a goal sneak instead
	if (bs->enemy < 0) {
		BotGoalSneak(bs);
		return;
	}

	if (!bs->inventory[INVENTORY_INVISIBILITY] || bs->inventory[INVENTORY_SIGHT] ||
			bs->inventory[INVENTORY_REDFLAG] || bs->inventory[INVENTORY_BLUEFLAG]) {
		bs->sneaky = qfalse;
		bs->sneaktime = 0.0f;
		return;
	}

	if (bs->sneaky) {
		if (BotVisibleEnemySeer(bs) || BotVisibleEnemyFlagCarrier(bs)) {
			bs->sneaky = qfalse;
			bs->sneaktime = FloatTime();
		}
		else {
			bs->stopfighting = qtrue;
		}
	}
	else {
		if (BotVisibleEnemySeer(bs) || BotVisibleEnemyFlagCarrier(bs)) {
			bs->sneaky = qfalse;
			bs->sneaktime = FloatTime();
		}
		else if (FloatTime() - bs->sneaktime > BotAggression(bs) * 10.0f / 100.0f) {
			bs->sneaky = qtrue;
			bs->sneaktime = FloatTime();
			bs->stopfighting = qtrue;
		}
	}
}

void BotGoalSneak(bot_state_t *bs) {
	if (!bs->inventory[INVENTORY_INVISIBILITY] || bs->inventory[INVENTORY_SIGHT] ||
			bs->inventory[INVENTORY_REDFLAG] || bs->inventory[INVENTORY_BLUEFLAG]) {
		bs->sneaky = qfalse;
		bs->sneaktime = 0.0f;
		return;
	}

	if (bs->sneaky) {
		if (BotVisibleEnemySeer(bs) || BotVisibleEnemyFlagCarrier(bs)) {
			bs->sneaky = qfalse;
			bs->sneaktime = FloatTime();
		}
		else if (!BotVisibleEnemies(bs)) {
			bs->sneaky = qfalse;
			bs->sneaktime = FloatTime();
		}
	}
	else {
		if (BotVisibleEnemySeer(bs) || BotVisibleEnemyFlagCarrier(bs)) {
			bs->sneaky = qfalse;
			bs->sneaktime = FloatTime();
		}
		else if (BotVisibleEnemies(bs)) {
			bs->sneaky = qtrue;
			bs->sneaktime = FloatTime();
		}
		else if (FloatTime() - bs->sneaktime > 3.0f) {
			bs->sneaky = qtrue;
			bs->sneaktime = FloatTime();
		}
	}
}

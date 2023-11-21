#include "g_local.h"
#include "g_ssdemo.h"

#ifdef HUMAN_READABLE_DEMOS
static const char *g_humanDirs[DIR_NUMDIRECTIVES] = {
	"bf",
	"cd",
	"ed",
	"ef",
	"dh",
	"cs",
	"sc",
};

static demoDirective_t G_GetDirectiveForString( const char *dir ) {
	int i;

	for ( i = 0; i < DIR_NUMDIRECTIVES; i++ ) {
		if ( strcmp(dir, g_humanDirs[i]) == 0 ) {
			return i;
		}
	}

	return DIR_UNKNOWN;
}

static const char *G_GetStringForDirective( demoDirective_t dir ) {
	return g_humanDirs[dir];
}
#endif

/*
G_AdjustEntityNum

Adjusts an entity number (or client number) to be in the range 32..63 if
in the range 0..31.

Player slots 0..31 are reserved for legit clients during demo playback,
so we have to adjust all client and entity numbers up a bit to keep
them from colliding.
*/
static void G_AdjustEntityNum( int *num ) {
	if ( *num >= 0 && *num < HALF_CLIENTS ) *num += HALF_CLIENTS;
}

/*
G_AdjustedEntityNum

Same as above.
*/
static int G_AdjustedEntityNum( int num ) {
	if ( num >= 0 && num < HALF_CLIENTS ) return num + HALF_CLIENTS;

	return num;
}

/*
G_AdjustTime

Changes a demo time to a level time.
*/
static void G_AdjustTime( int *time ) {
	*time -= level.demoTimeDiff;
}

/*
G_AdjustedTime

Same as above.
*/
static int G_AdjustedTime( int time ) {
	return time - level.demoTimeDiff;
}

typedef struct {
	// always change
	int commandTime; // 32 bits

	// high degree of change
	vec3_s origin;
	vec3_s velocity;
	vec3_s viewangles;
	char bobCycle; // 8 bits
	short weaponTime; // 16 bits
	//char pmove_framecount; // 6 bits - not needed?

	// medium-high degree of change
	vec3_s grapplePoint;
	int eventSequence; // 32 bits
	short events[MAX_PS_EVENTS]; // 10 bits each
	char eventParms[MAX_PS_EVENTS]; // 8 bits each
	int entityEventSequence; // 32 bits
	short ammo[WP_NUM_WEAPONS - 1]; // 16 bits each
	char movementDir; // 3 bits

	// medium degree of change
	char legsAnim; // 8 bits
	char torsoAnim; // 8 bits
	short eFlags; // 16 bits
	short pm_time; // 16 bits
	short pm_flags; // 16 bits
	short grappleTime; // 16 bits
	short grappleEntity; // 10 bits
	char weapon; // 4 bits
	char weaponstate; // 2 bits
	char damageEvent; // 8 bits
	char damageYaw; // 8 bits
	char damagePitch; // 8 bits
	char damageCount; // 8 bits
	short stats[STAT_NUM_SSDEMO_STATS]; // 16 bits each
	short persistant[PERS_NUM_PERSISTANT];	// 16 bits each
	vec3_s grappleVelocity;

	// medium-low degree of change
	short groundEntityNum; // 10 bits
	short delta_angles[3]; // 16 bits
	short legsTimer; // 16 bits
	short torsoTimer; // 16 bits
	short externalEvent; // 10 bits
	char externalEventParm; // 8 bits
	int externalEventTime; // 32 bits
	short event; // 10 bits
	char eventParm; // 8 bits
	//char jumppad_frame; // 6 bits - not needed?

	// low degree of change
	char viewheight; // 8 bits
	int powerups[PW_NUM_POWERUPS - 1]; // 32 bits each
	int generic1; // 32 bits
	short loopSound; // 16 bits
	int enterTime; // 32 bits
	int respawnTime; // 32 bits
	vec3_s origin2; // from entityState_t
	vec3_s mins, maxs; // from entityShared_t
	short target_ent; // 10 bits - for following frozen players
	short jumppad_ent; // 10 bits
	char freezeState; // 1 bit - for following frozen players
	char pm_type; // 3 bits

	// almost never change
	char clientNum; // 6 bits
	char connected; // 2 bits
	short gravity; // 16 bits
	short speed; // 16 bits
	char netname[MAX_NETNAME];
} clientData_t;

static void G_SerializeClientData( clientData_t *cd, char *ret, int *size ) {
	void *curr = ret;
	int pack;

	// always change
	SERIALIZE_INT(cd->commandTime, curr);

	// high degree of change
	SERIALIZE_VEC3S(cd->origin, curr);
	SERIALIZE_VEC3S(cd->velocity, curr);
	SERIALIZE_VEC3S(cd->viewangles, curr);
	SERIALIZE_CHAR(cd->bobCycle, curr);
	SERIALIZE_SHORT(cd->weaponTime, curr);
	//SERIALIZE_CHAR(cd->pmove_framecount, curr); // not needed?

	// medium-high degree of change
	SERIALIZE_VEC3S(cd->grapplePoint, curr);
	SERIALIZE_INT(cd->eventSequence, curr);

	pack = (cd->events[0] & 1023) | ((cd->events[1] & 1023) << 10);
	SERIALIZE_CHARARRAY((char *)&pack, 3, curr);

	SERIALIZE_CHAR(cd->eventParms[0], curr);
	SERIALIZE_CHAR(cd->eventParms[1], curr);
	SERIALIZE_INT(cd->entityEventSequence, curr);
	SERIALIZE_SHORTARRAY(cd->ammo, WP_NUM_WEAPONS - 1, curr);
	SERIALIZE_CHAR(cd->movementDir, curr);

	// medium degree of change
	SERIALIZE_CHAR(cd->legsAnim, curr);
	SERIALIZE_CHAR(cd->torsoAnim, curr);
	SERIALIZE_SHORT(cd->eFlags, curr);
	SERIALIZE_SHORT(cd->pm_time, curr);
	SERIALIZE_SHORT(cd->pm_flags, curr);
	SERIALIZE_SHORT(cd->grappleTime, curr);

	pack = (cd->grappleEntity & 1023) | ((cd->weapon & 15) << 10) | ((cd->weaponstate & 3) << 14);
	SERIALIZE_SHORT((short)pack, curr);

	SERIALIZE_CHAR(cd->damageEvent, curr);
	SERIALIZE_CHAR(cd->damageYaw, curr);
	SERIALIZE_CHAR(cd->damagePitch, curr);
	SERIALIZE_CHAR(cd->damageCount, curr);
	SERIALIZE_SHORTARRAY(cd->stats, STAT_NUM_SSDEMO_STATS, curr);
	SERIALIZE_SHORTARRAY(cd->persistant, PERS_NUM_PERSISTANT, curr);
	SERIALIZE_VEC3S(cd->grappleVelocity, curr);

	// medium-low degree of change
	SERIALIZE_SHORT(cd->groundEntityNum, curr);
	SERIALIZE_SHORTARRAY(cd->delta_angles, 3, curr);
	SERIALIZE_SHORT(cd->legsTimer, curr);
	SERIALIZE_SHORT(cd->torsoTimer, curr);
	SERIALIZE_SHORT(cd->externalEvent, curr);
	SERIALIZE_CHAR(cd->externalEventParm, curr);
	SERIALIZE_INT(cd->externalEventTime, curr);
	SERIALIZE_SHORT(cd->event, curr);
	SERIALIZE_CHAR(cd->eventParm, curr);
	//SERIALIZE_CHAR(cd->jumppad_frame, curr); // not needed?

	// low degree of change
	SERIALIZE_CHAR(cd->viewheight, curr);
	SERIALIZE_INTARRAY(cd->powerups, PW_NUM_POWERUPS - 1, curr);
	SERIALIZE_INT(cd->generic1, curr);
	SERIALIZE_SHORT(cd->loopSound, curr);
	SERIALIZE_INT(cd->enterTime, curr);
	SERIALIZE_INT(cd->respawnTime, curr);
	SERIALIZE_VEC3S(cd->origin2, curr);
	SERIALIZE_VEC3S(cd->mins, curr);
	SERIALIZE_VEC3S(cd->maxs, curr);

	pack = (cd->target_ent & 1023) | ((cd->jumppad_ent & 1023) << 10) | ((cd->freezeState != 0) << 20) | ((cd->pm_type & 7) << 21);
	SERIALIZE_CHARARRAY((char *)&pack, 3, curr);

	// almost never change
	pack = (cd->clientNum & 63) | ((cd->connected & 3) << 6);
	SERIALIZE_CHAR((char)pack, curr);

	SERIALIZE_SHORT(cd->gravity, curr);
	SERIALIZE_SHORT(cd->speed, curr);
	SERIALIZE_CHARARRAY(cd->netname, MAX_NETNAME, curr);

	*size = (char *)curr - ret;
}

static void G_UnserializeClientData( clientData_t *cd, char *data ) {
	void *curr = data;
	int pack;

	// always change
	UNSERIALIZE_INT(cd->commandTime, curr);

	// high degree of change
	UNSERIALIZE_VEC3S(cd->origin, curr);
	UNSERIALIZE_VEC3S(cd->velocity, curr);
	UNSERIALIZE_VEC3S(cd->viewangles, curr);
	UNSERIALIZE_CHAR(cd->bobCycle, curr);
	UNSERIALIZE_SHORT(cd->weaponTime, curr);
	//UNSERIALIZE_CHAR(cd->pmove_framecount, curr); // not needed?

	// medium-high degree of change
	UNSERIALIZE_VEC3S(cd->grapplePoint, curr);
	UNSERIALIZE_INT(cd->eventSequence, curr);

	UNSERIALIZE_CHARARRAY((char *)&pack, 3, curr);
	cd->events[0] = pack & 1023;
	cd->events[1] = (pack >> 10) & 1023;

	UNSERIALIZE_CHAR(cd->eventParms[0], curr);
	UNSERIALIZE_CHAR(cd->eventParms[1], curr);
	UNSERIALIZE_INT(cd->entityEventSequence, curr);
	UNSERIALIZE_SHORTARRAY(cd->ammo, WP_NUM_WEAPONS - 1, curr);
	UNSERIALIZE_CHAR(cd->movementDir, curr);

	// medium degree of change
	UNSERIALIZE_CHAR(cd->legsAnim, curr);
	UNSERIALIZE_CHAR(cd->torsoAnim, curr);
	UNSERIALIZE_SHORT(cd->eFlags, curr);
	UNSERIALIZE_SHORT(cd->pm_time, curr);
	UNSERIALIZE_SHORT(cd->pm_flags, curr);
	UNSERIALIZE_SHORT(cd->grappleTime, curr);

	UNSERIALIZE_SHORT(pack, curr);
	cd->grappleEntity = pack & 1023;
	cd->weapon = (pack >> 10) & 15;
	cd->weaponstate = (pack >> 14) & 3;

	UNSERIALIZE_CHAR(cd->damageEvent, curr);
	UNSERIALIZE_CHAR(cd->damageYaw, curr);
	UNSERIALIZE_CHAR(cd->damagePitch, curr);
	UNSERIALIZE_CHAR(cd->damageCount, curr);
	UNSERIALIZE_SHORTARRAY(cd->stats, STAT_NUM_SSDEMO_STATS, curr);
	UNSERIALIZE_SHORTARRAY(cd->persistant, PERS_NUM_PERSISTANT, curr);
	UNSERIALIZE_VEC3S(cd->grappleVelocity, curr);

	// medium-low degree of change
	UNSERIALIZE_SHORT(cd->groundEntityNum, curr);
	UNSERIALIZE_SHORTARRAY(cd->delta_angles, 3, curr);
	UNSERIALIZE_SHORT(cd->legsTimer, curr);
	UNSERIALIZE_SHORT(cd->torsoTimer, curr);
	UNSERIALIZE_SHORT(cd->externalEvent, curr);
	UNSERIALIZE_CHAR(cd->externalEventParm, curr);
	UNSERIALIZE_INT(cd->externalEventTime, curr);
	UNSERIALIZE_SHORT(cd->event, curr);
	UNSERIALIZE_CHAR(cd->eventParm, curr);
	//UNSERIALIZE_CHAR(cd->jumppad_frame, curr); // not needed?

	// low degree of change
	UNSERIALIZE_CHAR(cd->viewheight, curr);
	UNSERIALIZE_INTARRAY(cd->powerups, PW_NUM_POWERUPS - 1, curr);
	UNSERIALIZE_INT(cd->generic1, curr);
	UNSERIALIZE_SHORT(cd->loopSound, curr);
	UNSERIALIZE_INT(cd->enterTime, curr);
	UNSERIALIZE_INT(cd->respawnTime, curr);
	UNSERIALIZE_VEC3S(cd->origin2, curr);
	UNSERIALIZE_VEC3S(cd->mins, curr);
	UNSERIALIZE_VEC3S(cd->maxs, curr);

	UNSERIALIZE_CHARARRAY((char *)&pack, 3, curr);
	cd->target_ent = pack & 1023;
	cd->jumppad_ent = (pack >> 10) & 1023;
	cd->freezeState = (pack >> 20) & 1;
	cd->pm_type = (pack >> 21) & 7;

	// almost never change
	UNSERIALIZE_CHAR(pack, curr);
	cd->clientNum = pack & 63;
	cd->connected = (pack >> 6) & 3;

	UNSERIALIZE_SHORT(cd->gravity, curr);
	UNSERIALIZE_SHORT(cd->speed, curr);
	UNSERIALIZE_CHARARRAY(cd->netname, MAX_NETNAME, curr);
}

static void G_AdjustClientForPlayback( gentity_t *iter ) {
	playerState_t *ps = &iter->client->ps;
	int i;

	// adjust entity numbers to shift all recorded clients >= HALF_CLIENTS
	G_AdjustEntityNum( &ps->clientNum );
	G_AdjustEntityNum( &ps->groundEntityNum );
	G_AdjustEntityNum( &ps->grappleEntity );
	G_AdjustEntityNum( &ps->jumppad_ent );
	G_AdjustEntityNum( &ps->persistant[PERS_ATTACKER] );

	// adjust times to reflect the real server time
	G_AdjustTime( &ps->commandTime );
	G_AdjustTime( &ps->externalEventTime );
	G_AdjustTime( &iter->client->respawnTime );
	G_AdjustTime( &iter->client->pers.enterTime );

	for ( i = PW_QUAD; i < PW_NUM_POWERUPS; i++ ) {
		if ( ps->powerups[i] != 0 ) {
			G_AdjustTime( &ps->powerups[i] );
		}
	}

	// fix up ready bits
	level.demoReadyMask[0] = ps->stats[STAT_CLIENTS_READY0];
	level.demoReadyMask[1] = ps->stats[STAT_CLIENTS_READY1];
	level.demoReadyMask[2] = ps->stats[STAT_CLIENTS_READY2];
	level.demoReadyMask[3] = ps->stats[STAT_CLIENTS_READY3];

	ps->stats[STAT_CLIENTS_READY2] = level.demoReadyMask[0];
	ps->stats[STAT_CLIENTS_READY3] = level.demoReadyMask[1];
	ps->stats[STAT_CLIENTS_READY0] = 0;
	ps->stats[STAT_CLIENTS_READY1] = 0;

	// fix up the team and health
	iter->client->sess.sessionTeam = ps->stats[STAT_TEAM];
	iter->health = ps->stats[STAT_HEALTH];
}

static void G_AdjustEntityForPlayback( gentity_t *iter ) {
	entityState_t *s = &iter->s;

	// adjust entity numbers to shift all recorded clients >= HALF_CLIENTS
	G_AdjustEntityNum( &s->number );

	// don't adjust for score plums because they abuse otherEntityNum
	if ( s->eType != ET_EVENTS + EV_SCOREPLUM ) {
		G_AdjustEntityNum( &s->otherEntityNum );
	}

	G_AdjustEntityNum( &s->otherEntityNum2 );
	G_AdjustEntityNum( &s->groundEntityNum );

	// don't adjust for speakers or portal cameras because
	// they abuse the clientNum field with non-client-number data
	if ( s->eType != ET_SPEAKER && 
			(iter->classname == NULL || strcmp( iter->classname, "misc_portal_camera" ) != 0) ) {
		G_AdjustEntityNum( &iter->s.clientNum );
	}

	G_AdjustEntityNum( &iter->r.ownerNum );

	G_AdjustTime( &s->pos.trTime );
	G_AdjustTime( &s->apos.trTime );
	G_AdjustTime( &s->time );
	G_AdjustTime( &s->time2 );
}

static void G_StuffClientData( gentity_t *iter, clientData_t *cd ) {
	playerState_t *ps = &iter->client->ps;
	clientPersistant_t *pers = &iter->client->pers;
	int i;

	// always change
	cd->commandTime = ps->commandTime;

	// high degree of change
	Vec3ToShort( ps->origin, cd->origin, 1.0f );
	Vec3ToShort( ps->velocity, cd->velocity, 1.0f );
	Vec3ToShort( ps->viewangles, cd->viewangles, 4.0f );
	cd->bobCycle = ps->bobCycle;
	cd->weaponTime = ps->weaponTime;

	// medium-high degree of change
	cd->movementDir = ps->movementDir;
	Vec3ToShort( ps->grapplePoint, cd->grapplePoint, 1.0f );
	cd->eventSequence = ps->eventSequence;
	cd->events[0] = ps->events[0];
	cd->events[1] = ps->events[1];
	cd->eventParms[0] = ps->eventParms[0];
	cd->eventParms[1] = ps->eventParms[1];
	cd->entityEventSequence = ps->entityEventSequence;
	for ( i = 1; i < WP_NUM_WEAPONS; i++ ) {
		cd->ammo[i - 1] = ps->ammo[i];
	}
	
	// medium degree of change
	cd->legsAnim = ps->legsAnim;
	cd->torsoAnim = ps->torsoAnim;
	cd->eFlags = ps->eFlags;
	cd->pm_time = ps->pm_time;
	cd->pm_flags = ps->pm_flags;
	cd->weapon = ps->weapon;
	cd->weaponstate = ps->weaponstate;
	cd->damageEvent = ps->damageEvent;
	cd->damageYaw = ps->damageYaw;
	cd->damagePitch = ps->damagePitch;
	cd->damageCount = ps->damageCount;
	for ( i = 0; i < STAT_NUM_SSDEMO_STATS; i++ ) {
		cd->stats[i] = ps->stats[i];
	}
	for ( i = 0; i < PERS_NUM_PERSISTANT; i++ ) {
		cd->persistant[i] = ps->persistant[i];
	}
	Vec3ToShort( ps->grappleVelocity, cd->grappleVelocity, 4.0f );
	cd->grappleTime = ps->grappleTime;
	cd->grappleEntity = ps->grappleEntity;

	// medium-low degree of change
	cd->groundEntityNum = ps->groundEntityNum;
	cd->delta_angles[0] = ps->delta_angles[0];
	cd->delta_angles[1] = ps->delta_angles[1];
	cd->delta_angles[2] = ps->delta_angles[2];
	cd->legsTimer = ps->legsTimer;
	cd->torsoTimer = ps->torsoTimer;
	cd->externalEvent = ps->externalEvent;
	cd->externalEventParm = ps->externalEventParm;
	cd->externalEventTime = ps->externalEventTime;
	cd->event = iter->s.event;
	cd->eventParm = iter->s.eventParm;

	// low degree of change
	cd->pm_type = ps->pm_type;
	cd->viewheight = ps->viewheight;
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		cd->powerups[i - 1] = ps->powerups[i];
	}
	cd->generic1 = ps->generic1;
	cd->loopSound = ps->loopSound;
	cd->jumppad_ent = ps->jumppad_ent;
	cd->enterTime = pers->enterTime;
	cd->respawnTime = iter->client->respawnTime;
	Vec3ToShort( iter->s.origin2, cd->origin2, 1.0f );
	Vec3ToShort( iter->r.mins, cd->mins, 1.0f );
	Vec3ToShort( iter->r.maxs, cd->maxs, 1.0f );
	cd->freezeState = iter->freezeState;
	cd->target_ent = iter->target_ent - g_entities;

	// almost never change
	cd->clientNum = ps->clientNum;
	cd->gravity = ps->gravity;
	cd->speed = ps->speed;
	memset( cd->netname, 0, MAX_NETNAME );
	strcpy( cd->netname, pers->netname );
	cd->connected = pers->connected;
}

static void G_UnstuffClientData( gentity_t *iter, clientData_t *cd ) {
	playerState_t *ps = &iter->client->ps;
	clientPersistant_t *pers = &iter->client->pers;
	int i;

	// always change
	ps->commandTime = cd->commandTime;

	// high degree of change
	ShortToVec3( cd->origin, ps->origin, 1.0f );
	VectorCopy( ps->origin, iter->r.currentOrigin );
	ShortToVec3( cd->velocity, ps->velocity, 1.0f );
	ShortToVec3( cd->viewangles, ps->viewangles, 4.0f );
	ps->bobCycle = cd->bobCycle;
	ps->weaponTime = cd->weaponTime;

	// medium-high degree of change
	ps->movementDir = cd->movementDir;
	ShortToVec3( cd->grapplePoint, ps->grapplePoint, 1.0f );
	ps->eventSequence = cd->eventSequence;
	ps->events[0] = cd->events[0];
	ps->events[1] = cd->events[1];
	ps->eventParms[0] = cd->eventParms[0];
	ps->eventParms[1] = cd->eventParms[1];
	ps->entityEventSequence = cd->entityEventSequence;
	for ( i = 1; i < WP_NUM_WEAPONS; i++ ) {
		ps->ammo[i] = cd->ammo[i - 1];
	}
	
	// medium degree of change
	ps->legsAnim = cd->legsAnim;
	ps->torsoAnim = cd->torsoAnim;
	ps->eFlags = cd->eFlags;
	ps->pm_time = cd->pm_time;
	ps->pm_flags = cd->pm_flags;
	ps->weapon = cd->weapon;
	ps->weaponstate = cd->weaponstate;
	ps->damageEvent = cd->damageEvent;
	ps->damageYaw = cd->damageYaw;
	ps->damagePitch = cd->damagePitch;
	ps->damageCount = cd->damageCount;
	for ( i = 0; i < STAT_NUM_SSDEMO_STATS; i++ ) {
		ps->stats[i] = cd->stats[i];
	}
	for ( i = 0; i < PERS_NUM_PERSISTANT; i++ ) {
		ps->persistant[i] = cd->persistant[i];
	}
	ShortToVec3( cd->grappleVelocity, ps->grappleVelocity, 4.0f );
	ps->grappleTime = cd->grappleTime;
	ps->grappleEntity = cd->grappleEntity;

	// medium-low degree of change
	ps->groundEntityNum = cd->groundEntityNum;
	ps->delta_angles[0] = cd->delta_angles[0];
	ps->delta_angles[1] = cd->delta_angles[1];
	ps->delta_angles[2] = cd->delta_angles[2];
	ps->legsTimer = cd->legsTimer;
	ps->torsoTimer = cd->torsoTimer;
	ps->externalEvent = cd->externalEvent;
	ps->externalEventParm = cd->externalEventParm;
	ps->externalEventTime = cd->externalEventTime;
	iter->s.event = cd->event;
	iter->s.eventParm = cd->eventParm;

	// low degree of change
	ps->pm_type = cd->pm_type;
	ps->viewheight = cd->viewheight;
	for ( i = 1; i < PW_NUM_POWERUPS; i++ ) {
		ps->powerups[i] = cd->powerups[i - 1];
	}
	ps->generic1 = cd->generic1;
	ps->loopSound = cd->loopSound;
	ps->jumppad_ent = cd->jumppad_ent;
	pers->enterTime = cd->enterTime;
	iter->client->respawnTime = cd->respawnTime;
	ShortToVec3( cd->origin2, iter->s.origin2, 1.0f );
	ShortToVec3( cd->mins, iter->r.mins, 1.0f );
	ShortToVec3( cd->maxs, iter->r.maxs, 1.0f );
	iter->freezeState = cd->freezeState;
	iter->target_ent = &g_entities[G_AdjustedEntityNum(cd->target_ent)];

	// almost never change
	ps->clientNum = cd->clientNum;
	ps->gravity = cd->gravity;
	ps->speed = cd->speed;
	strcpy( pers->netname, cd->netname );
	pers->connected = cd->connected;

	// derive the entityState_t for this client - but not the events
	BG_PlayerStateToEntityStateData( &iter->client->ps, &iter->s, qtrue );

	// fix up some other stuff
	iter->classname = "player";
	iter->r.contents = CONTENTS_BODY;
	iter->clipmask = MASK_PLAYERSOLID;

	G_AdjustClientForPlayback( iter );
	G_AdjustEntityForPlayback( iter );
}

#define MAX_ENTITYDATA_STRING 30

// one of these will be saved for every entity, including clients
typedef struct {
	// high degree of change
	vec3_s currentOrigin;
	vec3_s origin;
	vec3_s angles;

	// medium-high degree of change
	shortTrajectory_t pos;
	shortTrajectory_t apos;
	char clientNum; // 6 bits

	// medium degree of change
	short eFlags; // 16 bits
	int time; // 32 bits
	int time2; // 32 bits
	char event; // 8 bits
	char eventParm; // 8 bits
	char legsAnim; // 8 bits
	char torsoAnim; // 8 bits
	char weapon; // 4 bits

	// medium-low degree of change
	short otherEntityNum; // 10 bits
	short otherEntityNum2; // 10 bits
	short groundEntityNum; // 10 bits

	// low degree of change
	vec3_s origin2;
	vec3_s angles2;
	short loopSound; // 16 bits
	int solid; // 32 bits
	int generic1; // 32 bits
	short powerups; // 16 bits
	vec3_s mins, maxs;
	short svFlags; // 16 bits
	int singleClient; // 32 bits
	int contents; // 32 bits
	char areaPortalState; // for doors: keeps track of whether the area portal
			// inside the door is open (does not block PVS) or closed (blocks PVS)
	char freezeState; // for following frozen players
	short target_ent; // for following frozen players
	char moverState; // for doors: open, closed, or somewhere in between
	
	// almost never change
	short number; // 10 bits
	char eType; // 8 bits
	int constantLight; // 32 bits
	short modelindex; // 10 bits
	short modelindex2; // 10 bits
	short frame; // 16 bits
	qboolean bmodel; // 32 bits
	char classname[MAX_ENTITYDATA_STRING]; // so picking spawn points will always work
	char target[MAX_ENTITYDATA_STRING]; // so G_PickTarget() will work with teleporters
	char targetname[MAX_ENTITYDATA_STRING]; // so G_PickTarget() will work with teleporters
	char touchEnum; // 2 bits - for doors and teleporters: maps to a function pointer
	short parent; // for doors: the door trigger's parent is the door itself
	short count; // for doors: the door trigger's major axis
	short spawnflags; // for teleporters: spectators only? (bit 0)
} entityData_t;

// enumerations of touch functions
enum { TOUCH_NONE = 0, TOUCH_DOORTRIGGER, TOUCH_TELEPORTERTRIGGER };

// convenient typedef
typedef void (*touch_t)(gentity_t *self, gentity_t *other, trace_t *trace);

typedef struct {
	int touchEnum;
	touch_t touch;
} touchMap_t;

// mapping of above enumerations to function pointers
// there are really only a few "touch" functions we need to
// bother with since the only real clients are spectators
// and as such don't touch much (never items or hurt 
// triggers, for example)
touchMap_t g_touchMap[] = {
	{ TOUCH_DOORTRIGGER, Touch_DoorTrigger },
	{ TOUCH_TELEPORTERTRIGGER, trigger_teleporter_touch },
	{ TOUCH_NONE, NULL },
};

/*
G_GetEnumForTouch

Returns an enumeration for the given function pointer
*/
static int G_GetEnumForTouch(touch_t touch) {
	touchMap_t *map;
	for ( map = g_touchMap; (*map).touchEnum != TOUCH_NONE; map++ ) {
		if ( (*map).touch == touch ) {
			return (*map).touchEnum;
		}
	}

	return TOUCH_NONE;
}

/*
G_GetTouchForEnum

Returns a function pointer for the given touch enumeration
*/
static touch_t G_GetTouchForEnum(int touchEnum) {
	touchMap_t *map;
	for ( map = g_touchMap; (*map).touchEnum != TOUCH_NONE; map++ ) {
		if ( (*map).touchEnum == touchEnum ) {
			return (*map).touch;
		}
	}

	return NULL;
}

static void G_SerializeEntityData( entityData_t *ed, char *ret, int *size ) {
	void *curr = ret;
	int pack;

	// high degree of change
	SERIALIZE_VEC3S(ed->currentOrigin, curr);
	SERIALIZE_VEC3S(ed->origin, curr);
	SERIALIZE_VEC3S(ed->angles, curr);

	// medium-high degree of change
	SERIALIZE_TRAJECTORY(ed->pos, curr);
	SERIALIZE_TRAJECTORY(ed->apos, curr);
	SERIALIZE_CHAR(ed->clientNum, curr);

	// medium degree of change
	SERIALIZE_SHORT(ed->eFlags, curr);
	SERIALIZE_INT(ed->time, curr);
	SERIALIZE_INT(ed->time2, curr);
	SERIALIZE_CHAR(ed->event, curr);
	SERIALIZE_CHAR(ed->eventParm, curr);
	SERIALIZE_CHAR(ed->legsAnim, curr);
	SERIALIZE_CHAR(ed->torsoAnim, curr);
	SERIALIZE_CHAR(ed->weapon, curr);

	// medium-low degree of change
	pack = (ed->otherEntityNum & 1023) | ((ed->otherEntityNum2 & 1023) << 10) | ((ed->groundEntityNum & 1023) << 20);
	SERIALIZE_INT(pack, curr);

	// low degree of change
	SERIALIZE_VEC3S(ed->origin2, curr);
	SERIALIZE_VEC3S(ed->angles2, curr);
	SERIALIZE_SHORT(ed->loopSound, curr);
	SERIALIZE_INT(ed->solid, curr);
	SERIALIZE_INT(ed->generic1, curr);
	SERIALIZE_SHORT(ed->powerups, curr);
	SERIALIZE_VEC3S(ed->mins, curr);
	SERIALIZE_VEC3S(ed->maxs, curr);
	SERIALIZE_SHORT(ed->svFlags, curr);
	SERIALIZE_INT(ed->singleClient, curr);
	SERIALIZE_INT(ed->contents, curr);

	pack = (ed->target_ent & 1023) | ((ed->moverState & 3) << 10) | ((ed->areaPortalState & 1) << 12) | ((ed->freezeState != 0) << 13);
	SERIALIZE_SHORT(pack, curr);

	// almost never change
	pack = (ed->number & 1023) | ((ed->modelindex & 1023) << 10) | ((ed->modelindex2 & 1023) << 20) | ((ed->touchEnum & 3) << 30);
	SERIALIZE_INT(pack, curr);

	SERIALIZE_INT(ed->bmodel, curr);
	SERIALIZE_CHAR(ed->eType, curr);
	SERIALIZE_INT(ed->constantLight, curr);
	SERIALIZE_SHORT(ed->frame, curr);
	SERIALIZE_CHARARRAY(ed->classname, MAX_ENTITYDATA_STRING, curr);
	SERIALIZE_CHARARRAY(ed->target, MAX_ENTITYDATA_STRING, curr);
	SERIALIZE_CHARARRAY(ed->targetname, MAX_ENTITYDATA_STRING, curr);
	SERIALIZE_SHORT(ed->count, curr);
	SERIALIZE_SHORT(ed->spawnflags, curr);
	SERIALIZE_SHORT(ed->parent, curr);

	*size = (char *)curr - ret;
}

static void G_UnserializeEntityData( entityData_t *ed, char *data ) {
	void *curr = data;
	int pack;

	// high degree of change
	UNSERIALIZE_VEC3S(ed->currentOrigin, curr);
	UNSERIALIZE_VEC3S(ed->origin, curr);
	UNSERIALIZE_VEC3S(ed->angles, curr);

	// medium-high degree of change
	UNSERIALIZE_TRAJECTORY(ed->pos, curr);
	UNSERIALIZE_TRAJECTORY(ed->apos, curr);
	UNSERIALIZE_CHAR(ed->clientNum, curr);

	// medium degree of change
	UNSERIALIZE_SHORT(ed->eFlags, curr);
	UNSERIALIZE_INT(ed->time, curr);
	UNSERIALIZE_INT(ed->time2, curr);
	UNSERIALIZE_CHAR(ed->event, curr);
	UNSERIALIZE_CHAR(ed->eventParm, curr);
	UNSERIALIZE_CHAR(ed->legsAnim, curr);
	UNSERIALIZE_CHAR(ed->torsoAnim, curr);
	UNSERIALIZE_CHAR(ed->weapon, curr);

	// medium-low degree of change
	UNSERIALIZE_INT(pack, curr);
	ed->otherEntityNum = pack & 1023;
	ed->otherEntityNum2 = (pack >> 10) & 1023;
	ed->groundEntityNum = (pack >> 20) & 1023;

	// low degree of change
	UNSERIALIZE_VEC3S(ed->origin2, curr);
	UNSERIALIZE_VEC3S(ed->angles2, curr);
	UNSERIALIZE_SHORT(ed->loopSound, curr);
	UNSERIALIZE_INT(ed->solid, curr);
	UNSERIALIZE_INT(ed->generic1, curr);
	UNSERIALIZE_SHORT(ed->powerups, curr);
	UNSERIALIZE_VEC3S(ed->mins, curr);
	UNSERIALIZE_VEC3S(ed->maxs, curr);
	UNSERIALIZE_SHORT(ed->svFlags, curr);
	UNSERIALIZE_INT(ed->singleClient, curr);
	UNSERIALIZE_INT(ed->contents, curr);

	UNSERIALIZE_SHORT(pack, curr);
	ed->target_ent = pack & 1023;
	ed->moverState = (pack >> 10) & 3;
	ed->areaPortalState = (pack >> 12) & 1;
	ed->freezeState = (pack >> 13) & 1;

	// almost never change
	UNSERIALIZE_INT(pack, curr);
	ed->number = pack & 1023;
	ed->modelindex = (pack >> 10) & 1023;
	ed->modelindex2 = (pack >> 20) & 1023;
	ed->touchEnum = (pack >> 30) & 3;

	UNSERIALIZE_INT(ed->bmodel, curr);
	UNSERIALIZE_CHAR(ed->eType, curr);
	UNSERIALIZE_INT(ed->constantLight, curr);
	UNSERIALIZE_SHORT(ed->frame, curr);
	UNSERIALIZE_CHARARRAY(ed->classname, MAX_ENTITYDATA_STRING, curr);
	UNSERIALIZE_CHARARRAY(ed->target, MAX_ENTITYDATA_STRING, curr);
	UNSERIALIZE_CHARARRAY(ed->targetname, MAX_ENTITYDATA_STRING, curr);
	UNSERIALIZE_SHORT(ed->count, curr);
	UNSERIALIZE_SHORT(ed->spawnflags, curr);
	UNSERIALIZE_SHORT(ed->parent, curr);
}

static void G_StuffEntityData( gentity_t *iter, entityData_t *ed ) {
	entityState_t *s = &iter->s;
	entityShared_t *r = &iter->r;

	// high degree of change
	Vec3ToShort( r->currentOrigin, ed->currentOrigin, 1.0f );
	Vec3ToShort( s->origin, ed->origin, 1.0f );
	Vec3ToShort( s->angles, ed->angles, 4.0f );

	// medium-high degree of change
	TrajectoryToShort( s->pos, ed->pos );
	TrajectoryToShort( s->apos, ed->apos );
	ed->clientNum = s->clientNum;

	// medium degree of change
	ed->eFlags = s->eFlags;
	ed->time = s->time;
	ed->time2 = s->time2;
	ed->event = s->event;
	ed->eventParm = s->eventParm;
	ed->legsAnim = s->legsAnim;
	ed->torsoAnim = s->torsoAnim;
	ed->weapon = s->weapon;

	// medium-low degree of change
	ed->otherEntityNum = s->otherEntityNum;
	ed->otherEntityNum2 = s->otherEntityNum2;
	ed->groundEntityNum = s->groundEntityNum;

	// low degree of change
	Vec3ToShort( s->origin2, ed->origin2, 1.0f );
	Vec3ToShort( s->angles2, ed->angles2, 4.0f );
	ed->loopSound = s->loopSound;
	ed->solid = s->solid;
	ed->generic1 = s->generic1;
	ed->powerups = s->powerups;
	Vec3ToShort( r->mins, ed->mins, 1.0f );
	Vec3ToShort( r->maxs, ed->maxs, 1.0f );
	ed->svFlags = r->svFlags;
	ed->singleClient = r->singleClient;
	ed->contents = r->contents;
	ed->areaPortalState = iter->areaPortalState;
	ed->freezeState = iter->freezeState;
	ed->target_ent = iter->target_ent - g_entities;
	ed->moverState = iter->moverState;
	
	// almost never change
	ed->number = s->number;
	ed->eType = s->eType;
	ed->constantLight = s->constantLight;
	ed->modelindex = s->modelindex;
	ed->modelindex2 = s->modelindex2;
	ed->frame = s->frame;
	ed->bmodel = r->bmodel;
	memset( ed->classname, 0, MAX_ENTITYDATA_STRING );
	if ( iter->classname != NULL ) Q_strncpyz( ed->classname, iter->classname, MAX_ENTITYDATA_STRING );
	memset( ed->target, 0, MAX_ENTITYDATA_STRING );
	if ( iter->target != NULL ) Q_strncpyz( ed->target, iter->target, MAX_ENTITYDATA_STRING );
	memset( ed->targetname, 0, MAX_ENTITYDATA_STRING );
	if ( iter->targetname != NULL ) Q_strncpyz( ed->targetname, iter->targetname, MAX_ENTITYDATA_STRING );
	ed->touchEnum = G_GetEnumForTouch( iter->touch );
	ed->parent = iter->parent - g_entities;
	ed->count = iter->count;
	ed->spawnflags = iter->spawnflags;

	// unset the hashed flag
	ed->eFlags &= ~EF_HASHED;
}

static void G_UnstuffEntityData( gentity_t *iter, entityData_t *ed ) {
	entityState_t *s = &iter->s;
	entityShared_t *r = &iter->r;

	// high degree of change
	ShortToVec3( ed->currentOrigin, r->currentOrigin, 1.0f );
	ShortToVec3( ed->origin, s->origin, 1.0f );
	ShortToVec3( ed->angles, s->angles, 4.0f );

	// medium-high degree of change
	ShortToTrajectory( ed->pos, s->pos );
	ShortToTrajectory( ed->apos, s->apos );
	s->clientNum = ed->clientNum;

	// medium degree of change
	s->eFlags = ed->eFlags;
	s->time = ed->time;
	s->time2 = ed->time2;
	s->event = ed->event;
	s->eventParm = ed->eventParm;
	s->legsAnim = ed->legsAnim;
	s->torsoAnim = ed->torsoAnim;
	s->weapon = ed->weapon;

	// medium-low degree of change
	s->otherEntityNum = ed->otherEntityNum;
	s->otherEntityNum2 = ed->otherEntityNum2;
	s->groundEntityNum = ed->groundEntityNum;

	// low degree of change
	ShortToVec3( ed->origin2, s->origin2, 1.0f );
	ShortToVec3( ed->angles2, s->angles2, 4.0f );
	s->loopSound = ed->loopSound;
	s->solid = ed->solid;
	s->generic1 = ed->generic1;
	s->powerups = ed->powerups;
	ShortToVec3( ed->mins, r->mins, 1.0f );
	ShortToVec3( ed->maxs, r->maxs, 1.0f );
	r->svFlags = ed->svFlags;
	r->singleClient = ed->singleClient;
	r->contents = ed->contents;
	iter->freezeState = ed->freezeState;
	iter->target_ent = &g_entities[G_AdjustedEntityNum(ed->target_ent)];
	iter->moverState = ed->moverState;

	// almost never change
	s->number = ed->number;
	s->eType = ed->eType;
	s->constantLight = ed->constantLight;
	s->modelindex = ed->modelindex;
	s->modelindex2 = ed->modelindex2;
	s->frame = ed->frame;
	r->bmodel = ed->bmodel;
	if ( ed->classname[0] != '\0' ) {
		iter->classname = ed->classname; // simple assignment is fine in this case
	}
	else {
		iter->classname = NULL;
	}	
	if ( ed->target[0] != '\0' ) {
		iter->target = ed->target; // simple assignment is fine in this case
	}
	else {
		iter->target = NULL;
	}
	if ( ed->targetname[0] != '\0' ) {
		iter->targetname = ed->targetname; // simple assignment is fine in this case
	}
	else {
		iter->targetname = NULL;
	}
	iter->touch = G_GetTouchForEnum( ed->touchEnum );
	iter->parent = &g_entities[G_AdjustedEntityNum(ed->parent)];
	iter->count = ed->count;
	iter->spawnflags = ed->spawnflags;

	G_AdjustEntityForPlayback( iter );

	// we MUST do the areaportal state after some key element 
	// above has changed - dunno which one, but it's safe to do this here

	// if the portal state has changed
	if ( ed->areaPortalState != iter->areaPortalState ) {
		// tell the engine to adjust it
		trap_AdjustAreaPortalState( iter, ed->areaPortalState );
	}
	iter->areaPortalState = ed->areaPortalState;
}

// Cache information to aid in writing only changes and
// in reconstructing the whole data set when reading
clientData_t g_cdata[MAX_CLIENTS];
entityData_t g_edata[ENTITYNUM_MAX_NORMAL];
qboolean g_validCData[MAX_CLIENTS];
qboolean g_validEData[ENTITYNUM_MAX_NORMAL];
qboolean g_wasValidEData[ENTITYNUM_MAX_NORMAL];

/*
G_ClearEntityCache

Both demo writing and reading use caching for clientData_t and
entityData_t to keep track of what's been written and read.
Writing uses caching to only write changes, and reading uses
it because it's convenient.

This function clears the caches.
*/
static void G_ClearEntityCache( void ) {
	memset( g_cdata, 0, sizeof(g_cdata) );
	memset( g_edata, 0, sizeof(g_edata) );

	memset( g_validCData, 0, sizeof(g_validCData) );
	memset( g_validEData, 0, sizeof(g_validEData) );
	memset( g_wasValidEData, 0, sizeof(g_wasValidEData) );
}

/*
G_OpenDemoWrite

Opens a demo file for writing
*/
static void G_OpenDemoWrite( const char *filename ) {
	trap_FS_FOpenFile( filename, &level.outDemoFile, FS_WRITE );
	level.outDemoWrote = 0;
}

/*
G_OpenDemoAppend

Opens a demo file for appending; this is used after warmup
because ending a warmup period issues a map_restart
*/
static void G_OpenDemoAppend( const char *filename ) {
	trap_FS_FOpenFile( filename, &level.outDemoFile, FS_APPEND );
}

/*
G_WriteDemoData

Macro to write demo data
*/
#define G_WriteDemoData(data, size) \
	trap_FS_Write( data, size, level.outDemoFile ); \
	level.outDemoWrote += size;

/*
G_CloseDemoWrite

Closes a demo file that has been recorded to
*/
static void G_CloseDemoWrite() {
	trap_FS_FCloseFile( level.outDemoFile );
	level.outDemoFile = 0;
}

/*
G_OpenDemoRead

Opens a demo file for reading
*/
static void G_OpenDemoRead( const char *filename ) {
	level.inDemoSize = trap_FS_FOpenFile( filename, &level.inDemoFile, FS_READ );
	level.inDemoRead = 0;
}

/*
G_ReadDemoData

Macro to read demo data
*/
#define G_ReadDemoData(data, size) \
	trap_FS_Read( data, size, level.inDemoFile ); \
	level.inDemoRead += size;

/*
G_CloseDemoRead

Closes a demo file that has been read from
*/
static void G_CloseDemoRead() {
	trap_FS_FCloseFile( level.inDemoFile );
	level.inDemoFile = 0;
}

/*
G_ShutdownDemosClean

Called when the VM quits to make sure any open demo file
handle is cleanly closed
*/
void G_ShutdownDemosClean( void ) {
	// if a demo is being recorded
	if ( level.outDemoFile != 0 ) {
		qtime_t tm;
		unsigned time;

		trap_RealTime( &tm );
		time = Q_mktime( &tm );

		// set some latch cvars so the game can pick the demo back up again

		// keep track of when the game shut down
		trap_Cvar_Set( "##_continueRecordTime", va("%u", time) );

		// keep track of how much as been written
		trap_Cvar_Set( "##_continueRecordSize", va("%d", level.outDemoWrote) );

		// keep track of the demo name
		trap_Cvar_Set( "##_continueRecordName", level.demoName );

		// close the file
		G_CloseDemoWrite();
	}

	// if a demo is being played
	if ( level.inDemoFile != 0 ) G_CloseDemoRead();
}

#ifdef HUMAN_READABLE_DEMOS
/*
G_WriteDemoDirective1
*/
static void G_WriteDemoDirective1( demoDirective_t dir, int n1 ) {
	char header[11];

	Com_sprintf( header, sizeof(header), "%s%08d", G_GetStringForDirective(dir), n1 );
	G_WriteDemoData( header, strlen(header) );
}
#else
#define G_WriteDemoDirective1(dir, n1) G_WriteDemoDirective2(dir, n1, 0)
#endif

/*
G_WriteDemoDirective2
*/
static void G_WriteDemoDirective2( demoDirective_t dir, int n1, int n2 ) {
#ifdef HUMAN_READABLE_DEMOS
	char header[11];
	const char *str = G_GetStringForDirective( dir );

	// it's a stupid hack, but there you go: atoi doesn't work exactly the
	// same way in a DLL and in the VM (G_ReadDemoDirective can choke hard
	// on negative numbers in the VM), and this fixes it
	if ( n1 >= 0 ) {
		if ( n2 >= 0 ) {
			Com_sprintf( header, sizeof(header), "%s%04d%04d", str, n1, n2 );
		}
		else {
			Com_sprintf( header, sizeof(header), "%s%04d-%03d", str, n1, -n2 );
		}
	}
	else {
		if ( n2 >= 0 ) {
			Com_sprintf( header, sizeof(header), "%s-%03d%04d", str, -n1, n2 );
		}
		else {
			Com_sprintf( header, sizeof(header), "%s-%03d-%03d", str, -n1, -n2 );
		}
	}

	G_WriteDemoData( header, strlen(header) );
#else
	unsigned header;

	switch ( dir ) // 4 bits directive
	{
	case DIR_FRAMEBEGIN: // 4 bits zero, 32 bits level time
		G_WriteDemoData( &dir, 1);
		G_WriteDemoData( &n1, 4);
		break;

	case DIR_CLIENTDATA: // 6 bits client number, 14 bits size
		header = (dir & 15) | ((n1 & 63) << 4) | ((n2 & 16383) << 10);
		G_WriteDemoData( &header, 3);
		break;

	case DIR_ENTITYDATA: // 11 bits entity number, 9 bits size
		header = (dir & 15) | ((n1 & 2047) << 4) | ((n2 & 511) << 15);
		G_WriteDemoData( &header, 3);
		break;

	case DIR_FRAMEEND: // 0 bits
		G_WriteDemoData( &dir, 1);
		break;

	case DIR_DEMOHEADER: // 12 bits size
		header = (dir & 15) | ((n1 & 4095) << 4);
		G_WriteDemoData( &header, 2);
		break;

	case DIR_CONFIGSTRING: // 10 bits number, 10 bits size
		header = (dir & 15) | ((n1 & 1023) << 4) | ((n2 & 1023) << 14);
		G_WriteDemoData( &header, 3);
		break;

	case DIR_SERVERCOMMAND: // 7 bits client number, 13 bits size
		header = (dir & 15) | ((n1 & 127) << 4) | ((n2 & 8191) << 11);
		G_WriteDemoData( &header, 3);
		break;

	default:
		G_Error("Cannot write unknown demo directive\n");
		break;
	}
#endif
}

/*
G_WriteDirectiveEnd
*/
#ifdef HUMAN_READABLE_DEMOS
static void G_WriteDirectiveEnd( void ) {
	G_WriteDemoData( "\n", 1 );
}
#else
#define G_WriteDirectiveEnd()
#endif

static void G_ReadDemoDirective( demoDirective_t *dir, int *n1, int *n2 ) {
#ifdef HUMAN_READABLE_DEMOS
	char header[11];
	char temp;

	G_ReadDemoData( header, 10 );
	header[10] = '\0';

	temp = header[2];
	header[2] = '\0';
	*dir = G_GetDirectiveForString( header );
	header[2] = temp;

	switch ( *dir ) {
	case DIR_DEMOHEADER:
	case DIR_FRAMEBEGIN:
	case DIR_FRAMEEND:
		*n1 = atoi( &header[2] );
		*n2 = 0;
		return;

	case DIR_CLIENTDATA:
	case DIR_ENTITYDATA:
	case DIR_CONFIGSTRING:
	case DIR_SERVERCOMMAND:
		temp = header[6];

		header[6] = '\0';
		*n1 = atoi( &header[2] );

		header[6] = temp;
		*n2 = atoi( &header[6] );
		return;

	default:
		*n1 = 0;
		*n2 = 0;
		return;
	}
#else
	unsigned header;
	void *h1 = (char *)((void*)&header) + 1; // points to byte 1 in header

	G_ReadDemoData( &header, 1 );

	*dir = header & 15;

	switch ( *dir ) // 4 bits directive
	{
	case DIR_FRAMEBEGIN: // 4 bits zero, 32 bits level time
		G_ReadDemoData( n1, 4 );
		*n2 = 0;
		break;

	case DIR_CLIENTDATA: // 6 bits client number, 14 bits size
		G_ReadDemoData( h1, 2 );
		*n1 = (header >> 4) & 63;
		*n2 = (header >> 10) & 16383;
		break;

	case DIR_ENTITYDATA: // 11 bits entity number, 9 bits size
		G_ReadDemoData( h1, 2 );
		*n1 = (header >> 4) & 2047;
		*n2 = (header >> 15) & 511;
		break;

	case DIR_FRAMEEND: // 0 bits
		*n1 = *n2 = 0;
		break;

	case DIR_DEMOHEADER: // 12 bits size
		G_ReadDemoData( h1, 1 );
		*n1 = (header >> 4) & 4095;
		*n2 = 0;
		break;

	case DIR_CONFIGSTRING: // 10 bits number, 10 bits size
		G_ReadDemoData( h1, 2 );
		*n1 = (header >> 4) & 1023;
		*n2 = (header >> 14) & 1023;
		break;

	case DIR_SERVERCOMMAND: // 7 bits client number, 13 bits size
		G_ReadDemoData( h1, 2 );
		*n1 = (header >> 4) & 127;
		*n2 = (header >> 11) & 8191;

		if ( *n1 & 64 ) {
			*n1 -= 128;
		}
		break;

	default:
		G_Error("Cannot read unknown demo directive\n");
		break;
	}
#endif
}

/*
G_ReadDirectiveEnd
*/
#ifdef HUMAN_READABLE_DEMOS
static void G_ReadDirectiveEnd( void ) {
	char temp;
	G_ReadDemoData( &temp, 1 );
}
#else
#define G_ReadDirectiveEnd()
#endif

// this is where settings go that will be saved for the
// duration of the demo
static struct {
	qboolean valid;
	int svFps;
} g_savedSettings;

/*
G_BackupServerSettings

Saves server settings the demo is able to change
*/
static void G_BackupServerSettings( void ) {
	g_savedSettings.valid = qtrue;
	g_savedSettings.svFps = sv_fps.integer;
}


/*
G_RestoreServerSettings

Restores server settings the demo is able to change
*/
static void G_RestoreServerSettings( void ) {
	if ( g_savedSettings.valid ) {
		trap_Cvar_Set( "sv_fps", va("%d", g_savedSettings.svFps) );
	}
}

static char g_serverinfoConfigstring[BIG_INFO_STRING];
static char g_systeminfoConfigstring[BIG_INFO_STRING];

/*
G_RecordConfigstring

Writes the config string to the output demo file
*/
static void G_RecordConfigstring( int num, const char *string ) {
	char info[MAX_INFO_STRING];

	// this is only meaningful during demo playback, and we
	// don't want data collisions at that time
	if ( num == CS_SSDEMOSTATE ) return;

	// same here
	if ( num == CS_SSDEMOSERVERINFO ) return;

	// and here
	if ( num == CS_SSDEMOSYSTEMINFO ) return;

	if ( num == CS_SERVERINFO ) {
		// save it for later for checking
		strcpy( g_serverinfoConfigstring, string );

		// only record the stuff we'll want during demo playback
		Com_sprintf( info, sizeof(info), 
			"\\mapname\\%s\\sv_hostname\\%s\\g_ufreezeVersion\\%s"
			"\\timelimit\\%s\\fraglimit\\%s\\capturelimit\\%s"
			"\\g_gametype\\%s\\dmflags\\%s\\teamflags\\%s"
			"\\g_redTeam\\%s\\g_blueTeam\\%s\\g_reloadFactor\\%s"
			"\\g_railJumpKnock\\%s\\g_autoThawTime\\%s\\g_thawTime\\%s"
			"\\g_huntMode\\%s\\g_delagHitscan\\%s",
			BG_Cvar_GetString("mapname"), BG_Cvar_GetString("sv_hostname"), BG_Cvar_GetString("g_ufreezeVersion"),
			BG_Cvar_GetString("timelimit"), BG_Cvar_GetString("fraglimit"), BG_Cvar_GetString("capturelimit"),
			BG_Cvar_GetString("g_gametype"), BG_Cvar_GetString("dmflags"), BG_Cvar_GetString("teamflags"),
			BG_Cvar_GetString("g_redTeam"), BG_Cvar_GetString("g_blueTeam"), BG_Cvar_GetString("g_reloadFactor"),
			BG_Cvar_GetString("g_railJumpKnock"), BG_Cvar_GetString("g_autoThawTime"), BG_Cvar_GetString("g_thawTime"),
			BG_Cvar_GetString("g_huntMode"), BG_Cvar_GetString("g_delagHitscan") );
		string = info;

		// THE ABOVE CODE IS AN EVIL HACK.
		// CS_SERVERINFO is sometimes not constructed when the demo starts recording (like in
		// a continuation after a map change), so we have to use the actual variable values.
		// CS_SERVERINFO will be recorded twice if this happens, but it makes things actually
		// work so we're okay with a little more overhead.
	}
	else if ( num == CS_SYSTEMINFO ) {
		// save it for later for checking
		strcpy( g_systeminfoConfigstring, string );

		// only record the stuff we'll want during demo playback
		Com_sprintf( info, sizeof(info),
			"\\sv_pure\\%s\\sv_cheats\\%s\\g_friendlyFire\\%s\\g_hardcore\\%s"
			"\\g_doTimeouts\\%s\\g_numTimeouts\\%s\\g_doTeamLocks\\%s",
			BG_Cvar_GetString("sv_pure"), BG_Cvar_GetString("sv_cheats"), BG_Cvar_GetString("g_friendlyFire"), BG_Cvar_GetString("g_hardcore"),
			BG_Cvar_GetString("g_doTimeouts"), BG_Cvar_GetString("g_numTimeouts"), BG_Cvar_GetString("g_doTeamLocks"));
		string = info;

		// THE ABOVE CODE IS AN EVIL HACK.
		// See the comments for recording CS_SERVERINFO.
	}

	G_WriteDemoDirective2( DIR_CONFIGSTRING, num, strlen(string) );
	G_WriteDemoData( string, strlen(string) );
	G_WriteDirectiveEnd();
}

/*
G_RecordDemoHeader
*/
static void G_RecordDemoHeader( void ) {
	char info[MAX_STRING_CHARS];

	Com_sprintf( info, sizeof(info), "map\\%s\\version\\%s\\sv_fps\\%d", sv_mapname.string, SSDEMO_VERSION, sv_fps.integer );

	G_WriteDemoDirective1( DIR_DEMOHEADER, strlen(info) );
	G_WriteDemoData( info, strlen(info) );
	G_WriteDirectiveEnd();
}

/*
G_SRecord

Starts a server-side demo recording if the conditions are right.
*/
const char *G_SRecord( void ) {
	char filename[MAX_STRING_CHARS];
	char demoname[MAX_STRING_CHARS];
	int i;

	// stop any demos currently being played back
	if ( level.inDemoFile != 0 ) {
		const char *ret = G_SStopDemo();
		G_Printf( "%s\n", ret );

		// a map change is in the command queue now
	}

	// stop any recordings
	if ( level.outDemoFile != 0 ) {
		const char *ret = G_SStopRecord();
		G_Printf( "%s\n", ret );
	}

	// check arguments
	if ( trap_Argc() == 1 ) {
		static const char *gamenames[] = { "FFA", "1v1", "FFA", "TDM", "CTF" };
		const char *gamename = "UNK";
		qtime_t time;

		// get the game name as a 3-char string
		if ( g_gametype.integer >= GT_FFA && g_gametype.integer <= GT_CTF ) {
			gamename = gamenames[g_gametype.integer];
		}

		// get the date/time
		trap_RealTime( &time );

		// construct a meaningful, hopefully unique demo name
		Com_sprintf( demoname, sizeof(demoname), "%s-%s-%04d.%02d.%02d-%02d.%02d.%02d",
			gamename, sv_mapname.string,
			time.tm_year + 1900, time.tm_mon, time.tm_mday,
			time.tm_hour, time.tm_min, time.tm_sec );
	}
	else if ( trap_Argc() == 2 ) {
		// use the user-supplied demo name
		trap_Argv( 1, demoname, sizeof(demoname) );
	}
	else {
		return "too many arguments";
	}

	// construct a relative path to the demo
	Com_sprintf( filename, sizeof(filename), "ssdemos/%s" SSDEMO_EXTENSION, demoname );

	// open the demo file for writing
	G_OpenDemoWrite( filename );

	if ( !level.outDemoFile ) {
		return va( "unable to open file %s", filename );
	}

	// set this in case we need to continue the demo again
	strcpy( level.demoName, demoname );

	G_ClearEntityCache();

	// record the map we're on, sv_fps, etc.
	G_RecordDemoHeader();

	// record all configstrings
	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		char buf[BIG_INFO_STRING];
		trap_GetConfigstring( i, buf, sizeof(buf) );

		if ( buf[0] != '\0' || i < 2 ) {
			G_RecordConfigstring( i, buf );
		}
	}

	level.demoFrame = 0;

	// record a frame of data`
	G_RecordSDemoData();

	// log it
	G_LogPrintf( "demo: started recording %s\n", filename );

	return va( "recording %s", filename );
}

/*
Svcmd_SRecord_f
*/
void Svcmd_SRecord_f( void ) {
	const char *ret = G_SRecord();
	G_Printf( "%s\n", ret );
}

/*
G_ContinueRecord

Resumes a server-side demo recording if the conditions are right.
*/
void G_ContinueRecord( void ) {
	char demoname[MAX_STRING_CHARS];
	char demosize[MAX_STRING_CHARS];
	char demotime[MAX_STRING_CHARS];
	char filename[MAX_STRING_CHARS];
	int i;
	qtime_t tm;
	unsigned time;

	trap_Cvar_VariableStringBuffer( "##_continueRecordName", demoname, sizeof(demoname) );

	// if no demo was being played
	if ( demoname[0] == '\0' ) {
		// unset these for cleanness' sake
		trap_Cvar_Set( "##_continueRecordSize", "" );
		trap_Cvar_Set( "##_continueRecordTime", "" );
		return;
	}

	trap_Cvar_VariableStringBuffer( "##_continueRecordSize", demosize, sizeof(demosize) );
	trap_Cvar_VariableStringBuffer( "##_continueRecordTime", demotime, sizeof(demotime) );

	// unset these for cleanness' sake
	trap_Cvar_Set( "##_continueRecordName", "" );
	trap_Cvar_Set( "##_continueRecordSize", "" );
	trap_Cvar_Set( "##_continueRecordTime", "" );

	// get the actual time in seconds
	trap_RealTime( &tm );
	time = Q_mktime( &tm );

	// if the server was recording more than ss_mapRestartThresh seconds ago, forget it
	if ( time - (unsigned)atoi(demotime) > ss_mapRestartThresh.integer ) {
		return;
	}

	// put together the demo file name
	Com_sprintf( filename, sizeof(filename), "ssdemos/%s" SSDEMO_EXTENSION, demoname );

	// open the demo for appending
	G_OpenDemoAppend( filename );

	if ( !level.outDemoFile ) {
		G_Printf( "unable to open file %s\n", filename );
		return;
	}

	level.outDemoWrote = atoi( demosize );

	// set this in case we need to continue the demo again
	strcpy( level.demoName, demoname );

	G_ClearEntityCache();

	// record a demo header
	G_RecordDemoHeader();

	// record all configstrings
	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		char buf[BIG_INFO_STRING];
		trap_GetConfigstring( i, buf, sizeof(buf) );

		if ( buf[0] != '\0' || i < 2 ) {
			G_RecordConfigstring( i, buf );
		}
	}

	level.demoFrame = 0;

	// record a frame of data
	G_RecordSDemoData();

	// log it
	G_LogPrintf( "demo: continued recording %s\n", filename );
}

/*
G_SStopRecord

Stops writing a server-side demo if one is being written.
*/
const char *G_SStopRecord( void ) {
	if ( level.outDemoFile == 0 ) {
		return "there is no server-side demo being recorded";
	}

	G_CloseDemoWrite();

	// unset these so it doesn't pick up again
	trap_Cvar_Set( "##_continueRecordName", "" );
	trap_Cvar_Set( "##_continueRecordSize", "" );
	trap_Cvar_Set( "##_continueRecordTime", "" );

	return "recording stopped";
}

/*
Svcmd_SStopRecord_f
*/
void Svcmd_SStopRecord_f( void ) {
	const char *ret = G_SStopRecord();
	G_Printf( "%s\n", ret );
}

/*
G_SDemo

Checks to make sure the demo exists, and changes the map.  Sets
ss_continueDemoName to the name of the demo so when the server
restarts it'll pick it back up.

It's all to force a map restart for playing a demo.
*/
const char *G_SDemo( void ) {
	char demoname[MAX_STRING_CHARS];
	char filename[MAX_STRING_CHARS];
	char info[MAX_STRING_CHARS];
#ifdef HUMAN_READABLE_DEMOS
	char header[11];
#else
	unsigned header;
	demoDirective_t dir;
#endif
	char *s;
	int size;
	fileHandle_t inDemoFile;

	// stop any recording going on
	if ( level.outDemoFile != 0 ) {
		const char *ret = G_SStopRecord();
		G_Printf( "%s\n", ret );
	}

	if ( trap_Argc() != 2 ) {
		return "you need to supply a demo file name";
	}

	// get the demo name and make a file name
	trap_Argv( 1, demoname, sizeof(demoname) );
	Com_sprintf( filename, sizeof(filename), "ssdemos/%s" SSDEMO_EXTENSION, demoname );

	// try to open it
	trap_FS_FOpenFile( filename, &inDemoFile, FS_READ );

	if ( !inDemoFile ) {
		// try it without the 'ssdemos' directory
		Com_sprintf( filename, sizeof(filename), "%s" SSDEMO_EXTENSION, demoname );
		trap_FS_FOpenFile( filename, &inDemoFile, FS_READ );

		if ( !inDemoFile ) {
			return va( "unable to open file %s", filename );
		}
	}

#ifdef HUMAN_READABLE_DEMOS
	// read the header
	trap_FS_Read( header, 10, inDemoFile );
	header[10] = '\0';

	// make sure it's a demo header directive
	if ( header[0] != 'd' || header[1] != 'h' ) {
		trap_FS_FCloseFile( inDemoFile );
		return va( "invalid demo file %s", filename );
	}

	// get the info string size
	size = atoi( &header[2] );
#else
	// read the header
	trap_FS_Read( &header, 2, inDemoFile );

	dir = header & 15;

	// make sure it's a demo header directive
	if ( dir != DIR_DEMOHEADER ) {
		trap_FS_FCloseFile( inDemoFile );
		return va( "invalid demo file %s", filename );
	}

	// get the info string size
	size = (header >> 4) & 4095;
#endif

	// get the info string
	trap_FS_Read( info, size, inDemoFile );
	info[size] = '\0';

	// stop reading for now
	trap_FS_FCloseFile( inDemoFile );

	s = Info_ValueForKey(info, "version");
	// if we're on the wrong version
	if ( Q_stricmp(s, SSDEMO_VERSION) != 0 ) {
		// oops!
		return va( "demo version mismatch: version %s, demo version %s\n", SSDEMO_VERSION, s );
	}

	// get the map name
	s = Info_ValueForKey(info, "map");

	// set these so the demo can be picked back up
	trap_Cvar_Set( "##_continueDemoName", demoname );
	// start at the beginning
	trap_Cvar_Set( "##_continueDemoLoc", "0" );

	// log it
	G_LogPrintf( "demo: started demo %s\n", filename );

	// save the server settings the demo will change
	G_BackupServerSettings();

	// fake hard restart into starting a new map
	strcpy( sv_mapname.string, s );
	// enable cheats for demo playback
	sv_cheats.integer = 1;
	// call hard restart because it saves nextmap
	Svcmd_HardRestart_f();

	return "demo starting";
}

/*
Svcmd_SDemo_f
*/
void Svcmd_SDemo_f( void ) {
	const char *ret = G_SDemo();
	G_Printf( "%s\n", ret );
}

/*
G_SSkipTo
*/
const char *G_SSkipTo( int ms ) {
	int gametime = level.time - (level.demoStartTime - level.demoTimeDiff) - level.totaltimeout;
	int seconds;

	if ( level.inDemoFile == 0 ) {
		return "there is no server-side demo being played";
	}

	if ( ms < gametime ) {
		return "that time is in the past";
	}

	level.skipToDemoTime = ms;

	seconds = ms / 1000;
	return va("skipping to game time %d:%02d", seconds / 60, seconds % 60);
}

/*
Svcmd_SSkip_f

Sets the demo state up to skip the given number of
seconds.
*/
void Svcmd_SSkip_f( void ) {
	char time[MAX_STRING_CHARS];
	const char *ret;
	char *colon;
	int minutes, seconds, ms, gametime;

	if ( trap_Argc() != 2 ) {
		G_Printf("usage: ss_skip <mm:ss>|<seconds>\n");
		return;
	}

	trap_Argv( 1, time, sizeof(time) );

	// find the :
	colon = strchr( time, ':' );
	if ( colon == NULL ) {
		minutes = 0;
		seconds = atoi( time );
	}
	else {
		// split the string
		*colon = '\0';
		minutes = atoi( time );
		seconds = atoi( colon + 1 );
	}

	if ( minutes < 0 || seconds < 0 ) {
		G_Printf("usage: ss_skip <mm:ss>|<seconds>\n");
		return;
	}

	// calculate the game clock
	gametime = level.time - (level.demoStartTime - level.demoTimeDiff) - level.totaltimeout;
	// calculate the skip time in milliseconds
	ms = (minutes * 60 + seconds) * 1000 + gametime;

	ret = G_SSkipTo( ms );
	G_Printf( "%s\n", ret );
}

/*
Svcmd_SSkipTo_f

Sets the demo state up to skip to the given time.
*/
void Svcmd_SSkipTo_f( void ) {
	char time[MAX_STRING_CHARS];
	const char *ret;
	char *colon;
	int minutes, seconds, ms;

	if ( trap_Argc() != 2 ) {
		G_Printf("usage: ss_skipto <mm:ss>|<seconds>\n");
		return;
	}

	trap_Argv( 1, time, sizeof(time) );

	// find the :
	colon = strchr( time, ':' );
	if ( colon == NULL ) {
		minutes = 0;
		seconds = atoi( time );
	}
	else {
		// split the string
		*colon = '\0';
		minutes = atoi( time );
		seconds = atoi( colon + 1 );
	}

	if ( minutes < 0 || seconds < 0 ) {
		G_Printf("usage: ss_skipto <mm:ss>|<seconds>\n");
		return;
	}

	// calculate the time in milliseconds
	ms = (minutes * 60 + seconds) * 1000;

	ret = G_SSkipTo( ms );
	G_Printf( "%s\n", ret );
}

/*
G_SPause
*/
const char *G_SPause( void ) {
	if ( level.inDemoFile == 0 ) {
		return "there is no server-side demo being played";
	}

	level.demoPaused = !level.demoPaused;

	if ( level.demoPaused ) {
		return "demo paused";
	}
	else {
		return "demo unpaused";
	}
}

/*
Svcmd_SPause_f

Sets the demo state up to pause.
*/
void Svcmd_SPause_f( void ) {
	const char *ret = G_SPause();
	G_Printf( "%s\n", ret );
}

/*
G_ContinueDemo

Actually starts the demo running
*/
void G_ContinueDemo( void ) {
	char demoname[MAX_STRING_CHARS];
	char demoloc[MAX_STRING_CHARS];
	char filename[MAX_STRING_CHARS];
	int fileloc;
	gentity_t *iter;

	trap_Cvar_VariableStringBuffer( "##_continueDemoName", demoname, sizeof(demoname) );

	// if no demo was being played
	if ( demoname[0] == '\0' ) {
		trap_Cvar_Set( "##_continueDemoLoc", "" );
		return;
	}

	trap_Cvar_VariableStringBuffer( "##_continueDemoLoc", demoloc, sizeof(demoloc) );
	fileloc = atoi( demoloc );

	// unset these so it doesn't continue forever
	trap_Cvar_Set( "##_continueDemoName", "" );
	trap_Cvar_Set( "##_continueDemoLoc", "" );

	// put together the demo file name
	Com_sprintf( filename, sizeof(filename), "ssdemos/%s" SSDEMO_EXTENSION, demoname );

	// open the demo for reading
	G_OpenDemoRead( filename );

	if ( !level.inDemoFile ) {
		// try it inside the main/mod dir instead
		Com_sprintf( filename, sizeof(filename), "%s" SSDEMO_EXTENSION, demoname );
		G_OpenDemoRead( filename );

		if ( !level.inDemoFile ) {
			G_Printf("unable to open file %s\n", filename);
			return;
		}
	}

	level.inDemoRead = fileloc;

	// set this in case we need to continue the demo again
	strcpy( level.demoName, demoname );

	// seek forward to where the demo starts
	trap_FS_Seek( level.inDemoFile, fileloc, FS_SEEK_SET );

	// open up every entity slot
	level.num_entities = ENTITYNUM_MAX_NORMAL;

	// let the engine know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// free them all
	for ( iter = &g_entities[HALF_CLIENTS]; iter < &g_entities[ENTITYNUM_MAX_NORMAL]; iter++ ) {
		iter->neverFree = qfalse;
		G_FreeEntity( iter );
	}

	G_ClearEntityCache();

	G_Printf( "playing server-side demo %s\n", level.demoName );

	level.demoFrame = 0;
}

/*
G_SStopDemo

Stops a server-side demo playback if one is running.  Either execs
nextmap or does a map_restart.
*/
const char *G_SStopDemo( void ) {
	char nextmap[MAX_STRING_CHARS];

	if ( level.inDemoFile == 0 ) {
		return "there is no server-side demo being played";
	}

	G_CloseDemoRead();

	// restore the settings the demo changed
	G_RestoreServerSettings();

	// signal the client that there is no demo being played
	G_SetDemoState( qfalse );

	// if there's a nextmap defined
	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	if ( nextmap[0] != '\0' ) {
		// exec it
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", nextmap) );
	}
	else {
		// otherwise just restart the map
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}

	return "demo stopped";
}

/*
Svcmd_SStopDemo_f
*/
void Svcmd_SStopDemo_f( void ) {
	const char *ret = G_SStopDemo();
	G_Printf( "%s\n", ret );
}

/*
G_SDemoStatus
*/
const char *G_SDemoStatus( void ) {
	static char ret[MAX_STRING_CHARS];

	if ( level.inDemoFile ) {
		Com_sprintf( ret, sizeof(ret), "playing %.1fk/%.1fk of demo %s%s",
			(float)level.inDemoRead / 1024,
			(float)level.inDemoSize / 1024,
			level.demoName,
			level.demoPaused ? ", paused" : "" );
	}
	else if ( level.outDemoFile ) {
		Com_sprintf( ret, sizeof(ret), "recording %.1fk of demo %s", 
			(float)level.outDemoWrote / 1024,
			level.demoName );
	}
	else {
		return "there is no demo being played back or recorded";
	}

	return ret;
}

/*
Svcmd_SDemoStatus_f
*/
void Svcmd_SDemoStatus_f( void ) {
	const char *ret = G_SDemoStatus();
	G_Printf( "%s\n", ret );
}

/*
G_RecordClientData
*/
static void G_RecordClientData( gentity_t *iter ) {
	int num = iter - g_entities;

	char rle[sizeof(clientData_t) + (sizeof(clientData_t) * 4) / 3];
	int rlesize = sizeof(rle);

	// if not being used
	if ( !iter->inuse || iter->client == NULL ) {
		// and it *wasn't* being used
		if ( !g_validCData[num] ) {
			// not used, wasn't used - not interested
			return;
		}

		// it's stopped being used - we'll write a terminator record
		G_WriteDemoDirective2( DIR_CLIENTDATA, num, 0 );

		g_validCData[num] = qfalse;
	}
	// if being used
	else {
		clientData_t cd;
		clientData_t delta;
		char serialized[1024];
		int serialsize;
		clientPersistant_t *pers = &iter->client->pers;

		// don't record spectators
		if ( is_spectator(iter->client) ) return;

		// stuff iter, iter->client, iter->client.pers, iter->client.ps, iter->s,
		// and iter->r into a clientData_t
		G_StuffClientData( iter, &cd );

		// if it wasn't being used before
		if ( !g_validCData[num] ) {
			// key frame
			memcpy( &delta, &cd, sizeof(clientData_t) );
		}
		else {
			// get a delta
			BG_DoXORDelta( (char *)&g_cdata[num], (char *)&cd, (char *)&delta, sizeof(clientData_t) );
		}

		// pack it
		G_SerializeClientData( &delta, serialized, &serialsize );

		// compress the delta
		BG_RLE_Compress( (unsigned char *)serialized, serialsize, (unsigned char *)rle, &rlesize );

		// write it all out
		G_WriteDemoDirective2( DIR_CLIENTDATA, num, rlesize );
		G_WriteDemoData( rle, rlesize );

		// cache the data
		memcpy( &g_cdata[num], &cd, sizeof(clientData_t) );
		g_validCData[num] = qtrue;
	}

	G_WriteDirectiveEnd();
}

/*
G_RecordEntityData
*/
static void G_RecordEntityData( gentity_t *iter ) {
	int num = iter - g_entities;

	char rle[sizeof(entityData_t) + (sizeof(entityData_t) * 4) / 3];
	int rlesize = sizeof(rle);

	// if not being used
	if ( !iter->inuse ) {
		// and it *wasn't* being used
		if ( !g_validEData[num] ) {
			// not used, wasn't used - not interested
			return;
		}

		// it's stopped being used - we'll write a terminator record
		G_WriteDemoDirective2( DIR_ENTITYDATA, num, 0 );

		g_validEData[num] = qfalse;
	}
	// if being used
	else {
		entityData_t ed;
		entityData_t delta;
		char serialized[1024];
		int serialsize;

		// if there was no change on this frame, forget it
		if ( iter->demoFrameValid != level.demoFrame ) return;

		// don't record PIP portals
		if ( iter->s.eType == ET_INVISIBLE && (iter->r.svFlags & SVF_PORTAL) ) return;

		// stuff iter, iter->s, and iter->r into an entityData_t
		G_StuffEntityData( iter, &ed );
		
		// if it wasn't being used before
		if ( !g_validEData[num] ) {
			// key frame
			memcpy( &delta, &ed, sizeof(entityData_t) );
		}
		else {
			// get a delta
			BG_DoXORDelta( (char *)&g_edata[num], (char *)&ed, (char *)&delta, sizeof(entityData_t) );
		}

		// pack it
		G_SerializeEntityData( &delta, serialized, &serialsize );

		// compress the delta
		BG_RLE_Compress( (unsigned char *)serialized, serialsize, (unsigned char *)rle, &rlesize );

		G_WriteDemoDirective2( DIR_ENTITYDATA, num, rlesize );
		G_WriteDemoData( rle, rlesize );

		memcpy( &g_edata[num], &ed, sizeof(entityData_t) );
		g_validEData[num] = qtrue;
	}

	G_WriteDirectiveEnd();
}

/*
G_RecordServerCommand

Records a server command.
*/
void G_RecordServerCommand( int clientNum, const char *text ) {
	G_WriteDemoDirective2( DIR_SERVERCOMMAND, clientNum, strlen(text) );
	G_WriteDemoData( text, strlen(text) );
	G_WriteDirectiveEnd();
}

/*
G_RecordSDemoData

Called at the end of G_RunFrame() when a server-side demo
is recording.  Saves playerState_t, entityState_t, and
entityShared_t records for all valid entities and players.
*/
void G_RecordSDemoData( void ) {
	gentity_t *iter;
	char info[BIG_INFO_STRING];
	//int start, end;

	if ( level.outDemoFile == 0 ) {
		return;
	}

	//start = trap_Milliseconds();

	trap_GetConfigstring( CS_SERVERINFO, info, sizeof(info) );
	// if the serverinfo config string changed
	if ( strcmp(g_serverinfoConfigstring, info) != 0 ) {
		// record it
		G_RecordConfigstring( CS_SERVERINFO, info );
	}

	trap_GetConfigstring( CS_SYSTEMINFO, info, sizeof(info) );
	// if the systeminfo config string changed
	if ( strcmp(g_systeminfoConfigstring, info) != 0 ) {
		// record it
		G_RecordConfigstring( CS_SYSTEMINFO, info );
	}

	// write a beginning-of-frame
	G_WriteDemoDirective1( DIR_FRAMEBEGIN, level.time );
	G_WriteDirectiveEnd();

	// write playerState_t for up to 32 clients
	for ( iter = g_entities; iter < &g_entities[HALF_CLIENTS]; iter++ ) {
		G_RecordClientData( iter );
	}

	// write entityState_t and entityShared_t for non-client entities
	for ( iter = &g_entities[MAX_CLIENTS]; iter < &g_entities[ENTITYNUM_MAX_NORMAL]; iter++ ) {
		G_RecordEntityData( iter );
	}

	// write an end-of-frame
	G_WriteDemoDirective1( DIR_FRAMEEND, level.time );
	G_WriteDirectiveEnd();

	//end = trap_Milliseconds();
	//G_Printf("demo record time: %d\n", end - start);

	level.demoFrame++;

	if ( ss_demoLimit.integer != 0 && level.outDemoWrote >= ss_demoLimit.integer * 1024 * 1024 ) {
		G_Printf( "demo exceeded %d MB: stopped recording\n", ss_demoLimit.integer );

		// notify all referees
		for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
			if ( !iter->inuse || !iter->client || !iter->client->sess.referee ) continue;
			trap_SendServerCommand( iter - g_entities, va("rn \"^7demo exceeded %d MB\n^1stopped recording\n\"", ss_demoLimit.integer) );
			trap_SendServerCommand( iter - g_entities, va("print \"^7demo exceeded %d MB: ^7stopped recording\n\"", ss_demoLimit.integer) );
		}

		// stop recording the demo
		G_SStopRecord();
	}
}

/*
G_SetConfigstring

Wrapper for trap_SetConfigstring that saves the configstring
to the server-side demo if one is being recorded.
*/
void G_SetConfigstring( int num, const char *string ) {
	trap_SetConfigstring( num, string );

	if ( level.outDemoFile != 0 ) {
		G_RecordConfigstring( num, string );
	}
}

/*
G_LinkEntity

Wrapper for trap_LinkEntity which marks entities as changed
for server-side demo purposes.

The demo only writes changes, and changes can only be sent
to clients when they're linked.  This is a HUGE speed 
optimization.  It avoids tons of manual delta checking.
*/
void G_LinkEntity( gentity_t *ent ) {
	trap_LinkEntity( ent );
	ent->demoFrameValid = level.demoFrame;
}

/*
G_EntityChanged

Same as above, but doesn't link.
*/
void G_EntityChanged( gentity_t *ent ) {
	ent->demoFrameValid = level.demoFrame;
}

/*
G_UnlinkEntity

Wrapper for trap_UnlinkEntity which marks entities as changed
for server-side demo purposes.
*/
void G_UnlinkEntity( gentity_t *ent ) {
	trap_UnlinkEntity( ent );
	ent->demoFrameValid = level.demoFrame;
}

/*
G_SendServerCommand

Wrapper for trap_SendServerCommand that records the command
to the server-side demo if one is in progress.
*/
void G_SendServerCommand( int clientNum, const char *text ) {
	trap_SendServerCommand( clientNum, text );

	if ( level.outDemoFile != 0 ) {
		G_RecordServerCommand( clientNum, text );
	}
}

/*
G_AdjustAreaPortalState

Wrapper for trap_AdjustAreaPortalState that saves the portal state.
This has to be done or the areas on the other sides of most open
doors won't be drawn.
*/
void G_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	trap_AdjustAreaPortalState( ent, open );
	ent->areaPortalState = open;
}

/*
G_DoNewMap
*/
static void G_DoNewMap( char *mapname, int fileloc ) {
	// set these so the demo can be picked back up
	trap_Cvar_Set( "##_continueDemoName", level.demoName );
	// start at the beginning
	trap_Cvar_Set( "##_continueDemoLoc", va("%d", fileloc) );

	// fake hard restart into starting a new map
	strcpy( sv_mapname.string, mapname );
	// enable cheats for demo playback
	sv_cheats.integer = 1;
	// call hard restart because it saves nextmap
	Svcmd_HardRestart_f();
}

/*
G_DoMapSettings
*/
static void G_DoMapSettings( char *info ) {
	char *s;

	s = Info_ValueForKey(info, "version");
	// if we're on the wrong version
	if ( Q_stricmp(s, SSDEMO_VERSION) != 0 ) {
		// oops!
		G_Error( "Demo version mismatch: version %s, demo version %s\n", SSDEMO_VERSION, s );
	}

	s = Info_ValueForKey(info, "sv_fps");
	// if there's an sv_fps setting
	if ( s[0] != '\0' ) {
		// change it
		trap_Cvar_Set( "sv_fps", s );
	}
}

/*
G_DoMapRestart
*/
static void G_DoMapRestart( void ) {
	gentity_t *iter;

	// end any intermission
	level.intermissiontime = 0;

	// reset the spectators so they feel like the map restarted
	for ( iter = g_entities; iter < &g_entities[HALF_CLIENTS]; iter++ ) {
		if ( !iter->inuse || iter->client == NULL ) continue;
		SetTeam( iter, "s", qtrue );
	}

	G_ClearEntityCache();

	// stop skipping at a map restart
	level.skipToDemoTime = 0;

	// signal that warmup is over
	level.demoWarmupTime = 0;
	trap_SetConfigstring( CS_WARMUP, "" );

	// a map_restart is sent to clients when a map_restart is done on
	// the server - but in server-side demo playback this doesn't happen
	// so we do it manually
	trap_SendServerCommand( -1, "map_restart" );
}

/*
G_ReadDemoHeader
*/
static qboolean G_ReadDemoHeader( int size ) {
	char info[MAX_STRING_CHARS];
	char *s;
#ifdef HUMAN_READABLE_DEMOS
	int fileloc = level.inDemoRead - 10; // the beginning of this header
#else
	int fileloc = level.inDemoRead - 2; // the beginning of this header
#endif

	// stop skipping - if we don't, the loop in G_RunDemoLoop() will
	// read the next map's configstrings and stuff which will screw things up
	level.skipToDemoTime = 0;

	G_ReadDemoData( info, size );
	info[size] = '\0';

	s = Info_ValueForKey(info, "map");
	// if we're not already on the map
	if ( Q_stricmp(s, sv_mapname.string) != 0 ) {
		// it's a screw-up, or it's a multi-map demo
		// either way, start the actual map
		G_DoNewMap( s, fileloc );
		return qtrue;
	}

	// read in the map settings (sv_fps, etc.)
	G_DoMapSettings( info );

	// if it's not at the beginning of the file
	if ( fileloc > 0 ) {
		// this demo header marks a map restart
		G_DoMapRestart();
	}

	return qfalse;
}

/*
G_ReadFrameBegin

Reads a "bf" directive.  Basically just sets the demo time right.
*/
static void G_ReadFrameBegin( int demoTime ) {
	level.previousDemoTime = level.demoTime;
	level.demoTime = demoTime;
	level.demoTimeDiff = level.demoTime - level.time;
}

/*
G_ReadClientData

Reads a "cd" directive.
*/
static void G_ReadClientData( int entnum, int size ) {
	G_AdjustEntityNum( &entnum );

	if ( size == 0 ) {
		// terminator record
		g_validCData[entnum] = qfalse;
		memset( &g_cdata[entnum], 0, sizeof(clientData_t) );
	}
	else {
		char buf[sizeof(clientData_t)];
		char unserialized[1024];
		int serialsize = sizeof(unserialized);
		clientData_t delta;
		clientData_t *cd;

		G_ReadDemoData( buf, size );

		// decompress the packed delta
		BG_RLE_Decompress( (unsigned char *)buf, size, (unsigned char *)unserialized, &serialsize );

		if ( serialsize == -1 ) {
			G_Error("RLE decompression error\n");
		}

		// unpack it
		G_UnserializeClientData( &delta, unserialized );

		cd = &g_cdata[entnum];

		BG_UndoXORDelta( (char *)cd, (char *)&delta, sizeof(clientData_t) );
		g_validCData[entnum] = qtrue;

		// if one of the recorded players is in intermission, they all are
		if ( cd->pm_type == PM_INTERMISSION ) {
			// stop skipping
			level.skipToDemoTime = 0;
			// set real clients to intermission
			BeginIntermission();
		}
	}
}

/*
G_ReadEntityData

Reads a "ed" directive.  Dumps the data into the entity.

Also responds to a terminator (empty) record.
*/
static void G_ReadEntityData( int entnum, int size ) {
	G_AdjustEntityNum( &entnum );

	if ( size == 0 ) {
		// terminator record
		g_validEData[entnum] = qfalse;
		memset( &g_edata[entnum], 0, sizeof(entityData_t) );
	}
	else {
		char buf[sizeof(entityData_t)];
		char unserialized[1024];
		int serialsize = sizeof(unserialized);
		entityData_t delta;
		entityData_t *ed;

		G_ReadDemoData( buf, size );

		// decompress the packed delta
		BG_RLE_Decompress( (unsigned char *)buf, size, (unsigned char *)unserialized, &serialsize );

		if ( serialsize == -1 ) {
			G_Error("RLE decompression error\n");
		}

		// unpack it
		G_UnserializeEntityData( &delta, unserialized );

		ed = &g_edata[entnum];

		BG_UndoXORDelta( (char *)ed, (char *)&delta, sizeof(entityData_t) );
		g_validEData[entnum] = qtrue;
	}
}

/*
Dumps cached entity data into the entities themselves and links them.
*/
static void G_RunFrameEnd( void ) {
	gentity_t *iter;

	// loop through the upper HALF_CLIENTS
	for ( iter = &g_entities[HALF_CLIENTS]; iter < &g_entities[MAX_CLIENTS]; iter++ ) {
		int entnum = iter - g_entities;

		// if the client data is valid
		if ( g_validCData[entnum] ) {
			// get a pointer to the source
			clientData_t *cd = &g_cdata[entnum];

			// make sure this is set right to avoid NULL pointer errors
			iter->client = level.clients + (iter - g_entities);

			// unstuff the client data into iter, iter->client, iter->client.pers,
			// iter->client.ps, iter->s, and iter->r
			G_UnstuffClientData( iter, cd );

			// mark as used and link
			iter->inuse = qtrue;
			trap_LinkEntity( iter );

			// don't send spectators to the clients (they shouldn't be recorded
			// but it doesn't hurt to be safe)
			if ( is_spectator(iter->client) ) {
				trap_UnlinkEntity( iter );
			}
		}
		else {
			if ( iter->client ) {
				// mark the player as disconnected (won't happen otherwise)
				iter->client->pers.connected = CON_DISCONNECTED;
			}
			iter->inuse = qfalse;
			trap_UnlinkEntity( iter );
		}
	}

	// loop through all entities except those belonging to clients
	for ( iter = &g_entities[MAX_CLIENTS]; iter < &g_entities[ENTITYNUM_MAX_NORMAL]; iter++ ) {
		int entnum = iter - g_entities;

		// if the entity data is valid
		if ( g_validEData[entnum] ) {
			// get a pointer to the source
			entityData_t *ed = &g_edata[entnum];

			// if this wasn't being used and it wasn't allocated by the demo
			if ( !g_wasValidEData[entnum] && iter->reference != NULL ) {
				// set the reference to NULL - the logic that relies on this
				// entity MUST be able to handle a sudden nullification
				*iter->reference = NULL;
				G_FreeEntity( iter );
			}

			// unstuff the entity data into iter, iter->s and iter->r
			G_UnstuffEntityData( iter, ed );

			// if a timeout entity
			if ( iter->s.eType == ET_TIMEOUT ) {
				// grab the timeout times
				level.timeout = iter->s.time;
				level.timein = iter->s.time2;
				level.totaltimeout = iter->s.powerups + iter->s.generic1 * 32768;
				level.timeoutteam = iter->s.eventParm;
			}

			// mark as used and link
			iter->inuse = qtrue;
			trap_LinkEntity( iter );
			g_wasValidEData[entnum] = qtrue;
		}
		else if ( g_wasValidEData[entnum] ) {
			G_FreeEntity( iter );
			g_wasValidEData[entnum] = qfalse;
		}
	}
}

/*
G_ReadFrameEnd

Reads a "ef" directive.
*/
static void G_ReadFrameEnd( void ) {

	// don't run the frame end now if skipping
	if ( level.skipToDemoTime != 0 ) {
		return;
	}

	// do a demo frame end
	G_RunFrameEnd();
}

/*
G_ReadConfigstring

Reads a "cs" directive.  Copies the read configstring into
the actual configstring.  Copies data into level_locals_t
when applicable.
*/
static void G_ReadConfigstring( int num, int size ) {
	char buf[BIG_INFO_STRING];

	G_ReadDemoData( buf, size );
	buf[size] = '\0';

	// adjust the client number up if this has to do with a client
	if ( num >= CS_PLAYERS && num < CS_PLAYERS + HALF_CLIENTS ) num += HALF_CLIENTS;

	// if it's a scores configstring, set the level data for
	// ranking purposes (so the scoreboard shows correctly)
	if ( num == CS_SCORES1 ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			level.teamScores[TEAM_RED] = atoi(buf);
		}
	}
	else if ( num == CS_SCORES2 ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			level.teamScores[TEAM_BLUE] = atoi(buf);
		}
	}
	// if it's a the recorded level.startTime
	else if ( num == CS_LEVEL_START_TIME ) {
		// grab it
		level.demoStartTime = atoi(buf);
		// adjust it for sending
		Com_sprintf( buf, sizeof(buf), "%d", G_AdjustedTime(level.demoStartTime) );
	}
	// if it's the recorded level.voteTime
	else if ( num == CS_VOTE_TIME ) {
		// if it's not blank
		if ( buf[0] != '\0' ) {
			// grab it
			level.demoVoteTime = atoi(buf);
			// adjust it for sending
			Com_sprintf( buf, sizeof(buf), "%d", G_AdjustedTime(level.demoVoteTime) );
		}
		else {
			// leave it alone, and set the demo's to 0
			level.demoVoteTime = 0;
		}
	}
	// if it's the recorded level.warmupTime
	else if ( num == CS_WARMUP ) {
		if ( buf[0] != '\0' ) {
			// grab it
			level.demoWarmupTime = atoi(buf);
			// if it's not a special negative
			if ( level.demoWarmupTime >= 0 ) {
				// adjust it
				Com_sprintf( buf, sizeof(buf), "%d", G_AdjustedTime(level.demoWarmupTime) );
			}
		}
		else {
			level.demoWarmupTime = 0;
		}
	}
	// if it's the reserved configstring containing all the CVAR_SERVERINFO vars
	else if ( num == CS_SERVERINFO ) {
		// send it as a different configstring
		num = CS_SSDEMOSERVERINFO;
	}
	// if it's the reserved configstring containing all the CVAR_SYSTEMINFO vars
	else if ( num == CS_SYSTEMINFO ) {
		// send this as a different configstring
		num = CS_SSDEMOSYSTEMINFO;
	}
	// never send these - they should never be recorded anyway
	else if ( num == CS_SSDEMOSTATE || num == CS_SSDEMOSERVERINFO || num == CS_SSDEMOSYSTEMINFO ) {
		return;
	}

	// actually send it
	trap_SetConfigstring( num, buf );
}

/*
G_ReadServerCommand

Reads a "sc" directive.  Sends the command.
*/
static void G_ReadServerCommand( int num, int size ) {
	char buf[8192];

	G_ReadDemoData( buf, size );
	buf[size] = '\0';

	// do the tinfo before adjusting for entity number because
	// the server command is recorded as being sent to a team
	// number rather than a client number
	if ( Q_strncmp(buf, "tinfo ", 6) == 0 ) {
		gentity_t *iter;

		// send no tinfo commands while skipping
		if ( level.skipToDemoTime != 0 ) {
			return;
		}

		// send the team info to every following spectator
		for ( iter = g_entities; iter < &g_entities[HALF_CLIENTS]; iter++ ) {
			if ( !iter->inuse || iter->client == NULL ) continue;

			if ( iter->client->sess.spectatorState == SPECTATOR_FOLLOW || 
					iter->client->sess.spectatorState == SPECTATOR_FROZEN ) {
				gentity_t *other;
				other = &g_entities[iter->client->sess.spectatorClient];

				if ( other->inuse && other->client != NULL &&
						other->client->sess.sessionTeam == num ) {
					trap_SendServerCommand( iter->s.clientNum, buf );
				}
			}
		}
		return;
	}

	// send no print, cp or chat commands while skipping
	if ( level.skipToDemoTime != 0 ) {
		if ( Q_strncmp(buf, "cp ", 3) == 0 ||
				Q_strncmp(buf, "print ", 6) == 0 ||
				Q_strncmp(buf, "chat ", 5) == 0 ) {
			return;
		}
	}

	G_AdjustEntityNum( &num );
	trap_SendServerCommand( num, buf );
}

//#define MAX_CSTRINGS_IN_FRAME 20

/*
G_PlayDemoFrame

Called by G_RunDemoLoop to run a demo frame.
*/
void G_PlayDemoFrame( int levelTime ) {
	int cstrings = 0;
	qboolean stop = qfalse;

	level.framenum++;

	while ( !stop ) {
		demoDirective_t dir;
		int n1, n2;

		// read a line header
		G_ReadDemoDirective( &dir, &n1, &n2 );

		switch ( dir ) {
		case DIR_DEMOHEADER:
			stop = G_ReadDemoHeader( n1 );
			break;

		case DIR_FRAMEBEGIN:
			G_ReadFrameBegin( n1 );
			break;

		case DIR_CLIENTDATA:
			G_ReadClientData( n1, n2 );
			break;

		case DIR_ENTITYDATA:
			G_ReadEntityData( n1, n2 );
			break;

		case DIR_FRAMEEND:
			G_ReadFrameEnd();
			stop = qtrue;
			break;

		case DIR_CONFIGSTRING:
			G_ReadConfigstring( n1, n2 );
			/*if ( ++cstrings > MAX_CSTRINGS_IN_FRAME ) {
				// don't want to overflow the clients
				stop = qtrue;
			}*/
			break;

		case DIR_SERVERCOMMAND:
			G_ReadServerCommand( n1, n2 );
			break;

		default:
			G_Error("Unknown server-side demo directive\n");
			break;
		}

		G_ReadDirectiveEnd();

		// if we've read past the end, stop now
		if ( level.inDemoRead >= level.inDemoSize ) {
			G_SStopDemo();
		}
	}

	// keep track of how many demo frames have gone by
	level.demoFrame++;
}

/*
G_SetDemoState

Sets the CS_SSDEMOSTATE configstring if it needs to change
*/
void G_SetDemoState( qboolean playing ) {
	char demostate[MAX_STRING_CHARS];

	trap_GetConfigstring( CS_SSDEMOSTATE, demostate, sizeof(demostate) );

	if ( !playing ) {
		if ( demostate[0] != '\0' ) {
			trap_SetConfigstring( CS_SSDEMOSTATE, "" );
			trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime) );

			if ( level.warmupTime == 0 ) {
				trap_SetConfigstring( CS_WARMUP, "" );
			}
			else {
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}

			if ( level.voteTime == 0 ) {
				trap_SetConfigstring( CS_VOTE_TIME, "" );
			}
			else {
				trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime) );
			}
		}
	}
	else {
		char buf[MAX_STRING_CHARS];

		Com_sprintf( buf, sizeof(buf), "td\\%d\\sk\\%d\\ps\\%d", level.demoTimeDiff, level.skipToDemoTime, level.demoPaused );
		if ( strcmp( buf, demostate) != 0 ) {
			trap_SetConfigstring( CS_SSDEMOSTATE, buf );
			trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", G_AdjustedTime(level.demoStartTime)) );

			if ( level.demoWarmupTime == 0 ) {
				// do nothing
			}
			else if ( level.demoWarmupTime > 0 ) {
				trap_SetConfigstring( CS_WARMUP, va("%i", G_AdjustedTime(level.demoWarmupTime)) );
			}
			else {
				trap_SetConfigstring( CS_WARMUP, va("%i", level.demoWarmupTime) );
			}

			if ( level.demoVoteTime == 0 ) {
				trap_SetConfigstring( CS_VOTE_TIME, "" );
			}
			else {
				trap_SetConfigstring( CS_VOTE_TIME, va("%i", G_AdjustedTime(level.demoVoteTime)) );
			}
		}
	}
}

/*
G_RunDemoLoop

Called instead of G_RunFrame() when a server-side dmeo is running.
Plays one or more demo frames depending on whether or not we're
currently skipping to a certain one.
*/
void G_RunDemoLoop( int levelTime ) {
	//int start;
	gentity_t *iter;

	if ( level.inDemoFile == 0 ) {
		return;
	}

	//start = trap_Milliseconds();

	level.previousTime = level.time;
	level.time = levelTime;

	// if we're paused
	if ( level.demoPaused ) {
		// adjust the time difference
		level.demoTimeDiff -= level.time - level.previousTime;
		// and run a demo frame end
		G_RunFrameEnd();
	}
	else {
		// if we're not skipping
		if ( level.skipToDemoTime == 0 ) {
			// play one frame
			G_PlayDemoFrame( levelTime );
		}
		else {
			int gametime;
			
			// play until we hit the frame we're skipping to
			do {
				G_PlayDemoFrame( levelTime );

				// if the demo is over
				if ( level.inDemoFile == 0 ) {
					level.skipToDemoTime = 0;
					break;
				}

				gametime = level.time - (level.demoStartTime - level.demoTimeDiff) - level.totaltimeout;
			}
			while ( level.skipToDemoTime != 0 && gametime < level.skipToDemoTime );

			// if we skipped to the destination
			if ( gametime == level.skipToDemoTime ) {
				// turn off skipping
				level.skipToDemoTime = 0;
			}

			// this is suppressed during playback if we're skipping, so
			// do it now
			G_RunFrameEnd();
		}
	}

	// set the demo state configstring and the level start time configstring
	G_SetDemoState( qtrue );

	// for all real clients
	for ( iter = g_entities; iter < &g_entities[level.maxclients]; iter++ ) {
		if ( !iter->inuse || iter->client == NULL ) continue;

		// call end-of-frame stuff
		ClientEndFrame( iter );

		// fix up ready mask for upper 32 clients
		iter->client->ps.stats[STAT_CLIENTS_READY2] = level.demoReadyMask[0];
		iter->client->ps.stats[STAT_CLIENTS_READY3] = level.demoReadyMask[1];
		// ready not defined during demo playback for real clients
		iter->client->ps.stats[STAT_CLIENTS_READY0] = 0;
		iter->client->ps.stats[STAT_CLIENTS_READY1] = 0;

		// make sure the clients are spectators
		if ( iter->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			SetTeam( iter, "s", qtrue );
		}
	}

	// set STAT_FLAGS for real clients
	G_SetFreezeStats();

	// if not during intermission
	if ( !level.intermissiontime ) {
		// recompute the scoreboard
		CalculateRanks();
	}

//multiview
	G_ReconcileClientNumIssues();
    G_ManagePipWindows();
//multiview

	// needed for realPing measurement
	level.frameStartTime = trap_Milliseconds();

	//G_Printf("demo playback time: %d\n", level.frameStartTime - start);
}

void G_SetFreezeStats( void ) {
	gentity_t *ent;

	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( !ent->inuse || !ent->client ) continue;

		//G_HashWeapon( ent );

		// clear all but the new round toggle
		ent->client->ps.stats[STAT_FLAGS] = ent->client->ps.stats[STAT_FLAGS] & (SF_NEWROUND | SF_FREEZEANGLES);

		if ( level.rain ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_RAIN;
		}

		if ( ent->client->sess.localClient ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_LOCAL;
		}

		if ( ent->client->sess.privateNetwork ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_PRIVATE;
		}

		if ( ent->client->sess.wallhack && ent->client->sess.sessionTeam >= TEAM_SPECTATOR &&
				ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_WALLHACK;
		}

		if ( ent->client->sess.referee ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_REF;
		}

		if ( level.timein > level.time ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_TIMEOUT;
		}

		if ( ent->freezeState ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_FROZEN;
		}

		if ( ent->client->forceScoreboardTime >= level.time ) {
			ent->client->ps.stats[STAT_FLAGS] |= SF_FORCESCOREBOARD;
		}

		ent->client->ps.stats[STAT_TEAM] = ent->client->sess.sessionTeam;
	}
}

static char *g_szCantExec[] = {
	// server console commands
	"remove",
	"putblue",
	"putred",
	"addbot",
	"forceteam",
	// client commands
	"autofollow",
	"stats",
	"statsacc",
	"statsred",
	"statsblue",
	"statsall",
	"statsteam",
	"statsteamred",
	"statsteamblue",
	"statsteamall",
	"topshots",
	"bottomshots",
	"timein",
	"timeout",
	"unpause",
	"pause",
	"lock",
	"unlock",
	"ref_clients",
	"ref_remove",
	"ref_putred",
	"ref_putblue",
	"ref_swap",
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
	"ref_timein",
	"ref_timeout",
	"ref_unpause",
	"ref_pause",
	"say_red",
	"say_blue",
	NULL
};

/*
G_CantExecDuringDemo

Returns qtrue if you're not allowed to execute the given
command during demo playback.
*/
qboolean G_CantExecDuringDemo( const char *cmd ) {
	int i;

	for ( i = 0; g_szCantExec[i] != NULL; i++ ) {
		if ( !Q_stricmp(cmd, g_szCantExec[i]) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
G_CleanUserinfo

Called by G_RunFrame() to make sure there is no valid userinfo
left for users >= 32.  This kind of crap would be in G_SStopDemo(),
except that a user can do a map_restart to avoid it.
*/
void G_CleanUserinfo( void ) {
	char buf[MAX_INFO_STRING];
	int i;

	for ( i = CS_PLAYERS + HALF_CLIENTS; i < CS_PLAYERS + MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( i, buf, sizeof(buf) );
		if ( buf[0] != '\0' ) {
			trap_SetConfigstring( i, "" );
		}
	}
}
//ssdemo

/*
TODO:

- test on multiplayer

- maps > 64k x 64k?

- ssdemo file transport

- automatically gzip/gunzip demo files

*/

// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t	bg_itemlist[] = 
{
	{
		NULL,
		NULL,
		{ NULL,
		NULL,
		0, 0} ,
/* icon */		NULL,
/* pickup */	NULL,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_shard", 
		"sound/misc/ar1_pkup.wav",
		{ "models/powerups/armor/shard.md3", 
		"models/powerups/armor/shard_sphere.md3",
		0, 0} ,
/* icon */		"icons/iconr_shard",
/* pickup */	"Armor Shard",
		5,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_combat", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_yel.md3",
		0, 0, 0},
/* icon */		"icons/iconr_yellow",
/* pickup */	"Armor",
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_body", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_red.md3",
		0, 0, 0},
/* icon */		"icons/iconr_red",
/* pickup */	"Heavy Armor",
		100,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	//
	// health
	//
/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_small",
		"sound/items/s_health.wav",
        { "models/powerups/health/small_cross.md3", 
		"models/powerups/health/small_sphere.md3", 
		0, 0 },
/* icon */		"icons/iconh_green",
/* pickup */	"5 Health",
		5,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health",
		"sound/items/n_health.wav",
        { "models/powerups/health/medium_cross.md3", 
		"models/powerups/health/medium_sphere.md3", 
		0, 0 },
/* icon */		"icons/iconh_yellow",
/* pickup */	"25 Health",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_large",
		"sound/items/l_health.wav",
        { "models/powerups/health/large_cross.md3", 
		"models/powerups/health/large_sphere.md3", 
		0, 0 },
/* icon */		"icons/iconh_red",
/* pickup */	"50 Health",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_mega",
		"sound/items/m_health.wav",
        { "models/powerups/health/mega_cross.md3", 
		"models/powerups/health/mega_sphere.md3", 
		0, 0 },
/* icon */		"icons/iconh_mega",
/* pickup */	"Mega Health",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},


	//
	// WEAPONS 
	//

/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_gauntlet", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/gauntlet/gauntlet.md3",
		0, 0, 0},
/* icon */		"icons/iconw_gauntlet",
/* pickup */	"Gauntlet",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shotgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/shotgun/shotgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_shotgun",
/* pickup */	"Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_machinegun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/machinegun/machinegun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_machinegun",
/* pickup */	"Machinegun",
		40,
		IT_WEAPON,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grenadel/grenadel.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_grenade",
/* pickup */	"Grenade Launcher",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/rocketl/rocketl.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_rocket",
/* pickup */	"Rocket Launcher",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_lightning", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/lightning/lightning.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_lightning",
/* pickup */	"Lightning Gun",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_railgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/railgun/railgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_railgun",
/* pickup */	"Railgun",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_plasmagun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/plasma/plasma.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_plasma",
/* pickup */	"Plasma Gun",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_bfg",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/bfg/bfg.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_bfg",
/* pickup */	"BFG10K",
		20,
		IT_WEAPON,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grapple/grapple.md3", 
		0, 0, 0},

/* icon */		"icons/iconw_grapple",
/* pickup */	"Grappling Hook",
		
		0,
		IT_WEAPON,
		WP_NONE, //WP_GRAPPLING_HOOK,

/* precache */ "",
/* sounds */ ""
	},

	//
	// AMMO ITEMS
	//

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_shells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/shotgunam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_shotgun",
/* pickup */	"Shells",
		10,
		IT_AMMO,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bullets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/machinegunam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_machinegun",
/* pickup */	"Bullets",
		50,
		IT_AMMO,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/grenadeam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_grenade",
/* pickup */	"Grenades",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_cells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/plasmaam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_plasma",
/* pickup */	"Cells",
		30,
		IT_AMMO,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_lightning",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/lightningam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_lightning",
/* pickup */	"Lightning",
		60,
		IT_AMMO,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/rocketam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_rocket",
/* pickup */	"Rockets",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_slugs",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/railgunam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_railgun",
/* pickup */	"Slugs",
		10,
		IT_AMMO,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bfg",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/bfgam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_bfg",
/* pickup */	"Bfg Ammo",
		15,
		IT_AMMO,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

	//
	// HOLDABLE ITEMS
	//
/*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_teleporter", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/teleporter.md3", 
		0, 0, 0},
/* icon */		"icons/teleporter",
/* pickup */	"Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_medkit", 
		"sound/items/holdable.wav",
        { 
		"models/powerups/holdable/medkit.md3", 
		"models/powerups/holdable/medkit_sphere.md3",
		0, 0},
/* icon */		"icons/medkit",
/* pickup */	"Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* precache */ "",
/* sounds */ "sound/items/use_medkit.wav"
	},

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_quad", 
		"sound/items/quaddamage.wav",
        { "models/powerups/instant/quad.md3", 
        "models/powerups/instant/quad_ring.md3",
		0, 0 },
/* icon */		"icons/quad",
/* pickup */	"Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
/* precache */ "",
/* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_enviro",
		"sound/items/protect.wav",
        { "models/powerups/instant/enviro.md3", 
		"models/powerups/instant/enviro_ring.md3", 
		0, 0 },
/* icon */		"icons/envirosuit",
/* pickup */	"Battle Suit",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* precache */ "",
/* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
	},

/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_haste",
		"sound/items/haste.wav",
        { "models/powerups/instant/haste.md3", 
		"models/powerups/instant/haste_ring.md3", 
		0, 0 },
/* icon */		"icons/haste",
/* pickup */	"Speed",
		30,
		IT_POWERUP,
		PW_HASTE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_invis",
		"sound/items/invisibility.wav",
        { "models/powerups/instant/invis.md3", 
		"models/powerups/instant/invis_ring.md3", 
		0, 0 },
/* icon */		"icons/invis",
/* pickup */	"Invisibility",
		30,
		IT_POWERUP,
		PW_INVIS,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_sight",
		"sound/items/protect.wav",
        { "models/powerups/instant/sight.md3", 
		"models/powerups/instant/quad_ring.md3", 
		0, 0 },
/* icon */		"icons/sight",
/* pickup */	"Sight",
		30,
		IT_POWERUP,
		PW_SIGHT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_regen",
		"sound/items/regeneration.wav",
        { "models/powerups/instant/regen.md3", 
		"models/powerups/instant/regen_ring.md3", 
		0, 0 },
/* icon */		"icons/regen",
/* pickup */	"Regeneration",
		30,
		IT_POWERUP,
		PW_REGEN,
/* precache */ "",
/* sounds */ "sound/items/regen.wav"
	},

/*QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_flight",
		"sound/items/flight.wav",
        { "models/powerups/instant/flight.md3", 
		"models/powerups/instant/flight_ring.md3", 
		0, 0 },
/* icon */		"icons/flight",
/* pickup */	"Flight",
		60,
		IT_POWERUP,
		PW_FLIGHT,
/* precache */ "",
/* sounds */ "sound/items/flight.wav"
	},

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_redflag",
		NULL,
        { "models/flags/r_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_red1",
/* pickup */	"Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_blueflag",
		NULL,
        { "models/flags/b_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_blu1",
/* pickup */	"Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ ""
	},

#ifdef MISSIONPACK
/*QUAKED holdable_kamikaze (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_kamikaze", 
		"sound/items/holdable.wav",
        { "models/powerups/kamikazi.md3", 
		0, 0, 0},
/* icon */		"icons/kamikaze",
/* pickup */	"Kamikaze",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* precache */ "",
/* sounds */ "sound/items/kamikazerespawn.wav"
	},

/*QUAKED holdable_portal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_portal", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/porter.md3",
		0, 0, 0},
/* icon */		"icons/portal",
/* pickup */	"Portal",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_invulnerability", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/invulnerability.md3", 
		0, 0, 0},
/* icon */		"icons/invulnerability",
/* pickup */	"Invulnerability",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_nails (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_nails",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/nailgunam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_nailgun",
/* pickup */	"Nails",
		20,
		IT_AMMO,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_mines",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/proxmineam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_proxlauncher",
/* pickup */	"Proximity Mines",
		10,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_belt",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/chaingunam.md3", 
		0, 0, 0},
/* icon */		"icons/icona_chaingun",
/* pickup */	"Chaingun Belt",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ ""
	},

	//
	// PERSISTANT POWERUP ITEMS
	//
/*QUAKED item_scout (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_scout",
		"sound/items/scout.wav",
        { "models/powerups/scout.md3", 
		0, 0, 0 },
/* icon */		"icons/scout",
/* pickup */	"Scout",
		30,
		IT_PERSISTANT_POWERUP,
		PW_SCOUT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_guard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_guard",
		"sound/items/guard.wav",
        { "models/powerups/guard.md3", 
		0, 0, 0 },
/* icon */		"icons/guard",
/* pickup */	"Guard",
		30,
		IT_PERSISTANT_POWERUP,
		PW_GUARD,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_doubler",
		"sound/items/doubler.wav",
        { "models/powerups/doubler.md3", 
		0, 0, 0 },
/* icon */		"icons/doubler",
/* pickup */	"Doubler",
		30,
		IT_PERSISTANT_POWERUP,
		PW_DOUBLER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_ammoregen",
		"sound/items/ammoregen.wav",
        { "models/powerups/ammo.md3",
		0, 0, 0 },
/* icon */		"icons/ammo_regen",
/* pickup */	"Ammo Regen",
		30,
		IT_PERSISTANT_POWERUP,
		PW_AMMOREGEN,
/* precache */ "",
/* sounds */ ""
	},

	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in One Flag CTF games
*/
	{
		"team_CTF_neutralflag",
		NULL,
        { "models/flags/n_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_neutral1",
/* pickup */	"Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_redcube",
		"sound/misc/am_pkup.wav",
        { "models/powerups/orb/r_orb.md3",
		0, 0, 0 },
/* icon */		"icons/iconh_rorb",
/* pickup */	"Red Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_bluecube",
		"sound/misc/am_pkup.wav",
        { "models/powerups/orb/b_orb.md3",
		0, 0, 0 },
/* icon */		"icons/iconh_borb",
/* pickup */	"Blue Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED weapon_nailgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_nailgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/nailgun/nailgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_nailgun",
/* pickup */	"Nailgun",
		10,
		IT_WEAPON,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_prox_launcher", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/proxmine/proxmine.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_proxlauncher",
/* pickup */	"Prox Launcher",
		5,
		IT_WEAPON,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons/proxmine/wstbtick.wav "
			"sound/weapons/proxmine/wstbactv.wav "
			"sound/weapons/proxmine/wstbimpl.wav "
			"sound/weapons/proxmine/wstbimpm.wav "
			"sound/weapons/proxmine/wstbimpd.wav "
			"sound/weapons/proxmine/wstbactv.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_chaingun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/vulcan/vulcan.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_chaingun",
/* pickup */	"Chaingun",
		80,
		IT_WEAPON,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ "sound/weapons/vulcan/wvulwind.wav"
	},
#endif

	{
		"item_botroam",
		NULL,
		{ NULL, NULL, NULL, NULL },
		"",
		"Bot Roam",
		0,
		IT_INVISIBLE, 0,
		"",
		""
	},

	// end of list marker
	{NULL}
};

int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( (bg_itemlist[i].giType == IT_POWERUP || 
					bg_itemlist[i].giType == IT_TEAM ||
					bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) && 
			bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *pickupName ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;
#ifdef MISSIONPACK
	int		upperBound;
#endif

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}
//freeze
	if ( ent->modelindex2 == 1 && ent->otherEntityNum == ps->clientNum + 1 ) {
		return qfalse;
	}
//freeze

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
//freeze
		if ( ent->modelindex2 == 255 && ps->stats[ STAT_WEAPONS ] & ( 1 << item->giTag ) ) {
			return qfalse;
		}
//freeze
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
#ifdef MISSIONPACK
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
			return qfalse;
		}

		// we also clamp armor to the maxhealth for handicapping
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else {
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;
		}

		if ( ps->stats[STAT_ARMOR] >= upperBound ) {
			return qfalse;
		}
#else
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
#endif
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
#ifdef MISSIONPACK
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else
#endif
		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		return qtrue;	// powerups are always picked up

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if ( ps->stats[STAT_PERSISTANT_POWERUP] ) {
			return qfalse;
		}

		// check team only
		if( ( ent->generic1 & 2 ) && ( ps->persistant[PERS_TEAM] != TEAM_RED ) ) {
			return qfalse;
		}
		if( ( ent->generic1 & 4 ) && ( ps->persistant[PERS_TEAM] != TEAM_BLUE ) ) {
			return qfalse;
		}

		return qtrue;
#endif

	case IT_TEAM: // team items, such as flags
#ifdef MISSIONPACK		
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG ) {
				return qtrue;
			}
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			}
		}
#endif
		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

#ifdef MISSIONPACK
		if( gametype == GT_HARVESTER ) {
			return qtrue;
		}
#endif
		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef Q3_VM
#ifndef NDEBUG // bk0001204
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

#ifdef _DEBUG
char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_POGO_SHORT",
	"EV_POGO_MEDIUM",
	"EV_POGO_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

//#ifdef MISSIONPACK
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
//#endif

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL",

//freeze
	"EV_ICE_CHIPS",
	"EV_TIMEOUT",
	"EV_LAST_PLAYER",
	"EV_FREEZING",
	"EV_GRAPPLE_FIRE",
	"EV_EXPLODING_ICE",
//freeze
};
#endif

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );

//freeze
	BG_ReleaseGrapple( ps );
//freeze
}

void BG_ReleaseGrapple( playerState_t *ps ) {
	if ( ps->pm_flags & (PMF_GRAPPLE_FLY | PMF_GRAPPLE_PULL) ) {
		ps->grappleTime = GRAPPLE_DELAY;
	}
	else {
		ps->grappleTime = 0;
	}
	ps->pm_flags &= ~(PMF_GRAPPLE_FLY | PMF_GRAPPLE_PULL);
}

static const char *g_weaponNames[WP_NUM_WEAPONS] = {
	"None",
	"Gauntlet",
	"Machinegun",
	"Shotgun",
	"G. Launcher",
	"R. Launcher",
	"Lightning Gun",
	"Railgun",
	"Plasma Gun",
	"BFG10K",
//	"Grapple",
#ifdef MISSIONPACK
	"Nailgun",
	"Prox Launcher",
	"Chaingun",
#endif
};

const char *BG_WeaponName( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return g_weaponNames[WP_NONE];
	}

	return g_weaponNames[weapon];
}

int BG_ModToWeapon( int mod ) {
	switch ( mod ) {
	case MOD_SHOTGUN:
		return WP_SHOTGUN;

	case MOD_GAUNTLET:
		return WP_GAUNTLET;

	case MOD_MACHINEGUN:
		return WP_MACHINEGUN;

	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
		return WP_GRENADE_LAUNCHER;

	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
		return WP_ROCKET_LAUNCHER;

	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
		return WP_PLASMAGUN;

	case MOD_RAILGUN:
		return WP_RAILGUN;

	case MOD_LIGHTNING:
		return WP_LIGHTNING;

	case MOD_BFG:
	case MOD_BFG_SPLASH:
		return WP_BFG;

#ifdef MISSIONPACK
	case MOD_NAIL:
		return WP_NAILGUN;

	case MOD_CHAINGUN:
		return WP_CHAINGUN;

	case MOD_PROXIMITY_MINE:
		return WP_PROX_LAUNCHER;
#endif
/*
	case MOD_GRAPPLE:
		return WP_GRAPPLING_HOOK;
*/
	}

	return WP_NONE;
}

int OtherTeam(int team) {
	if (team==TEAM_RED)
		return TEAM_BLUE;
	else if (team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *TeamNameColor( int team ) {
	if (team==TEAM_RED)
		return "^1Red";
	else if (team==TEAM_BLUE)
		return "^4Blue";
	else if (team==TEAM_REDCOACH)
		return "^3Red Coach";
	else if (team==TEAM_BLUECOACH)
		return "^3Blue Coach";
	else if (team==TEAM_SPECTATOR)
		return "^5Spec";

	return "^7Free";
}

int strlencolor( char *str ) {
	int len = 0;
	char *curr = str;
	qboolean lastcaret = qfalse;

	while ( *curr ) {
		if ( lastcaret && *curr != '^' ) {
			len -= 2;
			lastcaret = qfalse;
			continue;
		}

		if ( *curr == '^' ) {
			lastcaret = qtrue;
		}
		else {
			lastcaret = qfalse;
		}

		len++;
		curr++;
	}

	return len;
}

int HexChar2Int( char h ) {
	if ( h >= '0' && h <= '9' ) return h - '0';
	if ( h >= 'a' && h <= 'f' ) return h - 'a' + 10;
	if ( h >= 'A' && h <= 'F' ) return h - 'A' + 10;

	return 0;
}

qboolean String2RGB( char *hex, vec3_t rgb ) {
	rgb[3] = 1.0f;
	if(strlen(hex) == 1){
		switch(hex[0]){
		case '8': //black
		case '0': 
			rgb[0] = 0.0f; rgb[1] = 0.0f; rgb[2] = 0.0f;
			return qtrue;
		case '1': //red
			rgb[0] = 1.0f; rgb[1] = 0.0f; rgb[2] = 0.0f;
			return qtrue;
		case '2': //green
			rgb[0] = 0.0f; rgb[1] = 1.0f; rgb[2] = 0.0f;
			return qtrue;
		case '3': //yellow
			rgb[0] = 1.0f; rgb[1] = 1.0f; rgb[2] = 0.0f;
			return qtrue;
		case '4': //blue
			rgb[0] = 0.0f; rgb[1] = 0.0f; rgb[2] = 1.0f;
			return qtrue;
		case '5': //cyan (lt blue)
			rgb[0] = 0.0f; rgb[1] = 1.0f; rgb[2] = 1.0f;
			return qtrue;
		case '6': //magenta (pink)
			rgb[0] = 1.0f; rgb[1] = 0.0f; rgb[2] = 1.0f;
			return qtrue;
		case '7': //white
			rgb[0] = 1.0f; rgb[1] = 1.0f; rgb[2] = 1.0f;
			return qtrue;
		}
	}

	if ( hex[0] == '#') {
		char *str = &hex[1];

		if ( strlen(str) != 6 ) {
			rgb[0] = rgb[1] = rgb[2] = 1.0f;
			return qfalse;
		}

		rgb[0] = (float)(HexChar2Int(str[0]) * 16 + HexChar2Int(str[1])) / 255.0f;
		rgb[1] = (float)(HexChar2Int(str[2]) * 16 + HexChar2Int(str[3])) / 255.0f;
		rgb[2] = (float)(HexChar2Int(str[4]) * 16 + HexChar2Int(str[5])) / 255.0f;

		return qtrue;
	}
	else if ( hex[0] == '0' && hex[1] == 'x' ) {
		char *str = &hex[2];

		if ( strlen(str) != 6 ) {
			rgb[0] = rgb[1] = rgb[2] = 1.0f;
			return qfalse;
		}

		rgb[0] = (float)(HexChar2Int(str[0]) * 16 + HexChar2Int(str[1])) / 255.0f;
		rgb[1] = (float)(HexChar2Int(str[2]) * 16 + HexChar2Int(str[3])) / 255.0f;
		rgb[2] = (float)(HexChar2Int(str[4]) * 16 + HexChar2Int(str[5])) / 255.0f;

		return qtrue;
	}
	else {
		if ( !Q_stricmp(hex, "white") ) {
			rgb[0] = 1.0f;
			rgb[1] = 1.0f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "black") ) {
			rgb[0] = 0.0f;
			rgb[1] = 0.0f;
			rgb[2] = 0.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "red") ) {
			rgb[0] = 1.0f;
			rgb[1] = 0.0f;
			rgb[2] = 0.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "green") ) {
			rgb[0] = 0.0f;
			rgb[1] = 1.0f;
			rgb[2] = 0.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "blue") ) {
			rgb[0] = 0.0f;
			rgb[1] = 0.0f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "yellow") ) {
			rgb[0] = 1.0f;
			rgb[1] = 1.0f;
			rgb[2] = 0.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "magenta") ) {
			rgb[0] = 1.0f;
			rgb[1] = 0.0f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "cyan") ) {
			rgb[0] = 0.0f;
			rgb[1] = 1.0f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "gray") || !Q_stricmp(hex, "grey") ) {
			rgb[0] = 0.5f;
			rgb[1] = 0.5f;
			rgb[2] = 0.5f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltred") ) {
			rgb[0] = 1.0f;
			rgb[1] = 0.6f;
			rgb[2] = 0.5f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltgreen") ) {
			rgb[0] = 0.5f;
			rgb[1] = 1.0f;
			rgb[2] = 0.6f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltblue") ) {
			rgb[0] = 0.5f;
			rgb[1] = 0.6f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltyellow") ) {
			rgb[0] = 1.0f;
			rgb[1] = 1.0f;
			rgb[2] = 0.5f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltmagenta") ) {
			rgb[0] = 1.0f;
			rgb[1] = 0.5f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltcyan") ) {
			rgb[0] = 0.5f;
			rgb[1] = 1.0f;
			rgb[2] = 1.0f;
			return qtrue;
		}
		else if ( !Q_stricmp(hex, "ltgray") || !Q_stricmp(hex, "ltgrey") ) {
			rgb[0] = 0.75f;
			rgb[1] = 0.75f;
			rgb[2] = 0.75f;
			return qtrue;
		}
	}

	rgb[0] = rgb[1] = rgb[2] = 1.0f;
	return qfalse;
}

char *BG_IntToCheatKey( int cheatkey, int somebits ) {
	static char key[8];
	int temp = cheatkey, save;
	int i;

	for ( i = 0; i < 10; i++ ) {
		temp ^= somebits;
		somebits <<= 3;
	}

	if ( temp < 0 ) {
		temp = -temp;
	}

	save = temp;

	key[0] = (temp % 26) + 'a';
	temp /= 26;
	key[1] = (temp % 26) + 'a';
	temp /= 26;
	key[2] = (temp % 26) + 'a';
	temp /= 26;
	key[3] = (temp % 26) + 'a';
	temp /= 26;
	key[4] = (temp % 26) + 'a';
	temp /= 26;
	key[5] = (temp % 26) + 'a';
	temp /= 26;
	key[6] = (temp % 26) + 'a';

	i = Com_Clamp(4, 7, ((save & (1 << 13)) >> 13) + ((save & (1 << 29)) >> 28) + 4);
	key[i] = '\0';

	return key;
}

unsigned char g_lettersub[256];
qboolean g_donelettersub = qfalse;

static void BG_CheckLetterSub()
{
	int seed = 27736;
	int i, one, two;
	unsigned char temp;

	if ( !g_donelettersub ) {
		for ( i = 0; i < 256; i++ ) {
			g_lettersub[i] = i % 256;
		}

		for ( i = 0; i < 256 * 7; i++ ) {
			one = Q_random( &seed ) * 255;
			two = Q_random( &seed ) * 255;

			if ( one != two ) {
				temp = g_lettersub[one];
				g_lettersub[one] = g_lettersub[two];
				g_lettersub[two] = temp;
			}
		}

		g_donelettersub = qtrue;
	}
}

// Returns a 16-bit hash of the name
static int BG_HashForName( const char *name ) {
	int key, i;
	int numers[7] = { 17443, 11317, 31324, 27982, 23324, 11113, 29543 };
	int denoms[7] = {  9833,  9933,  7543,  8443,  8344,  9933,  7373 };
	const int numfactors = 7;

	BG_CheckLetterSub();

	key = 7;
	for ( i = 0; name[i] != '\0'; i++ ) {
		key = ((key + g_lettersub[name[i]]) * numers[i % numfactors] / denoms[i % numfactors]) % 65536;
	}

	return key;
}

int BG_HashTeam( int team, const char *name ) {
	int hash = BG_HashForName(name);
	int teamhash = (hash + team) % TEAM_NUM_TEAMS;

//	Com_Printf("name: %s  ^7hash: %d\n", name, hash);

	return teamhash;
}

int BG_UnhashTeam( int teamhash, const char *name ) {
	int i;

	for ( i = TEAM_FREE; i < TEAM_NUM_TEAMS; i++ ) {
		if ( teamhash == BG_HashTeam( i, name ) ) {
			return i;
		}
	}

	return TEAM_FREE;
}

/*
int BG_HashWeapon( int weaponnum, const char *name, int time ) {
	int weaponhash = (BG_HashForName(name) + (int)((float)time / 7763.7982) + weaponnum) % WP_NUM_WEAPONS;
	return weaponhash;
}

int BG_UnhashWeapon( int weaponhash, const char *name, int time ) {
	int i;

	for ( i = WP_NONE; i < WP_NUM_WEAPONS; i++ ) {
		if ( weaponhash == BG_HashWeapon( i, name, time ) ) {
			return i;
		}
	}

	return WP_NONE;
}
*/

#define HASH_DISTANCE 512.0f

// the name is the map name, and the number is the client number
void BG_HashOrigin( vec3_t origin, const char *name, int num, int dir ) {
	int fac, hash;

	//Com_Printf("name: %s, num: %d\n", name, num);

	num++;

	hash = 27129 * num / 1194 + 5577 * BG_HashForName(name) / 313;
	fac = 587 * hash / 47;

	origin[0] += ((int)(cos(177 * fac / 447) * HASH_DISTANCE)) * dir;
	origin[1] += ((int)(sin(29 * fac / 191) * HASH_DISTANCE)) * dir;
	origin[2] += ((int)(cos(47 * fac / 91) * HASH_DISTANCE)) * dir;
}

static qboolean ShouldSwapDeadFlag( const char *name, int num )
{
	int hash;

	num++;

	hash = 211232 * num / 1991 + 5775 * BG_HashForName(name) / 297;

	// this modulus needs to be prime and not a factor in any multiplication above
	if ( (hash % 13) >= 6 ) {
		return qtrue;
	}
	else {
		return qfalse;
	}
}

void BG_HashDeadFlag( int *eFlags, const char *name, int num ) {
	if ( ShouldSwapDeadFlag(name, num) ) {
		*eFlags ^= EF_DEAD;
	}
}

void BG_UnhashDeadFlag( int *eFlags, const char *name, int num ) {
	if ( ShouldSwapDeadFlag(name, num) ) {
		*eFlags ^= EF_DEAD;
	}
}


int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void trap_FS_FCloseFile( fileHandle_t f );

void BG_WriteDescriptionTxt( void ) {
	fileHandle_t f;

	trap_FS_FOpenFile( "description.txt", &f, FS_WRITE );
	if ( f ) {
		trap_FS_Write( DESCRIPTION_TXT, strlen(DESCRIPTION_TXT), f );
		trap_FS_FCloseFile( f );
	}
}

void BG_RLE_Compress(unsigned char *buf, int len, unsigned char *out, int *compressedLen)
{
	int i;
	unsigned char *outPtr = out;

	unsigned char byte1;
	unsigned char byte2;
	unsigned char frame_size;
	unsigned char array[129];

	while (len)
	{
		byte1 = *buf;
		buf++;
		len--;
		frame_size = 1;

		if (len)
		{
			byte2 = *buf;
			buf++;
			len--;
			frame_size = 2;

			do 
			{
				if (byte1 == byte2)
				{ 
					while (len && (byte1 == byte2) && (frame_size < 129))
					{ 
						byte2 = *buf;
						buf++;
						len--;
						frame_size++;
					}

					if (byte1 == byte2)
					{ 
						*outPtr = frame_size+126;
						outPtr++;
						*outPtr = byte1;
						outPtr++;

						if (len)
						{
							byte1=*buf;
							buf++;
							len--;
							frame_size = 1;
						}
						else
						{
							frame_size = 0;
						}
					}
					else  
					{ 
						*outPtr = 125+frame_size;
						outPtr++;
						*outPtr = byte1;
						outPtr++;
						byte1 = byte2;
						frame_size = 1;
					}

					if (len)
					{ 
						byte2 = *buf;
						buf++;
						len--;
						frame_size = 2;
					}
				}
				else // Prepare the array of comparisons where will be stored all the identical bytes
				{ 
					*array = byte1;
					array[1] = byte2;

					while (len && (array[frame_size-2] != array[frame_size-1]) && (frame_size  < 128))
					{ 
						array[frame_size] = *buf;
						buf++;
						len--;
						frame_size++;
					}

					// Do we meet a sequence of all different bytes followed by identical byte?
					if (array[frame_size-2] == array[frame_size-1])
					{
						// Yes, then don't count the two last bytes
						*outPtr = frame_size-3;
						outPtr++;

						for (i=0; i<frame_size-2; i++)
						{
							*outPtr = array[i];
							outPtr++;
						}

						byte1 = array[frame_size-2];
						byte2 = byte1;
						frame_size = 2;
					}
					else 
					{
						*outPtr = frame_size-1;
						outPtr++;

						for (i=0; i<frame_size; i++)
						{
							*outPtr = array[i];
							outPtr++;
						}

						if (!len)
						{
							frame_size = 0;
						}
						else
						{
							byte1 = *buf;
							buf++;
							len--;

							if (!len)
							{
								frame_size = 1;
							}
							else
							{
								byte2 = *buf;
								buf++;
								len--;
								frame_size = 2;
							}
						}
					}
				}
			}
			while (len || (frame_size >= 2));

			if (frame_size == 1)
			{
				*outPtr = 0;
				outPtr++;
				*outPtr = byte1;
				outPtr++;
			}
		}
	}

	*compressedLen = outPtr - out;
}

void BG_RLE_Decompress(unsigned char *buf, int len, unsigned char *out, int *uncompressLen)
{
	unsigned char header;
	unsigned char *outPtr = out;
	int i;
	int outSize = 0;

	while (len)
	{
		header = *buf;
		buf++;
		len--;

		if (!(header & 128))
		{
			// There are header+1 different bytes.
			for (i=0; i<=header; i++)
			{
				if (outSize >= *uncompressLen)
				{
					*uncompressLen = -1;
					return;
				}

				*outPtr = *buf;
				outPtr++;
				outSize++;
				buf++;
				len--;
			}
		}
		else
		{
			int n = (header & 127) + 2;

			for (i=0; i<n; i++)
			{
				if (outSize >= *uncompressLen)
				{
					*uncompressLen = -1;
					return;
				}

				*outPtr = *buf;
				outPtr++;
				outSize++;
			}

			buf++;
			len--;
		}
	}

	*uncompressLen = outSize;
}

const char *BG_Cvar_GetString( const char *name ) {
	static char stringbuffers[25][MAX_STRING_CHARS];
	static int buffernum = 0;
	char *ret = stringbuffers[buffernum];

	buffernum++;
	if ( buffernum >= 25 ) {
		buffernum = 0;
	}

	trap_Cvar_VariableStringBuffer( name, ret, MAX_STRING_CHARS );
	return ret;
}

// this is slightly munged because the engine truncates strings that contain //
// the number 63 SHOULD be represented by a '/' but we have to use a '-' instead

static char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-"
						 "????????????????????????????????????????????????????????????????"
						 "????????????????????????????????????????????????????????????????"
						 "????????????????????????????????????????????????????????????????";

static char index_64[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 63, -1, -1,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

#define CHAR64(c) (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

qboolean BG_Base64_Encode( const char *_in, int inlen, char *_out, int *outlen ) {
	const unsigned char *in = (const unsigned char *)_in;
	unsigned char *out = (unsigned char *)_out;
	unsigned char oval;
	char *blah;
	int olen;
	int outmax = *outlen;

	olen = BASE64_ENCODE_SIZE( inlen );

	if (outmax < olen) {
		return qfalse;
	}

	*outlen = olen;

	blah = (char *)out;
	while ( inlen >= 3 )
	{
		// user provided max buffer size; make sure we don't go over it
		*out++ = basis_64[in[0] >> 2];
		*out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = basis_64[in[2] & 0x3f];
		in += 3;
		inlen -= 3;
	}

	if ( inlen > 0 )
	{
		// user provided max buffer size; make sure we don't go over it
		*out++ = basis_64[in[0] >> 2];
		oval = (in[0] << 4) & 0x30;
		if ( inlen > 1 ) {
			oval |= in[1] >> 4;
		}
		*out++ = basis_64[oval];
		*out++ = (inlen < 2) ? '=' : basis_64[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}

	if ( olen < outmax ) {
		*out = '\0';
	}

	return qtrue;
}

qboolean BG_Base64_Decode(const char *in, int inlen, char *out, int *outlen) {
	int len = 0, lup;
	int c1, c2, c3, c4;

	if (in[0] == '+' && in[1] == ' ') {
		in += 2;
	}

	if (*in == '\0') {
		return qfalse;
	}

	for (lup = 0; lup < inlen / 4; lup++) {
		c1 = in[0];
		if (CHAR64(c1) == -1) {
			return qfalse;
		}

		c2 = in[1];
		if (CHAR64(c2) == -1) {
			return qfalse;
		}

		c3 = in[2];
		if (c3 != '=' && CHAR64(c3) == -1) {
			return qfalse;
		}

		c4 = in[3];
		if (c4 != '=' && CHAR64(c4) == -1) {
			return qfalse;
		}

		in += 4;

		*out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
		++len;

		if (c3 != '=') {
			*out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
			++len;
			if (c4 != '=') {
				*out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
				++len;
			}
		}
	}

	*out = 0;
	*outlen = len;

	return qtrue;
}

/*
BG_DoXORDelta

Populates "delta" with the XOR of "prev" and "next".
*/
void BG_DoXORDelta( const char *prev, const char *next, char *delta, int size ) {
	for ( ; size > 0; size-- ) {
		*(delta++) = *(next++) ^ *(prev++);
	}
}

/*
BG_UndoXORDelta

Populates "dest" with the XOR of "dest" and "delta".
*/
void BG_UndoXORDelta( char *dest, const char *delta, int size ) {
	for ( ; size > 0; size-- ) {
		*(dest++) ^= *(delta++);
	}
}

/*
BG_IsAll

Returns qtrue if the entire given string is equal to c
*/
qboolean BG_IsAllChar( const void *s, unsigned char c, int size ) {
	unsigned char *curr = (unsigned char *)s;

	while ( size-- ) {
		if ( *curr != c ) return qfalse;
		curr++;
	}

	return qtrue;
}

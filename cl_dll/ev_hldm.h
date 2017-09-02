//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT2, //YELLOWSHIFT Additional firing animations.
	GLOCK_SHOOT3,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};
 
enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_FIRE3,
	SHOTGUN_FIRE4, //YELLOWSHIFT Additional firing animations and rename of FIRE2 to BIGFIRE to prevent conflicts.
	SHOTGUN_FIRE5,
	SHOTGUN_FIREBIG,
	SHOTGUN_FIREBIG2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3,
};

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e {
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

//YELLOWSHIFT Weapon Entries

enum saw_e	{
	SAW_LONGIDLE = 0,
	SAW_IDLE1,
	SAW_RELOAD,
	SAW_HOLSTER,
	SAW_DEPLOY,
	SAW_FIRE1,
	SAW_FIRE2,
	SAW_FIRE3
}; 

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_IDLE5,
	DEAGLE_SHOOT,
	DEAGLE_SHOOTEMPTY,
	DEAGLE_RELOADEMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW,
	DEAGLE_HOLSTER
};

enum doublebarrel_e {
	DOUBLEBARREL_IDLE = 0,
	DOUBLEBARREL_FIRE,
	DOUBLEBARREL_FIRE2,
	DOUBLEBARREL_FIRE3,
	DOUBLEBARREL_FIRE4,
	DOUBLEBARREL_FIRE5,
	DOUBLEBARREL_FIREBIG,
	DOUBLEBARREL_FIREBIG2,
	DOUBLEBARREL_RELOAD,
	DOUBLEBARREL_PUMP,
	DOUBLEBARREL_START_RELOAD,
	DOUBLEBARREL_DRAW,
	DOUBLEBARREL_HOLSTER,
	DOUBLEBARREL_IDLE4,
	DOUBLEBARREL_IDLE_DEEP
};


enum m40a1_e {
	M40A1_IDLE1 = 0,
	M40A1_FIDGET,
	M40A1_FIRE1,
	M40A1_RELOAD,
	M40A1_HOLSTER,
	M40A1_DRAW,
	M40A1_IDLE2,
	M40A1_IDLE3,
};

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );
void EV_HLDM_MuzzleFlash( vec3_t pos, float amount ); //YELLOWSHIFT Credit to Cale 'Mazor' Dunlap for the Muzzleflash code!
#endif // EV_HLDMH
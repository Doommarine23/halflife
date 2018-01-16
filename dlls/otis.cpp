/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

/*

Thanks to Gman from HIT for pieces used from tutorial on how to do several sub-model groups!

YELLOWSHIFT CHANGE/ADDITIONS
Entire Otis NPC - Could I have just reused all of barney's code via inheritance? Yeah but I was feeling lazy :)

All changes marked with //YELLOWSHIFT 

TO DO LIST:

Pain Skins DONE - Needs more variations
Mourn dead allies and player - Almost done. Needs final code polish, then audio and visuals later.
"Forgive" Player for friendly fire, aka lose bits_MEMORY_SUSPICIOUS after a set amount of time.
Add back Holstering, but only after 2-3 minutes of no combat and no suspicious memory (suspicious the player would betray them).
Eject Magazine when reloading.
Additional heads/skins and other cool submodel stuff

Transfer all of this to Barney!

Maybe let Otis look around for candy similar to how bullsquids work.


*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER		( 4 )
#define		OTIS_AE_RELOAD		( 5 ) // YELLOWSHIFT reload event
#define		OTIS_AE_MOURN_PLAYER ( 6 )

#define GUN_GROUP	1
#define	GUN_GROUP_GUNHOLSTERED	0
#define	GUN_GROUP_GUNDRAWN		1
#define GUN_GROUP_GUNGONE		2

#define HEAD_GROUP 2
#define HEAD_GROUP_WHITEYOUNG 0
#define HEAD_GROUP_WHITEBALD 1

#define HELMET_GROUP					3
#define HELMET_NONE						0
#define HELMET_SECURITY					1

// Used to determine which sub-model parts are chosen both in engine and in hammer.

//#define Helmet_Choice // What helmet?


#define	OTIS_CLIP_SIZE			7		 // YELLOWSHIFT bullets in magazine
class COtis : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void CheckAmmo ( void );
	int  ISoundMask( void );
	void OtisFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int	 m_iBrassShell; // YELLOWSHIFT used for ejecting shells
	int	 m_iEmptyMag; //YELLOWSHIFT unused until later. Will eject out empty magazines on reload
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	//YELLOWSHIFT 
	
	int		m_cClipSize; //clip/magazine size
	int	    m_GriefTime; //timer used for mourning dead allies
	int		m_PlayerGrief; //We already mourned the player, time to stand up.
	float	m_forgiveplayertime; //timer used for forgiving player for friendly fire by suspicious monsters.

	CUSTOM_SCHEDULES;
};

enum // YELLOWSHIFT For Reload
{

SCHED_OTIS_RELOAD = LAST_TALKMONSTER_SCHEDULE + 1,
SCHED_OTIS_MOURN_PLAYER,
LAST_OTIS_SCHEDULE,

};



LINK_ENTITY_TO_CLASS( monster_otis, COtis );

TYPEDESCRIPTION	COtis::m_SaveData[] = 
{
	DEFINE_FIELD( COtis, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_cClipSize, FIELD_INTEGER ), // YELLOWSHIFT saves magazine size
	DEFINE_FIELD( COtis, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COtis, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlOtFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slOtFollow[] =
{
	{
		tlOtFollow,
		ARRAYSIZE ( tlOtFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// OtisDraw- much better looking draw schedule for when
// otis knows who he's gonna attack.
//=========================================================
Task_t	tlOtisEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slOtisEnemyDraw[] = 
{
	{
		tlOtisEnemyDraw,
		ARRAYSIZE ( tlOtisEnemyDraw ),
		0,
		0,
		"Otis Enemy Draw"
	}
};

Task_t	tlOtFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slOtFaceTarget[] =
{
	{
		tlOtFaceTarget,
		ARRAYSIZE ( tlOtFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleOtStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleOtStand[] =
{
	{ 
		tlIdleOtStand,
		ARRAYSIZE ( tlIdleOtStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

//=========================================================
// YELLOWSHIFT Otis reload schedule
//=========================================================
Task_t	tlOtisReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slOtisReload[] = 
{
	{
		tlOtisReload,
		ARRAYSIZE ( tlOtisReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"OtisReload"
	}
};

//=========================================================
// YELLOWSHIFT Used to Mourn dead friends and player
//=========================================================

//Move next to player, look at corpse for 3 seconds before returning to idle.
Task_t	tlOtisMournPlayer[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)43		},	// Move within 43 of target ent (client)
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EAT	}, // Piggyback off of EAT for Mourning Anim
	//{ TASK_WAIT_RANDOM,			(float)0.3		},
	//{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t slOtisMournPlayer[] = 
{
	{
		tlOtisMournPlayer,
		ARRAYSIZE ( tlOtisMournPlayer ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"OtisMournPlayer"
	}
};



DEFINE_CUSTOM_SCHEDULES( COtis )
{
	slOtFollow,
	slOtisEnemyDraw,
	slOtFaceTarget,
	slIdleOtStand,
	
	//YELLOWSHIFT
	slOtisReload,
	slOtisMournPlayer,
};


IMPLEMENT_CUSTOM_SCHEDULES( COtis, CTalkMonster );

void COtis :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void COtis :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COtis :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COtis :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//YELLOWSHIFT ammo logic check
void COtis :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// ALertSound - Otis says "Freeze!"
//=========================================================
void COtis :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COtis :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COtis :: CheckRangeAttack1 ( float flDot, float flDist )
{

	if (GetBodygroup( GUN_GROUP ) == GUN_GROUP_GUNGONE)
	{
	return FALSE;
	}


	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// OtisFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis :: OtisFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	//YELLOWSHIFT New fancy dynamic lighting, oh my!
	UTIL_DynamicMuzzleFlash( vecShootOrigin, 38,  255,  255,  128,  1, 00.1);

	//YELLOWSHIFT Shell Ejection and the Vectors for it
	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	//HACK HACK, shoots 4 bullets until I make a new damage type.
	FireBullets(4, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );
	
	/*
	Doesn't sound good with this gunshot, will test again later.
	
	int pitchShift = RANDOM_LONG( 0, 40 );

	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;*/															//+ pitchShift
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "otis/ot_attack1.wav", 1, ATTN_NORM, 0, 100  );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	//TESTING 

		
	if (GetBodygroup( GUN_GROUP ) != GUN_GROUP_GUNGONE)
	{	// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( GUN_GROUP, GUN_GROUP_GUNGONE );
		//pev->body = OTIS_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		//PLACEHOLDER
		CBaseEntity *pGun = DropItem( "weapon_deagle", vecGunPos, vecGunAngles );
	}



	// UNDONE: Reload?
	// DONE: Reload!
	m_cAmmoLoaded--; // YELLOWSHIFT remove a bullet
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

/*
YELLOWSHIFT

To allow for easier management of new sub-groups such as helmets, several pieces of logic that depended on pev->body 
have now been changed to check a specific bodygroup. This achieves the same exact effect in logic, but allows for all the
new sub model groups to work nicely together.

*/
void COtis :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case OTIS_AE_SHOOT:
		OtisFirePistol();
		break;

	case OTIS_AE_DRAW:
		// otis's bodygroup switches here so he can pull gun from holster
		SetBodygroup( GUN_GROUP, GUN_GROUP_GUNDRAWN);
		m_fGunDrawn = TRUE;
		break;

	case OTIS_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup( GUN_GROUP, GUN_GROUP_GUNHOLSTERED);
		m_fGunDrawn = FALSE;
		break;
	
	case OTIS_AE_RELOAD: // YELLOWSHIFT Animation Event for Reload
		//YELLOWSHIFT eject magazine later. See Barney
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "otis/ot_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

	case OTIS_AE_MOURN_PLAYER: // YELLOWSHIFT Animation Event for Mourning Player WIP
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "otis/player_die1.wav", 1, ATTN_NORM );
		//switch (RANDOM_LONG(0,1))
		//{case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "otis/friends.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		// case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "otis/player_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;}
		 m_PlayerGrief = 1;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/otis.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.barneyHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_PlayerGrief		= 0;

	//YELLOWSHIFT Set up default Otis submodels. 
	SetBodygroup( GUN_GROUP, GUN_GROUP_GUNHOLSTERED ); 
	SetBodygroup( HELMET_GROUP, HELMET_NONE ); 


	pev->body			= 0; // gun in holster
	pev->skin			= 0; // Default Body skin
	m_fGunDrawn			= FALSE;

	//YELLOWSHIFT Magazine Size and amount of ammo inside magazine.
	m_cClipSize			= OTIS_CLIP_SIZE;
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	//YELLOWSHIFT Change helmet according to body_sub1 setting.
	//YELLOWSHIFT Seriously this needs to be some kind of switch/case system.

	if ( pev->frags == -1 )
	{
	SetBodygroup( HELMET_GROUP, RANDOM_LONG(0,HELMET_GROUP-1));
	}

	if(pev->frags == 0)
	{
	SetBodygroup( HELMET_GROUP, HELMET_NONE);
	}
	
	if(pev->frags == 1)
	{
	SetBodygroup( HELMET_GROUP, HELMET_SECURITY);
	}
	
	MonsterInit();
	SetUse( &COtis::FollowerUse );
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis :: Precache()
{
	PRECACHE_MODEL("models/Otis.mdl");

	PRECACHE_SOUND("otis/ot_attack1.wav" );
	
	PRECACHE_SOUND("otis/ot_pain1.wav");
	PRECACHE_SOUND("otis/ot_pain2.wav");
	PRECACHE_SOUND("otis/ot_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	PRECACHE_SOUND( "otis/ot_reload1.wav" );
	
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iEmptyMag = PRECACHE_MODEL ("models/w_9mmclip.mdl"); // magazine model
	

	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void COtis :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"OT_ANSWER";
	m_szGrp[TLK_QUESTION] =	"OT_QUESTION";
	m_szGrp[TLK_IDLE] =		"OT_IDLE";
	m_szGrp[TLK_STARE] =	"OT_STARE";
	m_szGrp[TLK_USE] =		"OT_OK";
	m_szGrp[TLK_UNUSE] =	"OT_WAIT";
	m_szGrp[TLK_STOP] =		"OT_STOP";

	m_szGrp[TLK_NOSHOOT] =	"OT_SCARED";
	m_szGrp[TLK_HELLO] =	"OT_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!OT_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!OT_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!OT_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "OT_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"OT_SMELL";
	
	m_szGrp[TLK_WOUND] =	"OT_WOUND";
	m_szGrp[TLK_MORTAL] =	"OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int COtis :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "OT_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

//DAMAGE SKIN SYSTEM
if (pev->health <= 30)
{pev->skin = 1;}


return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void COtis :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "otis/ot_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "otis/ot_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "otis/ot_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void COtis :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void COtis::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{

	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10: //Change this
	if (GetBodygroup( 3 ) == 1 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

/*
YELLOWSHIFT

To allow for easier management of new sub-groups such as helmets, several pieces of logic that depended on pev->body 
have now been changed to check a specific bodygroup. This achieves the same exact effect in logic, but allows for all the
new sub model groups to work nicely together.

*/
void COtis::Killed( entvars_t *pevAttacker, int iGib )
{
	//GUNGONE implies Otis has no gun at all and thus we make sure he even has his deagle.
	//When new weapons are implemented, re-write this similar to HGrunts to account for other weapons being dropped.
	
	if (GetBodygroup( GUN_GROUP ) != GUN_GROUP_GUNGONE)
	{	// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( GUN_GROUP, GUN_GROUP_GUNGONE );
		//pev->body = OTIS_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		//PLACEHOLDER
		CBaseEntity *pGun = DropItem( "weapon_deagle", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* COtis :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slOtisEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slOtFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slOtFollow;


	case SCHED_OTIS_RELOAD:
		{
			return &slOtisReload[ 0 ];
		}


	case SCHED_OTIS_MOURN_PLAYER:
		{
			return &slOtisMournPlayer[ 0 ];
		}


	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleOtStand;
		}
		else
			return psched;	
	}


	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COtis :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "OT_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}


// no ammo
			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_OTIS_RELOAD );
			}

		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				// DONE: We *will* comment about the player here! YELLOWSHIFT

				//AI is meant to vocally mourn player while crouching near body.
				//Once this has been done once, we don't do it again.
			{	if ( m_PlayerGrief = 0)
			{return GetScheduleOfType ( SCHED_OTIS_MOURN_PLAYER );StopFollowing( FALSE );break;}
		else
			StopFollowing( FALSE );break;}

			}

			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE COtis :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void COtis::DeclineFollowing( void )
{
	PlaySentence( "OT_POK", 2, VOL_NORM, ATTN_NORM );
}





//=========================================================
// DEAD OTIS PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadOtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadOtis :: Spawn( )
{
	PRECACHE_MODEL("models/otis.mdl");
	SET_MODEL(ENT(pev), "models/otis.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead otis with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;//gSkillData.barneyHealth;

	MonsterInitDead();
}



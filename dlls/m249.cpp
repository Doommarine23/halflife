/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

/* YELLOW SHIFT

Recreation of the OP4 M249 SAW with some differences.

TODO
Alt Fire
Eject Shells done
Eject Links done
Balance
SAW Bullet Tracer Code
Submodel Bullets


*/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum saw_e
{
	SAW_LONGIDLE = 0,
	SAW_IDLE1,
	SAW_RELOAD,
	SAW_HOLSTER,
	SAW_DEPLOY,
	SAW_FIRE1,
	SAW_FIRE2,
	SAW_FIRE3,
};

#define BULLET_GROUP					1
#define BULLET_EIGHT					0
#define BULLET_SEVEN					1
#define BULLET_SIX						2
#define BULLET_FIVE						3
#define BULLET_FOUR						4
#define BULLET_THREE					5
#define BULLET_TWO						6
#define BULLET_ONE						7
#define BULLET_NONE						8
	

LINK_ENTITY_TO_CLASS( weapon_saw, CSAW );
LINK_ENTITY_TO_CLASS( weapon_m249, CSAW );
//=========================================================
//=========================================================
void CSAW::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_saw"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_saw.mdl");
	m_iId = WEAPON_SAW;

	m_iDefaultAmmo = SAW_DEFAULT_GIVE;
//	SetBodygroup( BULLET_GROUP, BULLET_EIGHT ); //FULL MAGAZINE

	FallInit();// get ready to fall down.
}


void CSAW::Precache( void )
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL ("models/saw_shell.mdl");// brass shellTE_MODEL
	m_iShellLink = PRECACHE_MODEL ("models/saw_link.mdl");
	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND ("weapons/saw_fire1.wav");// H to the K
	PRECACHE_SOUND ("weapons/saw_fire2.wav");// H to the K
	PRECACHE_SOUND ("weapons/saw_fire3.wav");// H to the K

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	//REPLACE
	m_usSAW = PRECACHE_EVENT( 1, "events/saw.sc" );

}

int CSAW::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = SAW_MAX_CARRY;
	p->iMaxClip = SAW_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 0; 
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SAW;
	p->iWeight = SAW_WEIGHT;

	return 1;
}

int CSAW::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CSAW::Deploy( )
{
	return DefaultDeploy( "models/v_saw.mdl", "models/p_saw.mdl", SAW_DEPLOY, "saw" );
}


void CSAW::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;



	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

#ifdef CLIENT_DLL
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSAW, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.075);

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

		SetBodygroup( BULLET_GROUP, BULLET_NONE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CSAW::SecondaryAttack( void )
{		//SetBodygroup( BULLET_GROUP, BULLET_TWO );
		//PlayEmptySound( );
		pev->body = 8;
		return;
}

void CSAW::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	DefaultReload( SAW_MAX_CLIP, SAW_RELOAD, 3.82 );
}


void CSAW::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = SAW_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = SAW_IDLE1;
		break;
	}

	switch ( m_iClip )
	{
	case 8:	
	SetBodygroup( BULLET_GROUP, BULLET_EIGHT ); break; //FULL MAGAZINE
	case 7:
	SetBodygroup( BULLET_GROUP, BULLET_SEVEN ); break;
	case 6:
	SetBodygroup( BULLET_GROUP, BULLET_SIX ); break;
	case 5:
	SetBodygroup( BULLET_GROUP, BULLET_FIVE ); break;
	case 4:
	SetBodygroup( BULLET_GROUP, BULLET_FOUR ); break;
	case 3:
	SetBodygroup( BULLET_GROUP, BULLET_THREE ); break;
	case 2:
	SetBodygroup( BULLET_GROUP, BULLET_TWO ); break;
	case 1:
	SetBodygroup( BULLET_GROUP, BULLET_ONE ); break;
	case 0:
	SetBodygroup( BULLET_GROUP, BULLET_NONE ); break;

	}


	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CSAWAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_saw_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_SAWCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_sawclip, CSAWAmmoClip );

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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


//YELLOWSHIFT Additional firing animations.
enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_IDLE1_EMPTY,
	GLOCK_IDLE2_EMPTY,
	GLOCK_IDLE3_EMPTY,
	GLOCK_SHOOT,
	GLOCK_SHOOT2,
	GLOCK_SHOOT3,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_EMPTY,
	GLOCK_RELOAD_EMPTY2,
	GLOCK_DRAW,
	GLOCK_DRAW_EMPTY,
	GLOCK_HOLSTER,
	GLOCK_HOLSTER_EMPTY
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );


void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND("weapons/Glock_Slide1.wav"); //Yellowshift Used for Reloads
	PRECACHE_SOUND ("weapons/glock_holster.wav");
	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun_empty1.wav"); // Handgun Empty
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun
	PRECACHE_SOUND ("weapons/pl_gun4.wav");//handgun
	PRECACHE_SOUND ("weapons/pl_gun5.wav");//handgun
	PRECACHE_SOUND ("weapons/pl_gun_empty.wav");

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;
	p->AudioEmpty = "weapons/pl_gun_empty.wav"; //Yellowshift required for unique dryfire sounds. If not defined, will default to 357_cock1.wav

	return 1;
}

BOOL CGlock::Deploy( )
{
	if (m_iClip > 0)
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
	else
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW_EMPTY, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlock::SecondaryAttack( void )
{ //YELLOWSHIFT Testing out a higher RoF for the Glock.
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}
	else
	GlockFire( 0.065, 0.1, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	GlockFire( 0.01, 0.3, TRUE );
}

void CGlock::GlockFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	#ifndef CLIENT_DLL
		UTIL_ScreenShake( m_pPlayer->pev->origin, 1.2, 2.5, 0.3, 2,true );
#endif

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	//PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
			m_flTimeWeaponIdle = 0.6;
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CGlock::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		 return;

	int iResult;

	if (m_iClip == 0)
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iResult = DefaultReload( 17, GLOCK_RELOAD_EMPTY, 2.35 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.55;
		break;

	default:
	case 1:
		DefaultReload( MP5_MAX_CLIP, GLOCK_RELOAD_EMPTY2, 2.35 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.55;
		break;
	}
	else
	{
		iResult = DefaultReload( 17, GLOCK_RELOAD, 1.5 ); 		
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.9;
	}
}

//YELLOWSHIFT Re-Wrote IDLE using Shotgun's Idle to better ensure animations do not cut out, and implemented empty idle animations.

void CGlock::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

int iAnim;

if (m_iClip != 0)
	{
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8) // Be very rare.
			{
				iAnim = GLOCK_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (29.0/10.0);
			}
			else if (flRand <= 0.95)
			{
				iAnim = GLOCK_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (31.0/10.0);
			}
			else
			{
				iAnim = GLOCK_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (111.0/20.0);
			}
	}
	else if (m_iClip <= 0)
	{
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8)
			{
				iAnim = GLOCK_IDLE1_EMPTY;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (28.0/10.0);
			}
			else if (flRand <= 0.95)
			{
				iAnim = GLOCK_IDLE2_EMPTY;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (30.0/10.0);
			}
			else
			{
				iAnim = GLOCK_IDLE3_EMPTY;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (110.0/20.0);
			}
	}
	SendWeaponAnim( iAnim );

}


class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );
















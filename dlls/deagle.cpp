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
#include "weapons.h"
#include "monsters.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

/* TODO:

1. Add laser sight
2. Ensure weapon functions correctly
3. Improve non-code functions (better model, edited animations, etc)
4. Balance pass

*/



enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_IDLE5,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOTEMPTY,
	DEAGLE_RELOADEMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW,
	DEAGLE_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDeagle );
LINK_ENTITY_TO_CLASS( weapon_desert_eagle, CDeagle );

LINK_ENTITY_TO_CLASS( laser_spot2, CLaserSpotDeagle ); // Might have to become its own class to prevent the classic OP4 laser bug


//=========================================================
//=========================================================
CLaserSpotDeagle *CLaserSpotDeagle::CreateSpot( void )
{
	CLaserSpotDeagle *pSpot = GetClassPtr( (CLaserSpotDeagle *)NULL );
	pSpot->Spawn();
	pSpot->pev->classname = MAKE_STRING("laser_spot2");

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpotDeagle::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpotDeagle::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( &CLaserSpotDeagle::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpotDeagle::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpotDeagle::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};

int CDeagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = PYTHON_WEIGHT; // Both are equally viable.

	return 1;
}

int CDeagle::AddToPlayer( CBasePlayer *pPlayer )
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

void CDeagle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");
	m_fSpotActive = 1;

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CDeagle::Precache( void )
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");
	UTIL_PrecacheOther( "laser_spot2" );

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");	// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND ("weapons/desert_eagle_sight2.wav");
	
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFireDeagle = PRECACHE_EVENT( 1, "events/deagle.sc" );
}

BOOL CDeagle::Deploy( )
{
	return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", DEAGLE_DRAW, "deagle" );
}


void CDeagle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( DEAGLE_HOLSTER );

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
#endif

}

void CDeagle::SecondaryAttack()
{
	m_fSpotActive = ! m_fSpotActive;

#ifndef CLIENT_DLL
	if (m_fSpotActive && !m_pSpot)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight.wav", 1, ATTN_NORM);
#endif

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight2.wav", 1, ATTN_NORM);
	}
#endif
	m_flNextSecondaryAttack = 0.2;
	//m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
}

/*
void CDeagle::SecondaryAttack( void )
{
	m_fSpotActive = ! m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;


/*	if (m_fLaser == 0 )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight.wav", 1, ATTN_NORM);
		m_fLaser = 1;
	}
	else if (m_fLaser == 1 )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight2.wav", 1, ATTN_NORM);
		m_fLaser = 0;
	}

		m_flNextSecondaryAttack = 0.5;*/

	/*
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if ( m_pPlayer->pev->fov != 40 )
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = 0.5;
}*/

void CDeagle::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		UpdateSpot( );
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		UpdateSpot( );
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


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	if (m_fSpotActive)
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	else
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireDeagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		if (m_fLaser == 1 )
				m_flNextPrimaryAttack = 0.70;
		else
				m_flNextPrimaryAttack = 0.22;
		UpdateSpot( );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

}


void CDeagle::Reload( void )
{
	if ( m_pPlayer->ammo_357 <= 0 )
		return;


#ifndef CLIENT_DLL
	if ( m_pSpot && m_fSpotActive )
	{
		m_pSpot->Suspend( 1.68 );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.68;
	}
#endif

	if (m_iClip == 0)
	DefaultReload( DEAGLE_MAX_CLIP, DEAGLE_RELOADEMPTY, 1.68, 2 );
	else
	DefaultReload( DEAGLE_MAX_CLIP, DEAGLE_RELOAD, 1.68, 2 );
}


void CDeagle::WeaponIdle( void )
{
	UpdateSpot( );
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.5)
	{
		iAnim = DEAGLE_IDLE1;
		m_flTimeWeaponIdle = (75.0/30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = DEAGLE_IDLE2;
		m_flTimeWeaponIdle = (60.0/24.0);
	}
	else if (flRand <= 0.9)
	{
		iAnim = DEAGLE_IDLE3;
		m_flTimeWeaponIdle = (49.0/30.0);
	}
	else
	{
		iAnim = DEAGLE_IDLE4;
		m_flTimeWeaponIdle = (75.0/30.0);
	}
}

void CDeagle::UpdateSpot( void )
{

#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpotDeagle::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
#endif

}

class CDeagleAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_357ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_deagle, CDeagleAmmo );
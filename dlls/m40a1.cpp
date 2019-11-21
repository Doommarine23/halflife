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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

//YELLOWSHIFT

/*

OP4 M40A1

TODO
Add empty reload logic DONE
Sniper Ammo NOT STARTED
ev_hldm code WIP
Add scope view WIP
Lots of polish! WIP

*/



enum m40a1_e {
	M40A1_DRAW = 0,
	M40A1_DRAW_EMPTY,
	M40A1_IDLE1,
	M40A1_IDLE_EMPTY,
	M40A1_FIRE1,
	M40A1_FIRE_EMPTY,
	M40A1_RELOAD,
	M40A1_RELOAD_EMPTY,
	M40A1_ENTER_AIM,
	M40A1_ENTER_AIM_EMPTY,
	M40A1_IDLE_AIM,
	M40A1_IDLE_AIM_EMPTY,
	M40A1_FIRE_AIM,
	M40A1_FIRE_AIM_EMPTY,
	M40A1_EXIT_AIM,
	M40A1_EXIT_AIM_EMPTY,
	M40A1_HOLSTER,
	M40A1_HOLSTER_EMPTY,
};

LINK_ENTITY_TO_CLASS( weapon_sniperrifle, CM40A1);
LINK_ENTITY_TO_CLASS( weapon_m40a1, CM40A1 );

int CM40A1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = M40A1_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M40A1_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_M40A1;
	p->iWeight = M40A1_WEIGHT;

	return 1;
}

int CM40A1::AddToPlayer( CBasePlayer *pPlayer )
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

void CM40A1::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m40a1"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_M40A1;
	SET_MODEL(ENT(pev), "models/w_m40a1.mdl");

	m_iDefaultAmmo = M40A1_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM40A1::Precache( void )
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	
	PRECACHE_SOUND("items/9mmclip1.wav");              
	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_fire.wav");
	PRECACHE_SOUND ("weapons/sniper_fire_last_round.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
	PRECACHE_SOUND ("weapons/sniper_zoom.wav");

	m_usFireM40A1 = PRECACHE_EVENT( 1, "events/m40a1.sc" );
	m_usFireM40A1Scoped = PRECACHE_EVENT( 1, "events/m40a1scoped.sc" );
}

BOOL CM40A1::Deploy( )
{
	if (m_iClip > 0)
		return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", M40A1_DRAW, "m40a1" );
	else
		return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", M40A1_DRAW_EMPTY, "m40a1" );
}


void CM40A1::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		if (m_iClip > 0)
			SendWeaponAnim( M40A1_HOLSTER );
		//else
			//SendWeaponAnim( M40A1_HOLSTEREMPTY );
}

void CM40A1::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		SendWeaponAnim( M40A1_EXIT_AIM );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_zoom.wav", 0.8, ATTN_NORM);
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov

	}
	else if ( m_pPlayer->pev->fov != 20 )
	{
		SendWeaponAnim( M40A1_ENTER_AIM );
		m_fInZoom = TRUE;
		SetThink( &CM40A1::Zoom );
		pev->nextthink = gpGlobals->time + 0.36;		// YELLOWSHIFT  Required for delay to function until I figure something better out

	}

	m_flNextSecondaryAttack = 0.40;
}

void CM40A1::Zoom( void ) // YELLOWSHIFT  Required for delay to function until I figure something better out
{
		SendWeaponAnim( M40A1_IDLE_AIM );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_zoom.wav", 0.8, ATTN_NORM);
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
}

void CM40A1::PrimaryAttack()
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

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir; 
	//YELLOWSHIFT Change the bullet type later
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if ( m_pPlayer->pev->fov != 0 ) // YELLOWSHIFT Scoped Anims
	{  //Just edit m_useFireM40a1 in ev_hldm.cpp instead of this sick fucking hack job of copy/pasting
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireM40A1Scoped, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip, 0 );
		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) // HEV suit - indicate out of ammo condition	
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
		m_flNextPrimaryAttack = 2.19; //1.78 original ROF
		m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
	else
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireM40A1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip, 0 );
		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) // HEV suit - indicate out of ammo condition	
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
		m_flNextPrimaryAttack = 2.19; //1.78 original ROF
		m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

}


void CM40A1::Reload( void )
{
	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif

	if (m_iClip <= 0)
	{
		DefaultReload( 5, M40A1_RELOAD_EMPTY, 3.82, bUseScope );
	}
	else
	DefaultReload( 5, M40A1_RELOAD, 2.35, bUseScope );
}


void CM40A1::WeaponIdle( void )
{
	int iAnim;
	int bUseScope = FALSE;
	ResetEmptySound( );
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES ); 

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if (m_pPlayer->pev->fov != 0)
			{
				if(m_iClip > 0)
				{iAnim = M40A1_IDLE_AIM;}	
				else
				{iAnim = M40A1_IDLE_EMPTY;} // REPLACE WITH EMPTY IDLE ONCE DONE
			}
	else
		if(m_iClip > 0)
			{iAnim = M40A1_IDLE1;}	
		else
			{iAnim = M40A1_IDLE_EMPTY;}

	SendWeaponAnim( iAnim );

	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;

	//if (!m_fInZoom)
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.


}
#endif
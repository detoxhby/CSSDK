/***
*
*   Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*   This product contains software technology licensed from Id
*   Software, Inc. ( "Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*   All Rights Reserved.
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
#include "player.h"
#include "weapons.h"

enum sg552_e
{
    SG552_IDLE1,
    SG552_RELOAD,
    SG552_DRAW,
    SG552_SHOOT1,
    SG552_SHOOT2,
    SG552_SHOOT3
};

#define SG552_MAX_SPEED         235
#define SG552_MAX_SPEED_ZOOM    200
#define SG552_RELOAD_TIME       3.0

LINK_ENTITY_TO_CLASS( weapon_sg552, CSG552 );

void CSG552::Spawn( void )
{
    pev->classname = MAKE_STRING( "weapon_sg552" );

    Precache();
    m_iId = WEAPON_SG552;
    SET_MODEL( edict(), "models/w_sg552.mdl" );

    m_iDefaultAmmo = SG552_DEFAULT_GIVE;
    m_flAccuracy   = 0.2;
    m_iShotsFired  = 0;

    FallInit();
}

void CSG552::Precache( void )
{
    PRECACHE_MODEL( "models/v_sg552.mdl" );
    PRECACHE_MODEL( "models/w_sg552.mdl" );

    PRECACHE_SOUND( "weapons/sg552-1.wav" );
    PRECACHE_SOUND( "weapons/sg552-2.wav" );
    PRECACHE_SOUND( "weapons/sg552_clipout.wav" );
    PRECACHE_SOUND( "weapons/sg552_clipin.wav" );
    PRECACHE_SOUND( "weapons/sg552_boltpull.wav" );

    m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
    m_usFireSG552 = PRECACHE_EVENT(1, "events/sg552.sc" );
}

int CSG552::GetItemInfo( ItemInfo *p )
{ 
    p->pszName   = STRING( pev->classname );
    p->pszAmmo1  = "556Nato";
    p->iMaxAmmo1 = _556NATO_MAX_CARRY;
    p->pszAmmo2  = NULL;
    p->iMaxAmmo2 = -1;
    p->iMaxClip  = SG552_MAX_CLIP;
    p->iSlot     = 0;
    p->iPosition = 10;
    p->iId       = m_iId = WEAPON_SG552;
    p->iFlags    = 0;
    p->iWeight   = SG552_WEIGHT;

    return 1;
}

int CSG552::iItemSlot( void )
{
    return PRIMARY_WEAPON_SLOT;
}

BOOL CSG552::Deploy( void )
{
    iShellOn = 1;

    m_flAccuracy  = 0.2;
    m_iShotsFired = 0;

    return DefaultDeploy( "models/v_sg552.mdl", "models/p_sg552.mdl", SG552_DRAW, "mp5", UseDecrement() != FALSE );
}

void CSG552::PrimaryAttack( void )
{
    if( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
    {
        SG552Fire( 0.035 + ( 0.45 * m_flAccuracy ), 0.0825, FALSE );
    }
    else if( m_pPlayer->pev->velocity.Length2D() > 140 )
    {
        SG552Fire( 0.035 + ( 0.075 * m_flAccuracy ), 0.0825, FALSE );
    }
    else if( m_pPlayer->pev->fov == 90 )
    {
        SG552Fire( 0.02 * m_flAccuracy, 0.0825, FALSE );
    }
    else
    {
        SG552Fire( 0.02 * m_flAccuracy, 0.135, FALSE );
    }
}

void CSG552::SecondaryAttack( void )
{
    if( m_pPlayer->m_iFOV != 90 )
        m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 90;
    else
        m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 55;

    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CSG552::SG552Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
    m_bDelayFire = true;
    m_iShotsFired++;

    m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 220 ) + 0.3;

    if( m_flAccuracy > 1 )
        m_flAccuracy = 1;

    if( m_iClip <= 0 )
    {
        if( m_fFireOnEmpty )
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = GetNextAttackDelay( 0.2 );
        }

        // TODO: Implement me.
        // if( TheBots )
        // {
        //     TheBots->OnEvent( EVENT_WEAPON_FIRED_ON_EMPTY, m_pPlayer, NULL );
        // }

        return;
    }

    m_iClip--;

    m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

    m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
    m_pPlayer->m_iWeaponFlash  = BRIGHT_GUN_FLASH;

    UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

    Vector vecDir = m_pPlayer->FireBullets3( m_pPlayer->GetGunPosition(), gpGlobals->v_forward, flSpread, 
        SG552_DISTANCE, SG552_PENETRATION, BULLET_PLAYER_556MM, SG552_DAMAGE, SG552_RANGE_MODIFER, m_pPlayer->pev, FALSE, m_pPlayer->random_seed );

    int flags;

    #if defined( CLIENT_WEAPONS )
        flags = FEV_NOTHOST;
    #else
        flags = 0;
    #endif

    PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict, m_usFireSG552, 0, ( float* )&g_vecZero, ( float* )&g_vecZero, vecDir.x, vecDir.y, 
        ( int )( m_pPlayer->pev->punchangle.x * 100 ), ( int )( m_pPlayer->pev->punchangle.y * 100 ), 5, FALSE );

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );

    if( !m_iClip && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
    {
        m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
    }

    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;

    if( m_pPlayer->pev->velocity.Length2D() > 0 )
    {
        KickBack( 1.0, 0.45, 0.28, 0.04, 4.25, 2.5, 7 );
    }
    else if( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
    {
        KickBack( 1.25, 0.45, 0.22, 0.18, 6.0, 4.0, 5 );
    }
    else if( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
    {
        KickBack( 0.6, 0.35, 0.2, 0.0125, 3.7, 2.0, 10 );
    }
    else
    {
        KickBack( 0.625, 0.375, 0.25, 0.0125, 4.0, 2.25, 9 );
    }
}

void CSG552::Reload( void )
{
    if( m_pPlayer->ammo_556nato <= 0 )
    {
        return;
    }

    if( DefaultReload( SG552_MAX_CLIP, SG552_RELOAD, SG552_RELOAD_TIME ) )
    {
        m_pPlayer->SetAnimation( PLAYER_RELOAD );

        if( m_pPlayer->m_iFOV != 90 )
        {
            SecondaryAttack();
        }

        m_flAccuracy  = 0.2;
        m_iShotsFired = 0;
        m_bDelayFire  = false;
    }
}

void CSG552::WeaponIdle( void )
{
    ResetEmptySound();
    m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

    if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
    {
        return;
    }

    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0;
    SendWeaponAnim( SG552_IDLE1, UseDecrement() != FALSE );
}

BOOL CSG552::UseDecrement( void )
{
    #if defined( CLIENT_WEAPONS )
        return TRUE;
    #else
        return FALSE;
    #endif
}

float CSG552::GetMaxSpeed( void )
{
    return m_pPlayer->m_iFOV == 90 ? SG552_MAX_SPEED : SG552_MAX_SPEED_ZOOM;
}

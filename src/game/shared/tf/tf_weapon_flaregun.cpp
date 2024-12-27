//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
// Server specific.
#else
#include "tf_gamestats.h"
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Flare Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlareGun, DT_WeaponFlareGun )

BEGIN_NETWORK_TABLE( CTFFlareGun, DT_WeaponFlareGun )
/*
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
#endif
*/
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlareGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flaregun, CTFFlareGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_flaregun );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFFlareGun )
END_DATADESC()
#endif


//=============================================================================
//
// Weapon Flare Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlareGun::CTFFlareGun()
{
	m_bEffectsThinking = false;
	m_flLastDenySoundTime = 0.0f;

#ifdef CLIENT_DLL
	m_bReadyToFire = false;
#endif
}

CTFFlareGun::~CTFFlareGun()
{
	//DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "stickybombtrail_blue" );
	PrecacheParticleSystem( "stickybombtrail_red" );
	PrecacheParticleSystem( "critical_grenade_blue" );
	PrecacheParticleSystem( "critical_grenade_red" );
}

void CTFFlareGun::DestroySounds( void )
{
	//StopCharge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		BaseClass::PrimaryAttack();
	}
	else
	{
		if ( gpGlobals->curtime > m_flLastDenySoundTime )
		{
			WeaponSound( SPECIAL2 );
			m_flLastDenySoundTime = gpGlobals->curtime + 1.0f;
		}
	}

#ifdef CLIENT_DLL
	m_bReadyToFire = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Detonate flare
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
	if ( GetFlareGunType() != FLAREGUN_DETONATE )
		return;

	if ( !CanAttack() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	if ( m_iFlareCount )
	{
		int iCount = m_Flares.Count();
		for ( int i = 0; i < iCount; i++ )
		{
			CTFProjectile_Flare *pTemp = m_Flares[i];
			if ( pTemp )
			{
				pTemp->Detonate();
			}
		}
	}
#endif // GAME_DLL
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::AddFlare( CTFProjectile_Flare *pFlare )
{
	FlareHandle hHandle;
	hHandle = pFlare;
	m_Flares.AddToTail( hHandle );

	m_iFlareCount = m_Flares.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::DeathNotice( CBaseEntity *pVictim )
{
	Assert( dynamic_cast<CTFProjectile_Flare*>( pVictim ) );

	FlareHandle hHandle;
	hHandle = (CTFProjectile_Flare*)pVictim;
	m_Flares.FindAndRemove( hHandle );

	m_iFlareCount = m_Flares.Count();
}
#endif

bool CTFFlareGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = false;

	m_bReadyToFire = false;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGun::Deploy( void )
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = true;
	SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );

	m_bReadyToFire = false;
#endif

	return BaseClass::Deploy();
}

void CTFFlareGun::WeaponReset( void )
{
	BaseClass::WeaponReset();
}

void CTFFlareGun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL
	if ( !m_bEffectsThinking )
	{
		m_bEffectsThinking = true;
		SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	}
#endif
}

#ifdef CLIENT_DLL
void CTFFlareGun::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	//DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( !m_bEffectsThinking )
		return;

	if ( !GetOwner() || GetOwner()->GetActiveWeapon() != this )
	{
		m_bEffectsThinking = false;
	}
	else
	{
		SetContextThink( &CTFFlareGun::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	}
}
#endif
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flamethrower.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"

#if defined( CLIENT_DLL )

	#include "c_tf_player.h"
	#include "vstdlib/random.h"
	#include "engine/IEngineSound.h"
	#include "soundenvelope.h"
	#include "prediction.h"
#ifdef FLAMETHROWER_DLIGHT
	#include "iefx.h"
extern ConVar tf_muzzleflash_light;
#endif

#else

	#include "explode.h"
	#include "tf_player.h"
	#include "tf_gamerules.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "tf_team.h"
	#include "tf_obj.h"
#include "particle_parse.h"
#include "NextBot/NextBotManager.h"

	ConVar	tf_debug_flamethrower("tf_debug_flamethrower", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Visualize the flamethrower damage." );
	ConVar  tf_flamethrower_velocity( "tf_flamethrower_velocity", "2300.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Initial velocity of flame damage entities." );
	ConVar	tf_flamethrower_drag("tf_flamethrower_drag", "0.87", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Air drag of flame damage entities." );
	ConVar	tf_flamethrower_float("tf_flamethrower_float", "50.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Upward float velocity of flame damage entities." );
	ConVar  tf_flamethrower_flametime("tf_flamethrower_flametime", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to live of flame damage entities." );
	ConVar  tf_flamethrower_vecrand("tf_flamethrower_vecrand", "0.05", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Random vector added to initial velocity of flame damage entities." );
	ConVar  tf_flamethrower_boxsize("tf_flamethrower_boxsize", "8.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Size of flame damage entities." );
	ConVar  tf_flamethrower_maxdamagedist("tf_flamethrower_maxdamagedist", "350.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Maximum damage distance for flamethrower." );
	ConVar  tf_flamethrower_shortrangedamagemultiplier("tf_flamethrower_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Damage multiplier for close-in flamethrower damage." );
	ConVar  tf_flamethrower_velocityfadestart("tf_flamethrower_velocityfadestart", ".3", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution starts to fade." );
	ConVar  tf_flamethrower_velocityfadeend("tf_flamethrower_velocityfadeend", ".5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution finishes fading." );
	//ConVar  tf_flame_force( "tf_flame_force", "30" );

	static const char* s_pszFlameThrowerHitTargetThink = "FlameThrowerHitTargetThink";
#endif

ConVar  tf_flamethrower_burstammo( "tf_flamethrower_burstammo", "20", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "How much ammo does the air burst uses per shot." );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// position of end of muzzle relative to shoot position
#define TF_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TF_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TF_FLAMETHROWER_MUZZLEPOS_UP			-12.0f

#define TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK		14.0f
#define TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK	25

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameThrower, DT_WeaponFlameThrower )

BEGIN_NETWORK_TABLE( CTFFlameThrower, DT_WeaponFlameThrower )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iWeaponState ) ),
		RecvPropBool( RECVINFO( m_bCritFire ) ),
		RecvPropBool( RECVINFO( m_bHitTarget ) )
	#else
		SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropBool( SENDINFO( m_bCritFire ) ),
		SendPropBool( SENDINFO( m_bHitTarget ) )
	#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFFlameThrower )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlameThrower );
PRECACHE_WEAPON_REGISTER( tf_weapon_flamethrower );

BEGIN_DATADESC( CTFFlameThrower )
END_DATADESC()

// ------------------------------------------------------------------------------------------------ //
// CTFFlameThrower implementation.
// ------------------------------------------------------------------------------------------------ //
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::CTFFlameThrower()
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_pFiringHitLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pPilotLightSound = NULL;
#ifdef FLAMETHROWER_DLIGHT
	m_pDynamicLight = NULL;
#endif
#else
	m_flTimeToStopHitSound = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::~CTFFlameThrower()
{
	DestroySounds();
}


void CTFFlameThrower::DestroySounds( void )
{
#if defined( CLIENT_DLL )
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}
	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}
	if ( m_pPilotLightSound )
	{
		controller.SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}

	StopHitSound();
#endif

}
void CTFFlameThrower::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;
	m_flStartFiringTime = 0;
	m_flAmmoUseRemainder = 0;

	DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::Precache( void )
{
	BaseClass::Precache();
	PrecacheParticleSystem( "pyro_blast" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	//PrecacheScriptSound( "TFPlayer.FlameOut" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheScriptSound( "Weapon_FlameThrower.FireHit" );
	PrecacheParticleSystem( "deflect_fx" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_FLAMETHROWER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;

#if defined ( CLIENT_DLL )
	StopFlame();
	StopPilotLight();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::ItemPostFrame()
{
	if ( m_bLowered )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );

	if ( (pOwner->m_nButtons & IN_ATTACK2) && m_flNextSecondaryAttack <= gpGlobals->curtime && CanAirBlast() )
	{
		float flAmmoPerSecondaryAttack = TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK;
		CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecondaryAttack, mult_airblast_cost );

		if ( iAmmo >= flAmmoPerSecondaryAttack )
		{
			SecondaryAttack();
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
		}
	}
	else if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK ) && iAmmo > 0 && m_iWeaponState != FT_STATE_SECONDARY )
	{
		PrimaryAttack();
	}
	else
	{
		if (m_iWeaponState > FT_STATE_IDLE)
		{
			SendWeaponAnim(ACT_MP_ATTACK_STAND_POSTFIRE);
			if ( m_iWeaponState != FT_STATE_SECONDARY )
				pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_POST);
			m_iWeaponState = FT_STATE_IDLE;
			m_bCritFire = false;
			m_bHitTarget = false;
		}

		if (!ReloadOrSwitchWeapons())
		{
			WeaponIdle();
		}
	}

#if defined(CLIENT_DLL) && defined(FLAMETHROWER_DLIGHT)
	if ( m_pDynamicLight )
	{
		Vector vecAttachMuzzle, vecForward;
		AngleVectors( pOwner->GetAbsAngles(), &vecForward );
		int iAttachment = LookupAttachment( "muzzle" );
		GetAttachment( iAttachment, vecAttachMuzzle );
		vecAttachMuzzle += vecForward * 115;
		m_pDynamicLight->origin = vecAttachMuzzle;
	}
#endif

	//BaseClass::ItemPostFrame();
}

class CTraceFilterIgnoreObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreObjects, CTraceFilterSimple );

	CTraceFilterIgnoreObjects( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity && pEntity->IsBaseObject() )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::PrimaryAttack()
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined ( CLIENT_DLL )
		StopFlame();
#endif
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	CalcIsAttackCritical();

	// Because the muzzle is so long, it can stick through a wall if the player is right up against it.
	// Make sure the weapon can't fire in this condition by tracing a line between the eye point and the end of the muzzle.
	trace_t trace;	
	Vector vecEye = pOwner->EyePosition();
	Vector vecMuzzlePos = GetVisualMuzzlePos();
	CTraceFilterIgnoreObjects traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecMuzzlePos, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction < 1.0 && ( !trace.m_pEnt || trace.m_pEnt->m_takedamage == DAMAGE_NO ) )
	{
		// there is something between the eye and the end of the muzzle, most likely a wall, don't fire, and stop firing if we already are
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
#if defined ( CLIENT_DLL )
			StopFlame();
#endif
			m_iWeaponState = FT_STATE_IDLE;
		}
		return;
	}

	switch ( m_iWeaponState )
	{
	case FT_STATE_IDLE:
		{
			// Just started, play PRE and start looping view model anim

			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			m_flStartFiringTime = gpGlobals->curtime + 0.16;	// 5 frames at 30 fps

			m_iWeaponState = FT_STATE_STARTFIRING;
		}
		break;
	case FT_STATE_STARTFIRING:
		{
			// if some time has elapsed, start playing the looping third person anim
			if ( gpGlobals->curtime > m_flStartFiringTime )
			{
				m_iWeaponState = FT_STATE_FIRING;
				m_flNextPrimaryAttackAnim = gpGlobals->curtime;
			}
		}
		break;
	case FT_STATE_FIRING:
		{
			if ( gpGlobals->curtime >= m_flNextPrimaryAttackAnim )
			{
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime + 1.4;		// fewer than 45 frames!
			}
		}
		break;

	default:
		break;
	}

#ifdef CLIENT_DLL
	// Restart our particle effect if we've transitioned across water boundaries
	if ( m_iParticleWaterLevel != -1 && pOwner->GetWaterLevel() != m_iParticleWaterLevel )
	{
		if ( m_iParticleWaterLevel == WL_Eyes || pOwner->GetWaterLevel() == WL_Eyes )
		{
			RestartParticleEffect();
		}
	}
#endif

#if !defined (CLIENT_DLL)
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, m_bCritFire );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
#endif

	float flFiringInterval = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		// Find eligible entities in a cone in front of us.
		Vector vOrigin = pOwner->Weapon_ShootPosition();
		Vector vForward, vRight, vUp;
		QAngle vAngles = pOwner->EyeAngles() + pOwner->GetPunchAngle();
		AngleVectors( vAngles, &vForward, &vRight, &vUp );

		#define NUM_TEST_VECTORS	30

#ifdef CLIENT_DLL
		bool bWasCritical = m_bCritFire;
#endif

		// Burn & Ignite 'em
		int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
		m_bCritFire = IsCurrentAttackACrit();
		if ( m_bCritFire )
		{
			iDmgType |= DMG_CRITICAL;
		}

#ifdef CLIENT_DLL
		if ( bWasCritical != m_bCritFire )
		{
			RestartParticleEffect();
		}
#endif


#ifdef GAME_DLL
		// create the flame entity
		int iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
		float flDamage = (float)iDamagePerSec * flFiringInterval;
		int iCritFromBehind = 0;
		CALL_ATTRIB_HOOK_INT( iCritFromBehind, set_flamethrower_back_crit );
		CTFFlameEntity::Create( GetFlameOriginPos(), pOwner->EyeAngles(), this, iDmgType, flDamage, iCritFromBehind == 1);
#endif
	}

#ifdef GAME_DLL
	// Figure how much ammo we're using per shot and add it to our remainder to subtract.  (We may be using less than 1.0 ammo units
	// per frame, depending on how constants are tuned, so keep an accumulator so we can expend fractional amounts of ammo per shot.)
	// Note we do this only on server and network it to client.  If we predict it on client, it can get slightly out of sync w/server
	// and cause ammo pickup indicators to appear
	m_flAmmoUseRemainder += TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK * flFiringInterval;
	// take the integer portion of the ammo use accumulator and subtract it from player's ammo count; any fractional amount of ammo use
	// remains and will get used in the next shot
	int iAmmoToSubtract = (int) m_flAmmoUseRemainder;
	if ( iAmmoToSubtract > 0 )
	{
		pOwner->RemoveAmmo( iAmmoToSubtract, m_iPrimaryAmmoType );
		m_flAmmoUseRemainder -= iAmmoToSubtract;
		// round to 2 digits of precision
		m_flAmmoUseRemainder = (float) ( (int) (m_flAmmoUseRemainder * 100) ) / 100.0f;
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + flFiringInterval;
	m_flTimeWeaponIdle = gpGlobals->curtime + flFiringInterval;

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pOwner );
#endif
}

bool CTFFlameThrower::CanAirBlast() const
{
	int iAirblastDisabled = 0;
	CALL_ATTRIB_HOOK_INT( iAirblastDisabled, airblast_disabled );

	bool bAllowed = (iAirblastDisabled == 0);

	return bAllowed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SecondaryAttack()
{
	//if ( !tf2c_airblast.GetBool() )
	//	return;

	if ( !CanAirBlast() )
		return;

	// Get the player owning the weapon.
	CTFPlayer* pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

#ifdef CLIENT_DLL
	StopFlame();
#endif

	m_iWeaponState = FT_STATE_SECONDARY;
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	WeaponSound( WPN_DOUBLE );

#ifdef CLIENT_DLL
	if ( prediction->IsFirstTimePredicted() )
	{
		StartFlame();
	}
#else
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, false );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );

	Vector vecDir;
	QAngle angDir = pOwner->EyeAngles();
	AngleVectors( angDir, &vecDir );

	const Vector vecBlastSize = Vector( 128, 128, 64 );

	// Picking max out of length, width, height for airblast distance.
	float flBlastDist = max( max( vecBlastSize.x, vecBlastSize.y ), vecBlastSize.z );

	Vector vecOrigin = pOwner->Weapon_ShootPosition() + vecDir * flBlastDist;

	CBaseEntity* pList[64];

	int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0 );

	//if ( tf2c_debug_airblast.GetBool() )
	//{
	//	NDebugOverlay::Box( vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0 );
	//}

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity* pEntity = pList[i];

		if ( !pEntity )
			continue;

		if ( pEntity == pOwner )
			continue;

		if ( !pEntity->IsDeflectable() )
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		trace_t tr;
		UTIL_TraceLine( pOwner->Weapon_ShootPosition(), pEntity->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
		if ( tr.fraction != 1.0f )
			continue;


		if ( pEntity->IsPlayer() )
		{
			if ( !pEntity->IsAlive() )
				continue;

			CTFPlayer* pTFPlayer = ToTFPlayer( pEntity );

			Vector vecPushDir;
			QAngle angPushDir = angDir;

			// Push them at least 45 degrees up.
			angPushDir[PITCH] = min( -45, angPushDir[PITCH] );

			AngleVectors( angPushDir, &vecPushDir );

			DeflectPlayer( pTFPlayer, pOwner, vecPushDir );
		}
		else
		{
			// Deflect projectile to the point that we're aiming at, similar to rockets.
			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecDeflect;
			GetProjectileReflectSetup( GetTFPlayerOwner(), vecPos, &vecDeflect, false );

			DeflectEntity( pEntity, pOwner, vecDeflect );
		}
	}

	lagcompensation->FinishLagCompensation( pOwner );
#endif

	float flAmmoPerSecondaryAttack = TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK;
	CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecondaryAttack, mult_airblast_cost );

	pOwner->RemoveAmmo( flAmmoPerSecondaryAttack, m_iPrimaryAmmoType );

	// Don't allow firing immediately after airblasting.
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 1.00f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		// If we were firing, stop
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			m_iWeaponState = FT_STATE_IDLE;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle at it appears visually
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetMuzzlePosHelper( bool bVisualPos )
{
	Vector vecMuzzlePos;
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner ) 
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->GetAbsAngles(), &vecForward, &vecRight, &vecUp );
		vecMuzzlePos = pOwner->Weapon_ShootPosition();
		vecMuzzlePos +=  vecRight * TF_FLAMETHROWER_MUZZLEPOS_RIGHT;
		// if asking for visual position of muzzle, include the forward component
		if ( bVisualPos )
		{
			vecMuzzlePos +=  vecForward * TF_FLAMETHROWER_MUZZLEPOS_FORWARD;
		}
	}
	return vecMuzzlePos;
}

#if defined( CLIENT_DLL )

bool CTFFlameThrower::Deploy( void )
{
	StartPilotLight();

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) && ( GetPlayerOwner()->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			if ( m_iWeaponState != FT_STATE_SECONDARY || !GetPlayerOwner()->IsLocalPlayer() )
			{
				StartFlame();
			}
		}
		else
		{
			StartPilotLight();
		}		
	}
	else 
	{
		StopFlame();
		StopPilotLight();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::UpdateOnRemove( void )
{
	StopFlame();
	StopPilotLight();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		if ( !IsDormant() && bDormant )
		{
			StopFlame();
			StopPilotLight();
		}
	}

	// Deliberately skip base combat weapon to avoid being holstered
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartFlame()
{
	// Airblast effect
	if ( m_iWeaponState == FT_STATE_SECONDARY )
	{
		C_BaseEntity* pModel = GetWeaponForEffect();

		if ( pModel )
		{
			pModel->ParticleProp()->Create( "pyro_blast", PATTACH_POINT_FOLLOW, "muzzle" );
		}

		//CLocalPlayerFilter filter;
		//EmitSound( filter, entindex(), "Weapon_FlameThrower.AirBurstAttack" );

		return;
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// normally, crossfade between start sound & firing loop in 3.5 sec
	float flCrossfadeTime = 3.5;

	if ( m_pFiringLoop && ( m_bCritFire != m_bFiringLoopCritical ) )
	{
		// If we're firing and changing between critical & noncritical, just need to change the firing loop.
		// Set crossfade time to zero so we skip the start sound and go to the loop immediately.

		flCrossfadeTime = 0;
		StopFlame( true );
	}

	StopPilotLight();

	if ( !m_pFiringStartSound && !m_pFiringLoop )
	{
		RestartParticleEffect();
		CLocalPlayerFilter filter;

		// Play the fire start sound
		const char *shootsound = GetShootSound( SINGLE );
		if ( flCrossfadeTime > 0.0 )
		{
			// play the firing start sound and fade it out
			m_pFiringStartSound = controller.SoundCreate( filter, entindex(), shootsound );		
			controller.Play( m_pFiringStartSound, 1.0, 100 );
			controller.SoundChangeVolume( m_pFiringStartSound, 0.0, flCrossfadeTime );
		}

		// Start the fire sound loop and fade it in
		if ( m_bCritFire )
		{
			shootsound = GetShootSound( BURST );
		}
		else
		{
			shootsound = GetShootSound( SPECIAL1 );
		}
		m_pFiringLoop = controller.SoundCreate( filter, entindex(), shootsound );
		m_bFiringLoopCritical = m_bCritFire;

		// play the firing loop sound and fade it in
		if ( flCrossfadeTime > 0.0 )
		{
			controller.Play( m_pFiringLoop, 0.0, 100 );
			controller.SoundChangeVolume( m_pFiringLoop, 1.0, flCrossfadeTime );
		}
		else
		{
			controller.Play( m_pFiringLoop, 1.0, 100 );
		}
	}

#ifdef FLAMETHROWER_DLIGHT
	if ( tf_muzzleflash_light.GetBool() && !m_pDynamicLight )
	{
		Vector vecAttachMuzzle;
		int iAttachment = LookupAttachment( "muzzle" );
		GetAttachment( iAttachment, vecAttachMuzzle );
		m_pDynamicLight = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + index );
		m_pDynamicLight->origin = vecAttachMuzzle;
		m_pDynamicLight->radius = 150;
		//m_pDynamicLight->decay = 100 / 0.05f;
		m_pDynamicLight->color.r = 255;
		m_pDynamicLight->color.g = 170;
		m_pDynamicLight->color.b = 8;
		m_pDynamicLight->die = gpGlobals->curtime + 10000000;
		m_pDynamicLight->color.exponent = 1;
	}
#endif

	// check our "hit" sound
	if ( m_bHitTarget != m_bFiringHitTarget )
	{
		if ( m_bHitTarget == false )
		{
			StopHitSound();
		}
		else
		{
			CLocalPlayerFilter filter;
			m_pFiringHitLoop = controller.SoundCreate( filter, entindex(), "Weapon_FlameThrower.FireHit" );
			controller.Play( m_pFiringHitLoop, 1.0, 100 );
		}

		m_bFiringHitTarget = m_bHitTarget;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopHitSound()
{
	if ( m_pFiringHitLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringHitLoop );
		m_pFiringHitLoop = NULL;
	}

	m_bHitTarget = m_bFiringHitTarget = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopFlame( bool bAbrupt /* = false */ )
{
	if ( ( m_pFiringLoop || m_pFiringStartSound ) && !bAbrupt )
	{
		// play a quick wind-down poof when the flame stops
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( SPECIAL3 );
		EmitSound( filter, entindex(), shootsound );
	}

	if ( m_pFiringLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pFiringStartSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_bFlameEffects )
	{
		// Stop the effect on the viewmodel if our owner is the local player
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer && pLocalPlayer == GetOwner() )
		{
			if ( pLocalPlayer->GetViewModel() )
			{
				pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission();
			}
		}
		else
		{
			ParticleProp()->StopEmission();
		}
	}

	if ( !bAbrupt )
	{
		StopHitSound();
	}

#ifdef FLAMETHROWER_DLIGHT
	if ( m_pDynamicLight && m_pDynamicLight->key == LIGHT_INDEX_MUZZLEFLASH + entindex() )
	{
		m_pDynamicLight->die = gpGlobals->curtime;
		m_pDynamicLight = NULL;
	}
#endif

	m_bFlameEffects = false;
	m_iParticleWaterLevel = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartPilotLight()
{
	if ( !m_pPilotLightSound )
	{
		StopFlame();

		// Create the looping pilot light sound
		const char *pilotlightsound = GetShootSound( SPECIAL2 );
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopPilotLight()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::RestartParticleEffect( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	m_iParticleWaterLevel = pOwner->GetWaterLevel();

	// Start the appropriate particle effect
	const char *pszParticleEffect;
	if ( pOwner->GetWaterLevel() == WL_Eyes )
	{
		pszParticleEffect = "flamethrower_underwater";
	}
	else
	{
		if ( m_bCritFire )
		{
			pszParticleEffect = ( pOwner->GetTeamNumber() == TF_TEAM_BLUE ? "flamethrower_crit_blue" : "flamethrower_crit_red" );
		}
		else 
		{
			pszParticleEffect = ( pOwner->GetTeamNumber() == TF_TEAM_BLUE ? "flamethrower_blue" : "flamethrower" );
		}		
	}

	// Start the effect on the viewmodel if our owner is the local player
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer == GetOwner() )
	{
		if ( pLocalPlayer->GetViewModel() )
		{
			pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission();
			pLocalPlayer->GetViewModel()->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" );
		}
	}
	else
	{
		ParticleProp()->StopEmission();
		ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" );
	}
	m_bFlameEffects = true;
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::HitTargetThink( void )
{
	if ( (m_flTimeToStopHitSound > 0) && (m_flTimeToStopHitSound < gpGlobals->curtime) )
	{
		m_bHitTarget = false;
		m_flTimeToStopHitSound = 0;
		SetContextThink( NULL, 0, s_pszFlameThrowerHitTargetThink );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1f, s_pszFlameThrowerHitTargetThink );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetHitTarget( void )
{
	if ( m_iWeaponState > FT_STATE_IDLE )
	{
		m_bHitTarget = true;
		m_flTimeToStopHitSound = gpGlobals->curtime + 0.2;

		// Start the hit target thinking
		SetContextThink( &CTFFlameThrower::HitTargetThink, gpGlobals->curtime + 0.1f, s_pszFlameThrowerHitTargetThink );
	}
}

void CTFFlameThrower::DeflectEntity( CBaseEntity* pEntity, CTFPlayer* pAttacker, Vector& vecDir )
{
	if ( !TFGameRules() )
		return;

	if ( (pEntity->GetTeamNumber() == pAttacker->GetTeamNumber()) )
		return;
#ifdef _DEBUG
	DevMsg( "AIRBLAST: Calling Deflected function on entity %s\n", pEntity->GetClassname() );
#endif
	pEntity->Deflected( pAttacker, vecDir );
	pEntity->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	DispatchParticleEffect( "deflect_fx", PATTACH_ABSORIGIN_FOLLOW, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: function taken from tf2c b4 because leak code version is proto-Jungle Inferno behaviour
//-----------------------------------------------------------------------------
void CTFFlameThrower::DeflectPlayer( CTFPlayer* pVictim, CTFPlayer* pAttacker, Vector& vecDir )
{
	if ( !pVictim )
		return;

	if ( pVictim->InSameTeam( pAttacker ) )
		return;

	// Don't push players if they're too far off to the side. Ignore Z.
	Vector vecVictimDir = pVictim->WorldSpaceCenter() - pAttacker->WorldSpaceCenter();

	Vector vecVictimDir2D( vecVictimDir.x, vecVictimDir.y, 0.0f );
	VectorNormalize( vecVictimDir2D );

	Vector vecDir2D( vecDir.x, vecDir.y, 0.0f );
	VectorNormalize( vecDir2D );

	float flDot = DotProduct( vecDir2D, vecVictimDir2D );
	if ( flDot >= 0.8 )
	{
		// Push enemy players.
#ifdef _DEBUG
		DevMsg( "AIRBLAST: Pushing player %i\n", pVictim->GetClientIndex() );
#endif
		pVictim->SetGroundEntity( NULL );
		pVictim->ApplyAbsVelocityImpulse( vecDir * 500 );
		pVictim->EmitSound( "TFPlayer.AirBlastImpact" );

		// Add pusher as recent damager so he can get a kill credit for pushing a player to his death.
		pVictim->AddDamagerToHistory( pAttacker );
	}
}
#endif

#ifdef GAME_DLL

LINK_ENTITY_TO_CLASS( tf_flame, CTFFlameEntity );

//-----------------------------------------------------------------------------
// Purpose: Spawns this entitye
//-----------------------------------------------------------------------------
void CTFFlameEntity::Spawn( void )
{
	BaseClass::Spawn();

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	// move noclip: update position from velocity, that's it
	SetMoveType( MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	float iBoxSize = tf_flamethrower_boxsize.GetFloat();
	UTIL_SetSize( this, -Vector( iBoxSize, iBoxSize, iBoxSize ), Vector( iBoxSize, iBoxSize, iBoxSize ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	m_vecInitialPos = GetAbsOrigin();
	m_vecPrevPos = m_vecInitialPos;
	m_flTimeRemove = gpGlobals->curtime + ( tf_flamethrower_flametime.GetFloat() * random->RandomFloat( 0.9, 1.1 ) );
	
	// Setup the think function.
	SetThink( &CTFFlameEntity::FlameThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Creates an instance of this entity
//-----------------------------------------------------------------------------
CTFFlameEntity *CTFFlameEntity::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float flDmgAmount, bool bAlwaysCritFromBehind )
{
	CTFFlameEntity *pFlame = static_cast<CTFFlameEntity*>( CBaseEntity::Create( "tf_flame", vecOrigin, vecAngles, pOwner ) );
	if ( !pFlame )
		return NULL;

	// Initialize the owner.
	pFlame->SetOwnerEntity( pOwner );
	pFlame->m_hAttacker = pOwner->GetOwnerEntity();
	CBaseEntity *pAttacker = (CBaseEntity *) pFlame->m_hAttacker;
	if ( pAttacker )
	{
		pFlame->m_iAttackerTeam = pAttacker->GetTeamNumber();
	}

	// Set team.
	pFlame->ChangeTeam( pOwner->GetTeamNumber() );
	pFlame->m_iDmgType = iDmgType;
	pFlame->m_flDmgAmount = flDmgAmount;

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float velocity = tf_flamethrower_velocity.GetFloat();
	float min = -velocity * tf_flamethrower_vecrand.GetFloat();
	float max = velocity * tf_flamethrower_vecrand.GetFloat();

	pFlame->m_vecBaseVelocity = vecForward * velocity;
	// RandomVector does not allow the flame entities to be spawned properly on linux, due to it using rand()
	pFlame->m_vecBaseVelocity += Vector( ::RandomFloat( min, max ), ::RandomFloat( min, max ), ::RandomFloat( min, max ) );
	pFlame->m_vecAttackerVelocity = pOwner->GetOwnerEntity()->GetAbsVelocity();
	pFlame->SetAbsVelocity( pFlame->m_vecBaseVelocity );	
	// Setup the initial angles.
	pFlame->SetAbsAngles( vecAngles );
	pFlame->SetCritFromBehind( bAlwaysCritFromBehind );

	return pFlame;
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFFlameEntity::FlameThink( void )
{
	// if we've expired, remove ourselves
	if ( gpGlobals->curtime >= m_flTimeRemove )
	{
		UTIL_Remove( this );
		return;
	}

	// Do collision detection.  We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server
	if ( GetAbsOrigin() != m_vecPrevPos )
	{
		CTFPlayer *pAttacker = dynamic_cast<CTFPlayer *>( (CBaseEntity *) m_hAttacker );
		if ( !pAttacker )
			return;

		CTFTeam *pTeam = pAttacker->GetOpposingTFTeam();
		if ( !pTeam )
			return;
	
		bool bHitWorld = false;

		// check collision against all enemy players
		for ( int iPlayer= 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
		{
			CBasePlayer *pPlayer = pTeam->GetPlayer( iPlayer );
			// Is this player connected, alive, and an enemy?
			if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
			{
				CheckCollision( pPlayer, &bHitWorld );
				if ( bHitWorld )
					return;
			}
		}

		// check collision against all enemy objects
		for ( int iObject = 0; iObject < pTeam->GetNumObjects(); iObject++ )
		{
			CBaseObject *pObject = pTeam->GetObject( iObject );
			if ( pObject )
			{
				CheckCollision( pObject, &bHitWorld );
				if ( bHitWorld )
					return;
			}
		}
	}

	// Calculate how long the flame has been alive for
	//float flFlameElapsedTime = tf_flamethrower_flametime.GetFloat() - ( m_flTimeRemove - gpGlobals->curtime );
	// Calculate how much of the attacker's velocity to blend in to the flame's velocity.  The flame gets the attacker's velocity
	// added right when the flame is fired, but that velocity addition fades quickly to zero.
	//float flAttackerVelocityBlend = RemapValClamped( flFlameElapsedTime, tf_flamethrower_velocityfadestart.GetFloat(), 
	//	tf_flamethrower_velocityfadeend.GetFloat(), 1.0, 0 );

	// Reduce our base velocity by the air drag constant
	m_vecBaseVelocity *= tf_flamethrower_drag.GetFloat();

	// Add our float upward velocity
	Vector vecVelocity = m_vecBaseVelocity + Vector( 0, 0, tf_flamethrower_float.GetFloat() ) + m_vecAttackerVelocity;

	// Update our velocity
	SetAbsVelocity( vecVelocity );

	// Render debug visualization if convar on
	if ( tf_debug_flamethrower.GetInt() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int) ( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		} 
		else 
		{
			NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	SetNextThink( gpGlobals->curtime );

	m_vecPrevPos = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities
//-----------------------------------------------------------------------------
void CTFFlameEntity::CheckCollision( CBaseEntity *pOther, bool *pbHitWorld )
{
	*pbHitWorld = false;

	// if we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity
	int iIndex = m_hEntitiesBurnt.Find( pOther );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;

	// Do a bounding box check against the entity
	Vector vecMins, vecMaxs;
	pOther->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
	CBaseTrace trace;
	Ray_t ray;
	float flFractionLeftSolid;				
	ray.Init( m_vecPrevPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs() );
	if ( IntersectRayWithBox( ray, vecMins, vecMaxs, 0.0, &trace, &flFractionLeftSolid ) )
	{
		// if bounding box check passes, check player hitboxes
		trace_t trHitbox;
		trace_t trWorld;
		bool bTested = pOther->GetCollideable()->TestHitboxes( ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox );
		if ( !bTested || !trHitbox.DidHit() )
			return;

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine( GetAbsOrigin() + vDir * WorldAlignMaxs().x, m_vecInitialPos, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );			

		if ( tf_debug_flamethrower.GetInt() )
		{
			NDebugOverlay::Line( trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f );
		}
		
		if ( trWorld.fraction == 1.0 )
		{						
			// if there is nothing solid in the way, damage the entity
			OnCollide( pOther );
		}					
		else
		{
			// we hit the world, remove ourselves
			*pbHitWorld = true;
			UTIL_Remove( this );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFFlameEntity::OnCollide( CBaseEntity *pOther )
{
	// remember that we've burnt this player
	m_hEntitiesBurnt.AddToTail( pOther );

	float flDistance = GetAbsOrigin().DistTo( m_vecInitialPos );
	float flMultiplier;
	if ( flDistance <= 125 )
	{
		// at very short range, apply short range damage multiplier
		flMultiplier = tf_flamethrower_shortrangedamagemultiplier.GetFloat();
	}
	else
	{
		// make damage ramp down from 100% to 60% from half the max dist to the max dist
		flMultiplier = RemapValClamped( flDistance, tf_flamethrower_maxdamagedist.GetFloat()/2, tf_flamethrower_maxdamagedist.GetFloat(), 1.0, 0.60 );
	}
	float flDamage = m_flDmgAmount * flMultiplier;
	flDamage = max( flDamage, 1.0 );
	if ( tf_debug_flamethrower.GetInt() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	CBaseEntity *pAttacker = m_hAttacker;
	if ( !pAttacker )
		return;

	CTFFlameThrower* hFlameThrower = dynamic_cast<CTFFlameThrower*>(GetOwnerEntity());
	if ( hFlameThrower )
		hFlameThrower->SetHitTarget();

	int iDamageType = m_iDmgType;

	if ( pOther && pOther->IsPlayer() )
	{
		if ( m_bCritFromBehind )
		{
			if ( IsBehindTarget( pOther ) )
				iDamageType |= DMG_CRITICAL;
		}
	}

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, GetOwnerEntity(), flDamage, iDamageType, TF_DMG_CUSTOM_BURNING);
	info.SetReportedPosition( pAttacker->GetAbsOrigin() );

	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &pTrace );

	pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );
	ApplyMultiDamage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameEntity::IsBehindTarget( CBaseEntity* pTarget )
{
	return (DotProductToTarget( pTarget ) > 0.8);
}

//-----------------------------------------------------------------------------
// Purpose: Utility to calculate dot product between facing angles of flame and target
//-----------------------------------------------------------------------------
float CTFFlameEntity::DotProductToTarget( CBaseEntity* pTarget )
{
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	Vector vecTraveling = m_vecBaseVelocity;
	vecTraveling.z = 0.0f;
	vecTraveling.NormalizeInPlace();

	return DotProduct( vecVictimForward, vecTraveling );
}

#endif // GAME_DLL

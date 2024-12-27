//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_ROCKET_H
#define TF_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "baseprojectile.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_baseanimating.h"
// Server specific.
#else
#include "baseanimating.h"
#include "smoke_trail.h"
#endif

#ifdef CLIENT_DLL
#define CTFBaseRocket C_TFBaseRocket
#endif

#define TF_ROCKET_RADIUS	(110.0f * 1.1f)	//radius * TF scale up factor
#define TF_FLARE_DET_RADIUS			(110)			// Special version of the flare that can be detonated by the player

//=============================================================================
//
// TF Base Rocket.
//
class CTFBaseRocket : public CBaseProjectile
{

//=============================================================================
//
// Shared (client/server).
//
public:

	DECLARE_CLASS( CTFBaseRocket, CBaseProjectile );
	DECLARE_NETWORKCLASS();

			CTFBaseRocket();
			~CTFBaseRocket();

	void	Precache( void );
	void	Spawn( void );

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );
	CNetworkVar( bool, m_bDeflected ); // Used mainly for killfeed

//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );
	CBaseEntity* GetLauncher( void ) { return m_hLauncher; }

protected:
	float	 m_flSpawnTime;
	CNetworkHandle( CBaseEntity, m_hLauncher );

//=============================================================================
//
// Server specific.
//
#else

public:

	DECLARE_DATADESC();

	static CTFBaseRocket *Create( const char *szClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );	

	virtual void	RocketTouch( CBaseEntity *pOther );
	void			Explode( trace_t *pTrace, CBaseEntity *pOther );

	virtual float	GetDamage() { return m_flDamage; }
	virtual int		GetDamageType() { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	virtual float	GetRadius() { return TF_ROCKET_RADIUS; }	
	void			DrawRadius( float flRadius );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }

	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity* pDeflectedBy, Vector& vecDir );
	//virtual void	IncremenentDeflected( void );

	virtual void	SetLauncher( CBaseEntity* pLauncher ) OVERRIDE { m_hLauncher = pLauncher; BaseClass::SetLauncher( pLauncher ); }
	CBaseEntity* GetLauncher( void ) { return m_hLauncher; }

protected:

	void			FlyThink( void );

protected:

	// Not networked.
	float					m_flDamage;

	CNetworkHandle( CBaseEntity, m_hLauncher );

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;


	CHandle<CBaseEntity>	m_hEnemy;

#endif
};

#endif // TF_WEAPONBASE_ROCKET_H
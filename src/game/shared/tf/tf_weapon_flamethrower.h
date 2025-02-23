//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_FLAMETHROWER_H
#define TF_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "particlemgr.h"
	#include "particle_util.h"
	#include "particles_simple.h"
	#include "c_tf_projectile_rocket.h"
	#include "dlight.h"

	#define CTFFlameThrower C_TFFlameThrower
#else
	#include "tf_projectile_rocket.h"
	#include "baseentity.h"
#endif

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING,
	FT_STATE_SECONDARY
};

//=========================================================
// Flamethrower Weapon
//=========================================================
class CTFFlameThrower : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CTFFlameThrower, CTFWeaponBaseGun );
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlameThrower();
	~CTFFlameThrower();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_FLAMETHROWER; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	Lower( void );
	virtual void	WeaponReset( void );
	virtual void	DestroySounds( void );

	bool			CanAirBlast( void ) const;
	//void			FireAirBlast( int iAmmoPerShot );

	Vector GetVisualMuzzlePos();
	Vector GetFlameOriginPos();
#if defined( CLIENT_DLL )
	virtual bool	Deploy( void );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove( void );
	virtual void	SetDormant( bool bDormant );

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	void			RestartParticleEffect();	

	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();

	// burning sound when you hit a player/building
	void			StopHitSound();
#else
	void			SetHitTarget( void );
	void			HitTargetThink( void );

	virtual void	DeflectEntity( CBaseEntity* pEntity, CTFPlayer* pAttacker, Vector& vecDir );
	virtual void	DeflectPlayer( CTFPlayer* pVictim, CTFPlayer* pAttacker, Vector& vecDir );
#endif

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( bool, m_bCritFire );
	CNetworkVar( bool, m_bHitTarget );

	float m_flStartFiringTime;
	float m_flNextPrimaryAttackAnim;

	int			m_iParticleWaterLevel;
	float		m_flAmmoUseRemainder;
	float		m_flResetBurstEffect;
	bool		m_bFiredSecondary;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pFiringStartSound;
	CSoundPatch	*m_pFiringLoop;
	CSoundPatch* m_pFiringHitLoop;
	bool		m_bFiringLoopCritical;
	bool		m_bFiringHitTarget;
	bool		m_bFlameEffects;
	CSoundPatch *m_pPilotLightSound;
#ifdef FLAMETHROWER_DLIGHT
	dlight_t* m_pDynamicLight;
#endif
#else
	float		m_flTimeToStopHitSound;
#endif

	CTFFlameThrower( const CTFFlameThrower & );
};

//=============================================================================
#define	TF_FLAMETHROWER_ROCKET_DAMAGE				15
#define TF_FLAMETHROWER_ROCKET_BURN_RADIUS			198

#ifdef GAME_DLL

class CTFFlameEntity : public CBaseEntity
{
	DECLARE_CLASS( CTFFlameEntity, CBaseEntity );
public:

	virtual void Spawn( void );

public:
	static CTFFlameEntity *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float m_flDmgAmount, bool bAlwaysCritFromBehind );

	void FlameThink( void );
	void SetCritFromBehind( bool bState ) { m_bCritFromBehind = bState; }
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );
private:
	void OnCollide( CBaseEntity *pOther );
	bool IsBehindTarget( CBaseEntity* pTarget );
	float DotProductToTarget( CBaseEntity* pTarget );

	Vector					m_vecInitialPos;		// position the flame was fired from
	Vector					m_vecPrevPos;			// position from previous frame
	Vector					m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector					m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	float					m_flTimeRemove;			// time at which the flame should be removed
	int						m_iDmgType;				// damage type
	float					m_flDmgAmount;			// amount of base damage
	CUtlVector<EHANDLE>		m_hEntitiesBurnt;		// list of entities this flame has burnt
	EHANDLE					m_hAttacker;			// attacking player
	int						m_iAttackerTeam;		// team of attacking player
	bool					m_bCritFromBehind;		// Always crits from behind.
};

#endif // GAME_DLL

#endif // TF_WEAPON_FLAMETHROWER_H

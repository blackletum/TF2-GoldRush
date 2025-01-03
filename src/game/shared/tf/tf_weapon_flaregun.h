//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_FLAREGUN_H
#define TF_WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#ifdef GAME_DLL
#include "tf_projectile_flare.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#define CTFFlareGun_Revenge C_TFFlareGun_Revenge
#endif

enum FlareGunTypes_t
{
	FLAREGUN_NORMAL = 0,
	FLAREGUN_DETONATE,
};

//=============================================================================
//
// TF Weapon Flare gun.
//
class CTFFlareGun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFFlareGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGun();
	~CTFFlareGun();

	virtual void	Precache();
	virtual void	DestroySounds( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool Deploy( void );
	virtual void ItemPostFrame( void );
	virtual void WeaponReset( void );

	int				GetFlareGunType( void ) const	{ int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; }

#ifdef GAME_DLL
	void			AddFlare( CTFProjectile_Flare *pFlare );
	void			DeathNotice( CBaseEntity *pVictim );
	void			DetonateFlare( void );
#endif

protected:

#ifdef GAME_DLL
	typedef CHandle<CTFProjectile_Flare>	FlareHandle;
	CUtlVector <FlareHandle>				m_Flares;
	int										m_iFlareCount;
#endif

protected:

#ifdef CLIENT_DLL
	virtual void DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	void ClientEffectsThink( void );
	virtual bool ShouldPlayClientReloadSound() { return true; }
#endif

private:

	bool m_bEffectsThinking;
	float m_flLastDenySoundTime;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pChargeLoop;
	bool m_bReadyToFire;
#endif

	CTFFlareGun( const CTFFlareGun & ) {}
};

#endif // TF_WEAPON_FLAREGUN_H

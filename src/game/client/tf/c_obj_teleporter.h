//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_TELEPORTER_H
#define C_OBJ_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"

class C_ObjectTeleporter : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectTeleporter, C_BaseObject );
public:
	DECLARE_CLIENTCLASS();

	C_ObjectTeleporter();

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual void GetStatusText( wchar_t *pStatus, int iMaxStatusLen );
	virtual void GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes );

	virtual void ClientThink( void );

	virtual void UpdateOnRemove();

	virtual CStudioHdr *OnNewModel( void );

	virtual bool IsPlacementPosValid( void );

	float GetChargeTime( void );

	float GetCurrentRechargeDuration( void ) { return m_flCurrentRechargeDuration; }

	int GetState( void ) { return m_iState; }

	int GetTimesUsed( void );

	void StartChargedEffects( void );
	void StopChargedEffects( void );

	void StartActiveEffects( void );
	void StopActiveEffects( void );

	virtual void UpdateDamageEffects( BuildingDamageLevel_t damageLevel );

	virtual int		GetUpgradeLevel( void ) { return m_iUpgradeLevel; }
	int				GetUpgradeMetal( void ) { return m_iUpgradeMetal; }

private:
	int m_iState;
	int m_iOldState;
	float m_flRechargeTime;
	float m_flCurrentRechargeDuration;
	int m_iTimesUsed;
	float m_flYawToExit;

	int m_iDirectionArrowPoseParam;

	CNewParticleEffect			*m_pChargedEffect;
	CNewParticleEffect			*m_pDirectionEffect;

	CNewParticleEffect			*m_pChargedLeftArmEffect;
	CNewParticleEffect			*m_pChargedRightArmEffect;

	CNewParticleEffect			*m_pDamageEffects;

	CSoundPatch		*m_pSpinSound;

private:
	C_ObjectTeleporter( const C_ObjectTeleporter & ); // not defined, not accessible
};

#endif	//C_OBJ_TELEPORTER_H
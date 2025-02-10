//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Teleporter
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_TELEPORTER_H
#define TF_OBJ_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"

class CTFPlayer;

#define TELEPORTER_MAX_HEALTH	150

// ------------------------------------------------------------------------ //
// Base Teleporter object
// ------------------------------------------------------------------------ //
class CObjectTeleporter : public CBaseObject
{
	DECLARE_CLASS( CObjectTeleporter, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectTeleporter();

	virtual void	Spawn();
	virtual void	FirstSpawn( void );
	virtual void	Precache();
	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void) ;
	virtual bool	IsPlacementPosValid( void );
	virtual void	SetModel( const char *pModel );

	virtual void	FinishedBuilding( void );

	void SetState( int state );
	virtual void	DeterminePlaybackRate( void );

	void TeleporterThink( void );
	void TeleporterTouch( CBaseEntity *pOther );

	virtual void TeleporterSend( CTFPlayer *pPlayer ) { Assert(0); }
	virtual void TeleporterReceive( CTFPlayer *pPlayer, float flDelay ) { Assert(0); }

	CObjectTeleporter *GetMatchingTeleporter( void );
	CObjectTeleporter *FindMatch( void );	// Find the teleport partner to this object

	bool IsMatchingTeleporterReady( void );

	int GetState( void ) { return m_iState; }	// state of the object ( building, charging, ready etc )

	void SetTeleportingPlayer( CTFPlayer *pPlayer )
	{
		m_hTeleportingPlayer = pPlayer;
	}

	bool IsReady( void )
	{
		if (!IsMatchingTeleporterReady())
			return false;

		return GetState() != TELEPORTER_STATE_BUILDING && !IsUpgrading() && !IsDisabled();
	}
	bool IsSendingPlayer( CTFPlayer* pPlayer )
	{
		return (GetState() == TELEPORTER_STATE_SENDING && m_hTeleportingPlayer == pPlayer);
	}

	// Upgrading
	virtual void	StartUpgrading( void );
	virtual void	FinishUpgrading( void );
	virtual bool	IsUpgrading( void ) const { return (m_iState == TELEPORTER_STATE_UPGRADING); }
	virtual bool	CheckUpgradeOnHit( CTFPlayer* pPlayer );
	void			CopyUpgradeStateToMatch( CObjectTeleporter* pMatch, bool bFrom );
	virtual void	Explode( void );

	virtual int		GetBaseHealth( void ) { return TELEPORTER_MAX_HEALTH; }

	virtual void	MakeCarriedObject( CTFPlayer* pCarrier );

protected:
	CNetworkVar( int, m_iState );
	CNetworkVar( float, m_flRechargeTime );
	CNetworkVar( float, m_flCurrentRechargeDuration );
	CNetworkVar( int, m_iTimesUsed );
	CNetworkVar( float, m_flYawToExit );

	CHandle<CObjectTeleporter> m_hMatchingTeleporter;

	float m_flLastStateChangeTime;

	float m_flMyNextThink;	// replace me

	CHandle<CTFPlayer> m_hTeleportingPlayer;

	float m_flNextEnemyTouchHint;

	// Direction Arrow, shows roughly what direction the exit is from the entrance
	void ShowDirectionArrow( bool bShow );

	bool m_bShowDirectionArrow;
	int m_iDirectionBodygroup;
	int m_iBlurBodygroup;

private:
	DECLARE_DATADESC();

	void UpdateMaxHealth( int nHealth, bool bForce = false );
};

class CObjectTeleporter_Entrance : public CObjectTeleporter
{
public:
	DECLARE_CLASS( CObjectTeleporter_Entrance, CObjectTeleporter );

	CObjectTeleporter_Entrance();

	virtual void Spawn();
	virtual void TeleporterSend( CTFPlayer *pPlayer );
};

class CObjectTeleporter_Exit : public CObjectTeleporter
{
public:
	DECLARE_CLASS( CObjectTeleporter_Exit, CObjectTeleporter );

	CObjectTeleporter_Exit();

	virtual void Spawn();
	virtual void TeleporterReceive( CTFPlayer *pPlayer, float flDelay );
};

#endif // TF_OBJ_TELEPORTER_H

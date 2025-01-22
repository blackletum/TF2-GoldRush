//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "tf_weapon_parse.h"

#define TE_RIFLE_SHELL 1024
#define TE_PISTOL_SHELL 2048

ConVar tf_brass_lifetime( "tf_brass_lifetime", "20.0", FCVAR_ARCHIVE, "How long ejected shells should last. 0 to disable shells." );

extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TF_EjectBrassCallback( const CEffectData &data )
{
	// Check if brass is disabled
	if ( !tf_brass_lifetime.GetBool() )
		return;

	CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfo( data.m_nHitBox );
	if ( !pWeaponInfo )
		return;
	if ( !pWeaponInfo->m_szBrassModel || !pWeaponInfo->m_szBrassModel[0] )
		return;

	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );
	
	Vector vecVelocity = random->RandomFloat( 130, 180 ) * vForward +
						 random->RandomFloat( -30, 30 ) * vRight +
						 random->RandomFloat( -30, 30 ) * vUp;

	float flLifeTime = tf_brass_lifetime.GetFloat();

	model_t *pModel = (model_t *)engine->LoadModel( pWeaponInfo->m_szBrassModel );
	if ( !pModel )
		return;
	
	int flags = FTENT_FADEOUT | FTENT_GRAVITY | FTENT_COLLIDEALL | FTENT_HITSOUND | FTENT_ROTATE;

	if ( data.m_nHitBox == TF_WEAPON_MINIGUN )
	{
		// More velocity for Jake
		vecVelocity = random->RandomFloat( 130, 250 ) * vForward +
			random->RandomFloat( -100, 100 ) * vRight +
			random->RandomFloat( -30, 80 ) * vUp;
	}

	Assert( pModel );	

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, data.m_vOrigin, vecShellAngles, vecVelocity, flLifeTime, FTENT_NEVERDIE );
	if ( pTemp == NULL )
		return;

	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat(-512,511);
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat(-255,255);

	if ( !Q_strcmp( pWeaponInfo->m_szBrassModel, "models/weapons/shells/shell_shotgun.mdl" ) )
	{
		pTemp->hitSound = BOUNCE_SHOTSHELL;
	}
	else
	{
		pTemp->hitSound = BOUNCE_SHELL;
	}

	pTemp->SetGravity( 0.4 );

	pTemp->m_flSpriteScale = 10;

	pTemp->flags = flags;

	// don't collide with owner
	pTemp->clientIndex = data.entindex();
	if ( pTemp->clientIndex < 0 )
	{
		pTemp->clientIndex = 0;
	}

	// ::ShouldCollide decides what this collides with
	pTemp->flags |= FTENT_COLLISIONGROUP;
	pTemp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
}

DECLARE_CLIENT_EFFECT( "TF_EjectBrass", TF_EjectBrassCallback );
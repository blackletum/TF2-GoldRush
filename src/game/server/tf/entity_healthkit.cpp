//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_healthkit.h"

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_MODEL			"models/items/healthkit.mdl"
#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKit.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_HEALTHKIT_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
		Assert( pTFPlayer );

		const bool bIsAnyHeavyWithSandvichEquippedPickingUp = pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX ) && pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS );

		// In the case of sandvich's owner, only restore ammo
		if ( GetOwnerEntity() == pPlayer && bIsAnyHeavyWithSandvichEquippedPickingUp )
		{
			if ( pPlayer->GiveAmmo( 1, TF_AMMO_GRENADES1, false ) )
			{
				bSuccess = true;
			}
		}
		else
		{
			float flHealth = ceil( pPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()] );
			int nHealthGiven = pPlayer->TakeHealth( flHealth, DMG_GENERIC );
			if ( nHealthGiven )
			{
				CSingleUserRecipientFilter user( pPlayer );
				user.MakeReliable();

				UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( GetClassname() );
				MessageEnd();

				EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );

				bSuccess = true;

				CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );

				Assert( pTFPlayer );

				// Healthkits also contain a fire blanket.
				if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
				{
					pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
				}

				// Spawns a number on the player's health bar in the HUD, and also
				// spawns a "+" particle over their head for enemies to see
				if ( nHealthGiven && !pTFPlayer->m_Shared.IsStealthed() )
				{
					IGameEvent* event = gameeventmanager->CreateEvent( "player_healed" );
					if ( event )
					{
						event->SetInt( "priority", 1 );	// HLTV event priority
						event->SetInt( "amount", nHealthGiven );
						event->SetInt( "patient", pPlayer->GetUserID() );
						event->SetInt( "healer", pPlayer->GetUserID() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

	}

	return bSuccess;
}

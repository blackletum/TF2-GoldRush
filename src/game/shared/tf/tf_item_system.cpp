//================================================================
// Goldrush Item Schema - powered by conneath industries ltd co tm
// credit to the tf2c team for making the original econ system
// that every tf2 mod seems to use these days (including us)
//================================================================
#include "cbase.h"
#include "tf_item_system.h"
#include "tf_item_schema_parser.h"
#include "script_parser.h"
#include "activitylist.h"
#include "animation.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IMDLCache* g_pMDLCache;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static CEconItemSchema g_EconItemSchema;
CEconItemSchema* GetItemSchema()
{
	return &g_EconItemSchema;
}

CEconSchemaParser g_EconSchemaParser;

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema()
{
	SetDefLessFunc( m_Items );
	SetDefLessFunc( m_Attributes );

	m_bInited = false;
}

CEconItemSchema::~CEconItemSchema()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CEconItemSchema::Init( void )
{
	if ( !m_bInited )
	{
		// Must register activities early so we can parse animation replacements.
		ActivityList_Free();
		ActivityList_RegisterSharedActivities();

		float flStartTime = engine->Time();
		g_EconSchemaParser.InitParser( "scripts/items/items_game.txt", true, false );
		float flEndTime = engine->Time();
		Msg( "[%s] Processing item schema took %.02fms. Parsed %d items and %d attributes.\n",
			CBaseEntity::IsServer() ? "SERVER" : "CLIENT",
			(flEndTime - flStartTime) * 1000.0f,
			m_Items.Count(),
			m_Attributes.Count() );

		m_bInited = true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Runs on level start, precaches models and sounds from schema.
//-----------------------------------------------------------------------------
void CEconItemSchema::Precache( void )
{
	string_t strPrecacheAttribute = AllocPooledString( "custom_projectile_model" );

	// Precache everything from schema.
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition* pItem = m_Items[i];

		// Precache models.
		if ( pItem->model_world[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_world );

		if ( pItem->model_player[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_player );

		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			const char* pszModel = pItem->model_player_per_class[iClass];
			if ( pszModel[0] != '\0' )
				CBaseEntity::PrecacheModel( pszModel );
		}

		// Precache visuals.
		for ( int i = 0; i < TF_TEAM_COUNT; i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			EconItemVisuals* pVisuals = &pItem->visual[i];

			// Precache attachments.
			for ( int i = 0; i < pVisuals->m_AttachedModels.Count(); i++ )
			{
				if ( pVisuals->m_AttachedModels[i].m_szModelName != '\0' )
				{
#if 0
					// ULTRAHACK: setting bodygroups is relatively straightforward on C_BaseAnimating, however we have to get creative if we want to set it here...
					// while we precache, check our model to see if we have a viewmodel bodygroup we can use and save it for later, we'll set it in tf_econ_entity.cpp's DrawEconEntityAttachedModels
					// basically the same as what C_BaseAnimating does but our CStudioHdr is locally sourced, delicious
					pVisuals->m_AttachedModels[i].m_iViewModelBodygroup = -1;
					const model_t* pModel = modelinfo->GetModel( CBaseEntity::PrecacheModel( pVisuals->m_AttachedModels[i].m_szModelName ) );
					if ( pModel )
					{
						CStudioHdr* pStudioHdr = new CStudioHdr;
						pStudioHdr->Init( modelinfo->GetStudiomodel( pModel ), g_pMDLCache );
						if ( pStudioHdr->IsValid() )
						{
							int iAttachmentBodyGroup = ::FindBodygroupByName( pStudioHdr, "v_model_gr" );
							if ( iAttachmentBodyGroup != -1 )
							{
								::SetBodygroup( pStudioHdr, pVisuals->m_AttachedModels[i].m_iViewModelBodygroup, iAttachmentBodyGroup, 1);
							}
						}
						delete pStudioHdr;
					}
#else
					CBaseEntity::PrecacheModel( pVisuals->m_AttachedModels[i].m_szModelName );
#endif
				}
			}

			// Precache sounds.
			for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
			{
				if ( pVisuals->aWeaponSounds[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->aWeaponSounds[i] );
			}
		}

		// Cache all attrbute names.
		for ( int i = 0; i < pItem->attributes.Count(); i++ )
		{
			CEconItemAttribute* pAttribute = &pItem->attributes[i];
			pAttribute->m_strAttributeClass = AllocPooledString( pAttribute->attribute_class );

			// Special case for custom_projectile_model attribute.
			if ( pAttribute->m_strAttributeClass == strPrecacheAttribute )
			{
				CBaseEntity::PrecacheModel( pAttribute->value_string.Get() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemDefinition* CEconItemSchema::GetItemDefinition( int id )
{
	if ( id < 0 )
		return NULL;

	CEconItemDefinition* itemdef = NULL;
	FIND_ELEMENT( m_Items, id, itemdef );
	return itemdef;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EconAttributeDefinition* CEconItemSchema::GetAttributeDefinition( int id )
{
	if ( id < 0 )
		return NULL;

	EconAttributeDefinition* attribdef = NULL;
	FIND_ELEMENT( m_Attributes, id, attribdef );
	return attribdef;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EconAttributeDefinition* CEconItemSchema::GetAttributeDefinitionByName( const char* name )
{
	//unsigned int index = m_Attributes.Find(name);
	//if (index < m_Attributes.Count())
	//{
	//	return &m_Attributes[index];
	//}
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EconAttributeDefinition* CEconItemSchema::GetAttributeDefinitionByClass( const char* classname )
{
	//unsigned int index = m_Attributes.Find(name);
	//if (index < m_Attributes.Count())
	//{
	//	return &m_Attributes[index];
	//}
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->attribute_class, classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemSchema::GetAttributeIndex( const char* name )
{
	if ( !name )
		return -1;

	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes.Key( i );
		}
	}

	return -1;
}
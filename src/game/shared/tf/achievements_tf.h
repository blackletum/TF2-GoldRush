//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: achievement helper classes, used by all of the tf achievement cpp files
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL
#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"

bool CheckWinNoEnemyCaps( IGameEvent* event, int iRole );
bool IsLocalTFPlayerClass( int iClass );

// helper class for achievements that check that the player was playing on a game team for the full round
class CTFAchievementFullRound : public CBaseAchievement
{
	DECLARE_CLASS( CTFAchievementFullRound, CBaseAchievement );
public:
	void Init();
	virtual void ListenForEvents();
	void FireGameEvent_Internal( IGameEvent* event );
	bool PlayerWasInEntireRound( float flRoundTime );

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent* event ) = 0;
};

class CAchievementTopScoreboard : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTopScoreboard, CTFAchievementFullRound );

public:
	void Init();
	virtual void ListenForEvents();
	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent* event );
};

//----------------------------------------------------------------------------------------------------------------
// All class specific achievements should derive from this. It takes care of ensuring that the class
// check is performed, and saves needless event handling for other class's achievements.
class CTFClassSpecificAchievement : public CBaseAchievement
{
	DECLARE_CLASS( CTFClassSpecificAchievement, CBaseAchievement );
public:
	virtual bool LocalPlayerCanEarn( void );
};

extern CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

#endif // CLIENT_DLL

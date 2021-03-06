#ifndef __BOTRIX_BOT_H__
#define __BOTRIX_BOT_H__


#include "vector.h"
#include "eiface.h"
#include "in_buttons.h"
#include "usercmd.h"

#include "types.h"

#include "chat.h"
#include "item.h"
#include "mod.h"
#include "players.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "waypoint_navigator.h"
#include "weapon.h"

#include "good/bitset.h"


#define BotMessage(...)             { if ( m_bDebugging ) CUtil::Message(NULL, __VA_ARGS__); }


class CBotChat; // Forward declaration.


//****************************************************************************************************************
/// Class that represents atomic command, such as aim or move or attack, etc.
/** The execution will first perform command, and then will wait until given condition becomes true. */
//****************************************************************************************************************
class CBotAction
{
public:
	TBotAtomicAction iActionType;   ///< Action type, like aim or move.
	union UArgument {
		TWaypointId iWaypointId;    ///< Waypoint id.
		int iWeapon;                ///< Weapon index.
		float vPosition[3];         ///< Vector.
		float vAngles[2];           ///< Angles: pitch and yaw.
		float fTime[2];             ///< Action start time (0) and action end time (1). Relative, not absolute.
	} uniArgument;                  ///< Argument of action.

	TWaitCondition iWaitCondition;  ///< Wait condition after execution of this action.
	void* pWaitArgument;            ///< Pointer to CPlayer, CEntity or wait time (as float).
};


//****************************************************************************************************************
/// Abstract class representing a bot.
/** This class allows to bot ability to move and use waypoints, but not to shoot. Shooting and setting
  * objectives/waypoint-destination is a child class responsability (which should derive from CBot class). */
//****************************************************************************************************************
class CBot: public CPlayer
{

public: // Methods.
	/// Constructor.
	CBot( edict_t* pEdict, TPlayerIndex iIndex, TBotIntelligence iIntelligence );

	/// Destructor.
	virtual ~CBot() {}

	/// Show or hide bot messages.
	void SetDebugging( bool bOn ) { m_bDebugging = bOn; }

	/// Bot is created for testing path between two waypoints. Will be kicked if fails, killed, or reaches destination.
	void TestWaypoints( TWaypointId iFrom, TWaypointId iTo );

	/// Emulate console command for bot.
	void ConsoleCommand(const char* szFormat, ...);

	/// Use say command.
	void Say(bool bTeamOnly, const char* szFormat, ...);

	//------------------------------------------------------------------------------------------------------------
	// Next functions are mod dependent.
	//------------------------------------------------------------------------------------------------------------
	/// Called when player becomes active, before first respawn. Sets players model and team.
	virtual void Activated();

	/// Respawned on map. Reset all variables, waypoint navigator.
	virtual void Respawned();

	/// Called when player becomes dead. Will kick it if bot was created for testing purposes.
	virtual void Dead();

	/// Called when player got disconnected / kicked.
	virtual void PlayerDisconnect( int iPlayerIndex, CPlayer* pPlayer );

	/// Called when this bot just killed an enemy.
	virtual void KilledEnemy( int iPlayerIndex, CPlayer* pPlayer ) = 0;

	/// Called when enemy just shot this bot.
	virtual void HurtBy( int iPlayerIndex, CPlayer* pPlayer , int iHealthNow ) = 0;

	/// Called when chat arrives from other player.
	virtual void ReceiveChat( int iPlayerIndex, CPlayer* pPlayer, bool bTeamOnly, const char* szText ) = 0;

	/// Called when chat request arrives from other player.
	virtual void ReceiveChatRequest( const CBotChat& cRequest );

	/// Called when bot decides to help teammate.
	virtual void StartPerformingChatRequest( const CBotChat& cRequest );

	/// Called when 30 seconds has passed.
	virtual void EndPerformingChatRequest( bool bSayGoodbye );


private:
	// Called every frame to evaluate next move. Note that this method is private, use Move() method in subclasses.
	virtual void PreThink();


protected: // Mod dependend protected functions.
	//------------------------------------------------------------------------------------------------------------
	// Next functions are mod dependent.
	//------------------------------------------------------------------------------------------------------------
	// Called each frame. Set move and look variables. You can also set shooting/crouching/jumping buttons in m_cCmd.buttons.
	virtual void Think() = 0;

	// This function get's called when next waypoint in path becomes closer than current one. By default sets new
	// look forward to next waypoint in path (after current one).
	virtual void CurrentWaypointJustChanged();

	// Check waypoint flags and perform waypoint action. Called when arrives to next waypoint. By default this 
	// will check if waypoint has health/armor machine flags and if bot needs to use it. If so, then this function 
	// returns true to not to call ApplyPathFlags() / DoPathAction(), while performing USE action (for machine),
	// and ApplyPathFlags() / DoPathAction() will be called after using machine.
	virtual bool DoWaypointAction();

	// Check waypoint path flags and set bot flags accordingly. Called when arrives to next waypoint.
	// Note that you need to set action variables in DoPathAction(). This function is just to
	// figure out if need to crouch/sprint/etc and set new aim/destination to next waypoint in path.
	virtual void ApplyPathFlags();

	// Called when started to touch waypoint. You can use iCurrentWaypoint and iNextWaypoint to get path flags.
	// Will set all needed action variables to jump / jump with duck / break.
	virtual void DoPathAction();

	// Return true if given player is enemy. Really it is mod's dependant.
	virtual bool IsEnemy( CPlayer* pPlayer ) const
	{
		int idx = m_pPlayerInfo->GetTeamIndex();
		return (idx == CMod::iUnassignedTeam) || (idx != pPlayer->GetPlayerInfo()->GetTeamIndex());
	}

	// Bot just picked up given item.
	virtual void PickItem( const CEntity& cItem, TEntityType iEntityType, TEntityIndex iIndex );


protected: // Methods.
	static const int SIZE_ACTIONS = 32; // Size of circular buffer of actions.


	/// Say current chat.
	void Speak( bool bTeamSay );

	// Return true if entity is inside bot's view cone.
	bool IsVisible( CPlayer* pPlayer ) const;

	// Move failure is produced when current waypoint is invalid or when using navigator,
	// there is no path from current to next waypoint.
	bool CheckMoveFailure()
	{
		if ( !m_bMoveFailure )
		{
			if ( !CWaypoint::IsValid(iCurrentWaypoint) )
				m_bMoveFailure = true;
			else if ( m_bNeedMove && m_bUseNavigatorToMove && m_pNavigator.SearchEnded() && !m_bDestinationChanged &&
			        ( !CWaypoint::IsValid(iNextWaypoint) || 
					( (iCurrentWaypoint != iNextWaypoint) && !CWaypoints::HasPath(iCurrentWaypoint, iNextWaypoint) ) ) )
				m_bMoveFailure = true;
		}
		return m_bMoveFailure;
	}

	// Get time to end aiming to sinchronize angles. Depends on how much 'mouse' distance there are and bot's intelligence.
	float GetEndLookTime();

	// Update nearest objects, players, items and weapons.
	void UpdateWorld();

	// Check if this enemy can be seen / should be attacked.
	void CheckEnemy( int iPlayerIndex, CPlayer* pPlayer, bool bCheckVisibility );

	// Aim at enemy.
	void EnemyAim();

	// Choose best attack weapon from available weapons.
	void CheckWeapon();

	// Set active weapon.
	void SetActiveWeapon( int iIndex );

	// Shoot current weapon.
	void Shoot( bool bSecondary = false );

	// Toggle zoom.
	void ToggleZoom()
	{
		CWeaponWithAmmo& cWeapon = m_aWeapons[m_iWeapon];
		DebugAssert( cWeapon.IsSniper() && cWeapon.CanUse() );
		if ( cWeapon.IsUsingZoom() )
			cWeapon.ZoomOut();
		else
			cWeapon.ZoomIn();
		BotMessage( "%s -> Zoom %s: %s.", GetName(), m_aWeapons[m_iWeapon].GetName().c_str(), cWeapon.IsUsingZoom() ? "true" : "false" );
		FLAG_SET(IN_ATTACK2, m_cCmd.buttons);
	}

	// Reload current weapon.
	void Reload()
	{
		BotMessage( "%s -> Reload %s.", GetName(), m_aWeapons[m_iWeapon].GetName().c_str() );
		m_aWeapons[m_iWeapon].Reload(0);
		FLAG_SET(IN_RELOAD, m_cCmd.buttons);
	}

	// Check if bot needs to change side look.
	void CheckSideLook( bool bIsMoving, bool bForceCheck );

	// While stucked, return true if move destination have been calculated, i.e. bot can continue moving to destination, 
	// false if bot needs to restart thinking (in order to perform 'touch' on stucked waypoint for example).
	bool ResolveStuckMove();

	// Setup path search / continues searching / returns true if search finishes or bot arrived to next waypoint in path.
	bool NavigatorMove();

	// If not using navigator, returns true if bot arrived to m_vDestination.
	bool NormalMove();

	// If m_bNeedMove then moves toward m_vDestination. If m_bNeedAim then smoothly aims to m_vLook until m_fEndAimTime.
	void PerformMove( TWaypointId iPrevCurrentWaypoint, Vector const& vPrevOrigin );



protected: // Bot flags.
	bool m_bFirstRespawn:1;                                        // Spawn event is called before bot constructor returns. So first time we call Respawned() manually. TODO: dont create bot in bot constructor.
	bool m_bTest:1;                                                // Bot was created only for testing purposes, it will be eliminated after reaching needed waypoint. 
	bool m_bDebugging:1;                                           // Currently debugging this bot.

	bool m_bAimChanged:1;                                          // True if need to change bot's angles (if false m_angLook has been calculated).
	bool m_bNeedAim:1;                                             // True if need to change bot's angles. Will be set to false when finished aiming.
	bool m_bUseSideLook:1;                                         // If true then look at random: forward, left, right or back.
	bool m_bLockAim:1;                                             // Don't change bot's angles while look is locked. Used when preparing to jump, use, sprint, etc.

	bool m_bDestinationChanged:1;                                  // Set this to true when you change destination to go to.
	bool m_bNeedMove:1;                                            // True if need to move. Will be set to false if reached m_vDestination.
	bool m_bLastNeedMove:1;                                        // m_bNeedMove value of previous frame.
	bool m_bLockMove:1;                                            // Force not to move.
	
	bool m_bUseNavigatorToMove:1;                                  // If true then use waypoint navigator to get to m_vDestination, else just move there in line.
	bool m_bLockNavigatorMove:1;                                   // Don't move while this boolean is true. Used when preparing to jump/sprint/etc to next waypoint.
	bool m_bMoveFailure:1;                                         // Set to true when bot can't reach m_vDestination through navigator. Make sure it is false.

	bool m_bStuck:1;                                               // Will be set to true, if m_bNeedMove is set, but couldn't move for 1 second.
	bool m_bNeedCheckStuck:1;                                      // If true then check if stucked at m_fStuckCheckTime.
	bool m_bStuckBreakObject:1;                                    // If true then will try break m_aNearestItems.
	bool m_bStuckUsePhyscannon:1;                                  // If true then will try move m_aNearestItems with gravity gun.
	bool m_bStuckTryingSide:1;                                     // If true then bot is stucked, and going left or right for half second according to m_bStuckTryGoLeft.
	bool m_bStuckTryGoLeft:1;                                      // If true go left when stucked, else go right.
	bool m_bStuckGotoCurrent:1;                                    // When stucked will try go to current waypoint (for 'touching' and so performing action).
	                                                               // If false, then will try to do left or right move.
	bool m_bStuckGotoPrevious:1;                                   // 
	bool m_bRepeatWaypointAction;                                  // Set when stucked, and repeats go to current waypoint and touch it.

	bool m_bLadderMove:1;                                          // Will be set to true, when current waypoint path has ladder flag.

	bool m_bNeedStop:1;                                            // Need to stop when reach next waypoint.
	bool m_bNeedDuck:1;                                            // Will crouch until reaches next waypoint or until action time end (if used with jump).
	bool m_bNeedWalk:1;                                            // Will walk on straight line.
	bool m_bNeedSprint:1;                                          // Will sprint until reaches next waypoint. Used for long jumps.

	bool m_bNeedReload:1;                                          // Need to reload some weapons.

	bool m_bNeedFlashlight:1;                                      // Need to use flashlight.
	bool m_bUsingFlashlight:1;                                     // True if currently using flashlight.

	bool m_bNeedUse:1;                                             // Will starts using USE button at m_fStartActionTime until m_fEndActionTime.
	bool m_bAlreadyUsed:1;                                         // This var will be set when bot ends performing USE action. This will allow bot to not to do USE action again.
	bool m_bUsingHealthMachine:1;                                  // if m_bNeedUse and this variable is true then we are at health machine (else it is armor machine).

	bool m_bNeedAttack:1;                                          // Will press attack button at m_fStartActionTime until m_fEndActionTime.
	bool m_bNeedAttack2:1;                                         // Will press attack button 2 at m_fStartActionTime until m_fEndActionTime.
	bool m_bNeedJump:1;                                            // Will jump once at m_fStartActionTime.
	bool m_bNeedJumpDuck:1;                                        // Will start ducking at m_fStartActionTime and hold duck until m_fEndActionTime.
	
	bool m_bConditionSatisfied;                                    // True if action condition has been satisfied.

	bool m_bDontBreakObjects:1;                                    // Set to true to not to break nearby objects.
	bool m_bDontThrowObjects:1;                                    // Set to true to not to throw nearby objects.
	bool m_bDontAttack:1;                                          // Set to true to ignore enemy (for example if defusing the bomb).

	bool m_bUnderAttack:1;                                         // True if engaging enemy.
	bool m_bCloseAttack:1;                                         // True if need to stop, while attacking enemy.
	bool m_bFlee:1;                                                // True, if currenly running away from enemy.
	bool m_bAttackDuck:1;                                          // True if need to duck, while attacking enemy.
	bool m_bNeedSetWeapon:1;                                       // Need to set best weapon.
	bool m_bLockAll:1;                                             // Mod can lock aim/move/attack, to do instead whatever bot is doing (like defusing bomb in CSS).
	bool m_bEnemyAimed:1;                                          // If true, then enemy is already aimed.
	bool m_bStayReloading:1;                                       // If true, then don't change weapon at reload time, but reload current weapon instead.
	bool m_bShootAtHead:1;                                         // If true, then will shoot at head instead of body.

	bool m_bObjectiveChanged:1;                                    // If true, then objective was changed, recalculate next move.

	bool m_bTalkStarted:1;                                         // Conversation started.
	bool m_bHelpingMate:1;                                         // Helping teammate?
	bool m_bPerformingRequest:1;                                   // Currently performing chat request.
	bool m_bRequestTimeout:1;                                      // If true then end performing chat request after timeout.

protected: // Members.

	static float m_fTimeIntervalCheckUsingMachines;                // After this interval will check if health/armor is incrementing (when using health/armor machine).
	static int m_iCheckEntitiesPerFrame;                           // How much near items check per frame (to know if bot is stucked with objects or picked up item).

	IBotController* m_pController;                                 // Bot controller (used to apply bot's command).
	CBotCmd m_cCmd;                                                // Bot's command (virtual keyboard, used to move bot around and fire weapons).

	TBotIntelligence m_iIntelligence;                              // Bot's intelligence.

	float m_fPrevThinkTime;                                        // Previous think time (used to get time difference between this and previous frame).

	Vector m_vDestination;                                         // Vector, where bot tries to move to.
	//Vector m_vLastVelocity;                                       // Vector of velocity in previous frame.
	Vector m_vLook;                                                // Point where bot tries to aim to.

	//QAngle m_angLook;                                              // Angles for look.

	Vector m_vForward;                                             // Bot will look at this forward vector most time it is moving.
	                                                               // It is set to next waypoint origin in path when bot becomes closer to destination waypoint.
	TWaypointId m_iDestinationWaypoint;                            // Set this to waypoint you want to move to, also m_bDestinationChanged to true.
	TWaypointId m_iAfterNextWaypoint;                              // This is a next waypoint in path after iNextWaypoint. Used to look at, when iNextWaypoint is close.

	float m_fEndAimTime;                                           // Time bot should end "moving mouse" to aim at m_vLook.
	float m_fStartActionTime;                                      // Time when bot jumps / breaks / uses (check m_bNeedJump, m_bNeedAttack, m_bNeedUse).
	float m_fEndActionTime;                                        // Time to release duck button, which was pressed when bot jumps.

	float m_fStuckCheckTime;                                       // Time when bot got stucked.
	Vector m_vStuckCheck;                                          // Position of player at m_fStuckCheckTime.
	Vector m_vDisturbingObjectPosition;                            // Position of disturbing object in previous frame.

	int m_iLastHealthArmor;                                        // Amount of health/armor bot had at m_fStartActionTime.

	unsigned char r, g, b;                                         // Bot's path color.

	CWaypointNavigator m_pNavigator;                               // Waypoint navigator.
	good::vector<TAreaId> m_aAvoidAreas;                           // Array of areas waypoint navigator must avoid.

	good::vector<TEntityIndex> m_aNearestItems[EEntityTypeTotal];  // Nearest items from m_aNearItems that are checked every frame (to know if bot picked them up).
	good::vector<TEntityIndex> m_aNearItems[EEntityTypeTotal];     // Items in close range.
	int m_iNextNearItem[EEntityTypeTotal];                         // Next item to check if close (index in array CItems::GetItems()).

	good::bitset m_aNearPlayers;                                   // Bitset of players near (to know if bot can stuck with them).
	good::bitset m_aSeenEnemies;                                   // Bitset of enemies that bot can see right now.
	good::bitset m_aEnemies;                                       // Bitset of enemies that bot can't see right now, but it knows they are there.
	int m_iNextCheckPlayer;                                        // Next player to check if close.

	CPlayer* m_pCurrentEnemy;                                      // Current enemy.
	float m_fDistanceSqrToEnemy;                                   // If m_pCurrentEnemy is not NULL, squared distance to it.
	
	int m_iCurrentPickedItem;                                      // Index in m_aPickedItems to check next frame.
	good::vector<CPickedItem> m_aPickedItems;                      // Picked items (like health or weapon), for bot to know which items are available to pick on map.

	good::vector<CWeaponWithAmmo> m_aWeapons;                      // Weapons that bot actually has.
	TWeaponId m_iWeapon, m_iBestWeapon;                            // Current / previous / best weapon.
	TWeaponId m_iPhyscannon, m_iManualWeapon;                      // Index of gravity gun / manual gun, -1 if bot doesn't have it.

	float m_fNextDrawNearObjectsTime;                              // Next time to draw near objects.

	TBotChat m_iObjective, m_iPrevRequest;                         // Current and last chat request.
	TBotChat m_iPrevTalk;                                          // Last chat talk.
	float m_fEndTalkActionTime;                                    // Time for bot to stop doing what other player asked (30 secs).

	CBotChat m_cChat;                                              // Last spoken phrase.
	TPlayerIndex m_iPrevChatMate;                                  // Previous chat mate.
};


#endif // __BOTRIX_BOT_H__

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CHuntEnemySchedule )
    bool carefulApproach = GetDecision()->ShouldMustBeCareful();

    // We run towards the target until we reach a distance of between 700 and 900 units
    ADD_TASK( BTASK_RUN, NULL )
    ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat(700.0, 900.0f) )

    // We must be careful!
    if ( carefulApproach ) {
        // We walked slowly until reaching a short distance and 
        // we waited a little in case the target leaves its coverage.
        ADD_TASK( BTASK_SNEAK, NULL )
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( 400.0f, 500.0f ) )
        ADD_TASK( BTASK_WAIT, RandomFloat( 0.5f, 3.5f ) )
    }
    
    ADD_TASK( BTASK_HUNT_ENEMY, NULL )

    // Interrupts
    ADD_INTERRUPT( BCOND_EMPTY_CLIP1_AMMO )
    ADD_INTERRUPT( BCOND_ENEMY_DEAD )
    ADD_INTERRUPT( BCOND_ENEMY_UNREACHABLE )

    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
	ADD_INTERRUPT( BCOND_HELPLESS )

    ADD_INTERRUPT( BCOND_TOO_CLOSE_TO_ATTACK )
    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire() const
{
    if ( !GetMemory() || !GetLocomotion() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanHuntThreat() )
        return BOT_DESIRE_NONE;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    if ( !memory )
        return BOT_DESIRE_NONE;

    float distance = GetMemory()->GetPrimaryThreatDistance();
    float tolerance = GetLocomotion()->GetTolerance();

    if ( distance <= tolerance )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_ENEMY_LOST ) || HasCondition( BCOND_ENEMY_OCCLUDED ) )
        return 0.65;

    if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) )
        return 0.38f;

    return BOT_DESIRE_NONE;
}
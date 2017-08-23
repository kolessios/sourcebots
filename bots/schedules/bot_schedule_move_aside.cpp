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
BEGIN_SETUP_SCHEDULE( CMoveAsideSchedule )
    ADD_TASK( BTASK_GET_SPOT_ASIDE,   NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )

    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire() const
{
    if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( GetLocomotion()->HasDestination() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->IsCombating() || GetBot()->IsAlerted() ) {
        if ( HasCondition( BCOND_LIGHT_DAMAGE ) )
            return 0.70f;
    }

    //if ( HasCondition( BCOND_ENEMY_OCCLUDED_BY_FRIEND ) )
        //return 0.91f;

    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && !GetBot()->IsCombating() ) {
        if ( HasCondition( BCOND_ENEMY_LOST ) || HasCondition( BCOND_HEAR_COMBAT ) )
            return 0.14f;
    }

    if ( GetBot()->IsIdle() && m_nMoveAsideTimer.IsElapsed() )
        return 0.13f;

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::Start()
{
	BaseClass::Start();
	m_nMoveAsideTimer.Start( RandomInt(5, 20) );
}
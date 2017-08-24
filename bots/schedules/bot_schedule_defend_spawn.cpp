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
BEGIN_SETUP_SCHEDULE( CDefendSpawnSchedule )
    ADD_TASK( BTASK_GET_SPAWN, NULL )
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL )

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "SpawnPosition" );

    if ( !memory )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !GetBot()->IsIdle() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetTacticalMode() != TACTICAL_MODE_DEFENSIVE )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetFollow() && GetBot()->GetFollow()->IsFollowingActive() )
        return BOT_DESIRE_NONE;

    float distance = GetHost()->GetAbsOrigin().DistTo( memory->GetVector() );

    if ( distance < 200.0f )
        return BOT_DESIRE_NONE;

    return 0.05f;
}
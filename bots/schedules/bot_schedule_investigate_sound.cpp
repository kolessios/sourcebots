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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CInvestigateSoundSchedule )
    if ( HasCondition( BCOND_HEAR_SPOOKY ) ) {
        ADD_TASK( BTASK_RUN, NULL )
    }
    else {
        ADD_TASK( BTASK_SNEAK, NULL )
    }

	ADD_TASK( BTASK_SAVE_POSITION,	  NULL )
    ADD_TASK( BTASK_MOVE_DESTINATION, m_vecSoundPosition )
	ADD_TASK( BTASK_WAIT,             RandomFloat(3.0f, 6.0f) )
	ADD_TASK( BTASK_RESTORE_POSITION, NULL )

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire() const
{
    if ( !GetDecision()->ShouldInvestigateSound() )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_HEAR_COMBAT ) || HasCondition( BCOND_HEAR_ENEMY ) )
        return 0.2f;

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskStart()
{
    switch ( GetActiveTask()->task ) {
        case BTASK_SAVE_POSITION:
        {
            CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER );

            if ( !pSound ) {
                Fail( "Invalid Sound" );
                return;
            }

            m_vecSoundPosition = pSound->GetSoundReactOrigin();

            BaseClass::TaskStart();
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}
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
BEGIN_SETUP_SCHEDULE( CHideAndReloadSchedule )
    ADD_TASK( BTASK_SAVE_POSITION,	  NULL )
	ADD_TASK( BTASK_RUN,	          NULL )
	ADD_TASK( BTASK_RELOAD,	          true )
    ADD_TASK( BTASK_GET_COVER,		  NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )
	ADD_TASK( BTASK_CROUCH,			  NULL )
    ADD_TASK( BTASK_WAIT,			  RandomFloat(0.5f, 2.5f) )
	ADD_TASK( BTASK_RESTORE_POSITION, NULL )

    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO )
    ADD_INTERRUPT( BCOND_ENEMY_DEAD )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire() const
{
    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->ShouldCover() )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_EMPTY_PRIMARY_AMMO ) ) {
        if ( GetBot()->IsCombating() || GetBot()->IsAlerted() )
            return 0.92f;
    }

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_MOVE_DESTINATION:
        case BTASK_CROUCH:
        case BTASK_WAIT:
        {
            // Si no somos noob, entonces cancelamos estas tareas
            // en cuanto tengamos el arma recargada.
            if ( !GetProfile()->IsEasy() ) {
                CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

                // Sin arma
                if ( !pWeapon ) {
                    TaskComplete();
                    return;
                }

                // Ya hemos terminado de recargar
#ifdef INSOURCE_DLL
                if ( !pWeapon->IsReloading() || !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
#else
                if ( !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
#endif
                    TaskComplete();
                    return;
                }
            }

            BaseClass::TaskRun();
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}
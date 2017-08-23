//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"

#include "bots\bot.h"
#include "bots\interfaces\ibotschedule.h"

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
void IBotSchedule::Reset()
{
    BaseClass::Reset();

    m_nActiveTask = NULL;
    m_bFailed = false;
    m_bStarted = false;
    m_bFinished = false;
    m_vecSavedLocation.Invalidate();
    m_vecLocation.Invalidate();
}

//================================================================================
//================================================================================
void IBotSchedule::Start()
{
    Assert( m_Tasks.Count() == 0 );

    Reset();

    m_bStarted = true;
    m_StartTimer.Start();

    if ( GetLocomotion() ) {
        GetLocomotion()->StopDrive();
        GetLocomotion()->Walk();
        GetLocomotion()->StandUp();
    }

    Setup();
}

//================================================================================
//================================================================================
void IBotSchedule::Finish()
{
    // Marcamos como terminado
    m_bFinished = true;
    m_bStarted = false;

    m_bFailed = false;
    m_nActiveTask = NULL;
    m_flLastDesire = BOT_DESIRE_NONE;
    m_StartTimer.Invalidate();

    // Limpiamos todo
    m_Tasks.Purge();
    m_Interrupts.Purge();

    GetBot()->DebugAddMessage( "Scheduled %s Finished", g_BotSchedules[GetID()] );

    if ( GetLocomotion() ) {
        GetLocomotion()->StopDrive();
        GetLocomotion()->Walk();
        GetLocomotion()->StandUp();
    }
}

//================================================================================
//================================================================================
void IBotSchedule::Fail( const char *pWhy )
{
	m_bFailed = true;
    GetBot()->DebugAddMessage("Scheduled %s Failed: %s", g_BotSchedules[GetID()], pWhy);
}

//================================================================================
//================================================================================
bool IBotSchedule::ShouldInterrupted() const
{
    if ( HasFinished() )
        return true;

    if ( HasFailed() )
        return true;

    if ( !GetHost()->IsAlive() )
        return true;

    return false;
}

//================================================================================
//================================================================================
float IBotSchedule::GetInternalDesire()
{
    if ( HasStarted() ) {
        if ( ShouldInterrupted() )
            return BOT_DESIRE_NONE;

        if ( ItsImportant() )
            return m_flLastDesire;
    }

	m_flLastDesire = GetDesire();
	return m_flLastDesire;
}

//================================================================================
//================================================================================
void IBotSchedule::Update()
{
    VPROF_BUDGET( "IBotSchedule::Update", VPROF_BUDGETGROUP_BOTS );

    FOR_EACH_VEC( m_Interrupts, it )
    {
        BCOND condition = m_Interrupts.Element( it );

        if ( GetBot()->HasCondition( condition ) ) {
            Fail( UTIL_VarArgs( "Interrupted by Condition (%s)", g_Conditions[condition] ) );
            return;
        }
    }

    Assert( m_Tasks.Count() > 0 );
    BotTaskInfo_t *idealTask = m_Tasks.Element( 0 );

    if ( idealTask != m_nActiveTask ) {
        m_nActiveTask = idealTask;
        TaskStart();
        return;
    }

    TaskRun();
}

//================================================================================
//================================================================================
void IBotSchedule::Wait( float seconds )
{
    m_WaitTimer.Start( seconds );
    GetBot()->DebugAddMessage("Task Wait for %.2fs", seconds);
}

//================================================================================
//================================================================================
const char *IBotSchedule::GetActiveTaskName() const
{
    BotTaskInfo_t *info = GetActiveTask();

    if ( !info ) {
        return "UNKNOWN";
    }

    if ( info->task >= BLAST_TASK ) {
        return UTIL_VarArgs( "CUSTOM: %i", info->task );
    }

    return g_BotTasks[ info->task ];
}

//================================================================================
//================================================================================
void IBotSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    if ( GetBot()->TaskStart( pTask ) ) {
        return;
    }

    switch ( pTask->task ) {
        case BTASK_WAIT:
        {
            Wait( pTask->flValue );
            break;
        }

        case BTASK_SET_TOLERANCE:
        {
            if ( GetLocomotion() ) {
                GetLocomotion()->SetTolerance( pTask->flValue );
            }

            TaskComplete();
            break;
        }

        case BTASK_PLAY_ANIMATION:
        {
            Activity activity = (Activity)pTask->iValue;
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, activity );
#else
            Assert( !"Implement in your mod" );
#endif
            break;
        }

        case BTASK_PLAY_GESTURE:
        {
            Activity activity = (Activity)pTask->iValue;
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, activity );
#else
            Assert( !"Implement in your mod" );
#endif
            TaskComplete();
            break;
        }

        case BTASK_PLAY_SEQUENCE:
        {
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, pTask->iValue );
#else
            Assert( !"Implement in your mod" );
#endif
            break;
        }

        case BTASK_SAVE_POSITION:
        {
            m_vecSavedLocation = GetAbsOrigin();
            TaskComplete();
            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            Assert( m_vecSavedLocation.IsValid() );

            if ( GetFollow() && GetFollow()->IsFollowingActive() ) {
                TaskComplete();
                return;
            }

            break;
        }

        case BTASK_MOVE_DESTINATION:
        {
            break;
        }

        case BTASK_MOVE_SPAWN:
        {
            // TODO
            //m_vecLocation = GetBot()->m_vecSpawnSpot;
            break;
        }

        case BTASK_MOVE_LIVE_DESTINATION:
        {
            // TODO
            break;
        }

        case BTASK_HUNT_ENEMY:
        {
            if ( !GetMemory() ) {
                Fail( "Bot without Memory" );
                return;
            }

            if ( !GetBot()->GetEnemy() ) {
                Fail( "Hunt Enemy: Without enemy" );
                return;
            }

            CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

            if ( !memory ) {
                Fail( "Without enemy memory" );
                return;
            }

            break;
        }

        case BTASK_GET_SPOT_ASIDE:
        {
            if ( !GetHost()->GetLastKnownArea() ) {
                Fail( "Bot without last known area" );
                return;
            }

            m_vecLocation.Invalidate();
            int attempts = 0;

            while ( !m_vecLocation.IsValid() && attempts < 30 ) {
                ++attempts;
                m_vecLocation = GetHost()->GetLastKnownArea()->GetRandomPoint();

                if ( GetAbsOrigin().DistTo( m_vecLocation ) > 250.0f ) {
                    m_vecLocation.Invalidate();
                }
            }

            if ( !m_vecLocation.IsValid() ) {
                Fail( "Could not get a valid near spot." );
                return;
            }

            TaskComplete();
            break;
        }

        case BTASK_GET_COVER:
        {
            if ( GetDecision()->IsInCoverPosition() )
            {
                TaskComplete();
                return;
            }

            float radius = pTask->flValue;

            if ( radius <= 0 ) {
                radius = GET_COVER_RADIUS;
            }

            if ( !GetDecision()->GetNearestCover( radius, &m_vecLocation ) ) {
                Fail( "No safe place found" );
                return;
            }

            TaskComplete();
            break;
        }

        case BTASK_GET_FAR_COVER:
        {
            float minRadius = pTask->flValue;

            if ( minRadius <= 0 ) {
                minRadius = (GET_COVER_RADIUS * 2);
             }

            float maxRadius = (minRadius * 3);

            CSpotCriteria criteria;
            criteria.SetMaxRange( maxRadius );
            criteria.SetMinDistanceAvoid( minRadius );
            criteria.UseNearest( false );
            criteria.UseRandom( true );
            criteria.OutOfVisibility( true );
            criteria.AvoidTeam( GetBot()->GetEnemy() );

            if ( !Utils::FindCoverPosition( &m_vecLocation, GetHost(), criteria ) ) {
                Fail( "No far safe place found" );
                return;
            }

            TaskComplete();
            break;
        }

        case BTASK_AIM:
        {
            break;
        }

        case BTASK_USE:
        {
            InjectButton( IN_USE );
            TaskComplete();
            break;
        }

        case BTASK_JUMP:
        {
            if ( GetLocomotion() ) {
                GetLocomotion()->Jump();
            }
            else {
                InjectButton( IN_JUMP );
            }

            TaskComplete();
            break;
        }

        case BTASK_CROUCH:
        {
            if ( !GetLocomotion() ) {
                Fail("Without Locomotion");
                return;
            }

            GetLocomotion()->Crouch();
            TaskComplete();
            break;
        }

        case BTASK_STANDUP:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->StandUp();
            TaskComplete();
            break;
        }

        case BTASK_RUN:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Run();
            TaskComplete();
            break;
        }

        case BTASK_SNEAK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Sneak();
            TaskComplete();
            break;
        }

        case BTASK_WALK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Walk();
            TaskComplete();
            break;
        }

        case BTASK_RELOAD:
        {
            InjectButton( IN_RELOAD );
            break;
        }

        case BTASK_RELOAD_SAFE:
        {
            bool reload = true;

            if ( GetBot()->GetEnemy() && GetSkill()->GetLevel() >= SKILL_HARD ) {
                if ( GetMemory()->GetPrimaryThreat()->GetDistance() <= 500.0f ) {
                    reload = false;
                }
            }

            if ( reload ) {
                InjectButton( IN_RELOAD );
            }
            else {
                TaskComplete();
            }

            break;
        }

        case BTASK_RELOAD_ASYNC:
        {
            InjectButton( IN_RELOAD );
            TaskComplete();
            break;
        }

        // Curarse
        // TODO
        case BTASK_HEAL:
        {
            GetHost()->TakeHealth( 30.0f, DMG_GENERIC );
            TaskComplete();
            break;
        }

        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        default:
        {
            Assert( !"Task Start not handled!" );
            break;
        }
    }
}

//================================================================================
// Ejecución de una tarea hasta terminarla
//================================================================================
void IBotSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    if ( GetBot()->TaskRun( pTask ) )
        return;

    switch ( pTask->task ) {
        case BTASK_WAIT:
        {
            if ( GetMemory() )
                GetMemory()->MaintainThreat();

            if ( IsWaitFinished() )
                TaskComplete();

            break;
        }

        case BTASK_PLAY_ANIMATION:
        case BTASK_PLAY_SEQUENCE:
        {
            if ( GetMemory() )
                GetMemory()->MaintainThreat();

            if ( GetHost()->IsActivityFinished() )
                TaskComplete();

            break;
        }

        case BTASK_SAVE_POSITION:
        {
            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            if ( GetMemory() )
                GetMemory()->MaintainThreat();

            if ( HasCondition( BCOND_SEE_HATE ) || HasCondition( BCOND_SEE_ENEMY ) ) {
                TaskComplete();
                return;
            }

            float distance = GetAbsOrigin().DistTo( m_vecSavedLocation );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Restoring Position", m_vecSavedLocation, PRIORITY_NORMAL );
            break;
        }

        case BTASK_MOVE_DESTINATION:
        case BTASK_MOVE_SPAWN:
        {
            CBaseEntity *pTarget = pTask->pszValue.Get();

            if ( pTarget ) {
                if ( GetMemory() ) {
                    CEntityMemory *memory = GetMemory()->GetEntityMemory( pTarget );

                    if ( memory ) {
                        m_vecLocation = memory->GetLastKnownPosition();
                    }
                    else {
                        m_vecLocation = pTarget->GetAbsOrigin();
                    }
                }
                else {
                    m_vecLocation = pTarget->GetAbsOrigin();
                }
            }
            else if ( pTask->vecValue.IsValid() ) {
                m_vecLocation = pTask->vecValue;
            }

            if ( !m_vecLocation.IsValid() ) {
                Fail( "Destination is invalid" );
                return;
            }

            float distance = GetAbsOrigin().DistTo( m_vecLocation );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Moving Destination", m_vecLocation, PRIORITY_HIGH );
            break;
        }

        case BTASK_HUNT_ENEMY:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            if ( !GetMemory() ) {
                Fail( "Without Memory" );
                return;
            }

            CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

            if ( !memory ) {
                Fail( "Without Enemy Memory" );
                return;
            }

            memory->Maintain();

            float distance = memory->GetDistance();
            float tolerance = pTask->flValue;

            if ( tolerance < 1.0f ) {
                tolerance = GetLocomotion()->GetTolerance();
            }

            // Queremos cazar a nuestro enemigo porque nuestra arma actual
            // no tiene rango para poder atacarlo...
            if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) ) {
                CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

                // Tenemos un arma
                if ( pWeapon ) {
#ifdef INSOURCE_DLL
                    float maxWeaponDistance = pWeapon->GetWeaponInfo().m_flIdealDistance;
#else
                    // How to know the maximum distance of a bullet?
                    float maxWeaponDistance = 3000.0f;
#endif

                    // Nos detendremos en cuanto tengamos rango para disparar
                    if ( pWeapon->IsMeleeWeapon() ) {
                        tolerance = maxWeaponDistance;
                    }
                    else {
                        tolerance = (maxWeaponDistance - 100.0f);
                    }
                }
            }

            bool completed = (distance <= tolerance);

            // Nuestra arma ya puede atacar a nuestro enemigo
            // nos detenemos en cuanto lo veamos!
            if ( !HasCondition( BCOND_TOO_FAR_TO_ATTACK ) && !completed )
                completed = HasCondition( BCOND_SEE_ENEMY );

            // Hemos llegado
            if ( completed ) {
                TaskComplete();
                return;
            }

            if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && memory->GetInformer() ) {
                GetLocomotion()->Approach( memory->GetInformer(), tolerance, PRIORITY_HIGH );
            }
            else {
                GetLocomotion()->Approach( memory->GetEntity(), tolerance, PRIORITY_HIGH );
            }


            break;
        }

        case BTASK_GET_COVER:
        case BTASK_GET_FAR_COVER:
        {
            break;
        }

        case BTASK_AIM:
        {
            if ( !GetVision() ) {
                Fail( "Without Vision" );
                return;
            }

            if ( pTask->pszValue.Get() ) {
                GetVision()->LookAt( "TASK_AIM", pTask->pszValue.Get(), PRIORITY_CRITICAL, 1.0f );
            }
            else {
                if ( !pTask->vecValue.IsValid() ) {
                    Fail( "Aim Goal Invalid" );
                    return;
                }

                GetVision()->LookAt( "TASK_AIM", pTask->vecValue, PRIORITY_CRITICAL, 1.0f );
            }

            if ( GetVision()->IsAimReady() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_RELOAD:
        case BTASK_RELOAD_SAFE:
        {
            if ( GetMemory() )
                GetMemory()->MaintainThreat();

            CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

            if ( !pWeapon ) {
                TaskComplete();
                return;
            }

#ifdef INSOURCE_DLL
            if ( !pWeapon->IsReloading() || (!HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO )) ) {
#else
            if ( !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO ) ) {
#endif
                TaskComplete();
                return;
            }

            break;
        }

        case BTASK_RELOAD_ASYNC:
        {
            break;
        }

        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        default:
        {
            Assert( !"Task Run not handled!" );
            break;
        }
    }
}

//================================================================================
// Marca una tarea como completada
//================================================================================
void IBotSchedule::TaskComplete()
{
    if ( GetLocomotion()->HasDestination() ) {
        GetLocomotion()->StopDrive();
    }

    if ( m_Tasks.Count() == 0 ) {
        Assert( !"m_Tasks.Count() == 0" );
        return;
    }

    m_Tasks.Remove( 0 );

    if ( m_Tasks.Count() == 0 ) {
        m_bFinished = true;
    }
}
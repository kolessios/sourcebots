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
    //m_vecSavedLocation.Invalidate();
    //m_vecLocation.Invalidate();
}

//================================================================================
//================================================================================
void IBotSchedule::Start()
{
    Assert( m_Tasks.Count() == 0 );

    Reset();

    m_bStarted = true;
    m_StartTimer.Start();
    m_Interrupts.Purge();

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
    //m_Interrupts.Purge();

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
    else {
        FOR_EACH_VEC( m_Interrupts, it )
        {
            BCOND condition = m_Interrupts.Element( it );

            if ( GetBot()->HasCondition( condition ) )
                return BOT_DESIRE_NONE;
        }
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
bool IBotSchedule::SavePosition( const Vector &position, float duration )
{
    Assert( GetMemory() );

    if ( !GetMemory() ) {
        Fail( "SavePosition() without memory." );
        return false;
    }

    GetMemory()->UpdateDataMemory( "SavedPosition", position, duration );
    return true;
}

//================================================================================
//================================================================================
const Vector &IBotSchedule::GetSavedPosition()
{
    Assert( GetMemory() );

    if ( !GetMemory() ) {
        Fail( "GetSavedPosition() without memory." );
        return vec3_invalid;
    }

    return GetDataMemoryVector( "SavedPosition" );
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
        case BTASK_MOVE_DESTINATION:
        case BTASK_AIM:
        case BTASK_CALL_FOR_BACKUP:
        case BTASK_HUNT_ENEMY:
        {
            // We place them to not generate an assert
            break;
        }

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
            SavePosition( GetAbsOrigin() );
            TaskComplete();
            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            Assert( GetSavedPosition().IsValid() );

            if ( GetFollow() && GetFollow()->IsFollowingActive() ) {
                TaskComplete();
                return;
            }

            break;
        }

        case BTASK_GET_SPAWN:
        {
            Assert( GetMemory() );

            if ( !GetMemory() ) {
                Fail( "Get Spawn: Without memory." );
                return;
            }

            SavePosition( GetDataMemoryVector( "SpawnPosition" ) );
            break;
        }

        case BTASK_MOVE_LIVE_DESTINATION:
        {
            // TODO
            break;
        }

        case BTASK_GET_SPOT_ASIDE:
        {
            CNavArea *pArea = GetHost()->GetLastKnownArea();

            if ( pArea == NULL ) {
                pArea = TheNavMesh->GetNearestNavArea( GetHost() );
            }

            if ( pArea == NULL ) {
                Fail( "Get Spot Aside: Bot without last known Area." );
            }

            SavePosition( pArea->GetRandomPoint(), 5.0f );
            TaskComplete();

            break;
        }

        case BTASK_GET_COVER:
        {
            if ( GetDecision()->IsInCoverPosition() ) {
                TaskComplete();
                TaskComplete();
                return;
            }

            Vector vecPosition;
            float radius = pTask->flValue;

            if ( radius <= 0 ) {
                radius = GET_COVER_RADIUS;
            }

            if ( !GetDecision()->GetNearestCover( radius, &vecPosition ) ) {
                Fail( "Get Cover: No cover spot found" );
                return;
            }

            Assert( vecPosition.IsValid() );
            SavePosition( vecPosition );

            TaskComplete();
            break;
        }

        case BTASK_GET_FAR_COVER:
        {
            Vector vecPosition;
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

            if ( !Utils::FindCoverPosition( &vecPosition, GetHost(), criteria ) ) {
                Fail( "Get Cover: No far cover spot found" );
                return;
            }

            Assert( vecPosition.IsValid() );
            SavePosition( vecPosition );

            TaskComplete();
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
                Fail( "Crouch: Without Locomotion" );
                return;
            }

            GetLocomotion()->Crouch();
            TaskComplete();
            break;
        }

        case BTASK_STANDUP:
        {
            if ( !GetLocomotion() ) {
                Fail( "Standup: Without Locomotion" );
                return;
            }

            GetLocomotion()->StandUp();
            TaskComplete();
            break;
        }

        case BTASK_RUN:
        {
            if ( !GetLocomotion() ) {
                Fail( "Run: Without Locomotion" );
                return;
            }

            GetLocomotion()->Run();
            TaskComplete();
            break;
        }

        case BTASK_SNEAK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Sneak: Without Locomotion" );
                return;
            }

            GetLocomotion()->Sneak();
            TaskComplete();
            break;
        }

        case BTASK_WALK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Walk: Without Locomotion" );
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

            if ( GetBot()->GetEnemy() ) {
                CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

                if ( memory->GetDistance() <= 600.0f ) {
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

        // TODO: This needs to be replaced by a custom Bot
        case BTASK_HEAL:
        {
            GetHost()->TakeHealth( 30.0f, DMG_GENERIC );
            TaskComplete();
            break;
        }

        default:
        {
            Assert( !"TaskStart(): Task not handled!" );
            break;
        }
    }
}

//================================================================================
//================================================================================
void IBotSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    if ( GetBot()->TaskRun( pTask ) )
        return;

    switch ( pTask->task ) {
        case BTASK_SAVE_POSITION:
        case BTASK_GET_COVER:
        case BTASK_GET_FAR_COVER:
        case BTASK_GET_SPAWN:
        case BTASK_RELOAD_ASYNC:
        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        case BTASK_WAIT:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( IsWaitFinished() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_PLAY_ANIMATION:
        case BTASK_PLAY_SEQUENCE:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( GetHost()->IsActivityFinished() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( HasCondition( BCOND_SEE_HATE ) || HasCondition( BCOND_SEE_ENEMY ) ) {
                TaskComplete();
                return;
            }

            Vector vecPosition = GetSavedPosition();

            float distance = GetAbsOrigin().DistTo( vecPosition );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Restoring Position", vecPosition, PRIORITY_NORMAL );
            break;
        }

        case BTASK_MOVE_DESTINATION:
        {
            Vector vecPosition = GetSavedPosition();
            CBaseEntity *pTarget = pTask->pszValue.Get();

            if ( pTarget ) {
                if ( GetMemory() ) {
                    CEntityMemory *memory = GetMemory()->GetEntityMemory( pTarget );

                    if ( memory ) {
                        vecPosition = memory->GetLastKnownPosition();
                    }
                    else {
                        vecPosition = pTarget->GetAbsOrigin();
                    }
                }
                else {
                    vecPosition = pTarget->GetAbsOrigin();
                }
            }
            else if ( pTask->vecValue.IsValid() ) {
                vecPosition = pTask->vecValue;
            }

            Assert( vecPosition.IsValid() );

            if ( !vecPosition.IsValid() ) {
                Fail( "Move Destination: Invalid!" );
                return;
            }

            float distance = GetAbsOrigin().DistTo( vecPosition );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Moving Destination", vecPosition, PRIORITY_HIGH );
            break;
        }

        case BTASK_HUNT_ENEMY:
        {
            if ( !GetLocomotion() ) {
                Fail( "Hunt Enemy: Without Locomotion" );
                return;
            }

            if ( !GetMemory() ) {
                Fail( "Hunt Enemy: Without Memory" );
                return;
            }

            CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

            if ( !memory ) {
                Fail( "Hunt Enemy: Without Enemy Memory" );
                return;
            }

            // We prevent memory from expiring
            memory->Maintain();

            float distance = memory->GetDistance();
            float tolerance = pTask->flValue;

            if ( tolerance < 1.0f ) {
                tolerance = GetLocomotion()->GetTolerance();
            }

            // We are approaching our enemy because our current weapon does not have enough range.
            if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) ) {
                CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

                if ( pWeapon ) {
                    float range = GetDecision()->GetWeaponIdealRange( pWeapon );

                    if ( pWeapon->IsMeleeWeapon() ) {
                        tolerance = range;
                    }
                    else {
                        tolerance = (range - 100.0f); // Safe
                    }
                }
            }

            bool completed = (distance <= tolerance);

            // We have range to attack, we stop as soon as we have a clear vision of the enemy.
            if ( !HasCondition( BCOND_TOO_FAR_TO_ATTACK ) && !completed ) {
                completed = HasCondition( BCOND_SEE_ENEMY ) && !HasCondition( BCOND_ENEMY_OCCLUDED );
            }

            if ( completed ) {
                TaskComplete();
                return;
            }

            if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && memory->GetInformer() ) {
                GetLocomotion()->DriveTo( "Hunt Threat - Informer", memory->GetInformer(), tolerance, PRIORITY_HIGH );
            }
            else {
                GetLocomotion()->DriveTo( "Hunt Threat", memory->GetEntity(), tolerance, PRIORITY_HIGH );
            }

            break;
        }

        case BTASK_AIM:
        {
            if ( !GetVision() ) {
                Fail( "Aim: Without Vision" );
                return;
            }

            if ( pTask->pszValue.Get() ) {
                GetVision()->LookAt( "Schedule Aim", pTask->pszValue.Get(), PRIORITY_CRITICAL, 1.0f );
            }
            else {
                if ( !pTask->vecValue.IsValid() ) {
                    Fail( "Aim: Invalid goal" );
                    return;
                }

                GetVision()->LookAt( "Schedule Aim", pTask->vecValue, PRIORITY_CRITICAL, 1.0f );
            }

            if ( GetVision()->IsAimReady() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_RELOAD:
        case BTASK_RELOAD_SAFE:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

            if ( pWeapon == NULL ) {
                TaskComplete();
                return;
            }

#ifdef INSOURCE_DLL
            if ( !pWeapon->IsReloading() || (!HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO )) ) {
#else
            if ( !pWeapon->m_bInReload || (!HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO )) ) {
#endif
                TaskComplete();
                return;
            }

            break;
        }

        default:
        {
            Assert( !"TaskRun(): Task not handled!" );
            break;
        }
    }
}

//================================================================================
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
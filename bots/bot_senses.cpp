//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"

#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#include "basecombatweapon.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_optimize;
extern ConVar bot_far_distance;

//================================================================================
// It is called when we already have information about Bot vision.
//================================================================================
void CBot::OnLooked( int iDistance )
{
    VPROF_BUDGET( "OnLooked", VPROF_BUDGETGROUP_BOTS );

    ClearCondition( BCOND_SEE_FRIEND );
    ClearCondition( BCOND_SEE_HATE );
    ClearCondition( BCOND_SEE_FEAR );
    ClearCondition( BCOND_SEE_DISLIKE );
    ClearCondition( BCOND_BETTER_WEAPON_AVAILABLE );
    ClearCondition( BCOND_SEE_DEJECTED_FRIEND );

    AISightIter_t iter;
    CBaseEntity *pSightEnt = GetSenses()->GetFirstSeenEntity( &iter );

	if ( !pSightEnt )
		return;

    // TODO: This "optimization" works?
	int limit = 25;

    if ( ShouldOptimize() ) {
        limit = 5;
    }

    while ( pSightEnt ) {
        OnLooked( pSightEnt );

        pSightEnt = GetSenses()->GetNextSeenEntity( &iter );
        --limit;

        if ( limit <= 0 )
            break;
    }
}

//================================================================================
// We can see the specified entity
//================================================================================
void CBot::OnLooked( CBaseEntity *pSightEnt )
{
    if ( !GetMemory() )
        return;

    CEntityMemory *memory = GetMemory()->UpdateEntityMemory( pSightEnt, pSightEnt->WorldSpaceCenter() );
    Assert( memory );

    if ( !memory )
        return;

    int relation = memory->GetRelationship();
    memory->UpdateVisibility( true );

    switch ( relation ) {
        case GR_ENEMY:
        case GR_NOTTEAMMATE:
        {
            int priority = GetHost()->IRelationPriority( pSightEnt );

            if ( priority < 0 ) {
                SetCondition( BCOND_SEE_DISLIKE );
            }
            else {
                SetCondition( BCOND_SEE_HATE );
            }

            break;
        }

        case GR_NEUTRAL:
        {
            // A weapon, maybe we can use it...
            if ( pSightEnt->IsBaseCombatWeapon() ) {
                CBaseWeapon *pWeapon = ToBaseWeapon( pSightEnt );
                Assert( pWeapon );

                if ( GetDecision()->ShouldGrabWeapon( pWeapon ) ) {
                    GetMemory()->UpdateDataMemory( "BestWeapon", pSightEnt, 30.0f );
                    SetCondition( BCOND_BETTER_WEAPON_AVAILABLE );
                }
            }

            break;
        }

        case GR_ALLY:
        case GR_TEAMMATE:
        {
            SetCondition( BCOND_SEE_FRIEND );

            if ( pSightEnt->IsPlayer() ) {
                CPlayer *pSightPlayer = ToInPlayer( pSightEnt );

                if ( GetDecision()->ShouldHelpDejectedFriend( pSightPlayer ) ) {
                    GetMemory()->UpdateDataMemory( "DejectedFriend", pSightEnt, 30.0f );
                    SetCondition( BCOND_SEE_DEJECTED_FRIEND );
                }
            }

            break;
        }
    }
}

//================================================================================
// It is called when we already have information about Bot hearing
//================================================================================
void CBot::OnListened()
{
    VPROF_BUDGET( "OnListened", VPROF_BUDGETGROUP_BOTS );

    ClearCondition( BCOND_HEAR_COMBAT );
    ClearCondition( BCOND_HEAR_WORLD );
    ClearCondition( BCOND_HEAR_ENEMY );
    ClearCondition( BCOND_HEAR_ENEMY_FOOTSTEP );
    ClearCondition( BCOND_HEAR_BULLET_IMPACT );
    ClearCondition( BCOND_HEAR_BULLET_IMPACT_SNIPER );
    ClearCondition( BCOND_HEAR_DANGER );
    ClearCondition( BCOND_HEAR_MOVE_AWAY );
    ClearCondition( BCOND_HEAR_SPOOKY );
    ClearCondition( BCOND_SMELL_MEAT );
    ClearCondition( BCOND_SMELL_CARCASS );
    ClearCondition( BCOND_SMELL_GARBAGE );

    AISoundIter_t iter;
    CSound *pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );

    while ( pCurrentSound ) {
        if ( pCurrentSound->IsSoundType( SOUND_DANGER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_BULLET_IMPACT ) ) {
                SetCondition( BCOND_HEAR_MOVE_AWAY );

                if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER ) ) {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT_SNIPER );
                }
                else {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT );
                }
            }

            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_EXPLOSION ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
                SetCondition( BCOND_HEAR_MOVE_AWAY );
            }

            SetCondition( BCOND_HEAR_DANGER );
        }

        if ( pCurrentSound->IsSoundType( SOUND_COMBAT ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
            }

            SetCondition( BCOND_HEAR_COMBAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_WORLD ) ) {
            SetCondition( BCOND_HEAR_WORLD );
        }

#ifdef INSOURCE_DLL
        if ( pCurrentSound->IsSoundType( SOUND_PLAYER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FOOTSTEP ) ) {
                SetCondition( BCOND_HEAR_ENEMY_FOOTSTEP );
            }

            SetCondition( BCOND_HEAR_ENEMY );
        }
#endif

        if ( pCurrentSound->IsSoundType( SOUND_CARCASS ) ) {
            SetCondition( BCOND_SMELL_CARCASS );
        }

        if ( pCurrentSound->IsSoundType( SOUND_MEAT ) ) {
            SetCondition( BCOND_SMELL_MEAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_GARBAGE ) ) {
            SetCondition( BCOND_SMELL_GARBAGE );
        }

        pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
    }
}

//================================================================================
// We have received damage
//================================================================================
void CBot::OnTakeDamage( const CTakeDamageInfo &info )
{
    CBaseEntity *pAttacker = info.GetAttacker();
    float farDistance = bot_far_distance.GetFloat();

    // The attacker is a character
    if ( pAttacker && pAttacker->MyCombatCharacterPointer() ) {
        int relationship = TheGameRules->PlayerRelationship( GetHost(), pAttacker );

        if ( relationship == GR_ENEMY || relationship == GR_NOTTEAMMATE ) {
            float distance = GetAbsOrigin().DistTo( pAttacker->GetAbsOrigin() );
            Vector vecEstimatedPosition = pAttacker->WorldSpaceCenter();

            // It is very far! We can not know the exact position
            if ( distance >= farDistance ) {
                float errorRange = 200.0f;

                // I'm noob, miscalculated
                if ( GetSkill()->GetLevel() <= SKILL_MEDIUM ) {
                    errorRange = 500.0f;
                }

                vecEstimatedPosition.x += RandomFloat( -errorRange, errorRange );
                vecEstimatedPosition.y += RandomFloat( -errorRange, errorRange );
            }

            // We were calm, without hurting anyone...
            if ( GetState() == STATE_IDLE ) {
                // We can not see it! We panicked!
                if ( !GetDecision()->IsAbleToSee( pAttacker ) ) {
                    Panic();
                }

                // We try to look at where we've been hurt
                //GetVision()->LookAt( "Unknown Threat Spot", vecEstimatedPosition, PRIORITY_HIGH, GetSkill()->GetAlertDuration() );
            }
            /*else {
                bool setAsEnemy = true;

                // If it is very far (Like a hidden sniper) 
                // then we only update its approximate position
                if ( distance >= farDistance ) {
                    setAsEnemy = false;
                }

                // Our new enemy
                // TODO: 
                if ( setAsEnemy ) {
                    // Noob: ¡Otro enemigo! ¡Panico total!
                    if ( GetSkill()->IsEasy() ) {
                        Panic( RandomFloat( 0.2f, 0.6f ) );
                    }

                    SetEnemy( pAttacker, true );
                }
                else {
                    GetMemory()->UpdateEntityMemory( pAttacker, vecEstimatedPosition );
                }
            }*/

            // We try to look at where we've been hurt
            GetVision()->LookAt( "Unknown Threat Spot", vecEstimatedPosition, PRIORITY_HIGH, GetSkill()->GetAlertDuration() );
        }
    }

#ifdef INSOURCE_DLL
    // El último daño recibido fue hace menos de 2s
    // Al parecer estamos recibiendo daño continuo
    if ( GetHost()->GetLastDamageTimer().IsLessThen( 2.0f ) ) {
        ++m_iRepeatedDamageTimes;
        m_flDamageAccumulated += info.GetDamage();
    }
#else
    // TODO: This does not work here
    if ( info.GetDamage() <= 10 ) {
        SetCondition( BCOND_LIGHT_DAMAGE );
    }
    else {
        SetCondition( BCOND_HEAVY_DAMAGE );
    }
#endif
}

//================================================================================
// Dead x(
//================================================================================
void CBot::OnDeath( const CTakeDamageInfo &info ) 
{
    if ( GetActiveSchedule() ) {
        GetActiveSchedule()->Fail("Player Death");
        GetActiveSchedule()->Finish();
        m_nActiveSchedule = NULL;
    }
}

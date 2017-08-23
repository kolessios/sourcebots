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
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
CBotSkill::CBotSkill()
{
    int minSkill = TheGameRules->GetSkillLevel();
    SetLevel( RandomInt( minSkill, minSkill+2 ) );
}

//================================================================================
//================================================================================
CBotSkill::CBotSkill( int skill )
{
    SetLevel( skill );
}

//================================================================================
// Sets the level of difficulty
//================================================================================
void CBotSkill::SetLevel( int skill )
{
    skill = clamp( skill, SKILL_EASIEST, SKILL_HARDEST );

    // TODO: Put all this in a script file

    switch ( skill ) {
#ifdef HL2MP
        case SKILL_EASY:
            SetMemoryDuration( 7.0f );
            SetPanicDuration( RandomFloat( 0.5f, 1.5f ) );
            SetAlertDuration( RandomFloat( 3.0f, 5.0f ) );
            SetMinAimSpeed( AIM_SPEED_LOW );
            SetMaxAimSpeed( AIM_SPEED_NORMAL );
            SetMinAttackRate( 0.05f );
            SetMaxAttackRate( 0.3f );
            SetFavoriteHitbox( HITGROUP_STOMACH );
            break;

        case SKILL_MEDIUM:
        default:
            SetMemoryDuration( 10.0f );
            SetPanicDuration( RandomFloat( 0.5, 1.0f ) );
            SetAlertDuration( RandomFloat( 3.0f, 6.0f ) );
            SetMinAimSpeed( AIM_SPEED_LOW );
            SetMaxAimSpeed( AIM_SPEED_FAST );
            SetMinAttackRate( 0.01f );
            SetMaxAttackRate( 0.2f );
            SetFavoriteHitbox( RandomInt( HITGROUP_CHEST, HITGROUP_STOMACH ) );
            break;

        case SKILL_HARD:
            SetMemoryDuration( 13.0f );
            SetPanicDuration( RandomFloat( 0.1f, 0.5f ) );
            SetAlertDuration( RandomFloat( 4.0f, 7.0f ) );
            SetMinAimSpeed( AIM_SPEED_NORMAL );
            SetMaxAimSpeed( AIM_SPEED_VERYFAST );
            SetMinAttackRate( 0.001f );
            SetMaxAttackRate( 0.1f );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_STOMACH ) );
            break;
#else
        case SKILL_EASY:
            SetMemoryDuration( 5.0f );
            SetPanicDuration( RandomFloat( 1.0f, 2.0f ) );
            SetAlertDuration( RandomFloat( 3.0f, 5.0f ) );
            SetMinAimSpeed( AIM_SPEED_VERYLOW );
            SetMaxAimSpeed( AIM_SPEED_LOW );
            SetMinAttackRate( 0.3f );
            SetMaxAttackRate( 0.8f );
            SetFavoriteHitbox( HITGROUP_STOMACH );
            break;

        case SKILL_MEDIUM:
        default:
            SetMemoryDuration( 7.0f );
            SetPanicDuration( RandomFloat( 1.0, 1.5f ) );
            SetAlertDuration( RandomFloat( 3.0f, 6.0f ) );
            SetMinAimSpeed( AIM_SPEED_VERYLOW );
            SetMaxAimSpeed( AIM_SPEED_NORMAL );
            SetMinAttackRate( 0.3f );
            SetMaxAttackRate( 0.6f );
            SetFavoriteHitbox( RandomInt( HITGROUP_CHEST, HITGROUP_STOMACH ) );
            break;

        case SKILL_HARD:
            SetMemoryDuration( 9.0f );
            SetPanicDuration( RandomFloat( 0.5f, 1.3f ) );
            SetAlertDuration( RandomFloat( 4.0f, 7.0f ) );
            SetMinAimSpeed( AIM_SPEED_LOW );
            SetMaxAimSpeed( AIM_SPEED_NORMAL );
            SetMinAttackRate( 0.2f );
            SetMaxAttackRate( 0.5f );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_STOMACH ) );
            break;
#endif

#ifdef INSOURCE_DLL
        case SKILL_VERY_HARD:
            SetMemoryDuration( 9.0f );
            SetPanicDuration( RandomFloat( 0.3f, 0.8f ) );
            SetAlertDuration( RandomFloat( 4.0f, 9.0f ) );
            SetMinAimSpeed( AIM_SPEED_NORMAL );
            SetMaxAimSpeed( AIM_SPEED_FAST );
            SetMinAttackRate( 0.08f );
            SetMaxAttackRate( 0.3f );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            break;

        case SKILL_ULTRA_HARD:
            SetMemoryDuration( 11.0f );
            SetPanicDuration( RandomFloat( 0.1f, 0.4f ) );
            SetAlertDuration( RandomFloat( 6.0f, 12.0f ) );
            SetMinAimSpeed( AIM_SPEED_NORMAL );
            SetMaxAimSpeed( AIM_SPEED_VERYFAST );
            SetMinAttackRate( 0.05f );
            SetMaxAttackRate( 0.2f );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            break;

        case SKILL_IMPOSIBLE:
            SetMemoryDuration( 16.0f );
            SetPanicDuration( RandomFloat( 0.0f, 0.1f ) );
            SetAlertDuration( RandomFloat( 6.0f, 16.0f ) );
            SetMinAimSpeed( AIM_SPEED_FAST );
            SetMaxAimSpeed( AIM_SPEED_VERYFAST );
            SetMinAttackRate( 0.001f );
            SetMaxAttackRate( 0.03f );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            break;
#endif
    }

    m_iSkillLevel = skill;
}

//================================================================================
// Returns the name of the difficulty level
//================================================================================
const char *CBotSkill::GetLevelName()
{
    switch ( m_iSkillLevel ) {
        case SKILL_EASY:
            return "EASY";
            break;

        case SKILL_MEDIUM:
            return "MEDIUM";
            break;

        case SKILL_HARD:
            return "HARD";
            break;

#ifdef INSOURCE_DLL
        case SKILL_VERY_HARD:
            return "VERY HARD";
            break;

        case SKILL_ULTRA_HARD:
            return "ULTRA HARD";
            break;

        case SKILL_IMPOSIBLE:
            return "IMPOSIBLE";
            break;
#endif
    }

    return "UNKNOWN";
}
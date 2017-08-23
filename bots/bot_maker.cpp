//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_maker.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_player.h"
#include "in_utils.h"
#include "players_system.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( info_bot_spawn, CBotSpawn );

BEGIN_DATADESC( CBotSpawn )
    DEFINE_KEYFIELD( m_nBotTargetname, FIELD_STRING, "BotTargetname" ),
    DEFINE_KEYFIELD( m_nBotPlayername, FIELD_STRING, "Playername" ),
    DEFINE_KEYFIELD( m_nBotSquadname, FIELD_STRING, "BotSquad" ),
    DEFINE_KEYFIELD( m_nAdditionalEquipment, FIELD_STRING, "AdditionalEquipment" ),
    DEFINE_KEYFIELD( m_iBotTeam, FIELD_INTEGER, "PlayerTeam" ),
    DEFINE_KEYFIELD( m_iBotClass, FIELD_INTEGER, "PlayerClass" ),
    DEFINE_KEYFIELD( m_iBotSkill, FIELD_INTEGER, "BotSkill" ),
    DEFINE_KEYFIELD( m_iBotTacticalMode, FIELD_INTEGER, "BotTacticalMode" ),
	DEFINE_KEYFIELD( m_iBlockLookAround, FIELD_INTEGER, "BlockLookAround" ),
    DEFINE_KEYFIELD( m_iPerformance, FIELD_INTEGER, "Performance" ),
	DEFINE_KEYFIELD( m_nFollowEntity, FIELD_STRING, "FollowEntity" ),
    DEFINE_KEYFIELD( m_bDisabledMovement, FIELD_BOOLEAN, "DisableMovement" ),
    DEFINE_KEYFIELD( m_bIsLeader, FIELD_BOOLEAN, "IsLeader" ),

    // Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Spawn", InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Respawn", InputRespawn ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSkill", InputSetSkill ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTacticalMode", InputSetTacticalMode ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "BlockLook", InputBlockLook ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetSquad", InputSetSquad ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableMovement", InputDisableMovement ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableMovement", InputEnableMovement ),
    DEFINE_INPUTFUNC( FIELD_VOID, "StartPeaceful", InputStartPeaceful ),
    DEFINE_INPUTFUNC( FIELD_VOID, "StopPeaceful", InputStopPeaceful ),

    DEFINE_INPUTFUNC( FIELD_EHANDLE, "DriveTo", InputDriveTo ),

    // Outputs
    DEFINE_OUTPUT( m_OnSpawn, "OnSpawn" ),
    DEFINE_OUTPUT( m_OnBotDead, "OnBotDead" ),
END_DATADESC()

//================================================================================
// Creación en el mapa
//================================================================================
void CBotSpawn::Spawn()
{
    m_bDisabled = false;
	m_nBot = NULL;

    // No somos solidos
    SetSolid( SOLID_NONE );

    // Creamos el Bot ahora mismo!
    if ( HasSpawnFlags(SF_SPAWN_IMMEDIATELY) )
        g_EventQueue.AddEvent( this, "Spawn", 1.0f, this, this );
}

//================================================================================
//================================================================================
void CBotSpawn::DeathNotice( CBaseEntity *pVictim )
{
    CPlayer *pPlayer = ToInPlayer( pVictim );
    AssertMsg( pPlayer && pPlayer->IsBot(), "info_bot_spawn ha recibido una notificación de muerte de una entidad que no es un bot" );

    // El Bot ha muerto :(
    m_OnBotDead.FireOutput( this, this );

    if ( HasSpawnFlags( SF_KICK_ON_DEAD ) ) {
#ifdef INSOURCE_DLL
        pPlayer->Kick();
#else
        engine->ServerCommand( UTIL_VarArgs( "kickid %i\n", pPlayer->GetPlayerInfo()->GetUserID() ) );
#endif

        if ( pPlayer == GetBot() ) {
            m_nBot = NULL;
        }
    }
    else {
        g_EventQueue.AddEvent( this, "Respawn", 1.0f, this, this );
    }
}

//================================================================================
// Devuelve si podemos crear un Bot
//================================================================================
bool CBotSpawn::CanMake( bool bRespawn )
{
    ConVarRef ai_inhibit_spawners( "ai_inhibit_spawners" );

    if ( ai_inhibit_spawners.GetBool() )
        return false;

    if ( m_bDisabled )
        return false;

    if ( !bRespawn ) {
        if ( HasSpawnFlags( SF_ONLY_ONE_ACTIVE_BOT ) ) {
            if ( GetBot() )
                return false;
        }
    }

    if ( HasSpawnFlags( SF_HIDE_FROM_PLAYERS ) ) {
#ifdef INSOURCE_DLL
        if ( ThePlayersSystem->IsEyesVisible( this ) )
            return false;
#else
        for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
            CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

            if ( !pPlayer )
                continue;

            if ( !pPlayer->IsAlive() )
                continue;

            if ( pPlayer->IsAbleToSee(this, CBaseCombatCharacter::USE_FOV ) )
                return false;
        }
#endif
    }

    return true;
}

//================================================================================
// Crea el Bot en el lugar de la entidad
//================================================================================
void CBotSpawn::SpawnBot()
{
    if ( !CanMake() )
        return;

    const char *pPlayername = ( m_nBotPlayername != NULL_STRING ) ? STRING(m_nBotPlayername) : NULL;

    m_nBot = CreateBot( pPlayername, NULL, NULL );
    Assert( m_nBot );

    if ( !m_nBot ) {
        return;
    }

    PreparePlayer();
}

//================================================================================
//================================================================================
void CBotSpawn::PreparePlayer()
{
    // Nombre para referirse al jugador
    if ( m_nBotTargetname != NULL_STRING ) {
        GetBot()->SetName( m_nBotTargetname );
    }

    // Establecemos el equipo
    // Esto es necesario para la creación de la I.A.
    GetBot()->ChangeTeam( m_iBotTeam );

#ifdef INSOURCE_DLL
    GetBot()->SetPlayerClass( m_iBotClass );
#endif

    // Creamos la I.A. por primera vez
    if ( !GetBot()->GetBotController() ) {
        GetBot()->SetUpBot();
        GetBot()->GetBotController()->Spawn();
    }

    IBot *pBot = GetBot()->GetBotController();
    Assert( pBot );

    pBot->SetSkill( m_iBotSkill );
    pBot->SetTacticalMode( m_iBotTacticalMode );
    pBot->SetPerformance( (BotPerformance)m_iPerformance );

    if ( m_bDisabledMovement ) {
        if ( pBot->GetLocomotion() ) {
            pBot->GetLocomotion()->SetDisabled( m_bDisabledMovement );
        }
    }

    if ( m_iBlockLookAround > 0 ) {
        if ( pBot->GetVision() ) {
            pBot->GetMemory()->UpdateDataMemory( "BlockLookAround", m_iBlockLookAround, m_iBlockLookAround );
        }
    }

    if ( m_nFollowEntity != NULL_STRING ) {
        if ( pBot->GetFollow() ) {
            pBot->GetFollow()->Start( STRING( m_nFollowEntity ) );
        }
    }

    if ( HasSpawnFlags( SF_PEACEFUL ) ) {
        pBot->SetPeaceful( true );
    }

    GetBot()->SetOwnerEntity( this );
    m_OnSpawn.Set( GetBot(), GetBot(), this );

#ifdef INSOURCE_DLL
    // Accede a la partida
    GetBot()->EnterToGame( true );
#endif

    if ( m_nBotSquadname != NULL_STRING ) {
        GetBot()->SetSquad( STRING( m_nBotSquadname ) );

        if ( m_bIsLeader )
            GetBot()->GetSquad()->SetLeader( GetBot() );
    }

    if ( HasSpawnFlags( SF_USE_SPAWNER_POSITION ) ) {
        if ( pBot->GetMemory() ) {
            pBot->GetMemory()->UpdateDataMemory( "SpawnPosition", GetAbsOrigin(), -1.0f );
        }

        GetBot()->Teleport( &GetAbsOrigin(), &GetAbsAngles(), NULL );
    }

    if ( m_nAdditionalEquipment != NULL_STRING ) {
        GetBot()->GiveNamedItem( STRING( m_nAdditionalEquipment ) );

#ifdef INSOURCE_DLL
#ifdef DEBUG
        GetBot()->GiveAmmo( 60, "AMMO_TYPE_SNIPERRIFLE" );
        GetBot()->GiveAmmo( 300, "AMMO_TYPE_ASSAULTRIFLE" );
        GetBot()->GiveAmmo( 999, "AMMO_TYPE_PISTOL" );
        GetBot()->GiveAmmo( 300, "AMMO_TYPE_SHOTGUN" );
#endif
#endif
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputSpawn( inputdata_t &inputdata )
{
    SpawnBot();
}

//================================================================================
//================================================================================
void CBotSpawn::InputRespawn( inputdata_t & inputdata ) 
{
	if ( !GetBot() )
		return;

    if ( !CanMake(true) ) {
        g_EventQueue.AddEvent( this, "Respawn", 1.0f, this, this );
        return;
    }

	PreparePlayer();
}

//================================================================================
//================================================================================
void CBotSpawn::InputEnable( inputdata_t &inputdata )
{
    m_bDisabled = false;
}

//================================================================================
//================================================================================
void CBotSpawn::InputDisable( inputdata_t &inputdata )
{
    m_bDisabled = true;
}

//================================================================================
//================================================================================
void CBotSpawn::InputToggle( inputdata_t &inputdata )
{
    m_bDisabled = !m_bDisabled;
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetSkill( inputdata_t &inputdata ) 
{
    m_iBotSkill = inputdata.value.Int();

    if ( GetBot() && GetBot()->GetBotController() )
    {
        GetBot()->GetBotController()->SetSkill( m_iBotSkill );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetTacticalMode( inputdata_t & inputdata )
{
    m_iBotTacticalMode = inputdata.value.Int();

    if ( GetBot() && GetBot()->GetBotController() )
        GetBot()->GetBotController()->SetTacticalMode( m_iBotTacticalMode );
}

//================================================================================
//================================================================================
void CBotSpawn::InputBlockLook( inputdata_t &inputdata ) 
{
    m_iBlockLookAround = inputdata.value.Int();

    if ( GetBot() && GetBot()->GetBotController() && GetBot()->GetBotController()->GetMemory() ) {
        GetBot()->GetBotController()->GetMemory()->UpdateDataMemory( "BlockLookAround", m_iBlockLookAround, -1.0f );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetSquad( inputdata_t &inputdata ) 
{
    m_nBotSquadname = MAKE_STRING( inputdata.value.String() );

    if ( GetBot() && GetBot()->GetBotController() ) {
        GetBot()->GetBotController()->SetSquad( inputdata.value.String() );
        GetBot()->GetBotController()->SetSkill( m_iBotSkill );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputDisableMovement( inputdata_t &inputdata ) 
{
    m_bDisabledMovement = true;

    if ( GetBot() && GetBot()->GetBotController() && GetBot()->GetBotController()->GetLocomotion() ) {
        GetBot()->GetBotController()->GetLocomotion()->SetDisabled( m_bDisabledMovement );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputEnableMovement( inputdata_t &inputdata ) 
{
    m_bDisabledMovement = false;

    if ( GetBot() && GetBot()->GetBotController() && GetBot()->GetBotController()->GetLocomotion() ) {
        GetBot()->GetBotController()->GetLocomotion()->SetDisabled( m_bDisabledMovement );
    }
}

void CBotSpawn::InputStartPeaceful( inputdata_t & inputdata )
{
    if ( !GetBot() )
        return;

    GetBot()->GetBotController()->SetPeaceful( true );
}

void CBotSpawn::InputStopPeaceful( inputdata_t & inputdata )
{
    if ( !GetBot() )
        return;

    GetBot()->GetBotController()->SetPeaceful( false );
}

void CBotSpawn::InputDriveTo( inputdata_t & inputdata )
{
    if( !GetBot() )
        return;

    if ( !GetBot()->GetBotController()->GetLocomotion() )
        return;

    CBaseEntity *pTarget = inputdata.value.Entity().Get();

    GetBot()->GetBotController()->GetLocomotion()->DriveTo( "Input DriveTo", pTarget, PRIORITY_CRITICAL );
}

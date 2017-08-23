//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_player.h"
#include "in_shareddefs.h"
#include "in_gamerules.h"
#include "players_system.h"
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

#include "bots\bot_defs.h"
#include "bots\squad_manager.h"
#include "bots\bot_manager.h"

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"

#include "fmtstr.h"
#include "in_buttons.h"

#include "ai_hint.h"
#include "movehelper_server.h"

#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

DECLARE_DEBUG_COMMAND( bot_frozen, "0", "" )
DECLARE_DEBUG_COMMAND( bot_crouch, "0", "" )
DECLARE_DEBUG_COMMAND( bot_flashlight, "0", "" )
DECLARE_DEBUG_COMMAND( bot_mimic, "0", "" )
DECLARE_DEBUG_COMMAND( bot_aim_player, "0", "" )
DECLARE_DEBUG_COMMAND( bot_primary_attack, "0", "" )

DECLARE_DEBUG_COMMAND( bot_sendcmd, "", 0, "Forces bots to send the specified command." );
DECLARE_DEBUG_COMMAND( bot_team, "0", "Force all bots created with bot_add to change to the specified team" )

DECLARE_DEBUG_COMMAND( bot_notarget, "0", "" )
DECLARE_DEBUG_COMMAND( bot_god, "0", "" )
DECLARE_DEBUG_COMMAND( bot_buddha, "0", "" )
DECLARE_DEBUG_COMMAND( bot_dont_attack, "0", "" )

DECLARE_DEBUG_COMMAND( bot_debug, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_locomotion, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_jump, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_memory, "0", "" )

DECLARE_DEBUG_COMMAND( bot_debug_cmd, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_conditions, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_desires, "0", "" )
DECLARE_DEBUG_COMMAND( bot_debug_max_msgs, "10", "" )

DECLARE_DEBUG_COMMAND( bot_optimize, "0", "" );
DECLARE_REPLICATED_COMMAND( bot_far_distance, "2500", "" )

//================================================================================
// Macros
//================================================================================

// Only a utility macro to check if the current state is finished
#define STATE_FINISHED m_iStateTimer.HasStarted() && m_iStateTimer.IsElapsed()

//================================================================================
// It allows to create a bot with the name and position specified.
//================================================================================
CPlayer *CreateBot( const char *pPlayername, const Vector *vecPosition, const QAngle *angles )
{
    MDLCACHE_CRITICAL_SECTION();

    if ( !pPlayername ) {
        pPlayername = m_botNames[RandomInt( 0, ARRAYSIZE( m_botNames ) - 1 )];
        pPlayername = UTIL_VarArgs( "%s Bot", pPlayername );
    }

    edict_t *pSoul = engine->CreateFakeClient( pPlayername );
    Assert( pSoul );

    if ( !pSoul ) {
        Warning( "There was a problem creating a bot. Maybe there is no more space for players on the server." );
        return NULL;
    }

    CPlayer *pPlayer = (CPlayer *)CBaseEntity::Instance( pSoul );
    Assert( pPlayer );

    pPlayer->ClearFlags();
    pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

    if ( vecPosition ) {
        pPlayer->Teleport( vecPosition, angles, NULL );
    }

#ifndef INSOURCE_DLL
    // This is where we implement the Artificial Intelligence. 
    // Unless you have a different spawn (team/class selection) system, this should work.
    if ( !pPlayer->GetBotController() ) {
        pPlayer->SetUpBot();

        Assert( pPlayer->GetBotController() );

        if ( pPlayer->GetBotController() ) {
            pPlayer->GetBotController()->Spawn();
        }
    }
#endif

    ++g_botID;
    return pPlayer;
}

//================================================================================
//================================================================================
void CBot::Spawn()
{
    m_nComponents.Purge();
    m_nSchedules.Purge();

    SetUpComponents();
    SetUpSchedules();

    FOR_EACH_COMPONENT
    {
        m_nComponents[it]->Reset();
    }

    m_cmd = NULL;
    m_nLastCmd = NULL;

    m_iState = STATE_IDLE;
    m_iTacticalMode = TACTICAL_MODE_NONE;
    m_iStateTimer.Invalidate();

    m_nActiveSchedule = NULL;
    m_nConditions.ClearAll();

    m_iRepeatedDamageTimes = 0;
    m_flDamageAccumulated = 0.0f;

    if ( !GetSkill() ) {
        SetSkill( 0 );
    }

    if ( GetMemory() ) {
        GetMemory()->UpdateDataMemory( MEMORY_SPAWN_POSITION, GetAbsOrigin() );
    }
}

//================================================================================
// Called every frame
//================================================================================
void CBot::Update()
{
    VPROF_BUDGET( "Update", VPROF_BUDGETGROUP_BOTS );

    if ( STATE_FINISHED ) {
        CleanState();
    }

    // TODO: FIXME
    if ( bot_mimic.GetInt() > 0 ) {
        MimicThink( bot_mimic.GetInt() );
        return;
    }

    m_cmd = new CBotCmd();
    m_cmd->viewangles = GetHost()->pl.v_angle;

    if ( ShouldThink() ) {
        Think( m_cmd );
    }

	PlayerMove( m_cmd );
}

//================================================================================
// Simulates all input as if it were a player
//================================================================================
void CBot::PlayerMove( CBotCmd *cmd )
{
    VPROF_BUDGET( "PlayerMove", VPROF_BUDGETGROUP_BOTS );

	m_nLastCmd = m_cmd;

    // This is not necessary if the player is a human
    if ( !GetHost()->IsBot() )
        return;

#ifdef INSOURCE_DLL
    PostClientMessagesSent();
#else
    GetHost()->RemoveEffects( EF_NOINTERP );
#endif

    RunPlayerMove( cmd );
}

//================================================================================
// Returns if we can process Artificial Intelligence
//================================================================================
bool CBot::ShouldThink()
{
    if ( !GetHost()->IsAlive() )
        return false;

    if ( GetHost()->IsMarkedForDeletion() )
        return false;

    if ( IsPanicked() )
        return false;

    AssertMsg( GetDecision(), "Bot without decision component!" );

    if ( !GetDecision() )
        return false;

    if ( GetLocomotion() && TheNavMesh->GetNavAreaCount() == 0 )
        return false;

    if ( bot_frozen.GetBool() )
        return false;

    if ( GetFollow() && GetFollow()->IsFollowingBot() ) {
        CPlayer *pLeader = ToInPlayer( GetFollow()->GetEntity() );

        if ( pLeader && pLeader->GetBotController() ) {
            return pLeader->GetBotController()->ShouldThink();
        }
    }

#ifdef INSOURCE_DLL
    if ( GetPerformance() == BOT_PERFORMANCE_SLEEP_PVS ) {
        CHumanPVSFilter filter( GetAbsOrigin() );

        if ( filter.GetRecipientCount() == 0 )
            return false;
    }
#endif

    return true;
}

//================================================================================
// I think then I exist
//================================================================================
void CBot::Think( CBotCmd* &cmd )
{
    ApplyDebugCommands();

    SelectPreConditions();

    FOR_EACH_COMPONENT
    {
        m_nComponents[it]->Update();
    }

    SelectPostConditions();

    UpdateSchedule();

    DebugDisplay();
}

//================================================================================
// Applies actions for debugging
//================================================================================
void CBot::ApplyDebugCommands()
{
    // God mode
    if ( bot_god.GetBool() ) GetHost()->AddFlag( FL_GODMODE );
    else GetHost()->RemoveFlag( FL_GODMODE );

    // Enemies can not see us
    if ( bot_notarget.GetBool() ) GetHost()->AddFlag( FL_NOTARGET );
    else GetHost()->RemoveFlag( FL_NOTARGET );

    // Buddha
    if ( bot_buddha.GetBool() ) GetHost()->m_debugOverlays = GetHost()->m_debugOverlays | OVERLAY_BUDDHA_MODE;
    else GetHost()->m_debugOverlays = GetHost()->m_debugOverlays & ~OVERLAY_BUDDHA_MODE;

    // Forced Crouch
    if ( bot_crouch.GetBool() ) {
        InjectButton( IN_DUCK );
    }
	
	// Forced primary attack
    if ( bot_primary_attack.GetBool() ) {
        InjectButton( IN_ATTACK );
    }

    // Forced flashlight
    if ( bot_flashlight.GetBool() ) {
        if ( !GetHost()->FlashlightIsOn() ) {
            GetHost()->FlashlightTurnOn();
        }
    }
    else {
        if ( GetHost()->FlashlightIsOn() ) {
            GetHost()->FlashlightTurnOff();
        }
    }

    if ( GetVision() ) {
        // We aim at the host
        if ( bot_aim_player.GetBool() ) {
            CBasePlayer *pPlayer = UTIL_GetListenServerHost();

            if ( pPlayer ) {
                GetVision()->LookAt( "bot_aim_player", pPlayer->EyePosition(), PRIORITY_UNINTERRUPTABLE, 1.0f );
            }
        }
    }
}

//================================================================================
// Returns whether we should optimize parts of the A.I.
//================================================================================
bool CBot::ShouldOptimize()
{
    if ( bot_optimize.GetBool() )
        return true;

    // TODO: This works?
    if ( (gpGlobals->tickcount % 2) )
        return false;

#ifdef INSOURCE_DLL
    // We optimize if no human player is watching us.
    if ( GetPerformance() == BOT_PERFORMANCE_VISIBILITY || GetPerformance() == BOT_PERFORMANCE_PVS_AND_VISIBILITY ) {
        if ( !ThePlayersSystem->IsVisible(GetHost()) )
            return true;
    }

    // We optimize if no human player is in our PVS
    if ( GetPerformance() == BOT_PERFORMANCE_PVS || GetPerformance() == BOT_PERFORMANCE_PVS_AND_VISIBILITY ) {
        CHumanPVSFilter filter(GetAbsOrigin());

        if ( filter.GetRecipientCount() == 0 )
            return true;
    }
#endif

    return false;
}

//================================================================================
// Imitates all inputs of the specified player.
// TODO: It does not work! It is easy to fix, but I want to give priority to other parts of the code
//================================================================================
void CBot::MimicThink( int playerIndex )
{
    CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );

    // El Jugador no existe
    if ( !pPlayer )
        return;

    // Sin input
    if ( !pPlayer->GetLastUserCommand() )
        return;

    m_flDebugYPosition = 0.34f;

    DebugDisplay();

    const CUserCmd *playercmd = pPlayer->GetLastUserCommand();
    m_cmd = new CBotCmd();

    m_cmd->command_number = playercmd->command_number;
    m_cmd->tick_count = playercmd->tick_count;
    m_cmd->viewangles = playercmd->viewangles;
    m_cmd->forwardmove = playercmd->forwardmove;
    m_cmd->sidemove = playercmd->sidemove;
    m_cmd->upmove = playercmd->upmove;
    m_cmd->buttons = playercmd->buttons;
    m_cmd->impulse = playercmd->impulse;
    m_cmd->weaponselect = playercmd->weaponselect;
    m_cmd->weaponsubtype = playercmd->weaponsubtype;
    m_cmd->random_seed = playercmd->random_seed;
    m_cmd->mousedx = playercmd->mousedx;
    m_cmd->mousedy = playercmd->mousedy;

    CFmtStr msg;
    DebugScreenText( msg.sprintf("command_number: %i", GetUserCommand()->command_number) );
    DebugScreenText( msg.sprintf("tick_count: %i", GetUserCommand()->tick_count) );
    DebugScreenText( msg.sprintf("viewangles: %.2f, %.2f", GetUserCommand()->viewangles.x, GetUserCommand()->viewangles.y) );
    DebugScreenText( msg.sprintf("forwardmove: %.2f", GetUserCommand()->forwardmove) );
    DebugScreenText( msg.sprintf("sidemove: %.2f", GetUserCommand()->sidemove) );
    DebugScreenText( msg.sprintf("upmove: %.2f", GetUserCommand()->upmove) );
    DebugScreenText( msg.sprintf("buttons:%i", (int)GetUserCommand()->buttons) );
    DebugScreenText( msg.sprintf("impulse: %i", (int)GetUserCommand()->impulse) );
    DebugScreenText( msg.sprintf("weaponselect: %i", GetUserCommand()->weaponselect) );
    DebugScreenText( msg.sprintf("weaponsubtype: %i", GetUserCommand()->weaponsubtype) );
    DebugScreenText( msg.sprintf("random_seed: %i", GetUserCommand()->random_seed) );
    DebugScreenText( msg.sprintf("mousedx: %i", (int)GetUserCommand()->mousedx) );
    DebugScreenText( msg.sprintf("mousedy: %i", (int)GetUserCommand()->mousedy) );
    DebugScreenText( msg.sprintf("hasbeenpredicted: %i", (int)GetUserCommand()->hasbeenpredicted) );

    //RunPlayerMove( m_cmd );
}

//================================================================================
// Kick the player
//================================================================================
void CBot::Kick()
{
#ifdef INSOURCE_DLL
    GetHost()->Kick();
#else
    engine->ServerCommand( UTIL_VarArgs( "kickid %i\n", GetHost()->GetPlayerInfo()->GetUserID() ) );
#endif
}

//================================================================================
// Move the bot to the desired direction by injecting the command during the current frame.
//================================================================================
void CBot::InjectMovement( NavRelativeDirType direction )
{
    if ( !GetUserCommand() )
        return;

    switch ( direction ) {
        case FORWARD:
        default:
            GetUserCommand()->forwardmove = 450.0f;
            InjectButton( IN_FORWARD );
            break;

        case UP:
            GetUserCommand()->upmove = 450.0f;
            break;

        case DOWN:
            GetUserCommand()->upmove = -450.0f;
            break;

        case BACKWARD:
            GetUserCommand()->forwardmove = -450.0f;
            InjectButton( IN_BACK );
            break;

        case LEFT:
            GetUserCommand()->sidemove = -450.0f;
            InjectButton( IN_LEFT );
            break;

        case RIGHT:
            GetUserCommand()->sidemove = 450.0f;
            InjectButton( IN_RIGHT );
            break;
    }
}

//================================================================================
// Injects the command to have pressed a button during the current frame.
//================================================================================
void CBot::InjectButton( int btn )
{
    if ( !GetUserCommand() )
        return;

    GetUserCommand()->buttons |= btn;
}

//================================================================================
// It allows the Bot to own another player. That is, the control is exchanged.
// TODO: It does not work as it should! And for now I have no idea how to fix it.
//================================================================================
void CBot::Possess( CPlayer *pOther )
{
#ifdef INSOURCE_DLL
    GetHost()->Possess(pOther);
#endif
}

//================================================================================
// Returns if the host is watching us in spectator mode.
// It should only be used to debug.
//================================================================================
bool CBot::IsLocalPlayerWatchingMe()
{
#ifdef INSOURCE_DLL
    return GetHost()->IsLocalPlayerWatchingMe();
#else
    if ( engine->IsDedicatedServer() )
        return false;

    CBasePlayer *pPlayer = UTIL_GetListenServerHost();

    if ( !pPlayer )
        return false;

    if ( pPlayer->IsAlive() || !pPlayer->IsObserver() )
        return false;

    if ( pPlayer->GetObserverMode() != OBS_MODE_IN_EYE && pPlayer->GetObserverMode() != OBS_MODE_CHASE )
        return false;

    if ( pPlayer->GetObserverTarget() != GetHost() )
        return false;

    return true;
#endif
}

//================================================================================

CON_COMMAND_F( bot_add, "Adds a specified number of generic bots", FCVAR_SERVER )
{
    // Look at -count.
    int count = args.FindArgInt( "-count", 1 );
    count = clamp( count, 1, 16 );

    // Ok, spawn all the bots.
    while ( --count >= 0 ) {
        CPlayer *pPlayer = CreateBot( NULL, NULL, NULL );
        Assert( pPlayer );

        if ( pPlayer ) {
            if ( bot_team.GetInt() > 0 ) {
                pPlayer->ChangeTeam( bot_team.GetInt() );
            }
        }
    }
}

CON_COMMAND_F( bot_kick, "Kick all bots on the server", FCVAR_SERVER )
{
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(it) );

        if ( !pPlayer || !pPlayer->IsAlive() )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        pPlayer->GetBotController()->Kick();
    }
}

CON_COMMAND_F( bot_debug_follow, "It causes all Bots to start following the host", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance( UTIL_GetCommandClientIndex() ));

    if ( !pOwner )
        return;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(it) );

        if ( !pPlayer || !pPlayer->IsAlive() )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        IBot *pBot = pPlayer->GetBotController();

        if ( pBot->GetFollow() ) {
            pBot->GetFollow()->Start( pOwner );
        }
    }
}

CON_COMMAND_F( bot_debug_stop_follow, "Causes all Bots to stop following", FCVAR_SERVER )
{
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

        if ( !pPlayer || !pPlayer->IsAlive() )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        IBot *pBot = pPlayer->GetBotController();

        if ( pBot->GetFollow() ) {
            pBot->GetFollow()->Stop();
        }
    }
}

CON_COMMAND_F( bot_debug_drive_random, "Orders all bots to move at random sites", FCVAR_SERVER )
{
    if ( TheNavAreas.Count() == 0 )
        return;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

        if ( !pPlayer || !pPlayer->IsAlive() )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        IBot *pBot = pPlayer->GetBotController();

        if ( !pBot->GetLocomotion() )
            continue;

        Vector vecFrom( pPlayer->GetAbsOrigin() );
        CNavArea *pArea = NULL;

        while ( !pArea ) {
            pArea = TheNavAreas[RandomInt( 0, TheNavAreas.Count() - 1 )];

            if ( !pArea )
                continue;

            Vector vecGoal( pArea->GetCenter() );

            if ( !pBot->GetLocomotion()->IsPotentiallyTraversable( vecFrom, vecGoal ) )
                continue;

            pBot->GetLocomotion()->DriveTo( "bot_debug_drive_random", pArea );
        }
    }
}

CON_COMMAND_F( bot_debug_drive_player, "Command all bots to move to host location", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

    CNavArea *pArea = pOwner->GetLastKnownArea();

    if ( !pArea )
        return;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

        if ( !pPlayer || !pPlayer->IsAlive() )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        IBot *pBot = pPlayer->GetBotController();

        if ( !pBot->GetLocomotion() )
            continue;

        pBot->GetLocomotion()->DriveTo( "bot_debug_drive_player", pArea );
    }
}

/*
CON_COMMAND_F( bot_possess, "", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

    if ( args.ArgC() != 2 )
    {
        Warning( "bot_possess <client index>\n" );
        return;
    }

    int iBotClient = atoi( args[1] );
    int iBotEnt = iBotClient + 1;

    if ( iBotClient < 0 || 
        iBotClient >= gpGlobals->maxClients || 
        pOwner->entindex() == iBotEnt )
    {
        Warning( "bot_possess <client index>\n" );
        return;
    }

    CPlayer *pBot = ToInPlayer(CBasePlayer::Instance( iBotEnt ));

    if ( !pBot )
        return;

    if ( !pBot->GetAI() )
        return;

    DevWarning("Possesing %s!! \n", pBot->GetPlayerName());
    pBot->GetAI()->Possess( pOwner );
}*/
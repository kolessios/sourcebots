//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_UTILS_H
#define BOT_UTILS_H

#ifdef _WIN32
#pragma once
#endif

#ifdef INSOURCE_DLL
#include "in_shareddefs.h"
#endif

#include "bots\bot_defs.h"

class IBot;

//================================================================================
// Memory about an entity
//================================================================================
class CEntityMemory
{
public:
    DECLARE_CLASS_NOBASE( CEntityMemory );

    CEntityMemory( IBot *pBot, CBaseEntity *pEntity, CBaseEntity *pInformer = NULL );

    virtual CBaseEntity *GetEntity() const {
        return m_hEntity.Get();
    }

    virtual bool Is( CBaseEntity *pEntity ) const {
        return (GetEntity() == pEntity);
    }

    virtual CBaseEntity *GetInformer() const {
        return m_hInformer.Get();
    }

    virtual void SetInformer( CBaseEntity *pInformer ) {
        m_hInformer = pInformer;
    }

    virtual void UpdatePosition( const Vector &pos ) {
        m_vecLastPosition = pos;
        m_vecIdealPosition = pos;
        m_LastUpdate.Start();
    }

    // Visibility
    virtual bool IsVisible() const {
        return m_bVisible;
    }

    virtual void UpdateVisibility( bool visible ) {
        m_bVisible = visible;

        if ( visible ) {
            m_LastVisible.Start();
        }
    }

    virtual bool IsVisibleRecently( float seconds ) const {
        return m_LastVisible.IsLessThen( seconds );
    }

    virtual bool WasEverVisible() const {
        return m_LastVisible.HasStarted();
    }

    virtual float GetElapsedTimeSinceVisible() const {
        return m_LastVisible.GetElapsedTime();
    }

    virtual float GetTimeLastVisible() const {
        return m_LastVisible.GetStartTime();
    }

    // Position
    virtual const Vector GetLastKnownPosition() const {
        return m_vecLastPosition;
    }

    virtual const Vector GetIdealPosition() const {
        return m_vecIdealPosition;
    }

    virtual bool IsUpdatedRecently( float seconds ) const {
        return m_LastUpdate.IsLessThen( seconds );
    }

    virtual float GetElapsedTimeSinceUpdate() const {
        return m_LastUpdate.GetElapsedTime();
    }

    virtual float GetTimeLastUpdate() const {
        return m_LastUpdate.GetStartTime();
    }

    virtual float GetFrameLastUpdate() const {
        return m_flFrameLastUpdate;
    }

    virtual void MarkLastFrame() {
        m_flFrameLastUpdate = gpGlobals->absoluteframetime;
    }

    //
    virtual float GetTimeLeft();
    virtual bool IsLost();

    virtual const HitboxPositions GetHitbox() const {
        return m_Hitbox;
    }

    virtual const HitboxPositions GetVisibleHitbox() const {
        return m_VisibleHitbox;
    }

    virtual bool IsHitboxVisible( HitboxType part );

    virtual CNavArea *GetLastKnownArea() const {
        return TheNavMesh->GetNearestNavArea( m_vecLastPosition );
    }

    virtual float GetDistance() const;
    virtual float GetDistanceSquare() const;

    virtual bool IsInRange( float distance ) const {
        return (GetDistance() <= distance);
    }

    virtual int GetRelationship() const;

    virtual void Maintain() {
        UpdatePosition( GetLastKnownPosition() );
    }

    virtual bool GetVisibleHitboxPosition( Vector &vecPosition, HitboxType favorite );
    virtual void UpdateHitboxAndVisibility();

protected:
    EHANDLE m_hEntity;
    EHANDLE m_hInformer;

    IBot *m_pBot;

    Vector m_vecLastPosition;
    Vector m_vecIdealPosition;

    HitboxPositions m_Hitbox;
    HitboxPositions m_VisibleHitbox;

    bool m_bVisible;

    IntervalTimer m_LastVisible;
    IntervalTimer m_LastUpdate;

    float m_flFrameLastUpdate;
};

//================================================================================
// Memory about certain information
//================================================================================
class CDataMemory : public CMultidata
{
public:
    DECLARE_CLASS_GAMEROOT( CDataMemory, CMultidata );

    CDataMemory() : BaseClass()
    {
    }

    CDataMemory( int value ) : BaseClass( value )
    {
    }

    CDataMemory( Vector value ) : BaseClass( value )
    {
    }

    CDataMemory( float value ) : BaseClass( value )
    {
    }

    CDataMemory( const char *value ) : BaseClass( value )
    {
    }

    CDataMemory( CBaseEntity *value ) : BaseClass( value )
    {
    }

    virtual void Reset() {
        BaseClass::Reset();

        m_LastUpdate.Invalidate();
        m_flForget = -1.0f;
    }

    virtual void OnSet() {
        m_LastUpdate.Start();
    }

    bool IsExpired() {
        // Never expires
        if ( m_flForget <= 0.0f )
            return false;

        return (m_LastUpdate.GetElapsedTime() >= m_flForget);
    }

    float GetElapsedTimeSinceUpdated() const {
        return m_LastUpdate.GetElapsedTime();
    }

    float GetTimeLastUpdate() const {
        return m_LastUpdate.GetStartTime();
    }

    void ForgetIn( float time ) {
        m_flForget = time;
    }

protected:
    IntervalTimer m_LastUpdate;
    float m_flForget;
};

//================================================================================
// Bot difficulty information
//================================================================================
class CBotSkill
{
public:
    CBotSkill();
    CBotSkill( int skill );

    virtual bool IsEasy() {
        return (GetLevel() == SKILL_EASY);
    }

    virtual bool IsMedium() {
        return (GetLevel() == SKILL_MEDIUM);
    }

    virtual bool IsHard() {
        return (GetLevel() == SKILL_HARD);
    }

    virtual bool IsVeryHard() {
        return (GetLevel() == SKILL_VERY_HARD);
    }

    virtual bool IsUltraHard() {
        return (GetLevel() == SKILL_ULTRA_HARD);
    }

    virtual bool IsRealistic() {
        return (GetLevel() == SKILL_IMPOSIBLE);
    }

    virtual bool IsEasiest() {
        return (GetLevel() == SKILL_EASIEST);
    }

    virtual bool IsHardest() {
        return (GetLevel() == SKILL_HARDEST);
    }

    virtual void SetLevel( int skill );

    virtual int GetLevel() {
        return m_iSkillLevel;
    }

    virtual const char *GetLevelName();

    virtual float GetMemoryDuration() {
        return m_flEnemyMemoryDuration;
    }

    virtual void SetMemoryDuration( float duration ) {
        m_flEnemyMemoryDuration = duration;
    }

    virtual float GetPanicDuration() {
        return m_flPanicDelay;
    }

    virtual void SetPanicDuration( float duration ) {
        m_flPanicDelay = duration;
    }

    virtual int GetMinAimSpeed() {
        return m_iMinAimSpeed;
    }

    virtual void SetMinAimSpeed( int speed ) {
        m_iMinAimSpeed = speed;
    }

    virtual int GetMaxAimSpeed() {
        return m_iMaxAimSpeed;
    }

    virtual void SetMaxAimSpeed( int speed ) {
        m_iMaxAimSpeed = speed;
    }

    virtual float GetMinAttackRate() {
        return m_flMinAttackRate;
    }

    virtual void SetMinAttackRate( float time ) {
        m_flMinAttackRate = time;
    }

    virtual float GetMaxAttackRate() {
        return m_flMaxAttackRate;
    }

    virtual void SetMaxAttackRate( float time ) {
        m_flMaxAttackRate = time;
    }

    virtual HitboxType GetFavoriteHitbox() {
        return m_iFavoriteHitbox;
    }

    virtual void SetFavoriteHitbox( HitboxType type ) {
        m_iFavoriteHitbox = type;
    }

    virtual float GetAlertDuration() {
        return m_flAlertDuration;
    }

    virtual void SetAlertDuration( float duration ) {
        m_flAlertDuration = duration;
    }

protected:
    int m_iSkillLevel;
    float m_flEnemyMemoryDuration;
    float m_flPanicDelay;

    int m_iMinAimSpeed;
    int m_iMaxAimSpeed;

    float m_flMinAttackRate;
    float m_flMaxAttackRate;

    HitboxType m_iFavoriteHitbox;

    float m_flAlertDuration;
};

//================================================================================
// Información acerca de una tarea, se conforma de la tarea que se debe ejecutar
// y un valor que puede ser un Vector, un flotante, un string, etc.
//================================================================================
struct BotTaskInfo_t
{
    BotTaskInfo_t( int iTask )
    {
        task = iTask;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, int value )
    {
        task = iTask;
        iValue = value;
        flValue = (float)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, Vector value )
    {
        task = iTask;
        vecValue = value;

        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, float value )
    {
        task = iTask;
        flValue = value;
        iValue = (int)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, const char *value )
    {
        task = iTask;
        iszValue = MAKE_STRING( value );

        vecValue.Invalidate();
        flValue = 0;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, CBaseEntity *value )
    {
        task = iTask;
        pszValue = value;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
    }

    int task;

    Vector vecValue;
    float flValue;
    int iValue;
    string_t iszValue;
    EHANDLE pszValue;
};

#endif // BOT_UTILS_H
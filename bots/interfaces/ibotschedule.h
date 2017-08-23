//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017
//
// Conjunto de tareas, aquí es donde esta la I.A. de los Bots, cada conjunto
// de tareas se activa según el nivel de deseo que devuelve la función
// GetDesire(), el conjunto con más deseo se activará y empezará a realizar cada
// una de las tareas que se le fue asignada.
//
// Si la función ItsImportant() devuelve false entonces cualquier otro conjunto
// que tenga más deseo terminara y reemplazara la activa.
//
// El funcionamiento predeterminado de cada tarea se encuentra en 
// el archivo bot_schedules.cpp. Se puede sobreescribir el funcionamiento
// de una tarea al devolver true en las funciones StartTask y RunTask de CBot
//
//=============================================================================//

#ifndef IBOT_SCHEDULE_H
#define IBOT_SCHEDULE_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\interfaces\ibotcomponent.h"

//================================================================================
// Macros
//================================================================================

#define ADD_TASK( task, value ) m_Tasks.AddToTail( new BotTaskInfo_t(task, value) );
#define ADD_INTERRUPT( condition ) m_Interrupts.AddToTail( condition );

#define DECLARE_SCHEDULE( id ) virtual int GetID() const { return id; } \
    virtual void Setup();

#define BEGIN_SETUP_SCHEDULE( classname ) typedef classname CurrentSchedule; \
    void CurrentSchedule::Setup() {

#define BEGIN_SCHEDULE( classname ) void classname::Setup() {

#define END_SCHEDULE() }

//================================================================================
// Base para crear un conjunto de tareas
//================================================================================
abstract_class IBotSchedule : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotSchedule, IBotComponent );

    IBotSchedule( IBot *bot ) : BaseClass( bot )
    {
        
    }

    virtual bool IsSchedule() const {
        return true;
    }

public:
    virtual void Setup() = 0;
    virtual float GetDesire() const = 0;

public:
    virtual bool HasFinished() const {
        return m_bFinished;
    }

    virtual bool HasStarted() const {
        return m_bStarted;
    }

    virtual bool HasFailed() const {
        return m_bFailed;
    }

    virtual bool HasTasks() const {
        return m_Tasks.Count() > 0;
    }

    // Indicates whether the schedule is important and should only stop with interruptions or when all tasks are completed.
    // False = Will stop as soon as there is another Schedule with more desire.
    virtual bool ItsImportant() const {
        return false;
    }

    virtual bool IsWaitFinished() const {
        return m_WaitTimer.IsElapsed();
    }

    virtual BotTaskInfo_t *GetActiveTask() const {
        return m_nActiveTask;
    }

public:
    virtual void Reset();
    virtual void Start();
    virtual void Finish();
    virtual void Fail( const char *pWhy );

    virtual bool ShouldInterrupted() const;
    virtual float GetInternalDesire();

    virtual void Update();    

    virtual void Wait( float seconds );

    virtual const char *GetActiveTaskName() const;

    virtual void TaskStart();
    virtual void TaskRun();
    virtual void TaskComplete();

protected:
    bool m_bFailed;
    bool m_bStarted;
    bool m_bFinished;

    BotTaskInfo_t *m_nActiveTask;
    float m_flLastDesire;

    CUtlVector<BotTaskInfo_t *> m_Tasks;
    CUtlVector<BCOND> m_Interrupts;

    CountdownTimer m_WaitTimer;
    IntervalTimer m_StartTimer;

    Vector m_vecSavedLocation;
    Vector m_vecLocation;
};

#endif // IBOT_SCHEDULE_H
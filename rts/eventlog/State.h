/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Watch after eventlog state, stores current state of caps, threads, tasks
 * and sparks to be able to dump the info on demand.
 *
 * ---------------------------------------------------------------------------*/
#ifndef EVENTLOG_STATE_H
#define EVENTLOG_STATE_H

#include "Rts.h"

#include "BeginPrivate.h"

#ifdef TRACING

#define THREAD_STATUS_CREATED  0
#define THREAD_STATUS_RUNNABLE 1
#define THREAD_STATUS_RUN      2 
#define THREAD_STATUS_STOPED   3

typedef struct _ThreadState {
  StgThreadID id; // id of the thread
  StgWord  cap; // current capability 
  StgWord  blockStatus; // why blocked
  StgWord  blockOn; // id of thread that the thread is blocked on
  StgWord8 status;  // created | runnable | run | stoped 
  StgBool  isSparkThread; // true if the thread is spark thread
  char *label;
} ThreadState;

// Initialise buffers
void initEventlogState(void);
// Finalise buffers
void destroyEventlogState(void);

#ifdef THREADED_RTS
// Capture the mutex of threads state
void lockThreadState(void);
// Free the mutex of threads state
void releaseThreadState(void);
#else 
INLINE_HEADER void lockThreadState(void) 
{ /* nothing */ }

INLINE_HEADER void releaseThreadState(void) 
{ /* nothing */ }
#endif 

// Record new thread
void eventlogStateCreateThread(StgThreadID id, 
                               StgWord cap,
                               StgBool  isSparkThread);
// Record that thread is ran
void eventlogStateRunThread(StgThreadID id);
// Record that thread is put into run queue
void eventlogStateRunnableThread(StgThreadID id);
// Record that thread is stoped 
void eventlogStateStopThread(StgThreadID id,
                             StgWord status, 
                             StgWord block);
// Record that thread is moved to another cap.
// The case when the thread is woke up is also included in the function.
void eventlogStateMigrateThread(StgThreadID id, 
                                StgWord cap);
#endif /* TRACING */

#include "EndPrivate.h"

#endif /* EVENTLOG_STATE_H */
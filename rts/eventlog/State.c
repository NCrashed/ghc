/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Watch after eventlog state, stores current state of caps, threads, tasks
 * and sparks to be able to dump the info on demand.
 *
 * ---------------------------------------------------------------------------*/

#include "PosixSource.h"
#include "Rts.h"
#include "RtsUtils.h"

#include "State.h"
#include "Vector.h"

#include <stdio.h>
#include <string.h>

#ifdef TRACING

// How much thread slots to preallocate in the state vector
#define PREALLOC_THREADS_COUNT 100

// Thread vector, must be protected by threadStatesMutex
Vector* threadStates = NULL;

#ifdef THREADED_RTS
Mutex threadStatesMutex; // protects threadStates vec
#endif 

static ThreadState* mallocThreadState(void);
static void freeThreadState(ThreadState *st);
static void freeThreadStates(void);
static ThreadState* findThreadState(StgThreadID i);

void initEventlogState(void) {
#ifdef THREADED_RTS
    initMutex(&threadStatesMutex);
#endif

    threadStates = mallocVector(PREALLOC_THREADS_COUNT, sizeof(ThreadState));
}

void destroyEventlogState(void)
{
    ACQUIRE_LOCK(&threadStatesMutex);
    
    freeThreadStates();

    RELEASE_LOCK(&threadStatesMutex);

#ifdef THREADED_RTS
    closeMutex(&threadStatesMutex);
#endif
}

#ifdef THREADED_RTS
void lockThreadState(void)
{
    ACQUIRE_LOCK(&threadStatesMutex);
}

void releaseThreadState(void)
{
    RELEASE_LOCK(&threadStatesMutex);
}
#endif

void eventlogStateCreateThread(StgThreadID id, 
                               StgWord cap,
                               StgBool  isSparkThread)
{
    ThreadState* st = mallocThreadState();
    st->id = id;
    st->cap = cap;
    st->blockStatus = 0;
    st->blockOn = 0;
    st->status = THREAD_STATUS_CREATED;
    st->isSparkThread = isSparkThread;
    st->label = NULL;

    ACQUIRE_LOCK(&threadStatesMutex);
    vectorPush(threadStates, (StgWord8*)st);
    RELEASE_LOCK(&threadStatesMutex);
}

void eventlogStateRunThread(StgThreadID id)
{
    ACQUIRE_LOCK(&threadStatesMutex);
    ThreadState* st = findThreadState(id);
    if (st != NULL) {
        st->status = THREAD_STATUS_RUN;
    }
    RELEASE_LOCK(&threadStatesMutex);
}

void eventlogStateRunnableThread(StgThreadID id)
{
    ACQUIRE_LOCK(&threadStatesMutex);
    ThreadState* st = findThreadState(id);
    if (st != NULL) {
        st->status = THREAD_STATUS_RUNNABLE;
    }
    RELEASE_LOCK(&threadStatesMutex);
}

void eventlogStateStopThread(StgThreadID id,
                             StgWord status, 
                             StgWord block)
{
    ACQUIRE_LOCK(&threadStatesMutex);
    ThreadState* st = findThreadState(id);
    if (st != NULL) {
        st->status = THREAD_STATUS_STOPED;
        st->blockStatus = status;
        st->blockOn = block;
    }
    RELEASE_LOCK(&threadStatesMutex);
}

void eventlogStateMigrateThread(StgThreadID id, 
                                StgWord cap)
{
    ACQUIRE_LOCK(&threadStatesMutex);
    ThreadState* st = findThreadState(id);
    if (st != NULL) {
        st->status = THREAD_STATUS_RUN;
        st->cap = cap;
    }
    RELEASE_LOCK(&threadStatesMutex);
}

ThreadState* mallocThreadState(void)
{
    return stgMallocBytes(sizeof(ThreadState), "mallocThreadState");
}

void freeThreadState(ThreadState *st)
{
    if (st != NULL) {
        stgFree(st);
    }
}

void freeThreadStates(void)
{
    if (threadStates != NULL) {
        for(StgWord64 i = 0; i < threadStates->pos; ++i) {
            freeThreadState((ThreadState*)vectorRead(threadStates, i));
        }
        freeVector(threadStates);
    }
}

ThreadState* findThreadState(StgThreadID id)
{
    if (threadStates == NULL) {
        return NULL;
    }

    for(StgWord64 i = 0; i < threadStates->pos; ++i) {
        ThreadState* e = (ThreadState*)vectorRead(threadStates, i);
        if (e->id == id) { // e is never NULL as mallocThreadState cannot return NULL
            return e;
        }
    }

    return NULL;
}
#endif /* TRACING */
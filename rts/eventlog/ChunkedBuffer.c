/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Support for chained memory buffer for eventlog.
 *
 * ---------------------------------------------------------------------------*/

#include "PosixSource.h"
#include "Rts.h"

#include "ChunkedBuffer.h"

#include <stdio.h>
#include <string.h>

#ifdef TRACING

ChunkedBuffer* head = NULL;
StgWord64 tailSize = 0;
StgWord64 bufferSize = 1024*1024*2; // by default

#ifdef THREADED_RTS
Mutex headMutex; // protected by this mutex
StgBool mutexInited = 0;
#endif

ChunkedBuffer* newChunkedBuffer(ChunkedBuffer* prev) {
  ChunkedBuffer *buf = malloc(sizeof(ChunkedBuffer));
  buf->mem = malloc(bufferSize);
  buf->next = NULL;
  if (prev != NULL) {
    prev->next = buf;
  }
  return buf;
}

void freeChunkedBuffer(ChunkedBuffer* buf) {
  ChunkedBuffer* curBuf = NULL;
  while (buf != NULL) {
    curBuf = buf;
    buf = curBuf->next;
    free(curBuf->mem);
    free(curBuf);
  }
}

ChunkedBuffer* getTail(void) {
  if (head == NULL) {
    head = newChunkedBuffer(NULL);
    return head;
  }

  ChunkedBuffer* curBuf = head;
  while(curBuf->next != NULL) {
    curBuf = curBuf->next;
  }

  if(tailSize == bufferSize) {
    curBuf->next = newChunkedBuffer(curBuf); 
    tailSize = 0;
    return curBuf->next;
  }

  return curBuf;
}

StgWord64 getChunksCount(void) {
  StgWord64 i = 0;
  ChunkedBuffer* cur = head;
  while(cur != NULL) {
    cur = cur->next;
    i = i + 1;
  }
  return i;
}

void writeChunkedLog(StgInt8 *buf, StgWord64 size) {
  ACQUIRE_LOCK(&headMutex);
  ChunkedBuffer* curTail = getTail();

  while(size > 0) {
    StgWord64 curReminder = bufferSize - tailSize;
    if (curReminder > size) {
      curReminder = size;
    }
    memcpy(curTail->mem + tailSize, buf, curReminder);
    tailSize = tailSize + curReminder;
    size = size - curReminder;
    buf = buf + curReminder;

    if (tailSize >= bufferSize) {
      curTail->next = newChunkedBuffer(curTail);
      tailSize = 0;
      curTail = curTail->next;
    }
  }
  RELEASE_LOCK(&headMutex);
}

ChunkedBuffer* popChunkedLog(void) {
  ACQUIRE_LOCK(&headMutex);
  if (head == NULL) {
    RELEASE_LOCK(&headMutex);
    return NULL;
  }

  if (head->next == NULL && tailSize != bufferSize) {
    RELEASE_LOCK(&headMutex);
    return NULL;
  }

  ChunkedBuffer* ret = head;
  head = head->next;
  if (head == NULL) {
    tailSize = 0;
  }
  RELEASE_LOCK(&headMutex);
  return ret;
}

StgWord64 getEventLogChunk(StgInt8** ptr) {
  StgWord64 size = 0;
  ChunkedBuffer* buff;

  buff = popChunkedLog();
  if (buff != NULL) {
    *ptr = buff->mem;
    size = bufferSize;
    free(buff);
  }
  return size;
}

void initEventLogChunkedBuffer(StgWord64 chunkSize) {
#ifdef THREADED_RTS
  if(mutexInited) {
    initMutex(&headMutex);
    mutexInited = 1;
  }
#endif
  bufferSize = chunkSize;
}

void destroyEventLogChunkedBuffer(void) {
  ACQUIRE_LOCK(&headMutex);
  freeChunkedBuffer(head);
  head = NULL;
  RELEASE_LOCK(&headMutex);
}

#endif /* TRACING */
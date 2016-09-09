/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Support for chained memory buffer for eventlog.
 *
 * ---------------------------------------------------------------------------*/

#include "PosixSource.h"
#include "Rts.h"
#include "RtsUtils.h"

#include "ChunkedBuffer.h"

#include <stdio.h>
#include <string.h>

#ifdef TRACING

ChunkedBuffer* eventlogBuffer = NULL;

#ifdef THREADED_RTS
Mutex eventlogMutex; // protected by this mutex
StgBool mutexInited = rtsFalse;
#endif

ChunkedNode* newChunkedNode(ChunkedNode *prev, StgWord64 chunkSize)
{
    ChunkedNode *node = stgMallocBytes(sizeof(ChunkedNode),
        "ChunkedNode struct");
    node->mem = stgMallocBytes(chunkSize, "ChunkedNode buffer");
    node->next = NULL;
    if (prev != NULL) {
        prev->next = node;
    }
    return node;
}

ChunkedBuffer* newChunkedBuffer(StgWord64 chunkSize)
{
    ChunkedBuffer *buf = stgMallocBytes(sizeof(ChunkedBuffer), "ChunkedBuffer");
    buf->head = newChunkedNode(NULL, chunkSize);
    buf->tailSize = 0;
    buf->chunkSize = chunkSize;
    return buf;
}

void freeChunkedNode(ChunkedNode *node)
{
    ChunkedNode* curNode = NULL;
    while (node != NULL) {
        curNode = node;
        node = curNode->next;
        stgFree(curNode->mem);
        stgFree(curNode);
    }
}

void freeChunkedBuffer(ChunkedBuffer *buf)
{
    if (buf != NULL) {
        freeChunkedNode(buf->head);
        stgFree(buf);
    }
}

ChunkedNode* getChunkedTail(ChunkedBuffer *buf)
{
    if (buf == NULL) {
        return NULL;
    }

    if (buf->head == NULL) {
        buf->head = newChunkedNode(NULL, buf->chunkSize);
    }

    ChunkedNode* curNode = buf->head;
    while(curNode->next != NULL) {
        curNode = curNode->next;
    }

    if(buf->tailSize >= buf->chunkSize) {
        curNode->next = newChunkedNode(curNode, buf->chunkSize);
        buf->tailSize = 0;
        return curNode->next;
    }

    return curNode;
}

StgWord64 getChunksCount(ChunkedBuffer *buf) {
    if(buf == NULL || buf->head == NULL) {
        return 0;
    }

    StgWord64 i = 0;
    ChunkedNode* cur = buf->head;
    while(cur != NULL) {
        cur = cur->next;
        i = i + 1;
    }
    return i;
}

void writeChunked(ChunkedBuffer *buf, StgInt8 *data, StgWord64 size)
{
    ChunkedNode* curTail = getChunkedTail(buf);
    if (curTail == NULL) {
        debugBelch("writeChunked: buffer isn't initalized!");
        return;
    }

    while(size > 0) {
        StgWord64 curReminder = buf->chunkSize - buf->tailSize;
        if (curReminder > size) {
            curReminder = size;
        }
        memcpy(curTail->mem + buf->tailSize, data, curReminder);
        buf->tailSize = buf->tailSize + curReminder;
        size = size - curReminder;
        data = data + curReminder;

        if (buf->tailSize >= buf->chunkSize) {
            curTail->next = newChunkedNode(curTail, buf->chunkSize);
            buf->tailSize = 0;
            curTail = curTail->next;
        }
    }
}

ChunkedNode* popChunkedLog(ChunkedBuffer *buf) {
    if (buf == NULL || buf->head == NULL) {
        return NULL;
    }

    if (buf->head->next == NULL && buf->tailSize != buf->chunkSize) {
        return NULL;
    }

    ChunkedNode *ret = buf->head;
    buf->head = buf->head->next;
    if (buf->head == NULL) {
        buf->tailSize = 0;
    }

    return ret;
}

void writeEventLogChunked(StgInt8 *data, StgWord64 size) {
    ACQUIRE_LOCK(&eventlogMutex);

    writeChunked(eventlogBuffer, data, size);

    RELEASE_LOCK(&eventlogMutex);
}

StgWord64 getEventLogChunk(StgInt8** ptr) {
    ACQUIRE_LOCK(&eventlogMutex);
    StgWord64 size = 0;
    ChunkedNode *node;

    node = popChunkedLog(eventlogBuffer);
    if (node != NULL) {
        *ptr = node->mem;
        size = eventlogBuffer->chunkSize;
        stgFree(node);
    }

    RELEASE_LOCK(&eventlogMutex);
    return size;
}

void initEventLogChunkedBuffer(StgWord64 chunkSize) {
#ifdef THREADED_RTS
    if (!mutexInited) {
        initMutex(&eventlogMutex);
        mutexInited = rtsTrue;
    }
#endif

    ACQUIRE_LOCK(&eventlogMutex);
    if (eventlogBuffer == NULL) {
        eventlogBuffer = newChunkedBuffer(chunkSize);
    }

    RELEASE_LOCK(&eventlogMutex);
}

void destroyEventLogChunkedBuffer(void) {
    ACQUIRE_LOCK(&eventlogMutex);

    if (eventlogBuffer != NULL) {
        freeChunkedBuffer(eventlogBuffer);
        eventlogBuffer = NULL;
    }

    RELEASE_LOCK(&eventlogMutex);
}

void resizeEventLogChunkedBuffer(StgWord64 chunkSize)
{
#ifdef THREADED_RTS
    if (!mutexInited) {
        initMutex(&eventlogMutex);
        mutexInited = 1;
    }
#endif

    ACQUIRE_LOCK(&eventlogMutex);

    if (eventlogBuffer == NULL) {
        eventlogBuffer = newChunkedBuffer(chunkSize);
    } else {
        ChunkedBuffer *buf = eventlogBuffer;
        eventlogBuffer = newChunkedBuffer(chunkSize);

        ChunkedNode *node = buf->head;
        while (node != NULL) {
            StgWord64 remain;
            if (node->next == NULL) {
                remain = buf->tailSize;
            } else {
                remain = buf->chunkSize;
            }

            writeChunked(eventlogBuffer, node->mem, remain);
            node = node->next;
        }

        freeChunkedBuffer(buf);
    }

    RELEASE_LOCK(&eventlogMutex);
}

StgWord64 getEventLogChunkedBufferSize(void)
{
    if (eventlogBuffer == NULL) {
        return 0;
    }

    return eventlogBuffer->chunkSize;
}

#endif /* TRACING */

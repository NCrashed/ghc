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

// Allocates new tail with specified size
ChunkedNode* allocateTail(ChunkedBuffer *buf, uint64_t chunkSize);

ChunkedNode* newChunkedNode(ChunkedNode *prev, uint64_t chunkSize)
{
    ChunkedNode *node = stgMallocBytes(sizeof(ChunkedNode),
        "ChunkedNode struct");
    node->mem = stgMallocBytes(chunkSize, "ChunkedNode buffer");
    node->next = NULL;
    node->size = chunkSize;
    if (prev != NULL) {
        prev->next = node;
    }
    return node;
}

ChunkedBuffer* newChunkedBuffer(void)
{
    ChunkedBuffer *buf = stgMallocBytes(sizeof(ChunkedBuffer), "ChunkedBuffer");
    buf->head = NULL;
    buf->tail = NULL;
    buf->chunksCount = 0;
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

ChunkedNode* allocateTail(ChunkedBuffer *buf, uint64_t chunkSize) 
{
    ChunkedNode *newTail;

    if (buf == NULL) {
        return NULL;
    }

    if (buf->head == NULL) {
        buf->head = newChunkedNode(NULL, chunkSize);
        buf->tail = buf->head;
        buf->chunksCount = 1;
        return buf->tail;
    }

    if (buf->tail == NULL) {
        debugBelch("allocateTail: inconsistent chunked buffer");
        return NULL;
    }

    newTail = newChunkedNode(buf->tail, chunkSize);
    buf->tail = newTail;
    buf->chunksCount += 1;

    return newTail;
}

void writeChunked(ChunkedBuffer *buf, uint8_t *data, uint64_t size)
{
    if (buf == NULL) {
        debugBelch("writeChunked: passed NULL buf!");
        return;
    }

    ChunkedNode* curTail = allocateTail(buf, size);
    if (curTail == NULL) {
        debugBelch("writeChunked: cannot allocate tail!");
        return;
    }

    memcpy(curTail->mem, data, size);
}

ChunkedNode* popChunk(ChunkedBuffer *buf) 
{
    if (buf == NULL || buf->head == NULL) {
        return NULL;
    }

    ChunkedNode *ret = buf->head;
    buf->head = buf->head->next;
    buf->chunksCount -= 1;
    if (buf->head == NULL) {
        buf->tail = NULL;
    }

    return ret;
}

uint64_t popChunkMemory(ChunkedBuffer *buf, uint8_t** ptr) 
{
    uint64_t size = 0;
    ChunkedNode *node;

    node = popChunk(buf);
    if (node != NULL) {
        *ptr = node->mem;
        size = node->size;
        stgFree(node);
    }

    return size;
}

void freeChunkMemory(uint8_t *ptr) 
{
    if (ptr != NULL) {
        stgFree(ptr);
    }
}

#endif /* TRACING */

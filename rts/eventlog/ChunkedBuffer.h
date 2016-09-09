/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Support for chained memory buffer for eventlog.
 *
 * ---------------------------------------------------------------------------*/

#ifndef CHUNKED_BUFFER_H
#define CHUNKED_BUFFER_H

#include "Rts.h"

#include "BeginPrivate.h"

#ifdef TRACING

typedef struct _ChunkedNode {
  StgInt8 *mem;
  struct _ChunkedNode *next;
} ChunkedNode;

typedef struct _ChunkedBuffer {
  ChunkedNode *head;
  StgWord64 tailSize;
  StgWord64 chunkSize;
} ChunkedBuffer;

// Allocate new chunk with current chunk size and link to previous chunk
ChunkedNode* newChunkedNode(ChunkedNode *prev, StgWord64 chunkSize);
// Allocate new chunked buffer and allocate head with given chunkSize
ChunkedBuffer* newChunkedBuffer(StgWord64 chunkSize);

// Destroy the chunk, its buffer and all childs
void freeChunkedNode(ChunkedNode *node);
// Destroy the chunk, its buffer and all childs
void freeChunkedBuffer(ChunkedBuffer *buf);

// Return current unfilled tail of chunks chain
ChunkedNode* getChunkedTail(ChunkedBuffer *buf);
// Return current length of chunks chain
StgWord64 getChunksCount(ChunkedBuffer *buf);
// Return filled head, or return NULL
ChunkedNode* popChunkedLog(ChunkedBuffer *buf);
// Write data to the chunked buffer
void writeChunked(ChunkedBuffer *buf, StgInt8 *data, StgWord64 size);

/*
 * Write data to eventlog chunked buffer, protected by mutex.
 */
void writeEventLogChunked(StgInt8 *buf, StgWord64 size);

/*
 * Read data from chunked buffer, protected by mutex.
 * If returned size is not zero, parameter contains buffer
 * that must be destroyed by caller.
 */
StgWord64 getEventLogChunk(StgInt8 **ptr);

/*
 * Initalize eventlog buffer with given chunk size.
 */
void initEventLogChunkedBuffer(StgWord64 chunkSize);
/*
 * Destroy eventlog buffer.
 */
void destroyEventLogChunkedBuffer(void);

/*
 * Resize chunks of inner buffer to the given size.
 */
void resizeEventLogChunkedBuffer(StgWord64 chunkSize);

/*
 * Return current size of chunk in eventlog memory buffer.
 */
StgWord64 getEventLogChunkedBufferSize(void);

#endif /* TRACING */

#include "EndPrivate.h"

#endif /* CHUNKED_BUFFER_H */

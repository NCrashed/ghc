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

typedef struct _ChunkedBuffer {
  StgInt8* mem;
  struct _ChunkedBuffer* next;
} ChunkedBuffer;

// Allocate new chunk with current chunk size and link to previous chunk
ChunkedBuffer* newChunkedBuffer(ChunkedBuffer* prev);
// Destroy the chunk, its buffer and all childs
void freeChunkedBuffer(ChunkedBuffer* buf);

// Return current unfilled tail of chunks chain
ChunkedBuffer* getTail(void);
// Return current length of chunks chain
StgWord64 getChunksCount(void);
// Return filled head, or return NULL
ChunkedBuffer* popChunkedLog(void);

// Write data to chunked buffer, protected by mutex
void writeChunkedLog(StgInt8 *buf, StgWord64 size);
/*
 * Read data from chunked buffer, protected by mutex.
 * If returned size is not zero, parameter contains buffer
 * that must be destroyed by caller.
 */
StgWord64 getEventLogChunk(StgInt8** ptr);

/*
 * Initalize eventlog buffer with given chunk size.
 */
void initEventLogChunkedBuffer(StgWord64 chunkSize);
/*
 * Destroy eventlog buffer.
 */
void destroyEventLogChunkedBuffer(void);

// TODO: void resizeEventLogChunkedBuffer(StgWord64 chunkSize);

#endif /* TRACING */

#include "EndPrivate.h"

#endif /* CHUNKED_BUFFER_H */

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

// Single chunk of chunked list
typedef struct _ChunkedNode {
  uint8_t *mem; // Payload, should be always size of chunkSize in ChunkedBuffer
  struct _ChunkedNode *next; // Next chunk in list, can be NULL
} ChunkedNode;

// Wrapper around a head of chunked list
typedef struct _ChunkedBuffer {
  ChunkedNode *head; // First chunk, can be NULL
  ChunkedNode *tail; // Last chunk, can be NULL
  uint64_t tailSize; // Memory used in the last chunk, when this become equal
                      // chunkSize a new chunk is allocated
  uint64_t chunksCount; // Current size of the list
  uint64_t chunkSize; // Current size of each chunk payload
} ChunkedBuffer;

// Allocate new chunk with current chunk size and link to previous chunk
ChunkedNode* newChunkedNode(ChunkedNode *prev, uint64_t chunkSize);
// Allocate new chunked buffer and allocate head with given chunkSize
ChunkedBuffer* newChunkedBuffer(uint64_t chunkSize);

// Destroy the chunk, its buffer and all children
void freeChunkedNode(ChunkedNode *node);
// Destroy the chunk, its buffer and all children
void freeChunkedBuffer(ChunkedBuffer *buf);

/* Return current unfilled tail of chunks chain. The function performs fair
 * descend from head to tail.
 * 
 * The more fast way is direct read of tailSize field. The function is used
 * for sanity checks. Also the function creates a new tail and head if the
 * buffer is empty or new tail if the tail node is full.
 */
ChunkedNode* getChunkedTail(ChunkedBuffer *buf);

/* Return current length of chunks chain. The function performs fair 
 * descend from head to tail. 
 *
 * The more fast way is direct read of chunksCount field. The function is used
 * for sanity checks.
 */
uint64_t getChunksCount(ChunkedBuffer *buf);

// Return filled head, or return NULL
ChunkedNode* popChunkedLog(ChunkedBuffer *buf);
// Write data to the chunked buffer
void writeChunked(ChunkedBuffer *buf, uint8_t *data, uint64_t size);

/*
 * Write data to eventlog chunked buffer, protected by mutex.
 */
void writeEventLogChunked(uint8_t *buf, uint64_t size);

/*
 * Read data from chunked buffer, protected by mutex.
 * If returned size is not zero, parameter contains buffer
 * that must be destroyed by caller.
 */
uint64_t getEventLogChunk(uint8_t **ptr);

/*
 * Initalize eventlog buffer with given chunk size.
 */
void initEventLogChunkedBuffer(uint64_t chunkSize);
/*
 * Destroy eventlog buffer.
 */
void destroyEventLogChunkedBuffer(void);

/*
 * Resize chunks of inner buffer to the given size.
 */
void resizeEventLogChunkedBuffer(uint64_t chunkSize);

/*
 * Return current size of chunk in eventlog memory buffer.
 */
uint64_t getEventLogChunkedBufferSize(void);

/*
 * Return current count of chunks in eventlog memory buffer.
 */
uint64_t getEventLogChunksCount(void);

#endif /* TRACING */

#include "EndPrivate.h"

#endif /* CHUNKED_BUFFER_H */

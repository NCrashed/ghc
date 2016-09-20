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
  uint64_t size; // Current size of each chunk payload
} ChunkedNode;

// Wrapper around a head of chunked list
typedef struct _ChunkedBuffer {
  ChunkedNode *head; // First chunk, can be NULL
  ChunkedNode *tail; // Last chunk, can be NULL
  uint64_t chunksCount; // Current size of the list
} ChunkedBuffer;

// Allocate new chunk with current chunk size and link to previous chunk
ChunkedNode* newChunkedNode(ChunkedNode *prev, uint64_t chunkSize);
// Allocate new chunked buffer
ChunkedBuffer* newChunkedBuffer(void);

// Destroy the chunk, its buffer and all children
void freeChunkedNode(ChunkedNode *node);
// Destroy the chunk, its buffer and all children
void freeChunkedBuffer(ChunkedBuffer *buf);

/* Return head chunk, or return NULL. 
 *
 * After the call, the buffer doesn't have any references to the chunk and the
 * caller have to deallocate it with freeChunkedNode.
 */
ChunkedNode* popChunk(ChunkedBuffer *buf);

/* Get head chunk memory.
 * 
 * If returned size is not zero, parameter contains buffer
 * that must be destroyed by caller with freeChunkMemory.
 */
uint64_t popChunkMemory(ChunkedBuffer *buf, uint8_t **ptr);

/* The recommended way to free memory got by popChunkMemory.
 *
 * After the popChunkMemory call the caller have to free 
 * provided buffer (if size returned is not zero).
 */
void freeChunkMemory(uint8_t *ptr);

/* Write data to the chunked buffer.
 * 
 * The call forms a new chunk at the tail of list of buffers. The data is copied
 * so the caller can deallocate input buffer immediately.
 */
void writeChunked(ChunkedBuffer *buf, uint8_t *data, uint64_t size);

#endif /* TRACING */

#include "EndPrivate.h"

#endif /* CHUNKED_BUFFER_H */

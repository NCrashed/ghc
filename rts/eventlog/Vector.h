/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Support for extendable array.
 *
 * ---------------------------------------------------------------------------*/
#ifndef EVENTLOG_VECTOR_H
#define EVENTLOG_VECTOR_H

#include "Rts.h"

#include "BeginPrivate.h"

#ifdef TRACING

// Array that is reallocated on demand
typedef struct _Vector {
  // Count of elements in the vector
  StgWord64 elements;
  // Length of a element
  StgWord64 elemLength;
  // Number of elements used (others are preallocated)
  StgWord64 pos;
  // Payload of the vector
  StgWord8 mem[];
} Vector;

// Initial allocation of a vector with particular size
Vector* mallocVector(StgWord64 elements, 
                     StgWord64 elemLength);

// Free memory of the vector
void freeVector(Vector *vec);

// Add element to the vector's tail
void vectorPush(Vector *vec,
                StgWord8 *element);

// Remove element at the index, moves elements after the index to squash the gap 
// from removed element
void vectorRemove(Vector *vec, 
                  StgWord64 i);

// Get pointer to element at specific index, returns NULL if the index is out of
// range
StgWord8* vectorRead(Vector *vec, 
                     StgWord64 i);

// Reallocate vector to match actual size of its payload
void vectorShrink(Vector **vec);

#endif /* TRACING */

#include "EndPrivate.h"

#endif /* CHUNKED_BUFFER_H */
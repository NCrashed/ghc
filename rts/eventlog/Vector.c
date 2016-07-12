/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Support for extendable array.
 *
 * ---------------------------------------------------------------------------*/

#include "PosixSource.h"
#include "Rts.h"
#include "RtsUtils.h"

#include "Vector.h"

#include <stdio.h>
#include <string.h>

#ifdef TRACING

// If the vector has too few elements, reallocate it with size doubled
void ensureSpace(Vector **vec,
                 StgWord64 elems);

Vector* mallocVector(StgWord64 elements, 
                     StgWord64 elemLength)
{
    Vector* vec = stgMallocBytes(sizeof(Vector) + elements * elemLength,
        "mallocVector");
    vec->elements = elements;
    vec->elemLength = elemLength;
    vec->pos = 0;
    return vec;
}

void freeVector(Vector *vec)
{
    stgFree(vec);
}

void vectorPush(Vector *vec,
                StgWord8 *element)
{
    ensureSpace(&vec, vec->pos + 1);
    memcpy(vec->mem + vec->pos * vec->elemLength, element, vec->elemLength);
    vec->pos = vec->pos + 1;
}

void vectorRemove(Vector *vec, 
                  StgWord64 i)
{
    if (i >= vec->pos) {
        return;
    }
    if (i == vec->pos - 1) {
        vec->pos = vec->pos - 1;
        return;
    }
    memmove(vec->mem + i * vec->elemLength, 
            vec->mem + (i + 1) * vec->elemLength,
            vec->pos - (i + 1) );
    vec->pos = vec->pos - 1;
}

StgWord8* vectorRead(Vector *vec, 
                     StgWord64 i)
{
    if (i >= vec->pos) {
        return NULL;
    } else {
        return vec->mem + i * vec->elemLength;
    }
}

void vectorShrink(Vector **pvec)
{
    Vector *newMem;
    Vector *vec = *pvec;

    if (vec->pos == vec->elements) {
        return;
    }

    newMem = stgReallocBytes(vec, sizeof(Vector) + vec->pos * vec->elemLength,
        "vectorShrink");
    if (newMem == NULL) {
        debugBelch("Failed to shrink vector from %"FMT_Word64
            " to %"FMT_Word64, vec->elements, vec->pos);
    } else {
        *pvec = newMem;
    }
}

void ensureSpace(Vector **pvec,
                 StgWord64 elems)
{
    Vector *newMem;
    Vector *vec = *pvec;

    if (elems <= vec->elements) {
        return;
    } else {
        newMem = stgReallocBytes(vec, sizeof(Vector) + elems * vec->elemLength,
            "vector ensureSpace");
        if (newMem == NULL) {
            sysErrorBelch("Failed to expand vector from %"FMT_Word64
                " to %"FMT_Word64, vec->elements, elems);
            stg_exit(EXIT_FAILURE);
        } else {
            *pvec = newMem;
            (*pvec)->elements = elems;
        }
    }
}

#endif /* TRACING */
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

#include <stdio.h>
#include <string.h>

#ifdef TRACING

#endif /* TRACING */
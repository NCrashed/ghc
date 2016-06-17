/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team, 2008-2016
 *
 * Event logging public API for:
 *
 *   - turning on/off event logging
 *   - setting custom file descriptor
 *   - tuning buffers size
 * -------------------------------------------------------------------------- */

#ifndef RTS_EVENTLOG_H
#define RTS_EVENTLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TRACING
/*
 * Set custom file stream for global event log sink.
 *
 * The function overwrites previous event log file pointer. Previous 
 * sink is closed only if closePrev flag is on.
 *
 * Writing to the sink is protected by global mutex.
 *
 * The function puts header to the new sink only when emitHeader flag
 * is on. User might not want the header if it is switching to 
 * already existed eventlog handle that was switched away recently.
 */
void rts_setEventLogSink(FILE *sink,
                         StgBool closePrev,
                         StgBool emitHeader);

/*
 * Get current file stream that is used for global event log sink.
 *
 * You shouldn't do anything with the pointer until 
 * rts_setEventLogSink(otherFileOrNull, false) is called. After that 
 * you can do anything with the file stream.
 */
FILE* rts_getEventLogSink(void);

#else /* !TRACING */

void rts_setEventLogSink(FILE    *sink       STG_UNUSED, 
                         StgBool  closePrev  STG_UNUSED,
                         StgBool  emitHeader STG_UNUSED)
{ /* nothing */ }

FILE* rts_getEventLogSink(void)
{ 
  return NULL;
}

#endif 

#ifdef __cplusplus
}
#endif

#endif /* RTS_EVENTLOG_H */

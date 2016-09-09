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

/*
 * Set custom file stream for global event log sink.
 *
 * The function overwrites previous event log file pointer. Previouss
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

/*
 * If RTS started with '-lm' flag then eventlog is stored in memory buffer.
 *
 * The function allows to pop chunks of the buffer. Return value of 0 means
 * that there is no any filled chunk of data.
 *
 * If the function returns nonzero value the parameter contains full chunk
 * of eventlog data with size of the returned value. Caller must free the
 * buffer, the buffer isn't referenced anywhere anymore.
 *
 * If nobody calls the function with '-lm' flag then the memory is kinda
 * to be exhausted.
 */
StgWord64 rts_getEventLogChunk(StgInt8** ptr);

/*
 * Reallocate inner buffers to match the new size. The size should be not
 * too small to contain at least one event.
 *
 * If RTS started with '-lm' the chunks of memory buffer is also resized.
 */
void rts_resizeEventLog(StgWord64 size);

/*
 * Return current size of eventlog buffers.
 */
StgWord64 rts_getEventLogBuffersSize(void);

#endif /* RTS_EVENTLOG_H */

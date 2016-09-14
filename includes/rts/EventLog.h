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
 * If RTS started with '-lm' flag then eventlog is stored in memory buffer.
 * The memory buffer is organized as linked list of memory chunks. The eventlog
 * system appends new events to current tail of the list and when it is full 
 * a new chunk is allocated. User is intended to consume chunks with 
 * rts_getEventLogChunk function, but if one doesn't do this or consumer is too 
 * slow then count of stacked chunk will grow without control.
 *
 * So there is a hard limit for length of the linked list. When the count is
 * reached new events are simply dropped off.
 */
#define EVENTLOG_MAX_MEMORY_CHUNKS 32

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
 * User is intended to consume chunks with the function periodically, 
 * but if one doesn't do this or consumer is too slow then count of stacked 
 * chunk will grow without control.
 *
 * So there is a hard limit for length of the linked list. When the count is
 * reached new events are simply dropped off. See EVENTLOG_MAX_MEMORY_CHUNKS
 * macro.
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

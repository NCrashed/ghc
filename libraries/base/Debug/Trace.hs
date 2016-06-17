{-# LANGUAGE Unsafe #-}
{-# LANGUAGE MagicHash #-}
{-# LANGUAGE NoImplicitPrelude #-}
{-# LANGUAGE UnboxedTuples #-}

-----------------------------------------------------------------------------
-- |
-- Module      :  Debug.Trace
-- Copyright   :  (c) The University of Glasgow 2001
-- License     :  BSD-style (see the file libraries/base/LICENSE)
--
-- Maintainer  :  libraries@haskell.org
-- Stability   :  provisional
-- Portability :  portable
--
-- Functions for tracing and monitoring execution.
--
-- These can be useful for investigating bugs or performance problems.
-- They should /not/ be used in production code.
--
-----------------------------------------------------------------------------

module Debug.Trace (
        -- * Tracing
        -- $tracing
        trace,
        traceId,
        traceShow,
        traceShowId,
        traceStack,
        traceIO,
        traceM,
        traceShowM,
        putTraceMsg,

        -- * Eventlog tracing
        -- $eventlog_tracing
        traceEvent,
        traceEventIO,

        -- * Execution phase markers
        -- $markers
        traceMarker,
        traceMarkerIO,

        -- * Eventlog options
        -- $eventlog_options
        setEventLogHandle,
        setEventLogCFile,
        getEventLogCFile
  ) where

import System.IO 
import System.IO.Unsafe
import System.IO.Error
import System.Posix.Types (Fd(..))

import Foreign.C.String
import Foreign.C.Types
import Foreign.Marshal.Utils (fromBool)
import GHC.Base
import qualified GHC.Foreign
import GHC.IO.Handle (hDuplicate)
import GHC.IO.Handle.Internals (withHandle', flushWriteBuffer)
import GHC.IO.Handle.Types (Handle(..), Handle__(..), HandleType(..))
import GHC.IO.Exception (IOErrorType(..))
import qualified GHC.IO.FD as FD
import qualified GHC.IO.Handle.FD as FD
import GHC.Ptr
import GHC.Show
import GHC.Stack
import Data.List
import Data.Typeable (cast)

-- $tracing
--
-- The 'trace', 'traceShow' and 'traceIO' functions print messages to an output
-- stream. They are intended for \"printf debugging\", that is: tracing the flow
-- of execution and printing interesting values.
--
-- All these functions evaluate the message completely before printing
-- it; so if the message is not fully defined, none of it will be
-- printed.
--
-- The usual output stream is 'System.IO.stderr'. For Windows GUI applications
-- (that have no stderr) the output is directed to the Windows debug console.
-- Some implementations of these functions may decorate the string that\'s
-- output to indicate that you\'re tracing.

-- | The 'traceIO' function outputs the trace message from the IO monad.
-- This sequences the output with respect to other IO actions.
--
-- @since 4.5.0.0
traceIO :: String -> IO ()
traceIO msg = do
    withCString "%s\n" $ \cfmt -> do
     -- NB: debugBelch can't deal with null bytes, so filter them
     -- out so we don't accidentally truncate the message.  See Trac #9395
     let (nulls, msg') = partition (=='\0') msg
     withCString msg' $ \cmsg ->
      debugBelch cfmt cmsg
     when (not (null nulls)) $
       withCString "WARNING: previous trace message had null bytes" $ \cmsg ->
         debugBelch cfmt cmsg

-- don't use debugBelch() directly, because we cannot call varargs functions
-- using the FFI.
foreign import ccall unsafe "HsBase.h debugBelch2"
   debugBelch :: CString -> CString -> IO ()

-- |
putTraceMsg :: String -> IO ()
putTraceMsg = traceIO
{-# DEPRECATED putTraceMsg "Use 'Debug.Trace.traceIO'" #-} -- deprecated in 7.4


{-# NOINLINE trace #-}
{-|
The 'trace' function outputs the trace message given as its first argument,
before returning the second argument as its result.

For example, this returns the value of @f x@ but first outputs the message.

> trace ("calling f with x = " ++ show x) (f x)

The 'trace' function should /only/ be used for debugging, or for monitoring
execution. The function is not referentially transparent: its type indicates
that it is a pure function but it has the side effect of outputting the
trace message.
-}
trace :: String -> a -> a
trace string expr = unsafePerformIO $ do
    traceIO string
    return expr

{-|
Like 'trace' but returns the message instead of a third value.

@since 4.7.0.0
-}
traceId :: String -> String
traceId a = trace a a

{-|
Like 'trace', but uses 'show' on the argument to convert it to a 'String'.

This makes it convenient for printing the values of interesting variables or
expressions inside a function. For example here we print the value of the
variables @x@ and @z@:

> f x y =
>     traceShow (x, z) $ result
>   where
>     z = ...
>     ...
-}
traceShow :: (Show a) => a -> b -> b
traceShow = trace . show

{-|
Like 'traceShow' but returns the shown value instead of a third value.

@since 4.7.0.0
-}
traceShowId :: (Show a) => a -> a
traceShowId a = trace (show a) a

{-|
Like 'trace' but returning unit in an arbitrary 'Applicative' context. Allows
for convenient use in do-notation.

Note that the application of 'traceM' is not an action in the 'Applicative'
context, as 'traceIO' is in the 'IO' type. While the fresh bindings in the
following example will force the 'traceM' expressions to be reduced every time
the @do@-block is executed, @traceM "not crashed"@ would only be reduced once,
and the message would only be printed once.  If your monad is in 'MonadIO',
@liftIO . traceIO@ may be a better option.

> ... = do
>   x <- ...
>   traceM $ "x: " ++ show x
>   y <- ...
>   traceM $ "y: " ++ show y

@since 4.7.0.0
-}
traceM :: (Applicative f) => String -> f ()
traceM string = trace string $ pure ()

{-|
Like 'traceM', but uses 'show' on the argument to convert it to a 'String'.

> ... = do
>   x <- ...
>   traceShowM $ x
>   y <- ...
>   traceShowM $ x + y

@since 4.7.0.0
-}
traceShowM :: (Show a, Applicative f) => a -> f ()
traceShowM = traceM . show

-- | like 'trace', but additionally prints a call stack if one is
-- available.
--
-- In the current GHC implementation, the call stack is only
-- available if the program was compiled with @-prof@; otherwise
-- 'traceStack' behaves exactly like 'trace'.  Entries in the call
-- stack correspond to @SCC@ annotations, so it is a good idea to use
-- @-fprof-auto@ or @-fprof-auto-calls@ to add SCC annotations automatically.
--
-- @since 4.5.0.0
traceStack :: String -> a -> a
traceStack str expr = unsafePerformIO $ do
   traceIO str
   stack <- currentCallStack
   when (not (null stack)) $ traceIO (renderStack stack)
   return expr


-- $eventlog_tracing
--
-- Eventlog tracing is a performance profiling system. These functions emit
-- extra events into the eventlog. In combination with eventlog profiling
-- tools these functions can be used for monitoring execution and
-- investigating performance problems.
--
-- Currently only GHC provides eventlog profiling, see the GHC user guide for
-- details on how to use it. These function exists for other Haskell
-- implementations but no events are emitted. Note that the string message is
-- always evaluated, whether or not profiling is available or enabled.

{-# NOINLINE traceEvent #-}
-- | The 'traceEvent' function behaves like 'trace' with the difference that
-- the message is emitted to the eventlog, if eventlog profiling is available
-- and enabled at runtime.
--
-- It is suitable for use in pure code. In an IO context use 'traceEventIO'
-- instead.
--
-- Note that when using GHC's SMP runtime, it is possible (but rare) to get
-- duplicate events emitted if two CPUs simultaneously evaluate the same thunk
-- that uses 'traceEvent'.
--
-- @since 4.5.0.0
traceEvent :: String -> a -> a
traceEvent msg expr = unsafeDupablePerformIO $ do
    traceEventIO msg
    return expr

-- | The 'traceEventIO' function emits a message to the eventlog, if eventlog
-- profiling is available and enabled at runtime.
--
-- Compared to 'traceEvent', 'traceEventIO' sequences the event with respect to
-- other IO actions.
--
-- @since 4.5.0.0
traceEventIO :: String -> IO ()
traceEventIO msg =
  GHC.Foreign.withCString utf8 msg $ \(Ptr p) -> IO $ \s ->
    case traceEvent# p s of s' -> (# s', () #)

-- $markers
--
-- When looking at a profile for the execution of a program we often want to
-- be able to mark certain points or phases in the execution and see that
-- visually in the profile.

-- For example, a program might have several distinct phases with different
-- performance or resource behaviour in each phase. To properly interpret the
-- profile graph we really want to see when each phase starts and ends.
--
-- Markers let us do this: we can annotate the program to emit a marker at
-- an appropriate point during execution and then see that in a profile.
--
-- Currently this feature is only supported in GHC by the eventlog tracing
-- system, but in future it may also be supported by the heap profiling or
-- other profiling tools. These function exists for other Haskell
-- implementations but they have no effect. Note that the string message is
-- always evaluated, whether or not profiling is available or enabled.

{-# NOINLINE traceMarker #-}
-- | The 'traceMarker' function emits a marker to the eventlog, if eventlog
-- profiling is available and enabled at runtime. The @String@ is the name of
-- the marker. The name is just used in the profiling tools to help you keep
-- clear which marker is which.
--
-- This function is suitable for use in pure code. In an IO context use
-- 'traceMarkerIO' instead.
--
-- Note that when using GHC's SMP runtime, it is possible (but rare) to get
-- duplicate events emitted if two CPUs simultaneously evaluate the same thunk
-- that uses 'traceMarker'.
--
-- @since 4.7.0.0
traceMarker :: String -> a -> a
traceMarker msg expr = unsafeDupablePerformIO $ do
    traceMarkerIO msg
    return expr

-- | The 'traceMarkerIO' function emits a marker to the eventlog, if eventlog
-- profiling is available and enabled at runtime.
--
-- Compared to 'traceMarker', 'traceMarkerIO' sequences the event with respect to
-- other IO actions.
--
-- @since 4.7.0.0
traceMarkerIO :: String -> IO ()
traceMarkerIO msg =
  GHC.Foreign.withCString utf8 msg $ \(Ptr p) -> IO $ \s ->
    case traceMarker# p s of s' -> (# s', () #)

-- $eventlog_options
-- 
-- By default the eventlog uses local file with name of the executable to dump all events.
-- The following functions allows to redefine the behavior and redirect the stream of 
-- bytes into user specified handler. Thus is very helpful for implementing complex 
-- tools, for instance remote profilers.

foreign import ccall unsafe "stdio.h fdopen" 
  fdopen :: Fd -> CString -> IO (Ptr CFile)

-- | The 'setEventLogHandle' function changes current sink of the eventlog, if eventlog
-- profiling is available and enabled at runtime.
--
-- The second parameter @setEventLogHandle h True@ defines whether old sink should be 
-- finalized and closed or not. Not closing it could be helpful for temporal redirection
-- of eventlog data into not standard sink and then restoring to the default file sink.
--
-- @since 4.10.0.0
setEventLogHandle :: Handle -> Bool -> IO ()
setEventLogHandle h closePrev = withCString "a" $ \iomode -> do 
  h' <- hDuplicate h -- to prevent closing original handle
  fd <- handleToFd h'
  cf <- fdopen fd iomode
  setEventLogCFile cf closePrev
  where 
    handleToFd h@(FileHandle _ m) = withHandle' "handleToFd" h m $ handleToFd' h
    handleToFd h@(DuplexHandle _ r w) = do 
      _ <- withHandle' "handleToFd" h r $ handleToFd' h
      withHandle' "handleToFd" h w $ handleToFd' h

    handleToFd' :: Handle -> Handle__ -> IO (Handle__, Fd)
    handleToFd' h h_@Handle__{haDevice = hdev} = do
      case cast hdev of
        Nothing -> ioError (ioeSetErrorString (mkIOError IllegalOperation
                                               "handleToFd" (Just h) Nothing)
                            "handle is not a file descriptor")
        Just fd -> do
         -- converting a Handle into an Fd effectively means
         -- letting go of the Handle; it is put into a closed
         -- state as a result.
         flushWriteBuffer h_
         FD.release fd
         return (h_ { haType = ClosedHandle }, Fd (FD.fdFD fd))

foreign import ccall unsafe "rts/EventLog.h rts_setEventLogSink" 
  rts_setEventLogSink :: Ptr CFile -> CInt -> IO ()

foreign import ccall unsafe "rts/EventLog.h rts_getEventLogSink" 
  rts_getEventLogSink :: IO (Ptr CFile)

-- | The 'setEventLogCFile' function changes current sink of the eventlog, if eventlog
-- profiling is available and enabled at runtime.
--
-- The second parameter @setEventLogCFile cf True@ defines whether old sink should be 
-- finalized and closed or not. Not closing it could be helpful for temporal redirection
-- of eventlog data into not standard sink and then restoring to the default file sink.
--
-- The function is more low-level than 'setEventLogHandle' but doesn't recreate underlying
-- file descriptor and is intended to use with 'getEventLogCFile' to save and restore 
-- current sink of the eventlog.
--
-- @since 4.10.0.0
setEventLogCFile :: Ptr CFile -> Bool -> IO ()
setEventLogCFile pf closePrev = rts_setEventLogSink pf (fromBool closePrev)

-- | The 'getEventLogCFile' function returns current sink of the eventlog, if eventlog
-- profiling is available and enabled at runtime.
--
-- The function is intented to be used with 'setEventLogCFile' to save and restore 
-- current sink of the eventlog.
--
-- @since 4.10.0.0
getEventLogCFile :: IO (Ptr CFile)
getEventLogCFile = rts_getEventLogSink
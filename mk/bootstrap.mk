# -----------------------------------------------------------------------------
# $Id: bootstrap.mk,v 1.18 2002/01/15 05:39:15 sof Exp $
#
# Makefile rules for booting from .hc files without a driver.
#
# When booting from .hc files without a compiler installed, we don't have
# the benefit of the GHC driver to add all the magic options required to
# compile the .hc files, so we have to duplicate that functionality here.
# The result is unfortunately ugly, but we don't have another choice.

TOP_SAVED := $(TOP)
TOP:=$(TOP)/ghc

include $(TOP)/mk/version.mk
include $(TOP)/mk/paths.mk

# Reset TOP
TOP:=$(TOP_SAVED)

# -----------------------------------------------------------------------------
# Set the platform-specific options to send to the C compiler.  These should
# match the list in machdepCCOpts in ghc/compiler/DriverFlags.hs.
#

PLATFORM_CC_OPTS =
PLATFORM_HC_BOOT_CC_OPTS =

ifeq "$(i386_TARGET_ARCH)" "1"
PLATFORM_CC_OPTS += -DDONT_WANT_WIN32_DLL_SUPPORT
PLATFORM_HC_BOOT_CC_OPTS += -fno-defer-pop -fomit-frame-pointer 
endif

ifeq "$(hppa_TARGET_ARCH)" "1"
PLATFORM_CC_OPTS += -static -D_HPUX_SOURCE
endif

ifeq "$(powerpc_TARGET_ARCH)" "1"
PLATFORM_CC_OPTS += -static
PLATFORM_HC_BOOT_CC_OPTS += -finhibit-size-directive
endif

ifeq "$(rs6000_TARGET_ARCH)" "1"
PLATFORM_CC_OPTS += -static
PLATFORM_HC_BOOT_CC_OPTS += -finhibit-size-directive
endif

ifeq "$(mingw32_TARGET_OS)" "1"
PLATFORM_CC_OPTS += -mno-cygwin
endif

ifeq "$(alpha_TARGET_ARCH)" "1"
PLATFORM_CC_OPTS += -static -w
PLATFORM_HC_BOOT_CC_OPTS += -mieee
endif

ifeq "$(sparc_TARGET_ARCH)" "1"
PLATFORM_HC_BOOT_CC_OPTS += -w
endif

PLATFORM_CC_OPTS += -D__GLASGOW_HASKELL__=$(ProjectVersionInt) 

HC_BOOT_CC_OPTS = $(PLATFORM_HC_BOOT_CC_OPTS) $(PLATFORM_CC_OPTS) $(CC_OPTS)

SRC_CC_OPTS += -I$(FPTOOLS_TOP_ABS)/ghc/includes -I$(FPTOOLS_TOP_ABS)/ghc/lib/std/cbits -I$(FPTOOLS_TOP_ABS)/hslibs/lang/cbits -I$(FPTOOLS_TOP_ABS)/hslibs/posix/cbits -I$(FPTOOLS_TOP_ABS)/hslibs/util/cbits -I$(FPTOOLS_TOP_ABS)/hslibs/text/cbits -I$(FPTOOLS_TOP_ABS)/hslibs/hssource/cbits

# -----------------------------------------------------------------------------
# Linking: we have to give all the libraries explicitly.

ifeq "$(LeadingUnderscore)" "YES"
UNDERSCORE=_
else
UNDERSCORE=
endif


HC_BOOT_LD_OPTS =				\
   -L$(FPTOOLS_TOP_ABS)/ghc/rts			\
   -L$(FPTOOLS_TOP_ABS)/ghc/rts/gmp		\
   -L$(FPTOOLS_TOP_ABS)/ghc/lib/std		\
   -L$(FPTOOLS_TOP_ABS)/ghc/lib/std/cbits	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/lang		\
   -L$(FPTOOLS_TOP_ABS)/hslibs/lang/cbits	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/concurrent	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/concurrent/cbits	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/posix		\
   -L$(FPTOOLS_TOP_ABS)/hslibs/posix/cbits	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/util		\
   -L$(FPTOOLS_TOP_ABS)/hslibs/util/cbits	\
   -L$(FPTOOLS_TOP_ABS)/hslibs/text		\
   -L$(FPTOOLS_TOP_ABS)/hslibs/text/cbits	\
   -u "$(UNDERSCORE)PrelBase_Izh_static_info"		\
   -u "$(UNDERSCORE)PrelBase_Czh_static_info"		\
   -u "$(UNDERSCORE)PrelFloat_Fzh_static_info"		\
   -u "$(UNDERSCORE)PrelFloat_Dzh_static_info"		\
   -u "$(UNDERSCORE)PrelPtr_Ptr_static_info"		\
   -u "$(UNDERSCORE)PrelWord_Wzh_static_info"		\
   -u "$(UNDERSCORE)PrelInt_I8zh_static_info"		\
   -u "$(UNDERSCORE)PrelInt_I16zh_static_info"		\
   -u "$(UNDERSCORE)PrelInt_I32zh_static_info"		\
   -u "$(UNDERSCORE)PrelInt_I64zh_static_info"		\
   -u "$(UNDERSCORE)PrelWord_W8zh_static_info"		\
   -u "$(UNDERSCORE)PrelWord_W16zh_static_info"		\
   -u "$(UNDERSCORE)PrelWord_W32zh_static_info"		\
   -u "$(UNDERSCORE)PrelWord_W64zh_static_info"		\
   -u "$(UNDERSCORE)PrelStable_StablePtr_static_info"	\
   -u "$(UNDERSCORE)PrelBase_Izh_con_info"		\
   -u "$(UNDERSCORE)PrelBase_Czh_con_info"		\
   -u "$(UNDERSCORE)PrelFloat_Fzh_con_info"		\
   -u "$(UNDERSCORE)PrelFloat_Dzh_con_info"		\
   -u "$(UNDERSCORE)PrelPtr_Ptr_con_info"		\
   -u "$(UNDERSCORE)PrelStable_StablePtr_con_info"	\
   -u "$(UNDERSCORE)PrelBase_False_closure"		\
   -u "$(UNDERSCORE)PrelBase_True_closure"		\
   -u "$(UNDERSCORE)PrelPack_unpackCString_closure"	\
   -u "$(UNDERSCORE)PrelIOBase_stackOverflow_closure"	\
   -u "$(UNDERSCORE)PrelIOBase_heapOverflow_closure"	\
   -u "$(UNDERSCORE)PrelIOBase_NonTermination_closure"	\
   -u "$(UNDERSCORE)PrelIOBase_BlockedOnDeadMVar_closure"	\
   -u "$(UNDERSCORE)PrelWeak_runFinalizzerBatch_closure"	\
   -u "$(UNDERSCORE)__stginit_Prelude"				\
   -u "$(UNDERSCORE)PrelMain_mainIO_closure"			\
   -u "$(UNDERSCORE)__stginit_PrelMain"

HC_BOOT_LIBS = -lHStext -lHStext_cbits -lHSutil -lHSposix -lHSposix_cbits -lHSconcurrent -lHSlang -lHSlang_cbits -lHSstd -lHSstd_cbits -lHSrts -lgmp -lm $(EXTRA_HC_BOOT_LIBS)

ifeq "$(GhcLibsWithReadline)" "YES"
HC_BOOT_LIBS += $(patsubst %, -l%, $(LibsReadline))
endif

ifeq "$(HaveLibDL)" "YES"
HC_BOOT_LIBS += -ldl
endif

# -----------------------------------------------------------------------------
# suffix rules for building a .o from a .hc file.

ifeq "$(BootingFromUnregisterisedHc)" "YES"

# without mangling

%.o : %.hc
	$(CC) -x c $< -o $@ -c -O $(HC_BOOT_CC_OPTS) -I.  `echo $(patsubst -monly-%-regs, -DSTOLEN_X86_REGS=%, $(filter -monly-%-regs, $($*_HC_OPTS))) | sed 's/^$$/-DSTOLEN_X86_REGS=4/'`

else

# with mangling

%.raw_s : %.hc
	$(CC) -x c $< -o $@ -S -O $(HC_BOOT_CC_OPTS) -I.  `echo $(patsubst -monly-%-regs, -DSTOLEN_X86_REGS=%, $(filter -monly-%-regs, $($*_HC_OPTS))) | sed 's/^$$/-DSTOLEN_X86_REGS=4/'`

%.s : %.raw_s
	$(FPTOOLS_TOP)/$(GHC_MANGLER_DIR)/$(GHC_MANGLER) $< $@ $(patsubst -monly-%-regs, %, $(filter -monly-%-regs, $($*_HC_OPTS)))

%.o : %.s
	$(CC) -c -o $@ $<

endif

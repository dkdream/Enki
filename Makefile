PREFIX  = /tools/Mercury
BINDIR  = $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

TIME := $(shell date +T=%s.%N)

ENKI      := enki.vm
ENKI.test := enki.vm
ENKI.ext  := ea

PATH := .:$(PATH)

GCC    := gcc
DIFF   := diff
AR     := ar
RANLIB := ranlib

TAILFLAGS := -O
TAILFLAGS += -fexpensive-optimizations
TAILFLAGS += -finline-functions
#TAILFLAGS += -fconserve-stack
TAILFLAGS += -foptimize-sibling-calls
TAILFLAGS += -ftracer
#TAILFLAGS += -findirect-inlining
TAILFLAGS += -finline-functions-called-once
TAILFLAGS += -fearly-inlining
TAILFLAGS += -fmerge-constants
TAILFLAGS += -fthread-jumps
TAILFLAGS += -fcrossjumping
#TAILFLAGS += -fdce
#TAILFLAGS += -fdse
TAILFLAGS += -fif-conversion
TAILFLAGS += -fif-conversion2
TAILFLAGS += -fdelete-null-pointer-checks

RUNFLAGS := 

INCFLAGS := -I. $(COPPER_INC)
DBFLAGS  := -ggdb -Wall -mtune=i686
CFLAGS   := $(DBFLAGS) $(INCFLAG) $(TAILFLAGS)
LIBFLAGS := $(COPPER_LIB)
ARFLAGS  := rcu


MAINS     := enki_main.c
C_SOURCES := $(filter-out $(MAINS),$(notdir $(wildcard *.c)))
H_SOURCES := $(filter-out enki.h, $(notdir $(wildcard *.h)))

ASMS    := $(C_SOURCES:%.c=%.s)
OBJS    := $(C_SOURCES:%.c=%.o)
TSTS    := $(notdir $(wildcard *.ea))
RUNS    := $(TSTS:test_%.ea=test_%.run)
DEPENDS := $(C_SOURCES:%.c=.depends/%.d) $(MAINS:%.c=.depends/%.d)

UNIT_TESTS := test_reader.gcc test_sizes.gcc

all :: enki.vm libEnki.a 

asm  :: $(ASMS)
test :: $(RUNS)
	@echo all test runs

units :: $(UNIT_TESTS:%.gcc=%.x)
	echo  $(UNIT_TESTS:%.gcc=%.x)

install : install.bin install.inc install.lib

install.bin :: $(BINDIR)/enki
install.inc :: $(H_SOURCES:%=$(INCDIR)/%)
install.lib :: $(LIBDIR)/libEnki.a

checkpoint : ; git checkpoint

$(RUNS) : enki.vm

clean ::
	@rm -rf .depends
	rm -f $(OBJS) $(ASMS) $(MAINS:%.c=%.o) enki.x test.x enki_ver.h

scrub :: 
	@make clean

enki.vm : enki_main.o libEnki.a 
	$(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

enki_main.o : enki_main.c

$(UNIT_TESTS:%.gcc=%.x) : libEnki.a

libEnki.a : $(H_SOURCES)
libEnki.a : $(C_SOURCES:%.c=%.o)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(C_SOURCES:%.c=%.o)
	$(RANLIB) $@

$(BINDIR) : ; [ -d $@ ] || mkdir -p $@
$(INCDIR) : ; [ -d $@ ] || mkdir -p $@
$(LIBDIR) : ; [ -d $@ ] || mkdir -p $@

$(BINDIR)/enki : $(BINDIR) $(ENKI)
	cp -p $(ENKI) $@
	strip $@

$(H_SOURCES:%=$(INCDIR)/%) : $(INCDIR)/%.h : %.h $(INCDIR)
	cp -p $< $@

$(LIBDIR)/libEnki.a : $(LIBDIR) libEnki.a
	cp -p libEnki.a $@

enki_ver.h : FORCE
	@./Version.gen ENKI_VERSION enki_ver.h

# --

.PHONY :: all
.PHONY :: asm
.PHONY :: test
.PHONY :: install
.PHONY :: install.bin
.PHONY :: install.inc
.PHONY :: install.lib
.PHONY :: checkpoint
.PHONY :: clear
.PHONY :: clean
.PHONY :: scrub
.PHONY :: scrub
.PHONY :: tail.check
.PHONY :: FORCE

##
## rules
##

%.s : %.c
	@echo $(GCC) $(DBFLAGS) -S -o $@ $<
	@$(GCC) $(CFLAGS) -S -fverbose-asm -o $@ $<

%.o : %.c
	@echo $(GCC) $(DBFLAGS) -c -o $@ $<
	@$(GCC) $(CFLAGS) -c -o $@ $<

%.run : %.ea
	@echo $(ENKI.test) $(RUNFLAGS) $<
	@$(ENKI.test) $(RUNFLAGS) $< >$@ 2>&1 || ( cat $@ ; rm -f $@; false )
	@cat $@
	@echo '==============================================================='

%.o : %.gcc
	@echo $(GCC) $(DBFLAGS) -c -o $@ $<
	@$(GCC) $(CFLAGS) -x c -c -o $@ $<

%.x : %.o
	$(GCC) $(CFLAGS) -o $@ $+ libEnki.a

.depends : ; @mkdir .depends

.depends/%.d : %.c .depends ; @$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

-include $(DEPENDS)



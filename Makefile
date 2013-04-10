PREFIX  = /tools/Enki
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
TAILFLAGS += -Wformat-security

RUNFLAGS := 

INCFLAGS := -I. $(COPPER_INC)
DBFLAGS  := -ggdb -Wall -mtune=i686 -rdynamic -fPIC
CFLAGS   := $(DBFLAGS) $(INCFLAG) $(TAILFLAGS)
SFLAGS   := -mtune=i686 -rdynamic -fdelete-null-pointer-checks -fverbose-asm
LIBFLAGS := $(COPPER_LIB)
ARFLAGS  := rcu


MAINS     := enki_main.c $(notdir $(wildcard link_*.c))
FOOS      := $(notdir $(wildcard foo_*.c))
C_SOURCES := $(filter-out $(MAINS) $(FOOS),$(notdir $(wildcard *.c)))
H_SOURCES := $(filter-out enki.h, $(notdir $(wildcard *.h)))
GCC_SRCS  := $(notdir $(wildcard *.gcc))

ASMS    := $(C_SOURCES:%.c=.assembly/%.s) $(FOOS:%.c=.assembly/%.s)
OBJS    := $(C_SOURCES:%.c=.objects/%.o)
TSTS    := $(notdir $(wildcard test_*.ea))
RUNS    := $(TSTS:test_%.ea=.run/test_%.log)
DEPENDS := $(C_SOURCES:%.c=.depends/%.d) $(MAINS:%.c=.depends/%.d)

UNIT_TESTS := test_reader.gcc test_sizes.gcc

all :: enki asm
	@make --touch --quiet --no-print-directory $@

enki :: $(RUNS)
asm  :: $(ASMS)
test :: $(RUNS)
	@echo all test runs

units :: $(UNIT_TESTS:%.gcc=%.x)
	ls -l $(UNIT_TESTS:%.gcc=%.x)

install : install.bin install.inc install.lib

install.bin :: $(BINDIR)/enki
install.inc :: $(H_SOURCES:%=$(INCDIR)/%)
install.lib :: $(LIBDIR)/libEnki.a

checkpoint : ; git checkpoint

depends : $(DEPENDS)

$(RUNS) : enki.vm

clean ::
	rm -fr .depends .objects .assembly .run
	rm -f enki.vm libEnki.a
	rm -f *~ ./#* *.x *.s
	rm -f test.*.out test.out

scrub :: 
	@make clean
	@rm -rf .depends .assembly

enki.vm : .objects/enki_main.o libEnki.a 
	$(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

enki_main.o : enki_main.c

$(UNIT_TESTS:%.gcc=%.x) : libEnki.a

libEnki.a : $(OBJS)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(OBJS)
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

.depends  : ; @mkdir .depends
.assembly : ; @mkdir .assembly
.objects  : ; @mkdir .objects
.run      : ; @mkdir .run

.objects/%.o : %.c .objects
	@echo $(GCC) $(DBFLAGS) -c -o $@ $<
	@$(GCC) $(CFLAGS) -c -o $@ $<

.run/%.log : %.ea .run
	@./run.it $(ENKI.test) "$(RUNFLAGS)" $< $@

.objects/%.o : %.gcc .objects
	@echo $(GCC) $(DBFLAGS) -c -o $@ $<
	@$(GCC) $(CFLAGS) -x c -c -o $@ $<

%.x             : .objects/%.o    ; $(GCC) $(CFLAGS) -o $@ $+ libEnki.a
.depends/%.d    : %.c .depends    ; @$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<
.assembly/%.s   : %.c .assembly   ; @$(GCC) $(SFLAGS) -S -fverbose-asm -o $@ $<

-include $(DEPENDS)



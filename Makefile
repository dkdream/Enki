PREFIX  = /tools/Enki
BINDIR  = $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

TIME := $(shell date +T=%s.%N)

ENKI      := enki.vm
ENKI.test := enki.vm
ENKI.ext  := ea

PATH := .:$(PATH)

MACHINE := $(shell uname --machine)

GCC    := gcc
AS     := as
DIFF   := diff
AR     := ar
RANLIB := ranlib

TAILFLAGS := -O
TAILFLAGS += -fexpensive-optimizations
TAILFLAGS += -finline-functions
TAILFLAGS += -foptimize-sibling-calls
TAILFLAGS += -ftracer
TAILFLAGS += -finline-functions-called-once
TAILFLAGS += -fearly-inlining
TAILFLAGS += -fmerge-constants
TAILFLAGS += -fthread-jumps
TAILFLAGS += -fcrossjumping
TAILFLAGS += -fif-conversion
TAILFLAGS += -fif-conversion2
TAILFLAGS += -fdelete-null-pointer-checks
TAILFLAGS += -Wformat-security

##TAILFLAGS += -fconserve-stack

RUNFLAGS := 

INCFLAGS := -I. $(COPPER_INC)
DBFLAGS  := -Wall -mtune=i686 -rdynamic -fPIC
CFLAGS   := $(DBFLAGS) $(INCFLAG) $(TAILFLAGS)
SFLAGS   := -mtune=i686 -rdynamic -fdelete-null-pointer-checks
ASFLAGS  := -Qy
LIBFLAGS := $(COPPER_LIB)
ARFLAGS  := rcu

MAINS     := enki_main.c $(notdir $(wildcard link_*.c))
FOOS      := $(notdir $(wildcard foo_*.c))
C_SOURCES := $(filter-out $(MAINS) $(FOOS),$(notdir $(wildcard *.c)))
H_SOURCES := $(filter-out enki.h, $(notdir $(wildcard *.h)))
GCC_SRCS  := $(notdir $(wildcard *.gcc))
BUILDINS  := $(wildcard ./buildins/*.c)

OBJS    := $(C_SOURCES:%.c=.objects/%_32.o)
TSTS    := $(notdir $(wildcard test_*.ea))
RUNS    := $(TSTS:test_%.ea=.run/test_%.log)

DEPENDS := $(C_SOURCES:%.c=.depends/%.d)
DEPENDS += $(MAINS:%.c=.depends/%.d)

UNIT_TESTS := test_reader.gcc test_sizes.gcc

all   :: enki test asm atoms
enki  :: enki.vm | lib
lib   :: libEnki_32.a
test  :: $(RUNS)
asm   :: $(FOOS:%.c=.dumps/%_32.s)
units :: $(UNIT_TESTS:%.gcc=%.x)
atoms :: $(BUILDINS:./buildins/%.c=.dumps/%_atom.s)

install : install.bin install.inc install.lib

install.bin :: $(BINDIR)/enki
install.inc :: $(H_SOURCES:%=$(INCDIR)/%)
install.lib :: $(LIBDIR)/libEnki.a

checkpoint : ; git checkpoint

depends : $(DEPENDS)

$(RUNS) : | enki.vm

clean ::
	rm -fr .depends .objects .assembly .run .dumps
	rm -f enki.vm libEnki.a libEnki_32.a libEnki_64.a
	rm -f *~ ./#* *.x *.s *.o
	rm -f test.*.out test.out

scrub :: 
	@make clean
	@rm -rf .depends

enki.vm : .objects/enki_main_32.o libEnki_32.a 
	$(GCC) $(CFLAGS) -m32 -o $@ $^ $(LIBFLAGS)

test :: link_main.x
	./link_main.x

test :: $(FOOS:%.c=.dumps/%_32.s)

link_main.x : .objects/link_main_32.o .objects/foo_32.o libEnki_32.a
	$(GCC) $(CFLAGS) -m32 -o $@ .objects/link_main_32.o .objects/foo_32.o -L. -lEnki_32

foo_32.s : foo.ea | enki.vm
	./enki.vm ./foo.ea >foo_32.s

$(UNIT_TESTS:%.gcc=%.x) : libEnki_32.a

libEnki_32.a : $(OBJS:%_n.o=%_32.o)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(OBJS:%_n.o=%_32.o)
	$(RANLIB) $@
	@touch $@

obj :: $(OBJS)
obj :: $(MAINS:%.c=.objects/%_32.o)

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
.PHONY :: checkpoint
.PHONY :: clean
.PHONY :: clear
.PHONY :: depends
.PHONY :: enki
.PHONY :: install
.PHONY :: install.bin
.PHONY :: install.inc
.PHONY :: install.lib
.PHONY :: lib
.PHONY :: obj
.PHONY :: scrub
.PHONY :: test
.PHONY :: units
.PHONY :: FORCE

##
## rules
##

.depends  : ; @mkdir .depends
.assembly : ; @mkdir .assembly
.objects  : ; @mkdir .objects
.dumps    : ; @mkdir .dumps
.run      : ; @mkdir .run

.run/%.log : %.ea | .run
	@./run.it $(ENKI.test) "$(RUNFLAGS)" $< $@

## === m32 c-compile ===

.PRECIOUS :: .dumps/%_32.s .objects/%_32.o .assembly/%_32.s

%_32.x : .objects/%_32.o
	$(GCC) $(CFLAGS) -m32 -o $@ $+ libEnki_32.a

.dumps/%_32.s : .objects/%_32.o | .dumps 
	@objdump --disassemble-all -x $< >$@

.objects/%_32.o : .assembly/%_32.s | .objects
	@$(AS) $(ASFLAGS) --32 -o $@ $< 

.assembly/%_32.s : %.c | .assembly
	$(GCC) $(SFLAGS) -S -m32 -fverbose-asm -o $@ $<

## ## ## ##

.objects/%_32.o : %_32.s | .objects
	$(AS) $(ASFLAGS) --32 -o $@ $< 

## ## ## ##

.PRECIOUS :: .dumps/%_atom.s .objects/%_atom.o .assembly/%_atom.s

.dumps/%_atom.s : .objects/%_atom.o | .dumps
	@objdump --disassemble-all -x $< >$@

.objects/%_atom.o : .assembly/%_atom.s | .objects
	@$(AS) $(ASFLAGS) --32 -o $@ $< 	

.assembly/%_atom.s : ./buildins/%.c | .assembly
	$(GCC) $(SFLAGS) -S -m32 -fverbose-asm -o $@ $<

## ## ## ##

.depends/%.d : %.c | .depends
	@$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

-include $(DEPENDS)



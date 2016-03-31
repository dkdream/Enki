PREFIX  = /tools/Enki
BINDIR  = $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

TIME := $(shell date +T=%s.%N)
ARCH := $(uname  --machine)

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

TAILFLAGS := -fexpensive-optimizations
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
TAILFLAGS += -fdelete-null-pointer-checks

#TAILFLAGS += -Wformat-security
##TAILFLAGS += -fconserve-stack

RUNFLAGS := 

XCFLAGS  := -mregparm=3
INCFLAGS := -I. $(COPPER_INC)
DBFLAGS  := -Wall -rdynamic -fPIC
CFLAGS   := $(DBFLAGS) $(INCFLAG) $(TAILFLAGS)
SFLAGS   := -rdynamic -O -m64 $(TAILFLAGS)
ASFLAGS  := -Qy --64
LIBFLAGS := $(COPPER_LIB)
ARFLAGS  := rcu

MAINS      := enki_main.c $(notdir $(wildcard link_*.c))
FOOS       := $(notdir $(wildcard foo_*.c))
C_SOURCES  := $(filter-out $(MAINS) $(FOOS) $(CTESTS),$(notdir $(wildcard *.c)))
H_SOURCES  := $(filter-out enki.h, $(notdir $(wildcard *.h)))
ATOMS      := $(wildcard ./atoms/*.c)
C_BUILDINS := $(notdir $(wildcard ./buildins/*.c))

OBJS := $(C_SOURCES:%.c=.objects/%.o)
OBJS += $(C_BUILDINS:%.c=.objects/%_buildin.o)
TSTS := $(notdir $(wildcard test_*.ea))
RUNS := $(TSTS:test_%.ea=.run/test_%.log)

DEPENDS := $(C_SOURCES:%.c=.depends/%.d)
DEPENDS += $(MAINS:%.c=.depends/%.d)
DEPENDS += $(C_BUILDINS:%.c=.depends/%_buildin.d)

UNIT_TESTS := $(notdir $(wildcard test_*.gcc))

all   :: enki test asm
enki  :: enki.vm | lib
lib   :: libEnki.a
test  :: $(RUNS)
asm   :: $(FOOS:%.c=.dumps/%.s)
units :: $(UNIT_TESTS:%.gcc=%.x)
	@ls -l $(UNIT_TESTS:%.gcc=%.x)

atoms ::  $(ATOMS:./atoms/%.c=.dumps/%_atom.s)
	@echo finished atoms

install : install.bin install.inc install.lib

install.bin :: $(BINDIR)/enki
install.inc :: $(H_SOURCES:%=$(INCDIR)/%)
install.lib :: $(LIBDIR)/libEnki.a

checkpoint : ; git checkpoint
pull       : ; git pull
push       : ; git push

depends : $(DEPENDS)

$(RUNS) : | enki.vm

clear ::
	rm -rf .run

clean ::
	rm -fr .depends .objects .assembly .run .dumps
	rm -f enki.vm libEnki.a libEnki_32.a libEnki.a
	rm -f *~ ./#* *.x *.s *.o buildins/SUBR.lst
	rm -f test.*.out test.out

scrub :: 
	@make clean
	@rm -rf .depends

enki.vm : .objects/enki_main.o libEnki.a 
	$(GCC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

test :: link_main.x
	./link_main.x

test :: $(FOOS:%.c=.dumps/%.s)

.assembly/link_main.s : link_main.c | .assembly
	$(GCC) -rdynamic  -O -m64 -S -fverbose-asm -o $@ $<

link_main.x : .objects/link_main.o .objects/foo_alloc.o libEnki.a
	$(GCC) $(CFLAGS) -o $@ .objects/link_main.o .objects/foo_alloc.o -L. -lEnki

foo.s : foo.ea compiler.ea | enki.vm
	./enki.vm ./foo.ea >foo.s

$(UNIT_TESTS:%.gcc=%.x) : libEnki.a

libEnki.a : $(OBJS:%_n.o=%.o)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(OBJS:%_n.o=%.o)
	$(RANLIB) $@
	@touch $@

obj :: $(OBJS)
obj :: $(MAINS:%.c=.objects/%.o)

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

echo_begin :: ; @echo begin atoms

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
.PHONY :: echo_begin

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

## ## ## ##

.PRECIOUS :: .dumps/%.s .objects/%.o .assembly/%.s

%.x : .objects/%.o
	$(GCC) $(CFLAGS) -o $@ $+ libEnki.a

.objects/%.o : .assembly/%.s | .objects
	@$(AS) $(ASFLAGS) -o $@ $<

.objects/%.o : %.s | .objects
	$(AS) $(ASFLAGS) -o $@ $< 

.assembly/%.s : %.c | .assembly
	$(GCC) $(SFLAGS) -S -fverbose-asm -o $@ $<

.assembly/%_buildin.s : ./buildins/%.c | .assembly
	$(GCC) $(SFLAGS) -I. -S -fverbose-asm -o $@ $<

$(FOOS:%.c=.assembly/%.s) : .assembly/foo_%.s : foo_%.c  | .assembly
	$(GCC) -rdynamic  -O -m64 -S -fverbose-asm -o $@ $<

.dumps/%.s : .objects/%.o | .dumps
	@objdump --disassemble-all -x $< >$@

.dumps/%_atom.s : ./atoms/%.c | .dumps
	$(GCC) -rdynamic -O -m64 -S -fverbose-asm -o $@ $<

$(FOOS:%.c=.dumps/%.s) : .dumps/foo_%.s : foo_%.c  | .dumps
	$(GCC) -rdynamic  -O -m64 -S -fverbose-asm -o $@ $<

.depends/%.d : %.c | .depends
	@$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

.depends/%_buildin.d : ./buildins/%.c | .depends
	@$(GCC) $(CFLAGS) -MM -MP -MG -MF $@ $<

## ## ## ##

.assembly/kernal.s : buildins/SUBR.lst

buildins/SUBR.lst : $(C_BUILDINS:%.c=./buildins/%.c)
	@./mk_subr.sh

## ## ## ##

-include $(DEPENDS)



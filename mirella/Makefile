#
# Mirella's Makefile.
#
# Allow platform-specific values
# 
ifeq ($(shell uname), Darwin)
   include Makefile.darwin
else ifeq ($(shell uname), Linux)
   include Makefile.linux
endif

A = .a
#
DMAKE = 1
LINUX = 1
#
MIRELSRC = .
RANLIB = /usr/bin/ranlib
SHELL = /bin/sh
#
X11_INCLUDE_DIR ?= /usr/include
X11_LIB_DIR ?= /usr/lib

LIBMIRELLA = libmirella$A
LIBSM = -L/u/rhl/sm/src -lplotsub -ldevices -lutils $(LIBX11)
LIBX11 =  -L$(X11_LIB_DIR) -lX11 
#
# Used by make install
#
DESTDIR ?= /home/jeg/mirella
BINDIR = $(DESTDIR)bin
ETCDIR = $(DESTDIR)etc
LIBDIR = $(DESTDIR)lib
DICTFILE = mirella.dic 
#
#ZORTECH = 1
# CC = gcc -Wall -Wno-uninitialized -fwritable-strings
# CC = gcc -Wall -Wno-uninitialized
# try optimization
OPT ?= -O2
CC = gcc -Wall -Wno-uninitialized -Wno-unused-result -fno-strict-aliasing $(OPT)
CFLAGS = -DDOT_MIRELLA=\\".mirella\\"
CFLAGS += -I$(X11_INCLUDE_DIR)
LDFLAGS =  -L$(MIRELSRC) -lmirella $(LIBX11) -lm
MINSTALL = ./minstall
#CFLAGS += -I/usr/openwin/include
#LDFLAGS += -L/usr/openwin/lib
#
 : $(LIBMIRELLA) fonts.bin
#
CFLAGS += -g
#
OBJS =  mirelic.o mirelec.o mirel1c.o mirel2c.o mirel3c.o \
	cmain.o dynamic.o mirelmem.o \
	mirhelp.o mirutils.o \
	mirarray.o mirgdev.o mirexpr.o mirdebug.o \
	mirorpol.o mirmatrx.o mirsplin.o mirameba.o mirhist.o \
	mirdir.o mirmain.o \
	history.o hardchar.o \
	qsort.o memmove.o mirsys.o primit.o \
	mirgraph.o mirpnm.o \
	images.o imageapp.o \
	mirast.o timer.o mirmathf.o mirrand.o \
	sm.o smutils.o $(DEVICES)
OBJS = mirelic.o mirelec.o mirel1c.o mirel2c.o mirel3c.o \
	cmain.o dynamic.o mirelmem.o \
	mirhelp.o mirutils.o \
	mirarray.o mirgdev.o mirexpr.o mirdebug.o \
	mirorpol.o mirmatrx.o mirsplin.o mirameba.o mirhist.o \
	mirdir.o mirmain.o \
	history.o hardchar.o \
	qsort.o memmove.o mirsys.o primit.o \
	mirgraph.o mirpnm.o \
	images.o imageapp.o \
	mirast.o timer.o mirmathf.o mirrand.o  \
	sm.o smutils.o $(DEVICES) $(ZDBOBJS)
#
# ASM_OBJS = cchandlr.o realint.o trap386.o vgafunc.o
DEVICES = gm.o graph32.o video.o vmsptk.o x11.o vga.o
DEVICES = video.o x11.o
#
I_MSC_FILES = intern.msc mirgraph.msc images.msc sm.msc
#
# Application files. $(ZDBOBJS) are usually in libmirella, but extracted
# for the sake of the debugger
#
ZDBOBJS = mirella.o mirellio.o mirellin.o mirplot.o mirgdis.o \
          termcap.o displays.o cursors.o mirelf.o mircintf.o  \
          mirscrn.o editor.o
AOBJS = $(MSC_OBJS)
#AOBJS += $(ZDBOBJS)
#
MSC_OBJS = $(E_OBJ) $(1_OBJ) $(2_OBJ) $(3_OBJ)
#
# Example of adding applications, but please don't do so in this directory!
#
E_MSC_FILES = valid.msc mappl.msc
E_OBJ = valid.o mappl.o
#
1_MSC_FILES = 
1_OBJ = 
#
# 1_MSC_FILES = arcacc.msc
# 1_OBJ = arcacc.o mirrs232.o
#
# 2_MSC_FILES = dyn.msc
# 2_OBJ = 
#
# 3_MSC_FILES = 
# 3_OBJ = 
#
mirella : Mirella
	@- echo 
Mirella : cmirella mirsys.c
	@- echo 
all : cmirella fonts
#
cmirella : $(AOBJS) $(LIBMIRELLA)
	 $(CC) -o cmirella $(AOBJS) $(LDFLAGS)
#
# Dependencies for mirsys.c
#
mirsys.c : mirunix.c 
# mir386zt.c
	touch $@
#
# The Mirella Library
#
$(LIBMIRELLA) : $(OBJS)
	ar r $(LIBMIRELLA) $?
	if test "$(RANLIB)" != "" -a -x "$(RANLIB)"; then \
		$(RANLIB) $(LIBMIRELLA); fi
#
# Make the init file for primitives
#
makeprim  : makeprim.c
	$(CC) -o makeprim makeprim.c
primit.c  : makeprim minit.x
	./makeprim minit.x primit.c
#
# Internal (i), external (e) and secondary (1, 2, 3) C-words
#
mirelic.c  : $(I_MSC_FILES) minstall mirelec.skl
	( \
	echo building mirelic.c from $(I_MSC_FILES); \
	$(MINSTALL) -ikqw -s mirelec.skl -o mirelic.c $(I_MSC_FILES); \
	)
#
mirelec.c  : $(E_MSC_FILES) minstall mirelec.skl
	( \
	echo building mirelec.c from $(E_MSC_FILES); \
	$(MINSTALL) -ekqw -s mirelec.skl -o mirelec.c $(E_MSC_FILES); \
	)
mirel1c.c  : $(1_MSC_FILES) minstall mirelec.skl
	( \
	echo building mirel1c.c from $(1_MSC_FILES); \
	$(MINSTALL) -1kqw -s mirelec.skl -o mirel1c.c $(1_MSC_FILES); \
	)
mirel2c.c  : $(2_MSC_FILES) minstall mirelec.skl
	( \
	echo building mirel2c.c from $(2_MSC_FILES); \
	$(MINSTALL) -2kqw -s mirelec.skl -o mirel2c.c $(2_MSC_FILES); \
	)
mirel3c.c  : $(3_MSC_FILES) minstall mirelec.skl
	( \
	echo building mirel3c.c from $(3_MSC_FILES); \
	$(MINSTALL) -3kqw -s mirelec.skl -o mirel3c.c $(3_MSC_FILES); \
	)
#
# And the dummies that go in libMirella for other users:
#
mirsetupe.o : minstall mirelec.skl
	@ $(MINSTALL) -ekq -s mirelec.skl -o mirsetupe.c
	$(CC) -c $(CFLAGS) mirsetup1.c
	@ $(AR) $(ARFLAGS) $(LIBMIRELLA) $(AR_OBJFLAG) mirsetupe.o
	@- rm mirsetupe.[co]
mirsetup1.o : minstall mirelec.skl
	@ $(MINSTALL) -1kq -s mirelec.skl -o mirsetup1.c
	$(CC) -c $(CFLAGS) mirsetup1.c
	@ $(AR) $(ARFLAGS) $(LIBMIRELLA) $(AR_OBJFLAG) mirsetup1.o
	@- rm mirsetup1.[co]
mirsetup2.o : minstall mirelec.skl
	@ $(MINSTALL) -2kq -s mirelec.skl -o mirsetup2.c
	$(CC) -c $(CFLAGS) mirsetup2.c
	@ $(AR) $(ARFLAGS) $(LIBMIRELLA) $(AR_OBJFLAG) mirsetup2.o
	@- rm mirsetup2.[co]
mirsetup3.o : minstall mirelec.skl
	@ $(MINSTALL) -3kq -s mirelec.skl -o mirsetup3.c
	$(CC) -c $(CFLAGS) mirsetup3.c
	@ $(AR) $(ARFLAGS) $(LIBMIRELLA) $(AR_OBJFLAG) mirsetup3.o
	@- rm mirsetup3.[co]
#
# Dependencies for .msc files:
#
$(I_MSC_FILES) $(E_MSC_FILES) $(1_MSC_FILES) $(2_MSC_FILES) $(3_MSC_FILES) :: \
				minstall.def
	touch $@
#minstall.def : options.h
#	touch $@
#
# this makes the program that installs C-words into Mirella
#
 : minstall.o mirexpr.oo
mirexpr.oo : mirexpr.c
	if [ ! -f mirexpr.o ]; then \
		$(CC) -c $(CFLAGS) mirexpr.c; \
	fi
	touch mirexpr.oo
minstall : minstall.o mirexpr.oo
	( \
	ar r $(LIBMIRELLA) mirexpr.o; \
	if test "$(RANLIB)" != "" -a -x "$(RANLIB)"; then \
		$(RANLIB) $(LIBMIRELLA); fi; \
	$(CC) minstall.o -o minstall $(LDFLAGS); \
	)
#
# this makes the standalone help program
# 
shelp.o: mirhelp.c mirella.h
	@ $(CP) mirhelp.c shelp.c
	$(CC) -c $(CFLAGS) -DSTANDALONE shelp.c 
	@ rm shelp.c
help: shelp.o mirsys.o
	$(CC) shelp.o mirsys.o -o shelp
#
# Header file dependencies
#
# o486zt.h arc386.h used to be deps in msysdef.h
msysdef.h: unix.h 
	touch msysdef.h
mirella.h: options.h msysdef.h
	touch mirella.h
mirkern.h: mirella.h
	touch mirkern.h
#
# Install a working copy of mirella somewhere
#
install :
	- $(CP) cmirella $(BINDIR)
	- if [ -f $(DICTFILE) ]; then $(CP) $(DICTFILE) $(ETCDIR); fi
	- $(CP) *.m dotokens.h mfont.dat $(ETCDIR)
	- ( cd ../utils; $(MAKE) "CC=$(CC)" \
		DESTDIR=$(DESTDIR) BINDIR=$(BINDIR) LIBDIR=$(LIBDIR) install; )
#
# Clean up directory
#
clean : tidy
	( \
	rm -f *.o *.obj *.dic fonts.bin read_fonts cmirella *.exp \
	minstall mirexpr.oo libmirella.a libmirel.lib \
	mirel[e123i]c.c makeprim primit.c; \
	)
tidy :
	( \
	rm -f *~ *.orig *.rej  *.bak *.log *.map core mirsetup[123].c \
	history.sav emacs.old; \
	)
#
# Make (emacs) tags file for Mirella objects (C and Forth)
#
tags :
	etags -et `echo $(OBJS) $(AOBJS) | sed -e 's/\.o/.c/g'`
	mtags -f TAGS -A -a -C -c -v -m *.{m,msc}
#
# Make a command script
#
script :
	@ echo You should run \"make clean\" first if you want a complete build
	@ echo "#!/bin/sh" > compile.com
	@ echo "#" >> compile.com
	@ echo "# Script prepared: `date`" >> compile.com
	@ echo "#" >> compile.com
	@ echo "CFLAGS=-g" >> compile.com
	@ echo "CC=cc" >> compile.com
	@ make CC=\$$\$$CC CFLAGS=\$$\$$CFLAGS -n mirella | \
		sed	-e '/.* is up to date\./d' \
			-e '/^echo .*$$/d' \
			-e '/^touch .*$$/d' \
			-e '/^[	 ]*\\$$/d' \
		>> compile.com
#
# make Makefile
#
# make :
#	make_make -u -f makefile.mk $(OBJS) $(AOBJS) \
#	minstall.o \
#	-O mireldec.h -O declare.h -O \\/dsigraph\\/libr786.h 
#
# dependencies
#
#START_DEPEND
cmain.o :	mirella.h
cmain.o :       mireldec.h
dynamic.o :	dld.h
dynamic.o :	mirella.h
mirelmem.o :	mirkern.h
mirelmem.o :    mireldec.h
mirhelp.o :	edit.h
mirhelp.o :	edit.h
mirhelp.o :	mirella.h
mirhelp.o :	scrbuf.h
mirutils.o :	mirella.h
mirarray.o :	mirella.h
mirgdev.o :	mirella.h
mirdebug.o :	mirella.h
mirorpol.o :	mirella.h
mirmatrx.o :	dotokens.h
mirmatrx.o :	mirella.h
mirsplin.o :	mirella.h
mirameba.o :	mirella.h
mirhist.o :	mirella.h
mirdir.o :	mirella.h
mirmain.o :	mirella.h
mirmathf.o :    mirella.h
mirrand.o :     mirella.h
history.o :	charset.h
history.o :	edit.h
history.o :	mirella.h
history.o :     mireldec.h
hardchar.o :	mirella.h
qsort.o :	mirella.h
mirsys.o :	mirunix.c
mirsys.o :	options.h
primit.o :	mircintf.h
mirgraph.o :	graphs.h
mirgraph.o :	mirella.h
mirpnm.o :      graphs.h
mirpnm.o :      mirella.h
images.o :	images.h
images.o :	mirella.h
imageapp.o :	images.h
imageapp.o :	mirella.h
sm.o :	        mirella.h
smutils.o :	mirella.h
smutils.o :	tty.h
gm.o :	        images.h
gm.o :	        mirella.h
graph32.o :	images.h
graph32.o :	mirella.h
video.o :	images.h
video.o :	mirella.h
vmsptk.o :	images.h
vmsptk.o :	mirella.h
x11.o :	        cursors.h
x11.o :	        icon.h
x11.o :	        images.h
x11.o :	        mirella.h
vga.o :	        images.h
vga.o :	        mirella.h
mirella.o :     dotokens.h
mirella.o :	images.h
mirella.o :	mircintf.h
mirella.o :	mirkern.h
mirella.o :	mprims.h
mirella.o :	options.h
mirellio.o :	dld.h
mirellio.o :	dotokens.h
mirellio.o :	mirkern.h
mirellio.o :    mireldec.h
mirellin.o :	dotokens.h
mirellin.o :	mirkern.h
mirellin.o :	mprims.h
mirellin.o :    mireldec.h
mirplot.o :	graphs.h
mirplot.o :	mirella.h
mirgdis.o :	edit.h
mirgdis.o :	graphs.h
mirgdis.o :	images.h
mirgdis.o :	mirella.h
termcap.o :	edit.h
termcap.o :	mirella.h
displays.o :	images.h
displays.o :	mirella.h
cursors.o :	images.h
cursors.o :	mirella.h
mirelf.o :	mirkern.h
mircintf.o :	dotokens.h
mircintf.o :	mircintf.h
mircintf.o :	mirkern.h
mircintf.o :	mprims.h
mircintf.o :    mireldec.h
dosterm.o :	edit.h
dosterm.o :	mirella.h
dosterm.o :	scrbuf.h
mirscrn.o :	edit.h
mirscrn.o :	mirella.h
mirscrn.o :	scrbuf.h
editor.o :	charset.h
editor.o :	edit.h
editor.o :	mirella.h
editor.o :	tty.h
mirelic.o :	mircintf.h
mirelic.o :	mircintf.h
mirelic.o :	mirella.h
mirelic.o :	mirkern.h
mirelic.o :	images.h
mirelec.o :	mircintf.h
mirelec.o :	mircintf.h
mirelec.o :	mirella.h
mirelec.o :	mirkern.h
mirel1c.o :	mircintf.h
mirel1c.o :	mircintf.h
mirel1c.o :	mirella.h
mirel1c.o :	mirkern.h
mirel2c.o :	mircintf.h
mirel2c.o :	mircintf.h
mirel2c.o :	mirella.h
mirel2c.o :	mirkern.h
mirel3c.o :	mircintf.h
mirel3c.o :	mircintf.h
mirel3c.o :	mirella.h
mirel3c.o :	mirkern.h
mirage.o :	mirella.h
valid.o :	mirella.h
minstall.o :	mircintf.h
minstall.o :	mirella.h

############################################################################
# Makefile for delta encode/decode library and sample program
#
# $Id: Makefile,v 1.2 2009/05/02 06:14:44 michael Exp $
# $Log: Makefile,v $
# Revision 1.2  2009/05/02 06:14:44  michael
# Refactor for easy changing of the rules for adjusting code size.
#
# Revision 1.1.1.1  2009/04/17 04:35:52  michael
# Initial release
#
############################################################################
CC = gcc
LD = gcc
CFLAGS = -O3 -Wall -Wextra -pedantic -ansi -c
LDFLAGS = -O3 -o

# libraries
LIBS = -L. -ldelta -loptlist -lbitfile

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
    OS = Windows
endif

ifeq ($(OS),Windows)
    EXE = .exe
    DEL = del
else    #assume Linux/Unix
    EXE =
    DEL = rm
endif

all:        sample$(EXE)

sample$(EXE):   sample.o libdelta.a libbitfile.a liboptlist.a
	$(LD) $< $(LIBS) $(LDFLAGS) $@

sample.o:   sample.c delta.h optlist.h
	$(CC) $(CFLAGS) $<

libdelta.a:  delta.o adapt.o
	ar crv $@ $^
	ranlib $@

delta.o: delta.c delta.h adapt.h bitfile.h
	$(CC) $(CFLAGS) $<

adapt.o:  adapt.c adapt.h
	$(CC) $(CFLAGS) $<

libbitfile.a:   bitfile.o
	ar crv $@ $^
	ranlib $@

bitfile.o:  bitfile.c bitfile.h
	$(CC) $(CFLAGS) $<

liboptlist.a:   optlist.o
	ar crv $@ $^
	ranlib $@

optlist.o:  optlist.c optlist.h
	$(CC) $(CFLAGS) $<

clean:
	$(DEL) *.o
	$(DEL) *.a
	$(DEL) sample$(EXE)

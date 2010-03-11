############################################################################
# Makefile for delta encode/decode library and sample program
#
# $Id: Makefile,v 1.1.1.1 2009/04/17 04:35:52 michael Exp $
# $Log: Makefile,v $
# Revision 1.1.1.1  2009/04/17 04:35:52  michael
# Initial release
#
############################################################################
CC = gcc
LD = gcc
CFLAGS = -O3 -Wall -pedantic -ansi -c
LDFLAGS = -O3 -o

# libraries
LIBS = -L. -ldelta -loptlist

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

sample$(EXE):   sample.o libdelta.a liboptlist.a
	$(LD) $^ $(LIBS) $(LDFLAGS) $@

sample.o:   sample.c delta.h optlist.h
	$(CC) $(CFLAGS) $<

libdelta.a:  delta.o bitfile.o
	ar crv libdelta.a delta.o bitfile.o
	ranlib libdelta.a

delta.o: delta.c delta.h bitfile.h
	$(CC) $(CFLAGS) $<

bitfile.o:  bitfile.c bitfile.h
	$(CC) $(CFLAGS) $<

liboptlist.a:   optlist.o
	ar crv liboptlist.a optlist.o
	ranlib liboptlist.a

optlist.o:  optlist.c optlist.h
	$(CC) $(CFLAGS) $<

clean:
	$(DEL) *.o
	$(DEL) *.a
	$(DEL) sample$(EXE)

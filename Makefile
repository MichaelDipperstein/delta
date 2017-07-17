############################################################################
# Makefile for delta encode/decode library and sample program
############################################################################
CC = gcc
LD = gcc
CFLAGS = -O3 -Wall -Wextra -pedantic -ansi -c
LDFLAGS = -O3 -o

# libraries
LIBS = -L. -Lbitfile -Loptlist -ldelta -loptlist -lbitfile

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
    OS = Windows
endif

ifeq ($(OS),Windows)
    EXE = .exe
    DEL = del
else    #assume Linux/Unix
    EXE =
    DEL = rm -f
endif

all:	sample$(EXE)

sample$(EXE):   sample.o libdelta.a bitfile/libbitfile.a optlist/liboptlist.a
	$(LD) $< $(LIBS) $(LDFLAGS) $@

sample.o:   sample.c delta.h optlist/optlist.h
	$(CC) $(CFLAGS) $<

libdelta.a:  delta.o adapt.o
	ar crv $@ $^
	ranlib $@

delta.o: delta.c delta.h adapt.h bitfile/bitfile.h
	$(CC) $(CFLAGS) $<

adapt.o:  adapt.c adapt.h
	$(CC) $(CFLAGS) $<

bitfile/libbitfile.a:
		cd bitfile && $(MAKE) libbitfile.a

optlist/liboptlist.a:
		cd optlist && $(MAKE) liboptlist.a

clean:
	$(DEL) *.o
	$(DEL) *.a
	$(DEL) sample$(EXE)
	cd optlist && $(MAKE) clean
	cd bitfile && $(MAKE) clean

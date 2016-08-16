#############################################################
# makefile for ARIBAS under UNIX
# author: Otto Forster <forster@mathematik.uni-muenchen.de>
# date: 2003-06-09
#############################################################

CC = gcc
CFLAGS = -DUNiX -g -ggdb
# for some compilers it might be necessary to remove the -O flag
# If you have the GNU gcc compiler on your system, you may wish
# to use gcc. Then outcomment the first two lines and activate 
# the next two lines.

#CC = gcc
#CFLAGS = -DUNiX -DPROTO -O -v

MEMFLAG1 = -DMEM=6
# MEM may be set to any integer value from 1 to 16.
# This is the size of the ARIBAS heap in Megabytes
# The value should not exceed one half of the RAM of your machine
# If MEMFLAG is not defined, MEM=2 will be used by default

MEMFLAG2 = -DINTSIZE=64
MEMFLAG = $(MEMFLAG1) $(MEMFLAG2)

OBJECTS = alloc.o analysis.o aritaux.o arith.o aritool0.o \
          aritools.o arity.o aritx.o aritz.o array.o control.o \
          errtext.o  eval.o file.o mainloop.o mem0.o parser.o print.o \
          scanner.o storage.o syntchk.o sysdep.o terminal.o 

PROGRAM = aribas

all:	$(PROGRAM)

$(OBJECTS):	common.h

terminal.o:	logscr.inc

.c.o:
	$(CC) $(CFLAGS) -c $<

alloc.o:    alloc.c common.h
	$(CC) $(CFLAGS) $(MEMFLAG) -c alloc.c

$(PROGRAM): $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS)
	strip $(PROGRAM)
clean:
	\rm *.o

############################ EOF ##############################

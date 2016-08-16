# --------------------------------------------------------------------
# This source distribution is placed in the public domain by its author,
# Jason Papadopoulos. You may use it for any purpose, free of charge,
# without having to notify anyone. I disclaim any responsibility for any
# errors.
# 
# Optionally, please be nice and tell me if you find this source to be
# useful. Again optionally, if you add to the functionality present here
# please consider making those additions public too, so that others may 
# benefit from your work.	
#        				   --jasonp@boo.net 6/19/05
# --------------------------------------------------------------------

# xlc on AIX; note that apparently a 64-bit binary crashes
# CC = xlc
# OPT_FLAGS = -O2 
# MACHINE_FLAGS = -DRS6K -qmaxmem=8192 -q32
#
# gcc on Apple G5; for 64-bit mode, add '-m64'
# CC = gcc
# OPT_FLAGS = -fast
# WARN_FLAGS = -Wall -W -Wconversion

# gcc for x86 and x86-64
CC = gcc
OPT_FLAGS = -O3 -fomit-frame-pointer
# MACHINE_FLAGS = -march=pentium4
# MACHINE_FLAGS = -march=k8
MACHINE_FLAGS = -march=athlon
WARN_FLAGS = -Wall -W -Wconversion
LIBFLAGS = -lpthread

CFLAGS = $(OPT_FLAGS) $(MACHINE_FLAGS) $(WARN_FLAGS) -Iinclude

HDR = \
	lib/msieve_int.h \
	include/msieve.h \
	include/mp.h \
	include/util.h

SRCS = 	  \
	lib/gf2.c \
	lib/mp.c \
	lib/msieve.c \
	lib/poly.c \
	lib/relation.c \
	lib/sieve.c \
	lib/sqrt.c \
	lib/squfof.c \
	lib/util.c

OBJS = $(SRCS:.c=.o)

all: $(OBJS)
	ar r libmsieve.a $(OBJS)
	ranlib libmsieve.a
	$(CC) $(CFLAGS) qsieve_mex.c -o msieve -lm libmsieve.a $(LIBFLAGS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f msieve $(OBJS) libmsieve.a

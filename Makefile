#  $Id: Makefile,v 1.5 2019-02-06 03:45:05 minmin Exp $
#

  SRCS = memspeed.c
  OBJS = memspeed.o
  PROG = memspeed
    RM = rm -f
CFLAGS = -O2
LDLIBS =

# -O2 is safe here.  The copy loop reads the buffers through volatile pointers
# as well, and an optimizing compiler may neither drop nor move such a read, so
# the memcpy() calls that are being measured cannot be optimized away any more.
# The optimization level does change how a copy ends up being carried out, so
# 'make testnoopt' builds the same source with an empty CFLAGS and runs the
# same cases next to each other for comparison.  To check the source for
# warnings, use
#   make CFLAGS='-O2 -Wall -Wextra -pedantic'
# because a CFLAGS given on the command line replaces the one above, and set
# LDLIBS=-lrt where clock_gettime() is not part of libc.

all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

clean:
	$(RM) $(OBJS) $(PROG)

test: $(PROG)
	./$(PROG) 10 10
	./$(PROG) 128k 1000
	./$(PROG) 256k 2000
	./$(PROG) -8 4 100

# The same cases measured without optimization, to compare with the default
# -O2 build; the plain objects are removed again at the end.
testnoopt: clean
	$(MAKE) CFLAGS=
	./$(PROG) 10 10
	./$(PROG) 128k 1000
	./$(PROG) 256k 2000
	./$(PROG) -8 4 100
	$(MAKE) clean

.PHONY: all clean test testnoopt

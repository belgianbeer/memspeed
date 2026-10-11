#  $Id: Makefile,v 1.5 2019-02-06 03:45:05 minmin Exp $
#

  SRCS = memspeed.c
  OBJS = memspeed.o
  PROG = memspeed
    RM = rm -f
CFLAGS =
LDLIBS =

# CFLAGS is left empty on purpose.  An optimizing compiler may replace a
# memcpy() call by something faster than a memory copy, which is exactly what
# this program tries to measure.  To check the source for warnings, use
#   make CFLAGS='-Wall -Wextra -pedantic'
# and set LDLIBS=-lrt where clock_gettime() is not part of libc.

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

.PHONY: all clean test

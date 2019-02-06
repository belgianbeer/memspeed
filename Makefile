#  $Id: Makefile,v 1.5 2019/02/06 03:45:05 minmin Exp $
#

  SRCS = memspeed.c
  OBJS = memspeed.o
  PROG = memspeed
    RM = rm -f
CFLAGS =

all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	$(RM) $(OBJS) $(PROG)

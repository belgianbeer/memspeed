#  $Id: Makefile,v 1.4 2003/08/05 04:58:19 minmin Exp $
#

  SRCS = memspeed.c
  OBJS = memspeed.o
  PROG = memspeed
    RM = rm -f
CFLAGS = -O

all:	$(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	$(RM) $(OBJS) $(PROG)

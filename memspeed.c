/*
 *  memspeed.c
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

static char Id[] = "$Id: memspeed.c,v 1.6 1999/01/27 09:19:45 minmin Exp $";

void
main (int argc, char **argv)
{
    char buf[100];
    char *k;
    struct timeval  begin_time, end_time;
    struct timezone tz = {0, 0};
    void  *src, *dst;
    int  i, s;
    size_t count, mem, memsize;
    double  totaltime;

    if (argc != 3)
    {
	fputs (
	    "Usage: memspeed size count\n"
	    "\tsize[k]: allocate size (Mbyte in default)\n"
	    "\t  count: repeat count\n",
	    stderr);
	exit (2);
    }

    if ((mem = (size_t)atoi(argv[1])) <= 0)
    {
	mem = 1;
    }

    memsize = mem * 1024 * (((k = strchr(argv[1], 'k')) == NULL) ?  1024: 1 );

    if ((count = (size_t)atoi(argv[2])) <= 0)
    {
	count = 1;
    }

    if (((src = malloc(memsize)) == NULL) ||
	((dst = malloc(memsize)) == NULL) )
    {
	fprintf (stderr, "Can't alloc memory\n");
	exit (1);
    }

    bzero(src, memsize);
    bzero(dst, memsize);

    s = gettimeofday(&begin_time, &tz);

    for (i = 0; i < count ; i ++)
	memcpy (src, dst, memsize);

    s = gettimeofday(&end_time, &tz);

    if (end_time.tv_usec < begin_time.tv_usec) {
	end_time.tv_usec += 1000000;
	end_time.tv_sec --;
    }

    sprintf (buf, "%ld.%06ld", 
	    end_time.tv_sec - begin_time.tv_sec,
	    end_time.tv_usec - begin_time.tv_usec);

    totaltime = atof(buf);
    printf ("copy %ld%s at %ld times (total %dM + %dK) "
	    "in %ld.%06ld sec  %d Kbyte/sec\n",
	    mem, k == NULL? "M": "K", count, 
	    (memsize * count) >> 20,
	    (((memsize * count) | 0x100 ) >> 10) & 0x3ff ,
	    end_time.tv_sec - begin_time.tv_sec,
	    end_time.tv_usec - begin_time.tv_usec,
	    (long)((double)(memsize * count) / totaltime + .5) >> 10
	    );
    
    exit (0);
}

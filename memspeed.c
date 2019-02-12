/*
 *  memspeed.c
 *
 *  benchmark program in memcpy
 *
 *    example:
 *
 *    memspeed 10 10 
 *       copy 10 Mbyte in 10 times
 *
 *    memspeed 128k 1000
 *       copy 128 kbyte in 10 times
 *
 *      Written by Masato Minda <minmin@wide.ad.jp>
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

static char Id[] = "$Id: memspeed.c,v 1.12 2019/02/12 05:08:50 minmin Exp $";

void usage(void);

int
main (int argc, char **argv)
{
    char buf[100];
    char *k;
    struct timeval  begin_time, end_time;
    struct timezone tz = {0, 0};
    void  *src, *dst;
    int  i, s, offset = 0;
    size_t count, mem, memsize;
    int64_t totalsize;
    double  totaltime;

    while (argc >= 2 && *argv[1] == '-')
    {
	offset = atoi(argv[1] + 1);
	--argc;
	++argv;
    }

    if (argc != 3)
    {
	usage ();
    }

    if ((mem = (size_t)atoi(argv[1])) <= 0)
    {
	mem = 1;
    }

    memsize = mem * 1000 * (((k = strchr(argv[1], 'k')) == NULL) ?  1000: 1 );

    if ((count = (size_t)atoi(argv[2])) <= 0)
    {
	count = 1;
    }

    if (((src = malloc(memsize)) == NULL) ||
	((dst = malloc(memsize + offset)) == NULL) )
    {
	fprintf (stderr, "Can't alloc memory\n");
	exit (1);
    }

    memset(src, 0, memsize);
    memset(dst, 0, memsize);

    s = gettimeofday(&begin_time, &tz);

    for (i = 0; i < count ; i ++)
	memcpy (dst + offset, src, memsize);

    s = gettimeofday(&end_time, &tz);

    if (end_time.tv_usec < begin_time.tv_usec) {
	end_time.tv_usec += 1000000;
	end_time.tv_sec --;
    }

    sprintf (buf, "%ld.%06ld", 
	    (long)(end_time.tv_sec - begin_time.tv_sec),
	    end_time.tv_usec - begin_time.tv_usec);

    totalsize = (int64_t)memsize * (int64_t)count;
    totaltime = atof(buf);
    printf ("copy %ld%s at %ld times (total %dM + %dK) "
	    "in %ld.%06ld sec  %ld Kbyte/sec\n",
	    (long int)mem,
	    k == NULL? "M": "K",
	    (long int)count, 
	    (int)(totalsize >> 20),
	    (int)(((totalsize | 0x100 ) >> 10) & 0x3ff),
	    (long)(end_time.tv_sec - begin_time.tv_sec),
	    end_time.tv_usec - begin_time.tv_usec,
	    (long int)((int64_t)((double)totalsize / totaltime + .5) >> 10)
	    );
    
    exit (0);
}

void usage()
{
    fputs (
	"Usage: memspeed size count\n"
	"\tsize[k]: allocate size (Mbyte in default)\n"
	"\t  count: repeat count\n",
	stderr);
    exit (1);
}

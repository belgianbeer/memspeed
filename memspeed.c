#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

void
main (int argc, char **argv)
{
    struct timeval  begin_time, end_time;
    struct timezone tz = {0, 0};
    void *src, *dst;
    int  i, s;
    int  count, memsize;

    if (argc != 3)
    {
	fputs (
	    "Usage: memtest size count\n"
	    "\tsize[k]: allocate size\n"
	    "\t  count: repeat count\n",
	    stderr);
	exit (2);
    }

    if ((memsize = (int)atoi(argv[1])) <= 0)
    {
	memsize = 1;
    }

    memsize *= 1024 * ((strchr(argv[1], 'k') == NULL) ?  1024: 1 );

    if ((count = (int)atoi(argv[2])) <= 0)
    {
	count = 1;
    }

    if (((src = malloc(memsize)) == NULL) ||
	((dst = malloc(memsize)) == NULL) )
    {
	fprintf (stderr, "Can't alloc memory\n");
	exit (1);
    }

    s = gettimeofday(&begin_time, &tz);

    for (i = 0; i < count ; i ++)
	memcpy (src, dst, memsize);

    s = gettimeofday(&end_time, &tz);

    if (end_time.tv_usec < begin_time.tv_usec) {
	end_time.tv_usec += 1000000;
	end_time.tv_sec --;
    }

    printf ("time is %ld.%03ld\n", end_time.tv_sec - begin_time.tv_sec,
	    end_time.tv_usec - begin_time.tv_usec);
    
    exit (0);
}

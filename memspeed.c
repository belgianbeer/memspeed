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
 *       copy 128 kbyte in 1000 times
 *
 *    memspeed -8 4 100
 *       copy 4 Mbyte in 100 times, destination shifted by 8 bytes
 *
 *      Written by Masato Minda <minmin@wide.ad.jp>
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 1999-2026, Masato Minda <minmin@wide.ad.jp>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

char Id[] = "$Id: memspeed.c,v 1.15 2020-11-19 23:58:20 minmin Exp $";

/*
 * Every unit is decimal (1 KB = 1000 bytes, 1 MB = 1000 KB) so that the MB/sec
 * figure reported at the end is just the byte rate divided by 1000000.
 */
#define KB_SIZE	(1000ULL)
#define MB_SIZE	(1000000ULL)
#define GB_SIZE	(1000000000ULL)

void usage(void);
static int parse_size(const char *str, uint64_t *valuep,
					  const char **unitp, uint64_t *bytesp);
static double now(void);

int main(int argc, char **argv)
{
	char *end;
	char *src = NULL, *dst = NULL;
	const volatile char *vsrc;	/* volatile views of the last byte */
	volatile char *vdst;		/* of each buffer, see the loop below */
	const char *unit = "MB";
	char speed[32];
	uint64_t mem, memsize, count, totalsize, offset = 0;
	uint64_t copies;
	uint64_t i;
	double begin, elapsed;
	size_t bytes;

	/* "-N" shifts the destination by N bytes to measure a misaligned copy */
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0')
	{
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		{
			usage();
		}
		errno = 0;
		offset = (uint64_t)strtoull(argv[1] + 1, &end, 10);
		if (end == argv[1] + 1 || *end != '\0' || errno != 0 ||
			offset > (uint64_t)SIZE_MAX)
		{
			fprintf(stderr, "memspeed: bad offset '%s'\n", argv[1] + 1);
			usage();
		}
		--argc;
		++argv;
	}

	if (argc != 3)
	{
		usage();
	}

	if (parse_size(argv[1], &mem, &unit, &memsize) != 0)
	{
		fprintf(stderr, "memspeed: bad size '%s' (try 10, 128k or 1g)\n", argv[1]);
		return 1;
	}

	errno = 0;
	count = (uint64_t)strtoull(argv[2], &end, 10);
	if (end == argv[2] || *end != '\0' || errno != 0 || count == 0 ||
		count > UINT64_MAX / memsize)
	{
		fprintf(stderr, "memspeed: bad count '%s'\n", argv[2]);
		return 1;
	}

	if (memsize > (uint64_t)SIZE_MAX - offset)
	{
		fprintf(stderr, "memspeed: size is too large for this platform\n");
		return 1;
	}
	bytes = (size_t)memsize;

	if (((src = malloc(bytes)) == NULL) ||
		((dst = malloc(bytes + (size_t)offset)) == NULL))
	{
		fprintf(stderr, "memspeed: Can't alloc memory (%llu bytes): %s\n",
				(unsigned long long)(bytes + (size_t)offset), strerror(errno));
		free(src);
		free(dst);
		return 1;
	}

	/*
	 * Seed the buffers with a non-zero pattern.  Untouched pages of a fresh
	 * malloc() are commonly backed by the shared zero page, so an all-zero
	 * buffer would let part of the copy avoid real memory traffic.
	 */
	memset(src, 0xA5, bytes);
	memset(dst, 0xA5, bytes + (size_t)offset);

	/*
	 * A volatile view of the last byte of each buffer.  Reading through a
	 * volatile-qualified pointer is a side effect the compiler has to keep,
	 * and the value read from the destination can only equal the one read
	 * from the source if the memcpy() below really ran.  That is what stops
	 * an optimizing compiler from noticing that every iteration repeats the
	 * same copy of data nobody reads, and deleting the whole loop -- which
	 * used to leave an elapsed time of exactly 0 sec under -O2.  As a side
	 * benefit the comparison is a copy integrity check.
	 */
	vsrc = (const volatile char *)src + bytes - 1;
	vdst = (volatile char *)dst + offset + bytes - 1;

	begin = now();

	copies = count;
	for (i = 0; i < count; i++)
	{
		memcpy(dst + offset, src, bytes);
		if (*vdst != *vsrc)		/* the barrier; never taken */
		{
			fprintf(stderr, "memspeed: copy mismatch after %llu of %llu copies\n",
					(unsigned long long)(i + 1),
					(unsigned long long)count);
			copies = i + 1;
			break;
		}
	}

	elapsed = now() - begin;
	totalsize = memsize * copies;
	if (elapsed < 0.0)			/* the wall clock stepped backwards */
	{
		elapsed = 0.0;
	}

	if (elapsed > 0.0)
	{
		double mb_per_sec = (double)totalsize / elapsed / (double)MB_SIZE;

		if (mb_per_sec >= 1e18)		/* unmeasurably short interval */
			snprintf(speed, sizeof(speed), "too fast");
		else
			snprintf(speed, sizeof(speed), "%llu",
					 (unsigned long long)(mb_per_sec + 0.5));
	}
	else
	{
		snprintf(speed, sizeof(speed), "N/A");
	}

	if (elapsed <= 0.0)
	{
		/* the volatile barrier keeps this from happening by -O */
		fprintf(stderr, "memspeed: nothing measurable was copied;"
				" use a larger size or count\n");
	}
	else if (elapsed < 0.001)
	{
		fprintf(stderr, "memspeed: warning: only %.6f sec measured;"
				" use a larger size or count\n", elapsed);
	}

	printf("copy %llu %s at %llu times (total %llu MB + %llu KB) "
		   "in %.6f sec  %s MB/sec\n",
		   (unsigned long long)mem,
		   unit,
		   (unsigned long long)count,
		   (unsigned long long)(totalsize / MB_SIZE),
		   (unsigned long long)((totalsize % MB_SIZE) / KB_SIZE),
		   elapsed,
		   speed);

	free(src);
	free(dst);
	return 0;
}

void usage(void)
{
	fputs(
		"Usage: memspeed [-offset] size count\n"
		"\t-offset: shift the destination by offset bytes\n"
		"\t   size: allocate size (k/KB, m/MB, g/GB; a bare number is MB)\n"
		"\t  count: repeat count\n",
		stderr);
	exit(1);
}

/*
 * Parse a size such as "10", "128k", "512kb" or "1G" into the number shown in
 * the report, the unit it is printed with, and the byte count it stands for.
 * A bare number keeps its historical meaning of megabytes, and the base is
 * always 1000 - never 1024.  Returns 0 on success, -1 if malformed or too big.
 */
static int parse_size(const char *str, uint64_t *valuep,
					  const char **unitp, uint64_t *bytesp)
{
	const char *p = str;
	char *end;
	unsigned long long value;
	uint64_t scale;
	const char *unit;
	int has_suffix = 1;

	if (*p < '0' || *p > '9')
	{
		return -1;
	}

	errno = 0;
	value = strtoull(p, &end, 10);
	if (end == p || errno != 0 || value == 0)
	{
		return -1;
	}
	p = end;

	switch (*p)
	{
	case 'k':
	case 'K':
		scale = KB_SIZE;
		unit = "KB";
		break;
	case 'm':
	case 'M':
		scale = MB_SIZE;
		unit = "MB";
		break;
	case 'g':
	case 'G':
		scale = GB_SIZE;
		unit = "GB";
		break;
	default:
		has_suffix = 0;
		scale = MB_SIZE;
		unit = "MB";
		break;
	}

	if (has_suffix)
	{
		p++;
		if (*p == 'b' || *p == 'B')	/* "128kb" is accepted as "128k" */
		{
			p++;
		}
	}
	if (*p != '\0' || (uint64_t)value > UINT64_MAX / scale)
	{
		return -1;
	}

	*valuep = (uint64_t)value;
	*unitp = unit;
	*bytesp = (uint64_t)value * scale;
	return 0;
}

/*
 * Seconds since an unspecified origin.  CLOCK_MONOTONIC is preferred because it
 * resolves to nanoseconds: a microsecond wall clock easily reports 0 sec for a
 * short run, which is how the old code ended up dividing totalsize by zero.
 * Compile with -DUSE_GETTIMEOFDAY to force the traditional implementation, or
 * with -lrt on the older systems where clock_gettime() is not in libc.
 */
static double now(void)
{
#if defined(CLOCK_MONOTONIC) && !defined(USE_GETTIMEOFDAY)
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
	{
		return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0);
	}
#endif
	{
		struct timeval tv;

		gettimeofday(&tv, NULL);
		return ((double)tv.tv_sec + (double)tv.tv_usec / 1000000.0);
	}
}
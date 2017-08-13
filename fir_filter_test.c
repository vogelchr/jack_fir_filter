#include "fir_filter.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NSAMPLES 1024

int32_t inbuf[NSAMPLES];
int32_t outbuf[NSAMPLES];

int
main(int argc, char **argv)
{
	struct fir_filter *p;
	unsigned int k, i;
	int32_t v;
	FILE *f;
	int infd, outfd;
	unsigned long Ns;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s tap_config infile outfile\n", argv[0]);
		exit(1);
	}

	if ( (f=fopen(argv[1],"r")) == NULL) {
		perror(argv[1]);
		exit(1);
	}

	if (fscanf(f, "%u", &k) != 1) {
		perror(argv[1]);
		exit(1);
	}

	printf("Creating filter for %u taps.\n", k);
	p = fir_filter_create(k);

	for (i=1; i<k; i++) {
		if (fscanf(f, "%d", &v) != 1) {
			perror(argv[1]);
			exit(1);
		}
		printf("a[%u]=%10d (%f)\n", i, v, (float)v/(float)(1<<24));
		fir_filter_set_a(p, i, v);
	}

	for (i=0; i<k; i++) {
		if (fscanf(f, "%d", &v) != 1) {
			perror(argv[1]);
			exit(1);
		}
		printf("b[%u]=%10d (%f)\n", i, v, (float)v/(float)(1<<24));
		fir_filter_set_b(p, i, v);
	}

	if ((infd = open(argv[2], O_RDONLY)) == -1) {
		perror(argv[2]);
		exit(1);
	}

	if ((outfd = open(argv[3], O_WRONLY|O_CREAT|O_EXCL, 0666)) == -1) {
		perror(argv[3]);
		exit(1);
	}

	Ns = 0;
	while (1) {
		ssize_t r, t;
		unsigned long nsamples;

		r = read(infd, inbuf, sizeof(inbuf));
		if (r == -1) {
			perror(argv[2]);
			exit(1);
		}

		if (r == 0) {
			fprintf(stderr, "End of input.\n");
			break;
		}

		if (r & (sizeof(int32_t)-1)) {
			fprintf(stderr, "Error: input not multiple of 4 bytes!\n");
			exit(1);
		}
		nsamples = (unsigned long)r / sizeof(int32_t);;
		Ns += nsamples;

		fir_filter_process_buf(p, inbuf, outbuf, nsamples);

		t = write(outfd, outbuf, r);
		if (t == -1) {
			perror(argv[3]);
			exit(1);
		}

		if (t != r) {
			fprintf(stderr,"Incomplete write to output file.\n");
			perror(argv[3]);
			exit(1);
		}

	}

	printf("Processed %lu samples.\n", Ns);

	if (close(infd) == -1) {
		perror(argv[2]);
		exit(1);
	}

	if (close(outfd) == -1) {
		perror(argv[3]);
		exit(1);
	}

	return 0;
}
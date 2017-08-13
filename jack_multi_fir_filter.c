/* based heavily on thru_client.c and simple_client.c from
   the jack audio connection kit examples */

#include "jack_interface.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

static const float fct_float_to_int = (float)(1<<24);
static const float fct_int_to_float = 1.0 / (float)(1<<24);

size_t   buf_fixed_size=0;
int32_t *inbuf_fixed=NULL;
int32_t *outbuf_fixed=NULL;

int
buffer_process_fixpoint(
	unsigned long nsamples,
	const int32_t * restrict inbuf,
	int32_t * restrict outbuf
) {
	memcpy(outbuf, inbuf, sizeof(*inbuf)*nsamples);
	return 0;
}

int
buffer_process(
	unsigned long nsamples,
	const float * restrict inbuf,
	float * restrict outbuf
) {
	unsigned long k;
	int ret;

	if (nsamples != buf_fixed_size) {
		size_t allocsz = sizeof(*inbuf_fixed) * nsamples;
		free(inbuf_fixed);  /* guaranteed to be a NOOP if arg == NULL */
		free(outbuf_fixed);
		inbuf_fixed = malloc(allocsz);
		outbuf_fixed = malloc(allocsz);
		buf_fixed_size = nsamples;
		fprintf(stderr, "Allocated buffers for %lu samples/frame.\n", nsamples);
	}

	for (k=0; k<nsamples; k++)
		inbuf_fixed[k] = lrintf(inbuf[k] * fct_float_to_int);
	ret = buffer_process_fixpoint(nsamples, inbuf_fixed, outbuf_fixed);
	for (k=0; k<nsamples; k++)
		outbuf[k] = outbuf_fixed[k] * fct_int_to_float;

	return ret;
}

void
usage(char *argv0)
{
	printf("Usage: %s [options]\n", argv0);
}

int
main(int argc, char **argv)
{
	int i;

	while ((i=getopt(argc, argv, "i:o:h")) != -1) {
		switch (i) {
		case 'h':
			usage(argv[0]);
			exit(1);
		}
	}

	jack_interface_set_cb(buffer_process);

	if (jack_interface_connect(argv[0], NULL) == -1)
		exit(1);

	printf("Current sample rate is %lu Hz.\n",
		jack_interface_framerate());

	while (1) {
		if (!jack_interface_is_alive(NULL))
			break;
		sleep(1);
	}
	exit(0);
}

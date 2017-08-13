#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <stdint.h>

struct fir_filter ;

extern struct fir_filter *
fir_filter_create(unsigned int ntaps);

extern void
fir_filter_set_a(struct fir_filter *p, unsigned int i, int32_t a);

extern void
fir_filter_set_b(struct fir_filter *p, unsigned int i, int32_t b);

extern int32_t
fir_filter_process_sample(struct fir_filter *p, int32_t x);

extern void
fir_filter_process_buf(
	struct fir_filter *p,
	const int32_t * restrict inbuf,
	int32_t * restrict outbuf,
	unsigned long buflen
);


#endif
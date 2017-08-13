#include "fir_filter.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FIR_B(p) ((p)->_data                   )
#define FIR_A(p) ((p)->_data + (p)->ntaps   - 1) /* see below for -1 */
#define FIR_V(p) ((p)->_data + (p)->ntaps*2 - 2) /* see below for -2 */

struct fir_filter {
	unsigned int ntaps;
	int32_t _data[0];
	/*
	   int32_t b[0], b[1], ... b[N-1]
	   int32_t       a[1], ... a[N-1] !!! starts at 1 !!!
	   int32_t       v[1], ... v[N-1] !!! starts at 1 !!!
	*/
};

struct fir_filter *
fir_filter_create(unsigned int ntaps) {
	struct fir_filter *p;
	size_t alloc_sz;

	alloc_sz = sizeof(struct fir_filter) + (3*ntaps-2)*sizeof(int32_t);
	p = malloc(alloc_sz);
	bzero(p, alloc_sz);

	p->ntaps = ntaps;

	return p;
}

void
fir_filter_set_a(struct fir_filter *p, unsigned int i, int32_t a) {
	if (i == 0) /* we only have a[1]..a[N-1] */
		return;
	if (i >= p->ntaps)
		return;
	FIR_A(p)[i] = -a;
}

void
fir_filter_set_b(struct fir_filter *p, unsigned int i, int32_t b) {
	if (i >= p->ntaps)
		return;
	FIR_B(p)[i] = b;
}

inline int32_t
fir_filter_fix_mult(int32_t a, int32_t b) {
	int64_t tmp;

	tmp = (int64_t)a * (int64_t)b;
	return (int32_t)(tmp >> 24);
}

extern inline int32_t
fir_filter_process_sample(struct fir_filter *p, int32_t x) {
	/*
	see https://ccrma.stanford.edu/~jos/filters/Direct_Form_II.html

	Nomenclature is different for delay elements, what the
	graph calls
		v(n-i) is called V[i] in the code [for i>0]
		v(n)   is called s                [ corresponds to i==0]

	     left col of     right col of
	     adders sums     adders sums
	     variable "s"    variable y

	                 s
	x ---->(+)-------+--B[0]->(+)----> y
	        ^        |         ^ 
	        |        V         |
	        |       V[1]       |
	       (+)<-A[1]-+--B[1]->(+)
	        ^        |         ^ 
	        |        V         |
	        |       V[2]       |
	       (+)<-A[2]-+--B[2]->(+)
	        ^        |         ^
	        |        V         |
	        +--A[3]-V[3]-B[3]--+
	*/

	int32_t y, s;
	unsigned int k;

	y = s = 0;
	for (k = p->ntaps-1; /*break*/; k--) {
		if (k == 0) {
			s += /* A[0] == 1.0 */   x;
			y += fir_filter_fix_mult(FIR_B(p)[k],s);
			if (p->ntaps >= 2)
				FIR_V(p)[1] = s;
			break;
		} else {
			s += fir_filter_fix_mult(FIR_A(p)[k],FIR_V(p)[k]);
			y += fir_filter_fix_mult(FIR_B(p)[k],FIR_V(p)[k]);			
		}

		/* update delay line for k = 1..N-1 */
		if (k > 1)
			FIR_V(p)[k] = FIR_V(p)[k-1];
	}

	return y;
}

void
fir_filter_process_buf(
	struct fir_filter *p,
	const int32_t * restrict inbuf,
	int32_t * restrict outbuf,
	unsigned long buflen
) {
	unsigned long i;

	for (i=0; i<buflen; i++)
		outbuf[i] = fir_filter_process_sample(p, inbuf[i]);
}


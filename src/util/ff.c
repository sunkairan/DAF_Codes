/*
  Finite field implementation using intel SSE 

  Portions derived from code by Phil Karn (karn@ka9q.ampr.org),
  Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) and Hari
  Thirumoorthy (harit@spectra.eng.hawaii.edu), Aug 1995
  Luigi Rizzo (luigi@iet.unipi.it)
  Jack Lloyd (lloyd@randombit.net), 2009
  Hu Ming (mh@ie.cuhk.edu.hk), 2012
  (C) 2012 Institute of Newtwork coding, CUHK, Hong Kong

*/

#include "platform.h"
#include "ff.h"

#include <emmintrin.h>
#include <assert.h>
#define PRIM_POLY 0435 /* octal */
#define ORDER 8
#define NUM_ELEMENTS 256
#define DIV_BY_ZERO -1

static int GF_MUL_TABLE[NUM_ELEMENTS * NUM_ELEMENTS];
static int GF_DIV_TABLE[NUM_ELEMENTS * NUM_ELEMENTS];

static void ff_swap(uint8_t *a, uint8_t *b, uint32_t size)
{
	uint8_t tmp;
	uint32_t i;

	for (i = 0; i < size; i++) {
		tmp = *a;
		*a = *b;
		*b = tmp;

		a++;
		b++;
	}
	
}

void ff_init()
{
	int *logf;
	int *expf;

	int j, b;

	/* create logf/expf table */
	logf = (int *)malloc(NUM_ELEMENTS * sizeof(int));
	if (!logf) {
		fprintf(stderr, "ff_init: logf malloc error\n");
		return;
	}

	expf = (int *)malloc(NUM_ELEMENTS * 4 * sizeof(int));
	if (!expf) {
		fprintf(stderr, "ff_init: expf malloc error\n");
		free(logf);
		return;
	}

	for (j = 0; j < NUM_ELEMENTS; j++) {
		logf[j] = NUM_ELEMENTS - 1;
		expf[j] = 0;
	}

	b = 1;
	for (j = 0; j < NUM_ELEMENTS - 1 ; j++) {
		if (logf[b] != NUM_ELEMENTS -1) {
			fprintf(stderr, "ff_init: create log error\n");
			free(logf);
			free(expf);
			return;
		}
		logf[b] = j;
		expf[j] = b;
		b = b << 1;
		if (b & NUM_ELEMENTS)
			b = (b ^ PRIM_POLY) & (NUM_ELEMENTS - 1);

	}

	logf[0] = 2 * (NUM_ELEMENTS - 1);
	
	for (j = 0; j < NUM_ELEMENTS - 1; j++) {
		expf[j + NUM_ELEMENTS - 1] = expf[j];
		expf[j + 2 * (NUM_ELEMENTS - 1)] = 0;
		expf[j + 3 * (NUM_ELEMENTS - 1)] = 0;
	}

	/* create mul/div table */
	j = 0;
	GF_MUL_TABLE[j] = 0;
	GF_DIV_TABLE[j] = DIV_BY_ZERO;

	j++;
	int x, y;
	uint8_t log_x;
	for (y = 1; y < NUM_ELEMENTS; y++) {
		GF_MUL_TABLE[j] = 0;
		GF_DIV_TABLE[j] = 0;
		j++;
	}

	for (x = 1; x < NUM_ELEMENTS; x++) {
		GF_MUL_TABLE[j] = 0;
		GF_DIV_TABLE[j] = DIV_BY_ZERO;
		j++;
		log_x = logf[x];
		for (y = 1; y < NUM_ELEMENTS; y++) {
			GF_MUL_TABLE[j] = expf[log_x + logf[y]];
			GF_DIV_TABLE[j] = expf[log_x + NUM_ELEMENTS - 1 - logf[y]];
			j++;
		}
	}
	
	free(logf);
	free(expf);

	//d_log("ff_init\n");
}



int ff_mul(int x, int y)
{
	return GF_MUL_TABLE[(x << ORDER) | y];
}


/* z[] = y*x[] */
void ff_mulv(uint8_t *z, const uint8_t *x, uint8_t y, uint32_t size)
{
	int i, j, y_index;
	//d_log("ff_mulv: z = 0x%x, x = 0x%x, y = 0x%x, size = %d\n",
	//     z, x, y, size);

	if (y == 0) {
		//printf("ff_mulv: y = 0\n");
		memset(z, 0, size);
		return;
	}

	y_index = y << ORDER;

#ifdef SSE_BOOST	

	while ((size != 0) && (((uintptr_t)z & 0x0f) != 0)) {
		z[0] = GF_MUL_TABLE[y_index | x[0]];
		z++;
		x++;
		size--;
	}

	const __m128i polynomial = _mm_set1_epi8(0x1D);
	const __m128i all_zeros = _mm_setzero_si128();
	const size_t y_bits = 32 - __builtin_clz(y);

	while(size >= 64) {
		__m128i x_1 = _mm_loadu_si128((const __m128i *)(x));
		__m128i x_2 = _mm_loadu_si128((const __m128i *)(x + 16));
		__m128i x_3 = _mm_loadu_si128((const __m128i *)(x + 32));
		__m128i x_4 = _mm_loadu_si128((const __m128i *)(x + 48));

		__m128i product_1 = all_zeros;
		__m128i product_2 = all_zeros;
		__m128i product_3 = all_zeros;
		__m128i product_4 = all_zeros;

		/* prefetch next two x and z blocks */
		_mm_prefetch(x + 64, _MM_HINT_T0);
		_mm_prefetch(x + 128, _MM_HINT_T0);


		if (y & 0x01) {
			product_1 = _mm_xor_si128(product_1, x_1);
			product_2 = _mm_xor_si128(product_2, x_2);
			product_3 = _mm_xor_si128(product_3, x_3);
			product_4 = _mm_xor_si128(product_4, x_4);
		}

		for (j = 1; j != y_bits; j++) {
			/* Each byte of each mask should be either 0 or the polynomial 0x1D, 
			   depending on if the high bit of x_1 is set or not */

			/* if all_zeros > x_1, then mask_1 is 0xff;
			 this means when x_1's high bit is set, mask_1 is 0xff, otherwise mask_1 is 0x00 */
			__m128i mask_1 = _mm_cmpgt_epi8(all_zeros, x_1);
		        __m128i mask_2 = _mm_cmpgt_epi8(all_zeros, x_2);
			__m128i mask_3 = _mm_cmpgt_epi8(all_zeros, x_3);
			__m128i mask_4 = _mm_cmpgt_epi8(all_zeros, x_4);

			x_1 = _mm_add_epi8(x_1, x_1);
			x_2 = _mm_add_epi8(x_2, x_2);
			x_3 = _mm_add_epi8(x_3, x_3);
			x_4 = _mm_add_epi8(x_4, x_4);

			/* mask_1 should be 0x00 or polynomial now*/
			mask_1 = _mm_and_si128(mask_1, polynomial);
			mask_2 = _mm_and_si128(mask_2, polynomial);
			mask_3 = _mm_and_si128(mask_3, polynomial);
			mask_4 = _mm_and_si128(mask_4, polynomial);

			x_1 = _mm_xor_si128(x_1, mask_1);
			x_2 = _mm_xor_si128(x_2, mask_2);
			x_3 = _mm_xor_si128(x_3, mask_3);
			x_4 = _mm_xor_si128(x_4, mask_4);

			if ((y >> j) & 1) {
				product_1 = _mm_xor_si128(product_1, x_1);
				product_2 = _mm_xor_si128(product_2, x_2);
				product_3 = _mm_xor_si128(product_3, x_3);
				product_4 = _mm_xor_si128(product_4, x_4);
			}


		}
		
		_mm_store_si128((__m128i *)(z), product_1);
		_mm_store_si128((__m128i *)(z + 16), product_2);
		_mm_store_si128((__m128i *)(z + 32), product_3);
		_mm_store_si128((__m128i *)(z + 48), product_4);

		x += 64;
		z += 64;
		size -= 64;
	}

#else

	while (size >= 16) {
		z[0] = GF_MUL_TABLE[y_index | x[0]];
		z[1] = GF_MUL_TABLE[y_index | x[1]];
		z[2] = GF_MUL_TABLE[y_index | x[2]];
		z[3] = GF_MUL_TABLE[y_index | x[3]];
		z[4] = GF_MUL_TABLE[y_index | x[4]];
		z[5] = GF_MUL_TABLE[y_index | x[5]];
		z[6] = GF_MUL_TABLE[y_index | x[6]];
		z[7] = GF_MUL_TABLE[y_index | x[7]];
		z[8] = GF_MUL_TABLE[y_index | x[8]];
		z[9] = GF_MUL_TABLE[y_index | x[9]];
		z[10] = GF_MUL_TABLE[y_index | x[10]];
		z[11] = GF_MUL_TABLE[y_index | x[11]];
		z[12] = GF_MUL_TABLE[y_index | x[12]];
		z[13] = GF_MUL_TABLE[y_index | x[13]];
		z[14] = GF_MUL_TABLE[y_index | x[14]];
		z[15] = GF_MUL_TABLE[y_index | x[15]];
		
		x += 16;
		z += 16;
		size -= 16;
	}
#endif

	for (i = 0; i < size; i++) {
	z[i] = GF_MUL_TABLE[y_index | x[i]];
}


}

/* x[] = y*x[] */
void ff_mulv_local(uint8_t *x, uint8_t y, uint32_t size)
{
	//d_log("ff_mulv_local: x = 0x%x, y = 0x%x, size = %d\n",
	//      x, y, size);

	int i, j, y_index;
	if (y == 0) {
		//d_log("ff_mulv_local: zero\n");
		memset(x, 0, size);
		return;
	}

	y_index = y << ORDER;

#ifdef SSE_BOOST	

	while ((size != 0) && (((uintptr_t)x & 0x0f) != 0)) {
		x[0] = GF_MUL_TABLE[(y << ORDER) | x[0]];
		x++;
		size--;
	}
	const __m128i polynomial = _mm_set1_epi8(0x1D);
	const __m128i all_zeros = _mm_setzero_si128();
	const size_t y_bits = 32 - __builtin_clz(y);


	while(size >= 64) {

		__m128i x_1 = _mm_loadu_si128((const __m128i *)(x));
		__m128i x_2 = _mm_loadu_si128((const __m128i *)(x + 16));
		__m128i x_3 = _mm_loadu_si128((const __m128i *)(x + 32));
		__m128i x_4 = _mm_loadu_si128((const __m128i *)(x + 48));

		__m128i product_1 = all_zeros;
		__m128i product_2 = all_zeros;
		__m128i product_3 = all_zeros;
		__m128i product_4 = all_zeros;

		/* prefetch next two x and z blocks */
		_mm_prefetch(x + 64, _MM_HINT_T0);
		_mm_prefetch(x + 128, _MM_HINT_T0);

		if (y & 0x01) {
			product_1 = _mm_xor_si128(product_1, x_1);
			product_2 = _mm_xor_si128(product_2, x_2);
			product_3 = _mm_xor_si128(product_3, x_3);
			product_4 = _mm_xor_si128(product_4, x_4);
		}

		for (j = 1; j != y_bits; j++) {
			/* Each byte of each mask is either 0 or the polynomial 0x1D, depending on if the high bit of x_1 is set or not */
			__m128i mask_1 = _mm_cmpgt_epi8(all_zeros, x_1);
		        __m128i mask_2 = _mm_cmpgt_epi8(all_zeros, x_2);
			__m128i mask_3 = _mm_cmpgt_epi8(all_zeros, x_3);
			__m128i mask_4 = _mm_cmpgt_epi8(all_zeros, x_4);

			x_1 = _mm_add_epi8(x_1, x_1);
			x_2 = _mm_add_epi8(x_2, x_2);
			x_3 = _mm_add_epi8(x_3, x_3);
			x_4 = _mm_add_epi8(x_4, x_4);

			mask_1 = _mm_and_si128(mask_1, polynomial);
			mask_2 = _mm_and_si128(mask_2, polynomial);
			mask_3 = _mm_and_si128(mask_3, polynomial);
			mask_4 = _mm_and_si128(mask_4, polynomial);

			x_1 = _mm_xor_si128(x_1, mask_1);
			x_2 = _mm_xor_si128(x_2, mask_2);
			x_3 = _mm_xor_si128(x_3, mask_3);
			x_4 = _mm_xor_si128(x_4, mask_4);

			if ((y >> j) & 1) {
				product_1 = _mm_xor_si128(product_1, x_1);
				product_2 = _mm_xor_si128(product_2, x_2);
				product_3 = _mm_xor_si128(product_3, x_3);
				product_4 = _mm_xor_si128(product_4, x_4);
			}

		}
		
		_mm_store_si128((__m128i *)(x), product_1);
		_mm_store_si128((__m128i *)(x + 16), product_2);
		_mm_store_si128((__m128i *)(x + 32), product_3);
		_mm_store_si128((__m128i *)(x + 48), product_4);

		x += 64;
		size -= 64;
	}

#else
	while (size >= 16) {
		x[0] = GF_MUL_TABLE[y_index | x[0]];
		x[1] = GF_MUL_TABLE[y_index | x[1]];
		x[2] = GF_MUL_TABLE[y_index | x[2]];
		x[3] = GF_MUL_TABLE[y_index | x[3]];
		x[4] = GF_MUL_TABLE[y_index | x[4]];
		x[5] = GF_MUL_TABLE[y_index | x[5]];
		x[6] = GF_MUL_TABLE[y_index | x[6]];
		x[7] = GF_MUL_TABLE[y_index | x[7]];
		x[8] = GF_MUL_TABLE[y_index | x[8]];
		x[9] = GF_MUL_TABLE[y_index | x[9]];
		x[10] = GF_MUL_TABLE[y_index | x[10]];
		x[11] = GF_MUL_TABLE[y_index | x[11]];
		x[12] = GF_MUL_TABLE[y_index | x[12]];
		x[13] = GF_MUL_TABLE[y_index | x[13]];
		x[14] = GF_MUL_TABLE[y_index | x[14]];
		x[15] = GF_MUL_TABLE[y_index | x[15]];
		
		x += 16;
		size -= 16;

	}
#endif
	for (i = 0; i < size; i++) {
		x[i] = GF_MUL_TABLE[y_index | x[i]];
	}

}



/* z[] = z[] + x[] */
void ff_addv_local(uint8_t *z, const uint8_t *x, uint32_t size)
{
	//d_log("ff_addv_local: z = 0x%x, x = 0x%x, size = %d\n",
	//     z, x, size);
	uint32_t i;

#ifdef SSE_BOOST
	__m128i x_1;
	__m128i x_2;
	__m128i x_3;
	__m128i x_4;
	__m128i z_1;
	__m128i z_2;
	__m128i z_3;
	__m128i z_4;

	while (size != 0 && ((uintptr_t)z & 0x0f) != 0) {
		z[0] = z[0] ^ x[0];
		z++;
		x++;
		size--;
	}

	while (size >= 64) {
		
		d_log("ff_addv_local: z = 0x%x, x = 0x%x\n", z, x);

		x_1 = _mm_loadu_si128((__m128i *)(x));
		x_2 = _mm_loadu_si128((__m128i *)(x + 16));
		x_3 = _mm_loadu_si128((__m128i *)(x + 32));
		x_4 = _mm_loadu_si128((__m128i *)(x + 48));

		z_1 = _mm_loadu_si128((__m128i *)(z));
		z_2 = _mm_loadu_si128((__m128i *)(z + 16));
		z_3 = _mm_loadu_si128((__m128i *)(z + 32));
		z_4 = _mm_loadu_si128((__m128i *)(z + 48));

		_mm_prefetch(x + 64, _MM_HINT_T0);
		_mm_prefetch(z + 64, _MM_HINT_T0);

		_mm_prefetch(x + 128, _MM_HINT_T1);
		_mm_prefetch(z + 128, _MM_HINT_T1);

		z_1 = _mm_xor_si128(x_1, z_1);
		z_2 = _mm_xor_si128(x_2, z_2);
		z_3 = _mm_xor_si128(x_3, z_3);
		z_4 = _mm_xor_si128(x_4, z_4);

		_mm_store_si128((__m128i *)(z), z_1);
		_mm_store_si128((__m128i *)(z + 16), z_2);
		_mm_store_si128((__m128i *)(z + 32), z_3);
		_mm_store_si128((__m128i *)(z + 48), z_4);

		x = x + 64;
		z = z + 64;
		size = size - 64;
	}


#else

	while (size >= 16) {

	z[0] = z[0] ^ x[0];
	z[1] = z[1] ^ x[1];
	z[2] = z[2] ^ x[2];
	z[3] = z[3] ^ x[3];
	z[4] = z[4] ^ x[4];
	z[5] = z[5] ^ x[5];
	z[6] = z[6] ^ x[6];
	z[7] = z[7] ^ x[7];
	z[8] = z[8] ^ x[8];
	z[9] = z[9] ^ x[9];
	z[10] = z[10] ^ x[10];
	z[11] = z[11] ^ x[11];
	z[12] = z[12] ^ x[12];
	z[13] = z[13] ^ x[13];
	z[14] = z[14] ^ x[14];
	z[15] = z[15] ^ x[15];

	z = z + 16;
	x = x + 16;
	size = size - 16;
}
#endif
	//d_log("ff_addv_local\n");
	for (i = 0; i < size; i++) {
		z[i] = z[i] ^ x[i];
	}

}

/* z[] = z[] + y*x[] */
void ff_add_mulv_local(uint8_t *z, const uint8_t *x, uint8_t y, uint32_t size)
{

	//d_log("ff_add_mulv_local: z = 0x%x, x = 0x%x, y = 0x%x, size = %d\n", 
	//     z, x, y, size);

	int i, j;
	int y_bits, y_index;

	if (y == 0) {

		return;
	}

	y_index = y << ORDER;

#ifdef SSE_BOOST
	/* align z to 16 bytes */
	while (size != 0 && ((uintptr_t)z & 0x0f) != 0) {
		z[0] ^= GF_MUL_TABLE[(y << ORDER) | x[0]];
		z++;
		x++;
		size--;
	}

	/* Mac OS's malloc will return address aligned on 16 bytes,
	 and can be used by SSE directly
	*/
	__m128i polynomial = _mm_set1_epi8(0x1D);
	__m128i all_zeros = _mm_setzero_si128();
	y_bits = 32 - __builtin_clz(y);

	/* unrolled out to cache line size;
	   64 byte is the L1 cache line size of Intel processor
	*/
	while(size >= 64) {
		__m128i x_1 = _mm_loadu_si128((const __m128i *)(x));
		__m128i x_2 = _mm_loadu_si128((const __m128i *)(x + 16));
		__m128i x_3 = _mm_loadu_si128((const __m128i *)(x + 32));
		__m128i x_4 = _mm_loadu_si128((const __m128i *)(x + 48));

		__m128i z_1 = _mm_loadu_si128((const __m128i *)(z));
		__m128i z_2 = _mm_loadu_si128((const __m128i *)(z + 16));
		__m128i z_3 = _mm_loadu_si128((const __m128i *)(z + 32));
		__m128i z_4 = _mm_loadu_si128((const __m128i *)(z + 48));

		/* prefetch next two x and z blocks */
		_mm_prefetch(x + 64, _MM_HINT_T0);
		_mm_prefetch(z + 64, _MM_HINT_T0);
		_mm_prefetch(x + 128, _MM_HINT_T1);
		_mm_prefetch(z + 128, _MM_HINT_T1);

		if (y & 0x01) {
			z_1 = _mm_xor_si128(z_1, x_1);
			z_2 = _mm_xor_si128(z_2, x_2);
			z_3 = _mm_xor_si128(z_3, x_3);
			z_4 = _mm_xor_si128(z_4, x_4);
		}

		for (j = 1; j < y_bits; j++) {

			/* Each byte of each mask should be either 0 or the polynomial 0x1D, 
			   depending on if the high bit of x_1 is set or not */

			/* if all_zeros > x_1, then mask_1 is 0xff;
			 this means when x_1's high bit is set, mask_1 is 0xff, otherwise mask_1 is 0x00 */

			__m128i mask_1 = _mm_cmpgt_epi8(all_zeros, x_1);
		        __m128i mask_2 = _mm_cmpgt_epi8(all_zeros, x_2);
			__m128i mask_3 = _mm_cmpgt_epi8(all_zeros, x_3);
			__m128i mask_4 = _mm_cmpgt_epi8(all_zeros, x_4);

			x_1 = _mm_add_epi8(x_1, x_1);
			x_2 = _mm_add_epi8(x_2, x_2);
			x_3 = _mm_add_epi8(x_3, x_3);
			x_4 = _mm_add_epi8(x_4, x_4);

			/* mask_1 should be 0x00 or polynomial now*/
			mask_1 = _mm_and_si128(mask_1, polynomial);
			mask_2 = _mm_and_si128(mask_2, polynomial);
			mask_3 = _mm_and_si128(mask_3, polynomial);
			mask_4 = _mm_and_si128(mask_4, polynomial);

			x_1 = _mm_xor_si128(x_1, mask_1);
			x_2 = _mm_xor_si128(x_2, mask_2);
			x_3 = _mm_xor_si128(x_3, mask_3);
			x_4 = _mm_xor_si128(x_4, mask_4);

			/* maybe we could eliminate this branch*/
			if ((y >> j) & 1) {
				z_1 = _mm_xor_si128(z_1, x_1);
				z_2 = _mm_xor_si128(z_2, x_2);
				z_3 = _mm_xor_si128(z_3, x_3);
				z_4 = _mm_xor_si128(z_4, x_4);
			}
		}
		
		_mm_store_si128((__m128i *)(z), z_1);
		_mm_store_si128((__m128i *)(z + 16), z_2);
		_mm_store_si128((__m128i *)(z + 32), z_3);
		_mm_store_si128((__m128i *)(z + 48), z_4);

		x += 64;
		z += 64;
		size -= 64;
	}

#else
	while (size >= 16) {
		z[0] ^= GF_MUL_TABLE[y_index | x[0]];
		z[1] ^= GF_MUL_TABLE[y_index | x[1]];
		z[2] ^= GF_MUL_TABLE[y_index | x[2]];
		z[3] ^= GF_MUL_TABLE[y_index | x[3]];
		z[4] ^= GF_MUL_TABLE[y_index | x[4]];
		z[5] ^= GF_MUL_TABLE[y_index | x[5]];
		z[6] ^= GF_MUL_TABLE[y_index | x[6]];
		z[7] ^= GF_MUL_TABLE[y_index | x[7]];
		z[8] ^= GF_MUL_TABLE[y_index | x[8]];
		z[9] ^= GF_MUL_TABLE[y_index | x[9]];
		z[10] ^= GF_MUL_TABLE[y_index | x[10]];
		z[11] ^= GF_MUL_TABLE[y_index | x[11]];
		z[12] ^= GF_MUL_TABLE[y_index | x[12]];
		z[13] ^= GF_MUL_TABLE[y_index | x[13]];
		z[14] ^= GF_MUL_TABLE[y_index | x[14]];
		z[15] ^= GF_MUL_TABLE[y_index | x[15]];

		z += 16;
		x += 16;
		size -= 16;

	}

#endif

	for (i = 0; i < size; i++) {
		z[i] ^= GF_MUL_TABLE[y_index | x[i]];
	}

}


/* a / b */
int ff_div(int a, int b)
{
	return GF_DIV_TABLE[(a << ORDER) | b];
}

/* optimize for sse later */
int ff_innerprod(const uint8_t *a, const uint8_t *b, const uint32_t size)
{
	uint8_t sum = 0;
	int i;
	//d_log("ff_innerprod: a = 0x%x, b = 0x%x, size = %d\n", a, b, size);

	for (i = 0; i < size; i++) {
		
		//d_log("ff_innerprod: a[%d] = 0x%x, b[%d] = 0x%x\n",
		//    i, a[i], i, b[i]);
		sum = ff_add(sum, ff_mul(a[i], b[i]));
	}

	return sum;
}

/* optimized for multi-thread operation in future */
uint32_t ff_rank(uint8_t **matrix, uint32_t rows, uint32_t columns)
{
	int r = 0;
	uint32_t i, j, i2;
	int non_zero = -1;
	uint32_t pkt_size = 16;
	uint32_t min = (columns < rows) ? columns : rows;

	//d_log("ff_rank\n");

	for (i = 0; i < min; i++) {
		while (1) {
			for (j = i; j < rows; j++) {
				if (matrix[j][r] != 0) {
					non_zero = j;
					break;
				}
			}

			if (non_zero < 0) {
				r++;
				if (r >= columns)
					return i;

				continue;
			}

			ff_swap(matrix[i], matrix[non_zero], pkt_size);
			break;
		}

		{
			int c = ff_div(1, matrix[i][r]);
			matrix[i][r] = 1;

			for (j = r + 1; j < columns; j++)
				matrix[i][j] = ff_mul(matrix[i][j], c);
		}


		for (i2 = i + 1; i2 < rows; i2++) {
			if (matrix[i2][r] != 0) {
				int c = matrix[i2][r];
				matrix[i2][r] = 0;

				for (j = r + 1; j < columns; j++)
					matrix[i2][j] = ff_sub(matrix[i2][j], ff_mul(matrix[i][j], c));
			}
		}


	}

	return i;
}


/* Ax = Y, A: a_rows*a_columns 
   optimized for multi-thread operation in future
*/
uint32_t ff_gaussian_elimination(uint8_t **matrix_a, uint8_t **matrix_y, 
				 uint32_t a_rows, uint32_t a_columns, 
				 uint32_t pkt_size)
{
	uint32_t i;
	uint32_t j;
	int i2;
	int non_zero_row = -1;

	//d_log("ff_gaussian_elimination\n");

	for (i = 0; i < a_columns; i++) {

		for (j = 1; j < a_rows; j++) {
			if (matrix_a[j][i] != 0) {
				non_zero_row = j;
				break;
			}
		}

		if (non_zero_row < 0)
			break;


		/* optimized for sse later */
		ff_swap(matrix_a[i], matrix_a[non_zero_row], 16);
		ff_swap(matrix_y[i], matrix_y[non_zero_row], 16);

		{
			uint8_t c = ff_div(1, matrix_a[i][i]);
			matrix_a[i][i] = 1;

			for (j = i + 1; j < a_columns; j++)
				matrix_a[i][j] = ff_mul(matrix_a[i][j], c);

			ff_mulv_local(matrix_y[i], c, pkt_size);

		}

		for (i2 = 0; i2 < a_rows; i2++) {
			if (i2 != i && matrix_a[i2][i] != 0) {
				int c = matrix_a[i2][i];
				matrix_a[i2][i] = 0;

				for (j = i + 1; j < a_columns; j++)
					matrix_a[i2][j] = ff_sub(matrix_a[i2][j], ff_mul(matrix_a[i][j], c));
				
				ff_add_mulv_local(matrix_y[i2], matrix_y[i], c, pkt_size);
			}
		}

	}

	return i;
}


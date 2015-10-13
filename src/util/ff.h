#ifndef __FINITE_FIELD__
#define __FINITE_FIELD__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void ff_init();

static inline int ff_add(int a , int  b)
{
	return a ^ b;
}

static inline int ff_sub(int a, int b)
{
	return a ^ b;
}

int ff_mul(int x, int y);

/* z[] = y*x[] */
void ff_mulv(uint8_t *z, const uint8_t *x, uint8_t y, uint32_t size);

/* x[] = y*x[] */
void ff_mulv_local(uint8_t *x, uint8_t y, uint32_t size);

/* z[] = z[] + x[] */
void ff_addv_local(uint8_t *z, const uint8_t *x, uint32_t size);

/* z[] = z[] + y*x[] */
void ff_add_mulv_local(uint8_t *z, const uint8_t *x, uint8_t y, uint32_t size);


int ff_div(int a, int b);

int ff_innerprod(const uint8_t *a, const uint8_t *b, const uint32_t size);

uint32_t ff_rank(uint8_t **matrix, uint32_t rows, uint32_t columns);

uint32_t ff_gaussian_elimination(uint8_t **matrix_a, uint8_t **matrix_y, 
				 uint32_t a_rows, uint32_t a_columns, 
				 uint32_t pkt_size);

#ifdef __cplusplus
}
#endif

#endif

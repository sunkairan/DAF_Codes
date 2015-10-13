#include <ff.h>
#include <assert.h>
#include "vand.h"

static struct vandermond_matrix matrix;

/* degree_index: [0, degree - 1]
   batch_index: [0, batch_size - 1]
 */
static int get(int degree_index, int batch_index)
{
	int row_index = degree_index % matrix.rows;
	assert(batch_index <= matrix.columns);

	return *(matrix.data + row_index*matrix.columns + batch_index);
}



/* 256 * batch_size matrix */
void init_vandermond_matrix(int batch_size)
{
	int i, j;
	int base;
	int pre_element;
	int pre_base;

	matrix.columns = batch_size;
	matrix.rows = 255;
	matrix.data = (int *)malloc(255*batch_size*sizeof(int));
	matrix.get = get;

	pre_base = 0;

	for (i = 0; i < matrix.rows; i++) {
		
		matrix.data[i*batch_size] = 1;

		base = i+1;
		pre_element = 1;
		for (j = 1; j < matrix.columns; j++) {

			*(matrix.data + i*batch_size + j) = ff_mul(base, pre_element);
			// vandermond_matrix[i*batch_size + j] = base * pre_element;
			pre_element = ff_mul(base, pre_element);
		}
	}

}

void destroy_vandermond_matrix()
{
	if (!matrix.data)
		free(matrix.data);
}


struct vandermond_matrix *get_vandermond_matrix(int batch_size)
{
	if (!matrix.data)
		init_vandermond_matrix(batch_size);

	return &matrix;
}

void print_vandermond_matrix()
{
	int i, j;
	printf("generator matrix: \n");
	for (i = 0; i < matrix.rows; i++) {
		for (j = 0; j < matrix.columns; j++) {
			// printf("%d\t", vandermond_matrix[i*vandmond_batch_size + j]);
			printf("%d\t", *(matrix.data + i*matrix.columns + j));
		}
		printf("\n");
	}
}
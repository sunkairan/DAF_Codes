#ifndef __VAND__H__
#define __VAND__H__


struct vandermond_matrix {
	int *data;
	int rows;
	int columns;

	int (*get) (int degree_index, int batch_index);
};

struct vandermond_matrix *get_vandermond_matrix(int batch_size);
void init_vandermond_matrix(int batch_size);
void destroy_vandermond_matrix();
void print_vandermond_matrix();

#endif
#include <ff.h>
#include <vand.h>

int main()
{
	int d_index = 0;
	int b_index = 0;

	struct vandermond_matrix *matrix;
	ff_init();
	matrix = get_vandermond_matrix(16);
	print_vandermond_matrix();
	printf("-----------------------------------------------\n");
	for (d_index = 0; d_index < 257; d_index++) {
		for (b_index = 0; b_index < 16; b_index++) {
			printf("%d\t", matrix->get(d_index, b_index));
		}
		printf("\n");
	}

	return 0;
}
 #include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "platform.h"
#include "ff.h"

#include "BatchEnc.h"
#include "BatchDec.h"

#include "bats.h"
#include <time.h>

int bats_test(int packet_size, int packet_num)
{
	int batch_size = 16;
	int ff_order = 8;
	int batch_num = 0;
	int num_inact = 0;
	int i;
	int j;
	BatsEncoder *encoder;
	BatsDecoder *decoder;
	symbol_t *input;
	symbol_t *output;

	symbol_t *batch;
	uint16_t batch_id;

	clock_t start;
	clock_t end;
	double time_spent;

	srand(7);

	/* init input  packets */
	input = (symbol_t *)malloc(packet_size * packet_num);
	if (!input) {
		d_error("input memory error\n");
		return -1;
	}
	for (i = 0; i < packet_size * packet_num; i++) {
		input[i] = rand() & 0xff;
		//input[i] = 0;
	}


	/* encode and decode */
	encoder = BatsEncoder_new(batch_size, packet_num, packet_size, input);
	if (!encoder) {
		d_error("encoder creation error\n");
		return -1;
	}

	BatsEncoder_selectDegree(encoder);
	



	output = (symbol_t *)malloc(packet_size * packet_num);
	if (!output) {
		d_error("output memory error\n");
		return -1;
	}
	for (i = 0; i < packet_size * packet_num; i++) {
		output[i] = 0xff;
	}



	decoder = BatsDecoder_new(batch_size, packet_num, packet_size, output);
	if (!decoder) {
		d_error("decoder creation error\n");
		return -1;
	}

	BatsDecoder_selectDegree(decoder);



	start = clock();
	while (1) {
		batch_num++;
		
		batch = BatsEncoder_genPacket(encoder);

		BatsDecoder_receivePacket(decoder, batch);

		if (BatsDecoder_complete(decoder, 1.0)) {
			num_inact = BatsDecoder_numInact(decoder);
			break;
		}

	}
	end = clock();


	for (i = 0; i < packet_size * packet_num; i++) {
		if (output[i] != input[i]) {
			d_printf("decode error\n");
			free(input);
			free(output);
			input = NULL;
			output = NULL;
			break;
		}
	}
	if (input)
		free(input);

	if (output)
		free(output);

	time_spent = (double)(end - start) / CLOCKS_PER_SEC;
	// d_printf("batch_num = %d, cycle = %ull, num_inact = %d\n", batch_num, end - start, num_inact);
	d_printf("=========================================\n");
	d_printf("batch_num = %d, num_inact = %d, time_spent = %f\n", batch_num, num_inact, time_spent);
	return 0;

}

int main(int argc, char *argv[])
{
	int packet_size = 1024;
	int packet_num = 0;

	int ret;

	bats_init();

	if (argc < 2) {
		printf("Input one parameter for packet-number\n");
		return 0;
	}

	packet_num = atoi(argv[1]);

	ret = bats_test(packet_size, packet_num);
	if (ret != 0)
		return -1;

	return 0;
}
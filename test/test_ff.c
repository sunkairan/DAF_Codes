#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "platform.h"
#include "ff.h"

#include "BatchEnc.h"
#include "BatchDec.h"

#include "bats.h"

int div_test()
{
	int a;
	int b;
	int c;
	int d;

	b = 10;
	for (a = 0; a < 256; a++) {

		for (b = 1; b < 256; b++) {
			c = ff_div(a, b);
			d = ff_mul(c, b);

			if (d != a) {
				printf("div != mul\n");
				return -1;
			}
		}

	}

	return 0;

}

int ff_test()
{
	symbol_t a = 0;
	symbol_t b = 0;
	symbol_t c;
	int i;

	for (i = 0; i <= 255; i++) {
		for (i = 0; i <= 255; i++) {
			if (ff_mul(a, b) != ff_mul(b, a)) {
				printf("mul error\n");
				return -1;
			}

			b++;
		}
		
		a++;
	}



	int len = 512;
	symbol_t *z = (symbol_t *)malloc(len);
	if (!z) {
		printf("z malloc error\n");
		return -1;
	}

	symbol_t *x = (symbol_t *)malloc(len);
	if (!x) {
		printf("x malloc error\n");
		return -1;
	}

	int y_val, x_val, product;

	/* ff_mulv */
	for (y_val = 0; y_val < 256; y_val++) {

		for (x_val = 0; x_val < 256; x_val++) {

			for (i = 0; i < len; i++) {
				x[i] = x_val;
				z[i] = 0;
			}

			ff_mulv(z, x, y_val, len);
			product = ff_mul(x_val, y_val);

			for (i = 0; i < len; i++) {
				if (z[i] != product) {
					printf("ff_mul error\n");
					return -1;
				}
			}
		}
	}


	/*ff_mulv_local*/
	for (y_val = 0; y_val < 256; y_val++) {
		for (x_val = 0; x_val < 256; x_val++) {
			for (i = 0; i < len; i++) {
				x[i] = x_val;
			}

			ff_mulv_local(x, y_val, len);

			product = ff_mul(x_val, y_val);

			for (i = 0; i < len; i++) {
				if (x[i] != product) {
					printf("ff_mulv_local: error: product = 0x%x, 0x%x = 0x%x * 0x%x\n",
					       product, x[i], x_val, y_val);

					return -1;
				}
			}
		}
	}


	int acc_product;
	/* ff_add_mulv_local */
	for (y_val = 0; y_val < 256; y_val++) {
		for (x_val = 0; x_val < 256; x_val++) {
			for (i = 0; i < len; i++) {
				x[i] = x_val;
				z[i] = 0x5a;
			}

			ff_add_mulv_local(z, x, y_val, len);
			product = ff_mul(x_val, y_val);
			acc_product = 0x5a ^ product;
			
			for (i = 0; i < len; i++) {
				if (z[i] != acc_product) {
					printf("ff_add_mulv_local: error\n");
					return -1;
				}
			}

		}
	}

	return 0;
}

int bats_test()
{
	int batch_size = 16;
	int packet_num = 1024 ;
	int packet_size = 1024;
	int ff_order = 8;
	int batch_num = 0;
	int i;
	int j;
	BatsEncoder *encoder;
	BatsDecoder *decoder;
	symbol_t *input;
	symbol_t *output;

	symbol_t *batch;
	uint16_t batch_id;

	uint64_t start;
	uint64_t end;
	int decoded_packets;
	srand(7);


	/* encode and decode */
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

	encoder = BatsEncoder_new(batch_size, packet_num, packet_size, input);
	if (!encoder) {
		d_error("encoder creation error\n");
		return -1;
	}

	BatsEncoder_selectDegree(encoder);
	


	/* decoder */
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


	start = read_tsc();
	while (1) {
		batch_num++;
		
		batch = BatsEncoder_genPacket(encoder);

		d_log("****** batch generated: batch_num = %d\n", batch_num);

		BatsDecoder_receivePacket(decoder, batch);

		d_log("****** batch recevied: batch_num = %d\n", batch_num);

		if (BatsDecoder_complete(decoder, 1.0)) {
			d_log("complete\n");
			break;
		} else {
			d_log("not complete\n");
		}


	}
	end = read_tsc();

	d_log("decoded packets: %d\n", decoded_packets);

	//free(input);
	//free(output);


	for (i = 0; i < packet_size * packet_num; i++) {
		if (output[i] != input[i]) {
			d_printf("decode error\n");
			free(input);
			free(output);
			return -1;
		}
	}
	free(input);
	free(output);
	//d_printf("decode success\n");
	return 0;

}


/* This program is dedicated to test the performance of finite field */
int main()
{
	int ret;

	bats_init();

	d_log("ff_init passed\n");

	ret = div_test();
	if (ret != 0)
		return -1;

	d_log("div_test passed\n");

	ff_test();
	if (ret != 0)
		return -1;

	d_log("ff_test passed\n");

	ret = bats_test();
	if (ret != 0)
		return -1;

	d_log("bats_test passed\n");

	return 0;
}

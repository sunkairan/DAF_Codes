#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <Utilities.h>
#include <BatchEnc.h>
#include <BatchDec.h>
#include <NCCoder.h>


#include <bats.h>
#include <time.h>

float LOSS_RATE = 0.1;

int run_packet_mode(BatsEncoder *encoder, BatsDecoder *decoder, NCCoder *recoder)
{
	int coded_len = recoder->L;

	SymbolType *packet = NULL;
	SymbolType *recoded_packet = new SymbolType[coded_len];
	SymbolType *recoder_buffer = new SymbolType[16*coded_len];
	int cnt = 0;
	int c1_sent = 0;
	int c1_received = 0;
	int c2_sent = 0;
	int c2_received = 0;

	MTRand *psrand_1 = new MTRand();
	MTRand *psrand_2 = new MTRand();

	//cout << "Packet mode" << endl;

	while (!decoder->complete(1.0)) {
		
		for (int i = 0; i < 16; i++) {

			packet = encoder->genPacket();
			c1_sent++;
			if (psrand_1->rand() > LOSS_RATE) {
				memcpy(&recoder_buffer[cnt*coded_len], 
				       packet,
					coded_len);
				cnt++;
				c1_received++;
			}

			if(cnt == 0)
				continue;
			
			recoder->genPacket(recoded_packet,
					   recoder_buffer, 
					   cnt);
			c2_sent++;
			if (psrand_2->rand() > LOSS_RATE) {
				decoder->receivePacket(recoded_packet);
				c2_received++;
				if (decoder->complete(1.0)) {
					decoder->logRankDist();
					break;
				}
			}

		}

		/* prepare for a new batch */
		cnt = 0;
		memset(recoder_buffer, 0 , 16 * coded_len);
	}

	delete [] recoded_packet;
	delete [] recoder_buffer;
	delete psrand_1;
	delete psrand_2;
	//cout << "c1_sent: " << c1_sent << " c1_received: " << c1_received << " c2_sent: " << c2_sent << " c2_received: " << c2_received << endl;

	return c2_received;
}

int run_batch_mode(BatsEncoder *encoder, BatsDecoder *decoder, NCCoder *recoder)
{
	int coded_len = recoder->L;

	SymbolType *packet = NULL;
	SymbolType *recoded_packet = new SymbolType[coded_len];
	SymbolType *recoder_buffer = new SymbolType[16*coded_len];
	int cnt = 0;
	int c1_sent = 0;
	int c1_received = 0;
	int c2_sent = 0;
	int c2_received = 0;

	MTRand *psrand_1 = new MTRand();
	MTRand *psrand_2 = new MTRand();

	//cout << "Batch mode" << endl;

	while (!decoder->complete(1.0)) {
		for (int i = 0; i < 16; i++) {
			packet = encoder->genPacket();
			c1_sent++;
			if (psrand_1->rand() > LOSS_RATE) {
				memcpy(&recoder_buffer[cnt*coded_len],
				       packet,
				       coded_len);
				cnt++;
				c1_received++;
			}
		}

		/* rarely happens */
		if (cnt == 0)
			continue;

		for (int i = 0; i < 16; i++) {
			recoder->genPacket(recoded_packet,
					   recoder_buffer,
					   cnt);
			c2_sent++;
			if (psrand_2->rand() > LOSS_RATE) {
				decoder->receivePacket(recoded_packet);
				c2_received++;
				if (decoder->complete(1.0)) {
					decoder->logRankDist();
					break;
				}
			}
		}

		/* prepare for a new batch */
		cnt = 0;
		memset(recoder_buffer, 0, 16 * coded_len);
	} /* end while */
	
	delete [] recoded_packet;
	delete [] recoder_buffer;
	delete psrand_1;
	delete psrand_2;
	//cout << "c1_sent: " << c1_sent << " c1_received: " << c1_received << " c2_sent: " << c2_sent << " c2_received: " << c2_received << endl;

	return c2_received;

}

void setup_degree(BatsBasic *role)
{
	/*
	double degreeDist[500];
	stringstream iss;
	iss << "ddM" << 16 << "m" << 8 << "r92TO.txt";

	ifstream filestr;

	filestr.open(iss.str().c_str());
	degreeDist[0] = 0.0;
	double x;
	int D = 1;
	while (filestr >> x && D < 500) {
		degreeDist[D] = x;
		D++;
	}
	for (int i = D; i < 500; i++) {
		degreeDist[i] = 0;
	}
	filestr.close();

	role->setDegreeDist(degreeDist, D);
	*/
	role->selectDegree();
	
}

int main(int argc, char *argv[]) 
{
	int batch_size = 16;
	int ff_order = 8;
	int pkt_num = 1024;
	int pkt_size = 1024;
	int received_pkg = 0;
	float fraction = 0.0;
	int retval = 0;
	clock_t start;
	clock_t end;
	double time_spent;

	/* library level init */
	bats_init();

	if (argc == 1) {
		pkt_num = 1024;
	} 
	else if (argc == 2) {
		pkt_num = atoi(argv[1]);
		// printf("pkt_num = %d\n", pkt_num);
	}

	SymbolType *input = new SymbolType[pkt_num * pkt_size];
	SymbolType *output = new SymbolType[pkt_num * pkt_size];
	
	for (int i = 0; i < pkt_num; i++) {
		input[i] = 0x5a;
		output[i] = 0xff;
	}

	/* setup encoder */
	BatsEncoder *encoder = new BatsEncoder(batch_size, pkt_num, pkt_size, input);
	setup_degree(encoder);



	/*setup decoder */
	BatsDecoder *decoder = new BatsDecoder(batch_size, pkt_num, pkt_size, output);
	setup_degree(decoder);


	/* setup recoder */
	NCCoder *recoder = new NCCoder(batch_size, pkt_size);

	start = clock();
	if (argv[1] != NULL) {
		received_pkg = run_batch_mode(encoder, decoder, recoder);
	} else {
		received_pkg = run_packet_mode(encoder, decoder, recoder);
	}
	end = clock();

	for (int i = 0; i < pkt_num; i++) {
		if (input[i] != output[i]) {
			cout << "==== Mem coding error ====" << endl;
			retval = -1;
			break;
		}
	}
	time_spent = (double)(end - start) / CLOCKS_PER_SEC;
	fraction = float(received_pkg - pkt_num) / float(pkt_num);
	d_printf("=========================================\n");
	d_printf("pkt_num = %d, received_pkt = %d, fraction = %f, num_inact = %d, time_spent = %f\n", 
			pkt_num, received_pkg, fraction, decoder->numInact(), time_spent);
	//cout << "Fraction: " << fraction << endl;

	delete [] input;
	delete [] output;
	//delete encoder;
	delete decoder;
	delete recoder;
	return retval;
}



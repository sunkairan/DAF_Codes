#include "bats.h"
#include "BatchEnc.h"
#include "BatchDec.h"
#include "NCCoder.h"

int main() 
{
	int batch_size = 16;
	int packet_size = 1;
	int packet_num = 1024;	
	SymbolType *input = NULL;
	SymbolType *output = NULL;

	/* allocate input and outpu buffer */
	input = (SymbolType *)malloc(packet_size * packet_num);
	if (!input) {
		d_error("malloc input error");
		return -1;
	}

	output = (SymbolType *)malloc(packet_size * packet_num);
	if (!output) {
		d_error(maloc output error);
		free(input);
		return -1;
	}



	BatchEncoder encoder(batchSize, packet_num, packet_size, input);
	
	BatchDecoder decoder(batchSize,	packet_num, packet_size, output);

	NCCoder recoder(batchSize, packet_size);

	/* while decode not complete*/ 

	/* if this is a new batch*/
	/* append this batch to a list used by recoder*/




	/* Calculate rand distribution from runtime condition*/
	cout << "Rank Distribution: ";

	double rd[batchSize + 1];

	decoder.rankDist(rd); 

	double Erk = 0.0;

	ofstream myfile;
	myfile.open("rankDistFromRuntime.txt", ios::out | ios::app);
	for (int i = 0; i <= batchSize; i++) {
		cout << rd[i] << " ";
		myfile << rd[i] << "\t";
		Erk += i * rd[i];
	}
	myfile << "\n";
	myfile.close();
	cout << endl;
	cout << "E[rank(H)] = " << Erk << endl;



}

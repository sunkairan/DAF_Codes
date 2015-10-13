#include "PracticalSocket.h"      // For UDPSocket and SocketException
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <Utilities.h>
#include <DelayEnc.h>
#include <DelayDec.h>

#include <bats.h>

double ETA = 0.94;
long data_rate;// = 350000; // data rate (byte / s)
float modeSelect;// = 0.0; // force the mode to be a value between [-1,1],
						 // to enable optimized values, give a number outside that interval

int dt;// = 5;
int window_size;// = 10;
int pkt_size;// = 1024;
double code_rate;
int seed;

int pkt_num = 1024;
int batch_size = 1;
int ff_order = 8;

int frame_rate;
int frame_num;
//int coded_pkt_num = 0;
int coded_window_num = 0;
long pkt_num_in_eval = 0;

long evalFrom = 0;
long evalTo = 0;


typedef struct HeaderType {
	PosType from;
	PosType window;
	ModeType mode;
} headerType;


double udp_receive_slide(unsigned short servPort,
		DelayDecoder *decoder)
{
	int coded_len = (batch_size * ff_order / SYMBOLSIZE) + pkt_size + sizeof(KeyType)
						+ 2*sizeof(PosType) + sizeof(ModeType);

	SymbolType *coded_buffer = new SymbolType[coded_len];
	int received_pkt = 0;
	double eta = ETA;
	string sourceAddress;             // Address of datagram source
	unsigned short sourcePort;        // Port of datagram source
	int recvMsgSize;
	double ratio_in_eval;
	double ratio_in_time;

	cout << "Waiting for receive sliding: eta: "<< eta  << " ..."<< endl;
	try {
	UDPSocket sock(servPort);
	for (;;){ // Run forever

		recvMsgSize = sock.recvFrom(coded_buffer, coded_len, sourceAddress, sourcePort);

		if(recvMsgSize == coded_len){
			cout << "Received packet from " << sourceAddress << ":" << sourcePort << ":  ";
			received_pkt++;
			decoder->receivePacket(coded_buffer,false);
			decoder->complete(eta);
			//ratio_in_eval = ((double)(decoder->nDecodedPkginEval) / (double)pkt_num_in_eval);
			ratio_in_time = ((double)(decoder->nDecodedInTime) / (double)pkt_num_in_eval);
			cout << "in-time decode ratio: " << ratio_in_time <<"\tcnt:" << received_pkt <<endl;
		}
		else if (recvMsgSize == 3){
			cout << "FIN received from " << sourceAddress << ":" << sourcePort << endl;
			decoder->complete(eta);
			ratio_in_eval = ((double)(decoder->nDecodedPkginEval) / (double)pkt_num_in_eval);
			ratio_in_time = ((double)(decoder->nDecodedInTime) / (double)pkt_num_in_eval);
			cout << "Final file ratio: " << ratio_in_eval << ", final in-time decode ratio: " << ratio_in_time << ", " << received_pkt << " pkts received." <<endl;
			return ratio_in_eval;
		}
		else {
			fputs ("Unexpected packet.\n",stderr);
			exit (1);
		}
	} /* end for */
	} catch (SocketException &e) {
	    cerr << e.what() << endl;
	    exit(1);
	}
}

void setup_degree(BatsBasic *role)
{
	role->selectDegree();
}

int main(int argc, char *argv[])
{
	float fraction = 0.0;
	int fountainMode = 0; // 0 is file, 1 is slide
	int result;
	bool fixLength = false;
	bool longWindows = false;
	long minWindow = 10000000;

	/* library level init */
	bats_init();

	if(argc != 6 && argc != 7) {
		fputs ("The input parameters should be: <Port> <NAME> <WSize> <dt> <Mode> [<seed>]",stderr);
		exit(1);
	}

	unsigned short servPort = atoi(argv[1]);     // First arg:  local port

	char * videoName = argv[2];
	char inputName[100];
	char fullName[100];
	char infoName[100];
	char fileName[100];
	char schedName[100];
	char outputName[100];
	strcpy(fullName, videoName);
	strcat(fullName,"_W");
	strcat(fullName,argv[3]);
	strcat(fullName,"_dt");
	strcat(fullName,argv[4]);
	strcat(fullName,"_P1024");
	if(strcmp(argv[5],"fun3")==0){
		modeSelect = 4.0;
	}
	else if(strcmp(argv[5],"nonopt")==0){
		modeSelect = 0.0;
	}
	else if(strcmp(argv[5],"block") == 0){
		strcpy(fullName, videoName);
		strcat(fullName,"_W");
		strcat(fullName,argv[3]);
		strcat(fullName,"_dt");
		strcat(fullName,argv[3]); // window size = dt
		strcat(fullName,"_P1024");
		modeSelect = 0.0;
	}
	else if(strcmp(argv[5],"fix") ==0){
		fixLength = true;
		modeSelect = 0.0;
	}
	else if(strcmp(argv[5],"fount") ==0){
		longWindows = true;
		modeSelect = 0.0;
	}
	else  {
		fputs ("Do not recognize mode: select between <fun3> <nonopt> <block> <fix> <fount>.",stderr);
		exit (3);
	}
	if(argc == 7){
		seed = atoi(argv[6]);
	}
	else{
		seed = 0;
	}


	strcpy(infoName, fullName);
	strcat(infoName,"_info.txt");
	strcpy(fileName, fullName);
	strcat(fileName,"_dummy.txt");
	strcpy(schedName, fullName);
	strcat(schedName,"_scheduler.txt");

	// read basic info.
	FILE * fileInfo = fopen (infoName, "r");
	if (fileInfo==NULL) {fputs ("Info file error",stderr); exit (1);}
	result = fscanf(fileInfo,"%s %d %d %d %d %d %d %d %ld %ld",
				inputName,
				&pkt_size,
				&frame_rate,
				&dt,
				&window_size,
				&frame_num,
				&pkt_num,
				&coded_window_num,
				&evalFrom,
				&evalTo);
	if (result != 10) {fputs ("Reading info file error",stderr); exit (3);}
	if (strcmp(inputName, videoName)!=0) {fputs ("File name does not match the info",stderr); exit (3);}
	evalFrom --;
	evalTo --;
	pkt_num_in_eval = evalTo - evalFrom + 1;
	fclose(fileInfo);

	// prepare file

	SymbolType *output = new SymbolType[pkt_num * pkt_size + 1];
	memset(output, '-',pkt_num * pkt_size);
	output[pkt_num * pkt_size] = 0;


	/*setup decoder */
	DelayDecoder *decoder = new DelayDecoder(pkt_num, pkt_size, output, evalFrom, evalTo, seed);
	setup_degree(decoder);

	// main function
	double ratio_in_eval = udp_receive_slide(servPort, decoder);

	// Write output file
	strcpy(outputName, fullName);
	strcat(outputName,"_output.txt");
	FILE *outputFile = fopen(outputName, "w");
	fwrite(output, sizeof(char), pkt_num * pkt_size + 1, outputFile);
	fclose(outputFile);
	cout<<"Received file is written to "<< outputName << endl;

	delete [] output;
	//delete [] scheduler;
	//delete encoder;
	//delete decoder;
	return 0;
}



#include "PracticalSocket.h"      // For UDPSocket and SocketException
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <Utilities.h>
#include <DelayEnc.h>
#include <DelayDec.h>
#include <LDPCStruct.h>

#include <bats.h>

double ETA = 0.94;
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


int udp_send_slide(string servAddress,
		unsigned short servPort,
		DelayEncoder *encoder,
		int coded_window_num,
		headerType scheduler[],
		unsigned int usInterval)
{
	int coded_len = (batch_size * ff_order / SYMBOLSIZE) + pkt_size + sizeof(KeyType)
						+ 2*sizeof(PosType) + sizeof(ModeType);

	SymbolType *packet = NULL;
	SymbolType *coded_buffer = new SymbolType[coded_len];
	int sent_pkt = 0;
	double eta = ETA;
	double NPacketInWindow = ((double)pkt_num) /( code_rate * ((double)coded_window_num));
	double RestPktInWindow = 0.0;
	int NInWindow = 0;
	int dataRate = (int)(NPacketInWindow * ((double)frame_rate) * ((double)pkt_size) / ((double)dt));

	cout << "Start sliding udp send: data rate: "<< dataRate <<" Bytes/s, interval: " << usInterval << " us" << endl;

	try {
	UDPSocket sock;

	// Send sliding window
	for (int i = 1; i <= coded_window_num; i++){
		RestPktInWindow += NPacketInWindow;
		NInWindow = ceil(RestPktInWindow);
		for (int j=1; j <=NInWindow; j++){
			RestPktInWindow -= 1.0;
			packet = encoder->genPacket(scheduler[i].from, scheduler[i].window, scheduler[i].mode);
			sent_pkt++;
			// Send the string to the server
			sock.sendTo(packet, coded_len, servAddress, servPort);
			// wait for interval
			usleep(usInterval);
		}
	} /* end for */

	// Send termination
	for (int i=1; i <= 10; i++){ // Send a lot termination signals
		sock.sendTo("FIN", 3, servAddress, servPort);
		usleep(usInterval);
	}

	} catch (SocketException &e) {
		cerr << e.what() << endl;
		exit(1);
	}

	cout <<"Send complete! packets sent: "<< sent_pkt << endl;
	return sent_pkt;
}

void setup_degree(BatsBasic *role)
{
	role->selectDegree();
}

int main(int argc, char *argv[])
{
	int fountainMode = 0; // 0 is file, 1 is slide
	int result;
	bool fixLength = false;
	bool longWindows = false;
	long minWindow = 10000000;

	/* library level init */
	bats_init();

	if(argc != 9 && argc != 10) {
		fputs ("The input parameters should be: <Server> <Port> <Duration (frms)> <NAME> <WSize> <dt> <CodeRate> <Mode> [<seed>]",stderr);
		exit(1);
	}

	string servAddress = argv[1];             // First arg: server address
	unsigned short servPort = Socket::resolveService(argv[2], "udp"); //Second arg: port
	unsigned int durFrms = atoi(argv[3]);

	char * videoName = argv[4];
	char inputName[100];
	char fullName[100];
	char infoName[100];
	char fileName[100];
	char schedName[100];
	strcpy(fullName, videoName);
	strcat(fullName,"_W");
	strcat(fullName,argv[5]);
	strcat(fullName,"_dt");
	strcat(fullName,argv[6]);
	strcat(fullName,"_P1024");
	code_rate = atof(argv[7]);
	if(strcmp(argv[8],"fun3")==0){
		modeSelect = 4.0;
	}
	else if(strcmp(argv[8],"nonopt")==0){
		modeSelect = 0.0;
	}
	else if(strcmp(argv[8],"block") == 0){
		strcpy(fullName, videoName);
		strcat(fullName,"_W");
		strcat(fullName,argv[5]);
		strcat(fullName,"_dt");
		strcat(fullName,argv[5]); // window size = dt
		strcat(fullName,"_P1024");
		modeSelect = 0.0;
	}
	else if(strcmp(argv[8],"fix") ==0){
		fixLength = true;
		modeSelect = 0.0;
	}
	else if(strcmp(argv[8],"fount") ==0){
		longWindows = true;
		modeSelect = 0.0;
	}
	else  {
		fputs ("Do not recognize mode: select between <fun3> <nonopt> <block> <fix> <fount>.",stderr);
		exit (3);
	}
	if(argc == 10){
		seed = atoi(argv[9]);
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

	// compute the time interval to send pkts in us
	unsigned int usInterval = (int)(((double)durFrms) * 1000000.0 * code_rate / ((double)pkt_num) / ((double)frame_rate));

	// prepare file
	SymbolType *input = new SymbolType[pkt_num * pkt_size];
	FILE * inputFile = fopen(fileName,"r");
	if (inputFile==NULL) {fputs ("Input file error",stderr); exit (1);}
	result = fread (input,1,pkt_num * pkt_size,inputFile);
	if (result != pkt_num * pkt_size) {fputs ("Reading file error",stderr); exit (3);}
	/*for (int i = 0; i < pkt_num; i++) {
		input[i] = 0x5a;
		output[i] = 0xff;
	}*/
	fclose(inputFile);

	// read scheduler
	headerType scheduler[coded_window_num + 1];
	FILE * fileScheduler = fopen (schedName, "r");
	if (fileScheduler==NULL) {fputs ("File scheduler error",stderr); exit (1);}
	for(int i = 1; i <= coded_window_num; i++ ) {
		result = fscanf(fileScheduler, "%*d %lu %lu %f",
				&(scheduler[i].from),
				&(scheduler[i].window),
				&(scheduler[i].mode));
		if (result != 3) {fputs ("Reading scheduler error",stderr); exit (3);}
		scheduler[i].from --; // positions in de/encoder starts with 0
		if(minWindow > scheduler[i].window) {
			minWindow = scheduler[i].window;
		}
		if(modeSelect <= 1.0 && modeSelect >= -1.0) {
			// make it a constant sampling distribution if modeSelect is [-1,1]
			scheduler[i].mode = modeSelect;
		}
		//scheduler[i].window = 700;
	}
	if(fixLength) {
		for(int i = 1; i <= coded_window_num; i++ ) {
			scheduler[i].window = minWindow;
		}
	}
	if(longWindows) {
		for(int i = 1; i <= coded_window_num; i++ ) {
			scheduler[i].window = 0; // 0 means largest window possible
		}
	}
	fclose(fileScheduler);

	/* setup encoder */
	LDPCStruct* encodeLDPC = new LDPCStruct(pkt_num, 97, 3, 6, 0, 0);
	DelayEncoder *encoder = new DelayEncoder(pkt_num, pkt_size, input, encodeLDPC, evalFrom, evalTo, seed);
	setup_degree(encoder);


	int sent_pkt = udp_send_slide(servAddress, servPort, encoder, coded_window_num, scheduler, usInterval);

	delete [] input;
	//delete [] scheduler;
	//delete encoder;
	//delete decoder;
	return 0;
}



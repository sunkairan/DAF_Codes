#ifndef CIRCULARBUF_H_
#define CIRCULARBUF_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

#include "Utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ThreadedCircularBuf
{
	int len; //Length in byte of a packet
	int N; //Maximum number of packets in buffer
	//int rcount;
	SymbolType* content;

	volatile int start; //Points to the first element in queue
	volatile int end;
	volatile int empty;
	//int cnt; //Number of packets in buffer currently
	//sem_t* fillCnt;
	//sem_t* emptyCnt;
	pthread_cond_t c;
	pthread_mutex_t m;
	pthread_t thread_id;
	char* name; //Name for the semaphore
};

void bufInit(struct ThreadedCircularBuf* buf, int len, int N, char* tag);
void bufWrite(struct ThreadedCircularBuf* buf, SymbolType* data); //Temp
int bufRead(struct ThreadedCircularBuf* buf, SymbolType* data);
void bufFree(struct ThreadedCircularBuf* buf);
double bufGetFilledRatio(struct ThreadedCircularBuf* buf);

#ifdef __cplusplus
}
#endif



#endif /* CIRCULARBUF_H_ */

#ifndef NIODECODER_H_
#define NIODECODER_H_

#include "BatchDec.h"
#include "CircularBuf.h"
#include "Utilities.h"

#include <stdlib.h>
#include <pthread.h>
//#include <Python.h>

#include <stdio.h>

//Packed structure for thread
struct ThreadArg
{
	int M, K, T, F;
	struct ThreadedCircularBuf* buf;
	SymbolType* out;
	int* numDec; //To store the current number of decoded packets. 4, Sep, Tom.
	int* numReceived; // To store the current number of packets received for decoding. Hu Ming.
	int (*callback)();
};

extern "C" {
	void* NonBlockingDecoder_Threadfunc(void* arg);
	struct ThreadedCircularBuf* NonBlockingDecoder_new(int M, int K, int T, int F, SymbolType* output, int* numDec, int* numReceived, int (*callback)(), char* tag);
	void NonBlockingDecoder_wait(struct ThreadedCircularBuf *buf);
}


#endif /* NIODECODER_H_ */

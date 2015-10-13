#include "NIODecoder.h"

void* NonBlockingDecoder_Threadfunc(void* arg)
{
	BatsDecoder* dec;
	struct ThreadedCircularBuf* buf;
	SymbolType* packet;
	int res;

	struct ThreadArg* t = (struct ThreadArg*)arg;

	dec = BatsDecoder_new(t->M, t->K, t->T, t->out);
	BatsDecoder_selectDegree(dec);
	//BatsDecoder_setOutputPacket(dec, t->out);

	buf = t->buf;
	packet = (SymbolType*)malloc(buf->len);

	*(t->numDec) = 0;

	while(!BatsDecoder_complete(dec, 1.0))
	{
		//Main loop
		res = bufRead(buf, packet);
		if (res == 1)
		{
			BatsDecoder_receivePacket(dec, packet);
			//printf("Lib::Receive pac\n");
			*(t->numDec) = BatsDecoder_getDecoded(dec);//Update current number of decoded packet. 4, Sep, Tom.
			*(t->numReceived) += 1;
		}
	}


	//Done, now what? (callback)
	(*(t->callback))();

	//Cleanup
	free(packet);
	//TODO: No C interface for deleting a decoder yet
	free(t); //Reason: Ownership of the ThreadArg struct is given to the thread
	return NULL;
}

struct ThreadedCircularBuf* NonBlockingDecoder_new(int M, int K, int T, int F, SymbolType* output, int* numDec, int* numReceived, int (*callback)(), char* tag)
{
	struct ThreadArg* arg;
	pthread_t pth;
	arg = (struct ThreadArg*) malloc(sizeof(struct ThreadArg));
	arg->M = M;
	arg->K = K;
	arg->T = T;
	arg->F = F;
	arg->out = output;
	arg->numDec = numDec;
	arg->numReceived = numReceived;
	arg->callback = callback;

	arg->buf = (struct ThreadedCircularBuf*) malloc(sizeof(struct ThreadedCircularBuf));
	//bufInit(arg->buf, M*F/8 + T + 2, 32767, tag); //Setup a 50MB buffer - 23 Aug, Tom (51200)
	bufInit(arg->buf, M*F/8 + T + 2, 327670, tag); //Setup a 500MB buffer - 1  Feb, Mic (512000)

	pthread_create(&arg->buf->thread_id, NULL, NonBlockingDecoder_Threadfunc, arg);

	return arg->buf;
}


void NonBlockingDecoder_wait(struct ThreadedCircularBuf *buf)
{
	pthread_t thread_id;
	void *value_ptr;
	int ret;

	thread_id = buf->thread_id;

	ret = pthread_join(thread_id, &value_ptr);
	if (ret != 0) {
		//perror("pthread join error");
	}
}

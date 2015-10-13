#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
// #include "Utilities.h"
#include "CircularBuf.h"


void bufInit(struct ThreadedCircularBuf* buf, int len, int N, char* tag)
{
	char tmpA[50];
	char tmpB[50];

	buf->len = len;
	buf->N = N;
	//buf->rcount = 0;

	buf->start = 0;
	buf->end = 0;
	buf->empty = 1;
	//buf->cnt = 0;

	buf->content = (SymbolType*) malloc(len*N);

	buf->name = (char*)malloc(strlen(tag)*sizeof(char));
	//strcpy(buf->name, tag);

	pthread_cond_init(&(buf->c), NULL);
	pthread_mutex_init(&(buf->m), NULL);

	//sprintf(tmpA, "/qfill_%s", buf->name);
	//sprintf(tmpB, "/qempty_%s", buf->name);

	//sem_unlink(tmpA);
	//sem_unlink(tmpB);

	//buf->fillCnt = sem_open(tmpA, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
	//buf->emptyCnt = sem_open(tmpB, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, N);
	/*if ((buf->fillCnt == SEM_FAILED) || (buf->emptyCnt == SEM_FAILED))
	{
		//Error!!
		printf("Error: cannot create semaphores: %s\n", strerror(errno));
		printf("Debug: SEM_VALUE_MAX = %d\n", SEM_VALUE_MAX);

		exit(1);
	}*/
	//sem_init(&(buf->fillCnt), 0, 0);
	//sem_init(&(buf->emptyCnt), 0, N);
	return;
}

void bufWrite(struct ThreadedCircularBuf* buf, SymbolType* data)
{
	//sem_wait(buf->emptyCnt);
	//pthread_mutex_lock(&(buf->m));
	/* Critical section */
	//int end = (buf->start + buf->cnt) % buf->N;
	//Update: if full, just drop
	//if (buf->cnt < buf->N) //Should never be full
	if (((buf->end+1)%buf->N) != buf->start)
	{
		memcpy(buf->content + buf->end*(buf->len), data, buf->len);
		buf->end = (buf->end+1)%buf->N;
		pthread_cond_signal(&(buf->c));
		//if (buf->empty)
		//{
		//	pthread_cond_signal(&(buf->c));
		//	buf->empty = 0;
		//}
		
		//buf->cnt++;
		//Performance Debug
		//if (buf->cnt >= 0.95*(buf->N))
		//{
		//	printf("Warning: Buffer almost full!\n");
		//}
	}
	/* End of critical section */
	//pthread_mutex_unlock(&(buf->m));
	//sem_post(buf->fillCnt);
	return;
}

int bufRead(struct ThreadedCircularBuf* buf, SymbolType* data)
{
	int res;
	res = 1;
	//sem_wait(buf->fillCnt);
	//pthread_mutex_lock(&(buf->m));
	/* Critical Section */
	//if (buf->cnt > 0) //Should never be empty
	if (buf->start != buf->end)
	{
		memcpy(data, buf->content + (buf->start)*(buf->len), buf->len);
		buf->start = (buf->start + 1) % buf->N;
		//printf("read:%d\n", buf->rcount++);
		//buf->cnt--;
	}
	else
	{
		//printf("Warning: empty buffer!\n");
		//buf->empty = 1;
		pthread_cond_wait(&(buf->c), &(buf->m));
		return bufRead(buf,data);
		res = 0;
	}
	/* End of critical section */
	//pthread_mutex_unlock(&(buf->m));
	//sem_post(buf->emptyCnt);
	return res;
}

//DONE: destructor
void bufFree(struct ThreadedCircularBuf* buf)
{
	char tmpA[50];
	char tmpB[50];

	free(buf->content);
	//sem_close(buf->fillCnt);
	//sem_close(buf->emptyCnt);

	//Need to unlink also becoz it may be a long time before we reuse the same filename again
	//sprintf(tmpA, "/qfill_%s", buf->name);
	//sprintf(tmpB, "/qempty_%s", buf->name);

	//sem_unlink(tmpA);
	//sem_unlink(tmpB);
	free(buf->name);
	//sem_destroy(&(buf->fillCnt));
	//sem_destroy(&(buf->emptyCnt));
	pthread_mutex_destroy(&(buf->m));
	pthread_cond_destroy(&(buf->c));
	return;
}

//Note: Not mutex protected for performance concern
double bufGetFilledRatio(struct ThreadedCircularBuf* buf)
{
	//return (buf->cnt)/(double)(buf->N);
	if (buf->end<buf->start)
		return (buf->end+buf->N-buf->start+1)/(double)(buf->N);
	return (buf->end-buf->start+1)/(double)(buf->N);
}


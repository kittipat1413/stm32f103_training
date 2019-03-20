#include "NBQueue.h"

void NBQueueInit(NBQueue* q){
	q->front = 0;
	q->rear = -1;
	q->itemCount = 0;
}

uint8_t NBQueuePeek(NBQueue* q){
	return q->buffer[q->front];
}

bool NBQueueIsEmpty(NBQueue* q){
	return q->itemCount == 0;
}

bool NBQueueIsFull(NBQueue* q){
	return q->itemCount == QUEUE_SIZE;
}

uint16_t NBQueueSize(NBQueue* q){
	return q->itemCount;
}

void NBQueueInsert(NBQueue* q, uint8_t data){
	if(!NBQueueIsFull(q)){
		if(q->rear == QUEUE_SIZE-1){
			q->rear = -1;
		}
		q->rear = q->rear + 1;
		q->buffer[q->rear] = data;
		q->itemCount = q->itemCount + 1;
	}
}

uint8_t NBQueueRemove(NBQueue* q){
	uint8_t data =  q->buffer[q->front];
	q->front = q->front+1;

	if(q->front == QUEUE_SIZE){
		q->front = 0;
	}
	q->itemCount = q->itemCount - 1;
	return data;
}
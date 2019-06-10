#include "UC15Queue.h"

void UC15QueueInit(UC15Queue* q){
	q->front = 0;
	q->rear = -1;
	q->itemCount = 0;
}

uint8_t UC15QueuePeek(UC15Queue* q){
	return q->buffer[q->front];
}

bool UC15QueueIsEmpty(UC15Queue* q){
	return q->itemCount == 0;
}

bool UC15QueueIsFull(UC15Queue* q){
	return q->itemCount == QUEUE_SIZE;
}

uint16_t UC15QueueSize(UC15Queue* q){
	return q->itemCount;
}

void UC15QueueInsert(UC15Queue* q, uint8_t data){
	if(!UC15QueueIsFull(q)){
		if(q->rear == QUEUE_SIZE-1){
			q->rear = -1;
		}
		q->rear = q->rear + 1;
		q->buffer[q->rear] = data;
		q->itemCount = q->itemCount + 1;
	}
}

uint8_t UC15QueueRemove(UC15Queue* q){
	uint8_t data =  q->buffer[q->front];
	q->front = q->front+1;

	if(q->front == QUEUE_SIZE){
		q->front = 0;
	}
	q->itemCount = q->itemCount - 1;
	return data;
}
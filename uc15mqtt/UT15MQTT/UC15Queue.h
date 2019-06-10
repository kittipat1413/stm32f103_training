#ifndef NBQUEUE_H_
#define NBQUEUE_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define QUEUE_SIZE 1024

typedef struct UC15Queue {
	uint8_t buffer[QUEUE_SIZE];
	uint16_t front;
	uint16_t rear;
	uint16_t itemCount;
} UC15Queue;


void UC15QueueInit(UC15Queue* q);
uint8_t UC15QueuePeek(UC15Queue* q);
bool UC15QueueIsEmpty(UC15Queue* q);
bool UC15QueueIsFull(UC15Queue* q);
uint16_t UC15QueueSize(UC15Queue* q);
void UC15QueueInsert(UC15Queue* q, uint8_t data);
uint8_t UC15QueueRemove(UC15Queue* q);
#endif
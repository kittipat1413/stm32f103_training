#ifndef EC_rs485_H_
#define EC_rs485_H_

#include "stm32f10x.h"
#include "ATparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void EC_rs485_init(atparser_t *P, uint8_t *buf);
bool EC_rs485_readTempHumi(uint16_t * temp , uint16_t* humi );
bool EC_rs485_readSaltEC(uint16_t * salt , uint16_t* ec );
bool EC_rs485_readSalt(uint16_t * salt);
bool EC_rs485_readEC(uint16_t * ec);
uint16_t crc16_update(uint16_t crc, uint8_t a); 


#endif
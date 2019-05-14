#ifndef __ONE_WIRE_H__
#define __ONE_WIRE_H__
#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


typedef enum {
	ONE_WIRE_SLAVE_PRESENT,
	ONE_WIRE_ERROR
} one_wire_state;

typedef struct {
	uint8_t address[8];
} one_wire_device;


void one_wire_init(GPIO_TypeDef *gpio, u16 port, TIM_TypeDef *timer);
bool one_wire_reset_pulse();
void one_wire_write_1();
void one_wire_write_0();
void one_wire_write_bit(bool bit);
bool one_wire_read_bit();
void one_wire_write_byte(uint8_t data);
uint8_t one_wire_read_byte();
void one_wire_reset_crc(void); 
uint8_t one_wire_get_crc(void); 
uint8_t one_wire_crc(uint8_t data); 
one_wire_device one_wire_read_rom(void); 
#endif // __ONE_WIRE_H__

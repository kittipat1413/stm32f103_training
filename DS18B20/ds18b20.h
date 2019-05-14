#ifndef __DS18B20_H__
#define __DS18B20_H__
#include "one_wire.h"
#include "math.h"
#include <stdlib.h>
#include <stdbool.h>

// Structure in which temperature is stored
typedef struct {
    int8_t integer;
    uint16_t fractional;
    bool is_valid;
} simple_float;


void ds18b20_init(GPIO_TypeDef *gpio, u16 port, TIM_TypeDef *timer);
void ds18b20_set_precission(uint8_t p);
void ds18b20_convert_temperature_simple(void);
simple_float ds18b20_read_temperature_simple(void);
simple_float ds18b20_decode_temperature(void);

#endif // __DS18B20_H__

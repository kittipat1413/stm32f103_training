#include "ds18b20.h"

//////////////////////////////////////////////
void ds18b20_init(GPIO_TypeDef *gpio, uint16_t port, TIM_TypeDef *timer);
void ds18b20_set_precission(u8 p);
void ds18b20_convert_temperature_simple(void);
simple_float ds18b20_read_temperature_simple(void);
simple_float ds18b20_decode_temperature(void); 
///////////////////////////////////////////////




void ds18b20_init(GPIO_TypeDef *gpio, uint16_t port, TIM_TypeDef *timer) {
	one_wire_init(gpio, port, timer);
}

void ds18b20_set_precission(uint8_t p) {
	uint8_t precission = p;
	one_wire_reset_pulse();

	one_wire_write_byte(0xCC); // Skip ROM
	one_wire_write_byte(0x4E); // Write scratchpad

	one_wire_write_byte(0x4B);
	one_wire_write_byte(0x46);
	// set precission
	one_wire_write_byte(0x1F | (precission << 5));
}

void ds18b20_convert_temperature_simple(void) {
	one_wire_reset_pulse();
	one_wire_write_byte(0xCC); // Skip ROM
	one_wire_write_byte(0x44); // Convert temperature
}


simple_float ds18b20_read_temperature_simple(void) {
	one_wire_reset_pulse();
	one_wire_write_byte(0xCC); // Skip ROM
	one_wire_write_byte(0xBE); // Read scratchpad

	return ds18b20_decode_temperature();
}



simple_float ds18b20_decode_temperature(void) {
	int i;
	uint8_t  crc;
	uint8_t  data[9];
    simple_float f;
    f.is_valid = false;
	one_wire_reset_crc();

	for (i = 0; i < 9; ++i) {
		data[i] = one_wire_read_byte();
		crc = one_wire_crc(data[i]);
	}
	if (crc != 0) {
		return f;
	}

	uint8_t  temp_msb = data[1]; // Sign byte + lsbit
	uint8_t  temp_lsb = data[0]; // Temp data plus lsb

    float temp = (temp_msb << 8 | temp_lsb) / powf(2, 4);
    int rest = (temp - (int)temp) * 1000.0f;

    f.integer = (uint8_t)temp;
    f.fractional = rest;
    f.is_valid = true;

    return f;
}


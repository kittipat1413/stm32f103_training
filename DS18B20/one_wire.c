#include "one_wire.h"

static void one_wire_delay_us(unsigned int time);
void one_wire_init(GPIO_TypeDef *g, uint16_t p, TIM_TypeDef *t);
bool one_wire_reset_pulse(void);
void one_wire_write_1(void);
void one_wire_write_0(void);
void one_wire_write_bit(bool bit); 
bool one_wire_read_bit(void);
void one_wire_write_byte(uint8_t data); 
uint8_t one_wire_read_byte(void); 
void one_wire_reset_crc(void); 
uint8_t one_wire_get_crc(void); 
uint8_t one_wire_crc(uint8_t data); 
one_wire_device one_wire_read_rom(void); 


GPIO_TypeDef *gpio;
uint16_t pin;
TIM_TypeDef *timer;
uint8_t crc8;
one_wire_state state;


static unsigned char crc_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95, 1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93, 3,128,222, 60, 98,
	190,224, 2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89, 7,
	219,133,103, 57,186,228, 6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135, 4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91, 5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73, 8, 86,180,234,105, 55,213,139,
	87, 9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};


static void one_wire_delay_us(unsigned int time) {
	timer->CNT = 0;
	time -= 3;
	while (timer->CNT <= time) {}
}

void one_wire_init(GPIO_TypeDef *g, uint16_t p, TIM_TypeDef *t) {
	gpio = g;
	pin = p;
	timer = t;
	state = ONE_WIRE_ERROR;

	/* Enable GPIO clock */
	if (gpio == GPIOA)
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	else
		while(1){} // not implemented

	// Use PC6 as bus master
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(gpio, &GPIO_InitStructure);


	// Setup clock
	if (timer == TIM2)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	else
		while(1){} // not implemented

	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Prescaler = 72 - 1;
	TIM_InitStructure.TIM_Period = 10000 - 1; // Update event every 10000 us / 10 ms
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(timer, &TIM_InitStructure);
//	TIM_ITConfig(timer, TIM_IT_Update, ENABLE);
	TIM_Cmd(timer, ENABLE);
}

bool one_wire_reset_pulse() {
	// Pull bus down for 500 us (min. 480 us)
	GPIO_ResetBits(gpio, pin);
	one_wire_delay_us(500);
	GPIO_SetBits(gpio, pin);

	// Wait 70 us, bus should be pulled up by resistor and then
	// pulled down by slave (15-60 us after detecting rising edge)
	one_wire_delay_us(70);
	BitAction bit = GPIO_ReadInputDataBit(gpio, pin);
	if (bit == Bit_RESET) {
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		state = ONE_WIRE_SLAVE_PRESENT;
	} else {
		state = ONE_WIRE_ERROR;
		return false;
	}

	// Wait additional 430 us until slave keeps bus down (total 500 us, min. 480 us)
	one_wire_delay_us(430);
	return true;
}

void one_wire_write_1() {
	// Pull bus down for 15 us
	GPIO_ResetBits(gpio, pin);
	one_wire_delay_us(15);
	GPIO_SetBits(gpio, pin);

	// Wait until end of timeslot (60 us) + 5 us for recovery
	one_wire_delay_us(50);
}

void one_wire_write_0() {
	// Pull bus down for 60 us
	GPIO_ResetBits(gpio, pin);
	one_wire_delay_us(60);
	GPIO_SetBits(gpio, pin);

	// Wait until end of timeslot (60 us) + 5 us for recovery
	one_wire_delay_us(5);
}

void one_wire_write_bit(bool bit) {
	if (bit) {
		one_wire_write_1();
	} else {
		one_wire_write_0();
	}
}

bool one_wire_read_bit() {
	// Pull bus down for 5 us
	GPIO_ResetBits(gpio, pin);
	one_wire_delay_us(5);
	GPIO_SetBits(gpio, pin);

	// Wait 5 us and check bus state
	one_wire_delay_us(5);

	static BitAction bit;
	bit = GPIO_ReadInputDataBit(gpio, pin);
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, bit);

	// Wait until end of timeslot (60 us) + 5 us for recovery
	one_wire_delay_us(55);

	if (bit == Bit_SET) {
		return true;
	} else {
		return false;
	}
}

void one_wire_write_byte(uint8_t data) {
	uint8_t i;
	for (i = 0; i < 8; ++i) {
		if ((data >> i) & 1) {
			one_wire_write_1();
		} else {
			one_wire_write_0();
		}
	}
}

uint8_t one_wire_read_byte() {
	uint8_t i;
	uint8_t data = 0;
	bool bit;
	for (i = 0; i < 8; ++i) {
		bit = one_wire_read_bit();
		data |= bit << i;
	}
	return data;
}

void one_wire_reset_crc() {
	crc8 = 0;
}

uint8_t one_wire_get_crc() {
	return crc8;
}

uint8_t one_wire_crc(uint8_t data) {
	crc8 = crc_table[crc8 ^ data];
	return crc8;
}

one_wire_device one_wire_read_rom() {

    int i;
    one_wire_device data;

	one_wire_reset_pulse();
	one_wire_write_byte(0x33);

	for (i = 0; i < 8; i++) {
		data.address[i] = one_wire_read_byte();
	}
	return data;
}


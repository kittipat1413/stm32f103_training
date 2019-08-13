#include "SHT20.h"

bool SHT20ReadTemperature(I2C_TypeDef* i2c, float* raw)
{
  uint16_t result=0;

  uint16_t msb=0;
  uint16_t lsb=0;
  uint8_t xsb=0;

  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x80, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0xE3);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x80, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  xsb = I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);
  
  
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  I2C_ReceiveData(i2c);

  result = (msb << 8) | lsb;

  //check xsb
  if(SHT20CheckCRC(result, xsb)){
  	result &= 0xFFFC;
  	*raw = result * (175.72 / 65536.0) - 46.85;
  	return true;
  }

  return false;
}

bool SHT20ReadHumidity(I2C_TypeDef* i2c, float* raw)
{
  uint16_t result=0;

  uint16_t msb=0;
  uint16_t lsb=0;
  uint8_t xsb=0;

  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0x80, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  I2C_SendData(i2c,0xE5);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c, 0x80, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  msb = (uint16_t)I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  lsb = (uint16_t)I2C_ReceiveData(i2c);

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  xsb = I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);
  
  
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  I2C_ReceiveData(i2c);

  result = (msb << 8) | lsb;

  //check xsb
  if(SHT20CheckCRC(result, xsb)){
  	result &= 0xFFFC;
  	*raw = result *(125.0/ 65536.0) - 6.0;
  	return true;
  }

  return false;
}

bool SHT20CheckCRC(uint16_t data, uint8_t crc_value){
	uint32_t remainder = (uint32_t)data << 8;
	remainder |= crc_value;
	uint32_t divisor = (uint32_t)0x988000;
	int i;
	for(i = 0 ; i < 16 ; i++){
        if(remainder & (uint32_t)1 << (23 - i)){
            remainder ^= divisor;
        }
        divisor >>= 1;
    }

    if(remainder == 0){
    	return true;
    }
    return false;
}

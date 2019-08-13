#include "PH_OEM.h"


bool OEM_READ_PH(I2C_TypeDef* i2c, float* raw)
{
  unsigned long result=0;

  //address = 0x65 || 0110 0101
  //address = 0xCA || 1100 1010 
  uint8_t bit1=0;
  uint8_t bit2=0;
  uint8_t bit3=0;
  uint8_t bit4=0;

  I2C_AcknowledgeConfig(i2c,ENABLE);
  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
 
  I2C_Send7bitAddress(i2c,0xCA, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

  I2C_SendData(i2c,0x16);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  I2C_GenerateSTOP(i2c, ENABLE);

  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c,0xCA, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  bit1 = (uint8_t)I2C_ReceiveData(i2c);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  bit2 = (uint8_t)I2C_ReceiveData(i2c);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  bit3 = (uint8_t)I2C_ReceiveData(i2c);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  bit4 = (uint8_t)I2C_ReceiveData(i2c);

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);
  
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_RECEIVED));
  I2C_ReceiveData(i2c);

  result = (bit1 << 24) | (bit2 << 16) | (bit3 << 8) | bit4;
  *raw = ((float)result)/1000;
  return true;
}


bool OEM_ACTIVE(I2C_TypeDef* i2c)
{

  I2C_AcknowledgeConfig(i2c,ENABLE);

  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(i2c,0xCA, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
 
  I2C_SendData(i2c,0x06);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x01);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  return true;

}



bool OEM_DEACTIVE(I2C_TypeDef* i2c)
{

  I2C_AcknowledgeConfig(i2c,ENABLE);

  I2C_GenerateSTART(i2c,ENABLE);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT));
  
  I2C_Send7bitAddress(i2c,0xCA, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

  I2C_SendData(i2c,0x06);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_SendData(i2c,0x00);
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_AcknowledgeConfig(i2c, DISABLE);
  I2C_GenerateSTOP(i2c, ENABLE);

  return true;

}
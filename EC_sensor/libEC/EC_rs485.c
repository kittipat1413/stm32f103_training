#include "EC_rs485.h"
#include "ATparser.h"

atparser_t *Parser;
uint8_t *data_buf;

uint8_t read_Temp_Humi1[8] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x02, 0x65, 0xCB};
uint8_t read_Temp_Humi2[8] = {0x01, 0x03, 0x00, 0x12, 0x00, 0x02, 0x64, 0x0E};
uint8_t read_Salt_EC[8] = {0x01, 0x03, 0x00, 0x14, 0x00, 0x02, 0x84, 0x0F};
uint8_t read_Salt[8] = {0x01, 0x03, 0x00, 0x14, 0x00, 0x01, 0xC4, 0x0E};
uint8_t read_EC[8] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xCE};



static void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1000; j++ );
}


void EC_rs485_init(atparser_t *P, uint8_t *buf) {
	Parser = P;
	data_buf = buf;
}


bool EC_rs485_readSalt(uint16_t * salt){

  //send command  
  atparser_write(Parser, &read_Salt[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 7);
  atparser_set_timeout(Parser, 8000000); 
  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 5; i++){
        crc = crc16_update(crc, data_buf[i]);
  	  }

    if( crc == ( ( (uint16_t)data_buf[6] )<<8 | (uint16_t)data_buf[5] ) ){
    	
    	*salt = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
    	return true;
  	  }
  	else{	
  		
  		return false;
  	  }


  	}

  	else{
  		
  		return false;
  	}

}

bool EC_rs485_readEC(uint16_t * ec){

  //send command  
  atparser_write(Parser, &read_EC[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 7);
  atparser_set_timeout(Parser, 8000000); 
  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 5; i++){
        crc = crc16_update(crc, data_buf[i]);
  	  }

    if( crc == ( ( (uint16_t)data_buf[6] )<<8 | (uint16_t)data_buf[5] ) ){
    	
    	*ec = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
    	return true;
  	  }
  	else{	
  		
  		return false;
  	  }


  	}

  	else{
  		
  		return false;
  	}

}



bool EC_rs485_readSaltEC(uint16_t * salt , uint16_t* ec ){

  //send command  
  atparser_write(Parser, &read_Salt_EC[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 9);
  atparser_set_timeout(Parser, 8000000); 
  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 7; i++){
        crc = crc16_update(crc, data_buf[i]);
  	  }

    if( crc == ( ( (uint16_t)data_buf[8] )<<8 | (uint16_t)data_buf[7] ) ){
    	
    	*salt = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
    	*ec = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
    	return true;
  	  }
  	else{	
  		
  		return false;
  	  }


  	}

  	else{
  		
  		return false;
  	}

}


bool EC_rs485_readTempHumi(uint16_t * temp , uint16_t* humi ){

  //send command  
  atparser_write(Parser, &read_Temp_Humi1[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 9);
  atparser_set_timeout(Parser, 8000000); 
  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 7; i++){
        crc = crc16_update(crc, data_buf[i]);
  	  }

    if( crc == ( ( (uint16_t)data_buf[8] )<<8 | (uint16_t)data_buf[7] ) ){
    	
    	*humi = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] )/10;
    	*temp = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] )/10;
    	return true;
  	  }
  	else{	
  		
  		return false;
  	  }


  	}

  	else{
  		
  		return false;
  	}

}


uint16_t crc16_update(uint16_t crc, uint8_t a) {
  int i;

  crc ^= (uint16_t)a;
  for (i = 0; i < 8; ++i) {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}







































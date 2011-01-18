//___________________________________________
//				Joshua Schapiro
//					rs232.c
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________


#include "avr_compiler.h"
#include "string.h"
#include "rs232.h"
#include "time.h"

#define Rs232_BufferSize 1024

volatile uint8_t    Rs232Buffer[Rs232_BufferSize];
volatile uint16_t  	Rs232_readLocation = 0;
volatile uint16_t   Rs232_writeLocation = 0;
volatile bool		okToSendAirQuality = false;
uint8_t airQualityString[50];
uint32_t largeCount;
uint32_t smallCount;

volatile bool rs232Recording = false;

void Rs232_Init(void){
	Rs232_Port.DIRSET = Rs232_TX_pin_bm;
	Rs232_Port.DIRCLR = Rs232_RX_pin_bm;
   
	Rs232_Usart.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | (false ? USART_SBMODE_bm : 0);
	 	    				
	Rs232_Usart.BAUDCTRLA = 95 & 0xFF;
	Rs232_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(95 >> 8);
	
	Rs232_Usart.CTRLB |= USART_RXEN_bm;
	Rs232_Usart.CTRLB |= USART_TXEN_bm;
	
	Rs232_Usart.CTRLA |= USART_RXCINTLVL_MED_gc;
}



bool Rs232_CharReadyToRead(void){
	if(Rs232_writeLocation == Rs232_readLocation){
		return false;  
	} else { 
		return true;
	}
}

uint8_t Rs232_GetByte(bool blocking){
	if(blocking){
		while(!Rs232_CharReadyToRead());
	}

	uint8_t tmp = Rs232Buffer[Rs232_readLocation];
	Rs232_readLocation++;
	if(Rs232_readLocation >= Rs232_BufferSize){
	  Rs232_readLocation=0;
	}
	return tmp;
}

void Rs232_ClearBuffer(void){
	Rs232_readLocation = Rs232_writeLocation;
}

void Rs232_SendByte(uint8_t data){
	while(!(Rs232_Usart.STATUS & USART_DREIF_bm));
	Rs232_Usart.DATA = data;	
}

void Rs232_SendString(char string [],bool CR){
	for(uint8_t i = 0; i < strlen(string); i++){
		Rs232_SendByte(string[i]);
	}

  if(CR){
		Rs232_SendByte(13);
		Rs232_SendByte(10);
	}
}

ISR(USARTD0_RXC_vect){
	Rs232Buffer[Rs232_writeLocation] = Rs232_Usart.DATA;

	if(rs232Recording){
		if(Rs232Buffer[Rs232_writeLocation] == 0x0A){
			uint8_t counter = 0;
			while(Rs232_CharReadyToRead()){
				airQualityString[counter] = Rs232_GetByte(false);
				counter++;
			}
			if(strstr(airQualityString,"Dylos") == NULL){
				airSampleTime = Time_Get32BitTimer();
				smallCount = atol(strtok(airQualityString,","));
				largeCount = atol(strtok(NULL,","));
				okToSendAirQuality = true;
			}

		}
	}
	Rs232_writeLocation++;
	if(Rs232_writeLocation >= Rs232_BufferSize){
		Rs232_writeLocation = 0;
	}
}

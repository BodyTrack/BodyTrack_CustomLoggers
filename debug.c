//___________________________________________
//				Joshua Schapiro
//					debug.c
//			  created: 08/27/2010
//			  updated: 09/14/2010
//
// 09/14/2010 - Got external oscillator working, so baud rate was changed
//___________________________________________


#include "avr_compiler.h"
#include "string.h"
#include "debug.h"

#define Debug_BufferSize 1024

volatile uint8_t    DebugBuffer[Debug_BufferSize];
volatile uint16_t  	Debug_readLocation = 0;
volatile uint16_t   Debug_writeLocation = 0;


void Debug_Init(void){
	Debug_Port.DIRSET = Debug_TX_pin_bm;
	Debug_Port.DIRCLR = Debug_RX_pin_bm;
   
	Debug_Usart.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | (false ? USART_SBMODE_bm : 0);	 	    				
	
	Debug_Usart.BAUDCTRLA = 95 & 0xFF;
	Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(95 >> 8);
	
	
	Debug_Usart.CTRLB |= USART_RXEN_bm;
	Debug_Usart.CTRLB |= USART_TXEN_bm;
	
	Debug_Usart.CTRLA |= USART_RXCINTLVL_MED_gc;
}



bool Debug_CharReadyToRead(void){
	if(Debug_writeLocation == Debug_readLocation){
		return false;  
	} else { 
		return true;
	}
}

uint8_t Debug_GetByte(bool blocking){
	if(blocking){
		while(!Debug_CharReadyToRead());
	}

	uint8_t tmp = DebugBuffer[Debug_readLocation];
	Debug_readLocation++;
	if(Debug_readLocation >= Debug_BufferSize){
	  Debug_readLocation=0;
	}
	return tmp;
}


void Debug_SendByte(uint8_t data){
	while(!(Debug_Usart.STATUS & USART_DREIF_bm));
	Debug_Usart.DATA = data;	
}

void Debug_SendString(char string [],bool CR){
	for(uint8_t i = 0; i < strlen(string); i++){
		Debug_SendByte(string[i]);
	}

  if(CR){
		Debug_SendByte(13);
		Debug_SendByte(10);
	}
}

ISR(USARTC0_RXC_vect){
	DebugBuffer[Debug_writeLocation] = Debug_Usart.DATA;
	Debug_writeLocation++;
	if(Debug_writeLocation >= Debug_BufferSize){
		Debug_writeLocation = 0;
	}
}

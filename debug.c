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



void Debug_Init(uint32_t baud){
	Debug_Port.DIRSET = Debug_TX_pin_bm;
	Debug_Port.DIRCLR = Debug_RX_pin_bm;


    Debug_Flow_Port.DIRCLR = (1<<Debug_RTS_pin);
    Debug_Flow_Port.Debug_RTS_CNTL = PORT_OPC_PULLUP_gc;


	Debug_Usart.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | (false ? USART_SBMODE_bm : 0);	 	    				

	if(baud == 9600){
		Debug_Usart.BAUDCTRLA = 95 & 0xFF;
		Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(95 >> 8);
	} else if(baud == 115200){
		Debug_Usart.BAUDCTRLA = 7 & 0xFF;
		Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(7 >> 8);
	} else if(baud == 230400){
		Debug_Usart.BAUDCTRLA = 3 & 0xFF;
		Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(3 >> 8);
	}else if(baud == 460800){
		Debug_Usart.BAUDCTRLA = 1 & 0xFF;
		Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(1 >> 8);
	} else if(baud == 921600){
		Debug_Usart.BAUDCTRLA = 0 & 0xFF;
		Debug_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(0 >> 8);
	}

	Debug_Usart.CTRLB |= USART_RXEN_bm;
	Debug_Usart.CTRLB |= USART_TXEN_bm;
	
	//Debug_Usart.CTRLA |= USART_RXCINTLVL_MED_gc;
}
void Debug_ClearBuffer(void){
	//Debug_writeLocation = Debug_readLocation;
	if(Debug_CharReadyToRead()){
		Debug_writeLocation  = DMA.CH1.DESTADDR1 << 8;
		Debug_writeLocation += DMA.CH1.DESTADDR0;
		Debug_writeLocation -= (uint16_t)(&DebugBuffer[0]);
		Debug_readLocation = Debug_writeLocation;
	}
}


bool Debug_CharReadyToRead(void){
    Debug_writeLocation  = DMA.CH1.DESTADDR1 << 8;
	Debug_writeLocation += DMA.CH1.DESTADDR0;
	Debug_writeLocation -= (uint16_t)(&DebugBuffer[0]);



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
	while(((Debug_Flow_Port.IN)&(1<<Debug_RTS_pin)) > 0){
	    if(useWifiForUploading) {} break;
	}                              // Wait for RTS to be low
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

/*ISR(USARTC0_RXC_vect){
	DebugBuffer[Debug_writeLocation] = Debug_Usart.DATA;
	Debug_writeLocation++;
	if(Debug_writeLocation >= Debug_BufferSize){
		Debug_writeLocation = 0;
	}
}    */

uint32_t Debug_GetTime(uint16_t timeOut){
    uint16_t to = timeOut;
    uint32_t tempTime = 0;
    uint8_t byteCounter = 0;
    Debug_ClearBuffer();
    Debug_SendByte('T');             // returns unix time MSB first

    while(to > 0){
      if(Debug_CharReadyToRead()){
         tempTime  |= Debug_GetByte(true) & 0xFF;
         byteCounter++;
         if(byteCounter == 4){
            return tempTime;
         } else {
             tempTime <<= 8;
         }
      }
      _delay_ms(1);
      to--;
    }
    return 0;
}

bool Debug_Connected(uint16_t timeOut){
    uint16_t to = timeOut;
    Debug_ClearBuffer();
    Debug_SendByte('P');
    while(to > 0){
      if(Debug_CharReadyToRead()){
         if(Debug_GetByte(true) == 'P'){
           return true;
         }
      }
      _delay_ms(1);
      to--;
    }
    return false;
}

bool Debug_TriggerUpload(uint32_t size, uint16_t timeOut){
   uint16_t to = timeOut;
   char fileSize [20];

   Debug_ClearBuffer();
   Debug_SendByte('U');

   sprintf(fileSize, "%12lu",size);
   Debug_SendString(fileSize,true);

    while(to > 0){
      if(Debug_CharReadyToRead()){
         if(Debug_GetByte(true) == 'Y'){
           return true;
         } else {
           return false;
         }
      }
      _delay_ms(1);
      to--;
    }
    return false;

}

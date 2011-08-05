//___________________________________________
//				Joshua Schapiro
//					debug.c
//			  created: 08/27/2010
//			  updated: 09/14/2010
//
// 09/14/2010 - Got external oscillator working, so baud rate was changed
//___________________________________________


#include "debug.h"


volatile uint8_t    DebugBuffer[Debug_BufferSize];
volatile uint16_t  	Debug_readLocation = 0;
volatile uint16_t   Debug_writeLocation = 0;

volatile uint32_t	Debug_timeOutCounter = 0;

void Debug_Init(uint32_t baud){
	
	DMA.CTRL |= DMA_ENABLE_bm; // enable DMA
	
	// Debug
	DMA.Debug_DMA_Channel.ADDRCTRL |= DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc	| DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;
	DMA.Debug_DMA_Channel.TRIGSRC = DMA_CH_TRIGSRC_USARTC0_RXC_gc;
	DMA.Debug_DMA_Channel.TRFCNT = Debug_BufferSize;	// 1024 bytes in block
	DMA.Debug_DMA_Channel.REPCNT  = 0;		// repeat forever
	
	DMA.Debug_DMA_Channel.SRCADDR0 = (((uint16_t)(&Debug_Usart.DATA) >> 0) & 0xFF);
	DMA.Debug_DMA_Channel.SRCADDR1 = (((uint16_t)(&Debug_Usart.DATA) >> 8) & 0xFF);
	DMA.Debug_DMA_Channel.SRCADDR2 = 0x00;
	
	DMA.Debug_DMA_Channel.DESTADDR0 = (((uint16_t)(&DebugBuffer[0]) >> 0) & 0xFF);
	DMA.Debug_DMA_Channel.DESTADDR1 = (((uint16_t)(&DebugBuffer[0]) >> 8) & 0xFF);
	DMA.Debug_DMA_Channel.DESTADDR2 = 0x00;
	
	DMA.Debug_DMA_Channel.CTRLA |= DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;
	
	Debug_Port.DIRSET = Debug_TX_pin_bm;
	Debug_Port.DIRCLR = Debug_RX_pin_bm;


    Debug_Flow_Port.DIRCLR = (1<<Debug_RTS_pin);
    Debug_Flow_Port.DIRSET = (1<<Debug_CTS_pin);
	Debug_Flow_Port.OUTCLR = (1<<Debug_CTS_pin);

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
	
	Debug_ClearBuffer();
	
}
void Debug_ClearBuffer(void){
	if(Debug_CharReadyToRead()){
		Debug_writeLocation  = DMA.Debug_DMA_Channel.DESTADDR1 << 8;
		Debug_writeLocation += DMA.Debug_DMA_Channel.DESTADDR0;
		Debug_writeLocation -= (uint16_t)(&DebugBuffer[0]);
		Debug_readLocation = Debug_writeLocation;
	}
}


bool Debug_CharReadyToRead(void){
    Debug_writeLocation  = DMA.Debug_DMA_Channel.DESTADDR1 << 8;
	Debug_writeLocation += DMA.Debug_DMA_Channel.DESTADDR0;
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


bool Debug_SendByte(uint8_t data){
	while(!(Debug_Usart.STATUS & USART_DREIF_bm));
	while(((Debug_Flow_Port.IN)&(1<<Debug_RTS_pin)) > 0){	// Wait for RTS to be low
		Debug_timeOutCounter++;
		if(Debug_timeOutCounter > 10000){
			Debug_timeOutCounter = 0;
			return false;
		} else {
			_delay_ms(1);
		}
	}
	Debug_Usart.DATA = data;
	return true;
}

bool Debug_SendString(char string [],bool CR){
	for(uint8_t i = 0; i < strlen(string); i++){
		if(!Debug_SendByte(string[i])){
			return false;
		}
	}

	if(CR){
		if(!Debug_SendByte(13)){
			return false;
		}
		if(!Debug_SendByte(10)){
			return false;
		}
	}
	return true;
}
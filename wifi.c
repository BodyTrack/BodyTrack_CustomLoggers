//___________________________________________
//				Joshua Schapiro
//					wifi.c
//			  created: 08/27/2010
//			  updated: 09/21/2010
//
// 09/14/2010 - Got external oscillator working, so baud rate was changed
// 09/21/2010 - Added functions for sending commands to wifi module	
//							> Wifi_EnterCMDMode
//							> Wifi_SendCommand
//
//___________________________________________

#include "avr_compiler.h"
#include "string.h"
#include "wifi.h"

#define Wifi_BufferSize 1024

volatile uint8_t    WifiBuffer[Wifi_BufferSize];
volatile uint16_t  	Wifi_readLocation = 0;
volatile uint16_t   Wifi_writeLocation = 0;

volatile char resp [50];
char string[50];
//volatile char time [25];
 char timeUpper[10];
volatile char timeLower[10];
volatile char ip   [20];
volatile uint8_t respLen = 0;
volatile uint8_t toSendLen = 0;
volatile uint8_t okLen = 0;
char timeString[15];
volatile uint8_t timeLength = 0;
volatile uint32_t time_secs = 0;
volatile uint32_t time_nanos = 0;
volatile bool connected = false;
volatile char connectionStatus[3];
char macAddr[20];
char rssi[10];

void Wifi_Init(uint32_t baud){

	//Wifi_Connected_Port.Wifi_Connected_PinCTRL = PORT_OPC_PULLUP_gc;
	Wifi_Connected_Port.DIRCLR = (1<<Wifi_Connected_pin);

	Wifi_Usart.CTRLB &= (~USART_RXEN_bm);
	Wifi_Usart.CTRLB &= (~USART_TXEN_bm);
	_delay_ms(1000);

	Wifi_Port.DIRSET = Wifi_TX_pin_bm;
	Wifi_Port.DIRCLR = Wifi_RX_pin_bm;

	Wifi_Usart.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | (false ? USART_SBMODE_bm : 0);
	 	    	
	if(baud == 9600){
		Wifi_Usart.BAUDCTRLA = 95 & 0xFF;
		Wifi_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(95 >> 8);
	} else if(baud == 115200){
		Wifi_Usart.BAUDCTRLA = 7 & 0xFF;
		Wifi_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(7 >> 8);
	} else if(baud == 230400){
		Wifi_Usart.BAUDCTRLA = 3 & 0xFF;
		Wifi_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(3 >> 8);
	} else if(baud == 460800){
		Wifi_Usart.BAUDCTRLA = 1 & 0xFF;
		Wifi_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(1 >> 8);
	} else if(baud == 921600){
		Wifi_Usart.BAUDCTRLA = 0 & 0xFF;
		Wifi_Usart.BAUDCTRLB = (0 << USART_BSCALE0_bp)|(0 >> 8);
	}
	
	Wifi_Usart.CTRLB |= USART_RXEN_bm;
	Wifi_Usart.CTRLB |= USART_TXEN_bm;
		
}

void Wifi_ClearBuffer(void){
	if(Wifi_CharReadyToRead()){
		Wifi_writeLocation  = DMA.CH0.DESTADDR1 << 8;
		Wifi_writeLocation += DMA.CH0.DESTADDR0;
		Wifi_writeLocation -= (uint16_t)(&WifiBuffer[0]);
		Wifi_readLocation = Wifi_writeLocation;
	}
}

bool Wifi_CharReadyToRead(void){
	Wifi_writeLocation  = DMA.CH0.DESTADDR1 << 8;
	Wifi_writeLocation += DMA.CH0.DESTADDR0;
	Wifi_writeLocation -= (uint16_t)(&WifiBuffer[0]);
	
	if(Wifi_writeLocation == Wifi_readLocation){
		return false;  
	} else { 
		return true;
	}
}

uint8_t Wifi_GetByte(bool blocking){
	if(blocking){
		while(!Wifi_CharReadyToRead());
	}

	uint8_t tmp = WifiBuffer[Wifi_readLocation];
	Wifi_readLocation++;
	if(Wifi_readLocation >= Wifi_BufferSize){
	  Wifi_readLocation=0;
	}
	return tmp;
}


void Wifi_SendByte(uint8_t data){
	while(!(Wifi_Usart.STATUS & USART_DREIF_bm));
	Wifi_Usart.DATA = data;
}

void Wifi_SendString(char string [],bool CR){
	for(uint8_t i = 0; i < strlen(string); i++){
		Wifi_SendByte(string[i]);
	}

if(CR){
		Wifi_SendByte(13);
		Wifi_SendByte(10);
	}
}

bool Wifi_EnterCMDMode(uint16_t timeOut){
	char resp [3];
	char   ok [3] = "CMD";
	uint8_t j = 0;
	
	Wifi_ClearBuffer();
	Wifi_SendString("$$$", false);
	for(uint16_t i = 0; i < timeOut; i++){
		if(Wifi_CharReadyToRead()){
			resp[j] = Wifi_GetByte(false);
			j++;
			if(j > 2){
				for(uint8_t k = 0; k < 3; k++){					
					if(resp[k] != ok[k]){
						return false;
					}
				}
				return true;
			}
		}
		_delay_ms(1);
	}
	return false;
}

bool Wifi_ExitCMDMode(uint16_t timeOut){
	if(Wifi_SendCommand("exit","EXIT","EXIT",timeOut)){
		return true;
	} else {
		return false;
	}
}

bool Wifi_SendCommand(char toSend [], char ok [], char ok2 [], uint16_t timeOut){
	respLen = 0;
	toSendLen = strlen(toSend);
	if(strlen(ok) > strlen(ok2)){
		okLen = strlen(ok2);
	} else {
		okLen = strlen(ok);
	}

	Debug_SendString("-----------------",true);
	Debug_SendString("Command: ",false);
	Debug_SendString(toSend,true);
	Wifi_ClearBuffer();
	Wifi_SendString(toSend, true);
	for(uint16_t i = 0; i < timeOut; i++){
		if(Wifi_CharReadyToRead()){ 
			resp[respLen] = Wifi_GetByte(false);
			respLen++;
			if(respLen == okLen + toSendLen + 3){		
				
				Debug_SendString("Responce: ",false);
				for(uint8_t j = 0; j < respLen; j++){
					Debug_SendByte(resp[j]);
				}
				Debug_SendString(",want: ",false);
				Debug_SendString(ok,true);
				
				
				for(uint8_t k = 0; k < toSendLen; k++){
					if(resp[k] != toSend[k]){
						return false;
					}
				}
				for(uint8_t l = (toSendLen+3); l < (respLen-3); l++){
					if(resp[l] != ok[l-toSendLen-3]){
						if(resp[l] != ok2[l-toSendLen-3]){
							return false;
						}
					}
				}
				Debug_SendByte(13);
				return true;
			}
		}
		_delay_ms(1);
	}
	return false;
}

bool Wifi_GetTime(uint16_t timeOut){
	uint8_t tmp=0;
	uint32_t tmp32 = 0;
	
	Wifi_ClearBuffer();
	Wifi_SendCommand("show t t","Time=","Time=",500);
	_delay_ms(10);
	
	while(Wifi_CharReadyToRead()){
		if(tmp < 50){
			string[tmp] = Wifi_GetByte(false);
			tmp++;
		} else {
			break;
		}
	}

	Debug_SendByte(13);
	
	if(tmp < 4){
		time_secs = 0;
		return false;
	}
	
	if(string[0] == 'N'){
		return false;
	}

	strtok(string,"=");
	strtok(NULL,"=");
	strcpy(timeString,strtok(NULL," "));
		
	for(uint8_t i = 0; i < 8; i++){
		timeUpper[i] = timeString[i];
	}
	
	tmp = 8;
	for(uint8_t i = 0; i < 5; i++){
		timeLower[i] = timeString[tmp];
		tmp++;
	}

	tmp32 = atol(timeUpper);
	tmp32 -= 22089888;
	tmp32 *= 100;
	tmp32 += ((timeLower[0]-'0')* 10);
	tmp32 += ((timeLower[1]-'0'));
	
	time_secs = tmp32;
	return true;
}

bool Wifi_Connected(uint16_t timeOut){
	for(uint16_t i = 0; i < timeOut; i++){
		if((Wifi_Connected_Port.IN & (1<<Wifi_Connected_pin)) >0 ){
			connected = true;
			return true;
		}
		_delay_ms(1);
	}
	connected = false;
	return false;
}

bool Wifi_GetMac(uint16_t timeOut){
	uint8_t tmp = 0;
	Wifi_ClearBuffer();
	Wifi_SendCommand("get mac","Mac Addr=","Mac Addr=",timeOut);
	for(uint16_t i = 0; i < timeOut; i++){
		if(Wifi_CharReadyToRead()){ 
			macAddr[tmp] = Wifi_GetByte(false);
			tmp++;
			if(tmp == 17){
				Debug_SendString("Got Mac=",false);
				Debug_SendString(macAddr,true);
				return true;
			}
		}
		_delay_ms(1);
	}
	return false;
}

uint8_t Wifi_GetSignalStrength(uint16_t timeOut){
	uint8_t tmp = 0;
	uint8_t ss = 0;
	uint32_t worker = 0;
	Wifi_SendCommand("show rssi","RSSI=(-","RSSI=(-",timeOut);
	for(uint16_t i = 0; i < timeOut; i++){
		if(Wifi_CharReadyToRead()){
			rssi[tmp] = Wifi_GetByte(false);
			if(rssi[tmp] == ')'){
				rssi[tmp] = 0;
				//Debug_SendString("rssi: ", false);
				//Debug_SendString(rssi, true);
				worker = atoi(rssi);
				worker*=9208;
				worker = 1045100 - worker;
				worker /= 10000;
				ss = worker & 0xFF;

				//sprintf(string, "strength: %u %",ss);
				//Debug_SendString(string,true);

				return ss;
			}
			tmp++;
		}
		_delay_ms(1);
	}
	return 0;


}

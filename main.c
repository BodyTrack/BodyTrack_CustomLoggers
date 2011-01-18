//___________________________________________
//				Joshua Schapiro
//					main.c
//			  created: 08/27/2010
//
//	Firmware:   1.00 - 08/27/2010 - Initial testing
//				1.01 - time per tick changed for 32-bit counter
//				2.00 - 01/17/2011 - Changes made to support new new hardware
//								  - RTC sync supported
//								  - recording multiple files supported
//
//___________________________________________

#include "avr_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "debug.c"
#include "wifi.c"
#include "dpad.c"
#include "leds.c"
#include "rs232.c"
#include "diskio.c"
#include "ff.c"
#include "sd.c"
#include "display.c"
#include "time.c"
#include "sensors.c"
#include "light.c"
#include "rtc.c"






// ********************************** Functions *********************************

void Clock_Init(void);
void Disable_JTAG(void);

void DMA_Init(void);
void Interrupt_Init(void);

void Read_config_file(void);
void Config_Wifi(void);



void Debug_To_Wifi(void);
void Rs232_To_Debug(void);

void SD_BackroundWriter_Init(void);
void SD_WriteTemperatureBuffer(uint8_t bufferNumber);
void SD_WriteHumidityBuffer(uint8_t bufferNumber);
void SD_WritePressureBuffer(uint8_t bufferNumber);
void SD_WriteMicrophoneBuffer(uint8_t bufferNumber);
void SD_WriteLightBuffer(uint8_t bufferNumber);
void SD_WriteAirSample(void);

void SD_WriteRTCBlock(uint32_t ticker, uint32_t time);




// ********************************** Variables *********************************

char temp [50];

char auth [50] = "set wlan auth ";
char phrase [50] = "set wlan phrase ";
char port [50] = "set ip localport ";
char ssid [50] = "join ";

bool authRead = false;
bool phraseRead = false;
bool portRead = false;
bool ssidRead = false;






char debugMsg[150];

	
int main(void){
	_delay_ms(5);


	Clock_Init();
	display_init();

	Time_Init();
	Sensors_Init();
	Leds_Init();
	Dpad_Init();
	Debug_Init();
	Rs232_Init();

	Wifi_Init(9600);
	Light_Init(LightAddress);
	SD_Init();

	SD_BackroundWriter_Init();
	DMA_Init();

	Interrupt_Init();
	Disable_JTAG();

	if(Dpad_CheckButton(Select)){
		Debug_SendString("Entering Debug to Wifi function",true);
		Debug_To_Wifi();
	}


	display_putString("booting up",0,0,System5x7);
	display_writeBufferToScreen();
	Debug_SendString("booting up...",true);
	for(uint8_t i = 60; i < 90; i+=6){
	  display_putString(".",0,i,System5x7);
	  display_writeBufferToScreen();
	  _delay_ms(1000);
	}
	
	SD_CD_Port.PIN1CTRL = PORT_OPC_PULLUP_gc;


	while(!SD_Inserted()){
		Debug_SendString("SD?", true);
		Leds_Toggle(sd_Red);
		_delay_ms(500);
	}
	Leds_Clear(sd_Red);
	Leds_Set(sd_Green);
	_delay_ms(1000);
	
	
	Read_config_file();

	while(!connected){
		Config_Wifi();
		_delay_ms(500);
	}
	Leds_Clear(wifi_Red);
	Time_Set(time_secs);
	
	

	_delay_ms(500);

	
	
	if(Dpad_CheckButton(Left)){
		Debug_SendString("Entering Debug to Wifi function",true);
		Debug_To_Wifi();
	}
	


	// starting recording
	while(true){

		while(true){
			if(SD_StartLogFile(UNIX_time) == 0){
				Leds_Clear(sd_Red);
				break;
			} else {
				Leds_Set(sd_Red);

				display_clearBuffer();
				display_writeBufferToScreen();
				display_putString("    uSD Error    ",3,0,System5x7);
				display_putString(" Please Remove & ",6,0,System5x7);
				display_putString("  Reinsert Card  ",7,0,System5x7);
				display_writeBufferToScreen();

				while(SD_Inserted());
				_delay_ms(100);
				while(!SD_Inserted());
				_delay_ms(100);
				SD_Init();
			}
		}

		Leds_Clear(sd_Green);
		Leds_Clear(wifi_Green);

		display_clearBuffer();
		display_writeBufferToScreen();
		display_putString("    RECORDING!   ",3,0,System5x7);
		display_putString(" press select to ",6,0,System5x7);
		display_putString(" stop recording ",7,0,System5x7);
		display_writeBufferToScreen();
	
		Debug_SendString("RTC Block: ",false);
		SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_time);
		Debug_SendString(ltoa(UNIX_time,temp,10),false);
		Debug_SendString(", ",false);
		Debug_SendString(ltoa(Time_Get32BitTimer(),temp,10),true);;
	

		Rs232_ClearBuffer();
		rs232Recording = true;
		recording = true;
	
		_delay_ms(2000);
		while(true){


			if((SD_CD_Port.IN & (2)) == 0 ){

				display_clearBuffer();
				display_writeBufferToScreen();

				Leds_Set(ext_Green);
				Leds_Set(ext_Red);
				display_putString("RTC connected... ",3,0,System5x7);
				display_writeBufferToScreen();
				_delay_ms(1000);
				RTC_init();
				RTC_setUTCSecs(UNIX_time);
				display_putString("RTC synced... ",5,0,System5x7);
				display_writeBufferToScreen();
				Leds_Clear(ext_Red);
				_delay_ms(1000);
				while((SD_CD_Port.IN & (2)) == 0 ){
					sprintf(temp, "Time: %lu", RTC_getUTCSecs());
					display_putString(temp,7,0,System5x7);
					display_writeBufferToScreen();
					_delay_ms(500);
				}
				Leds_Clear(ext_Green);

				display_clearBuffer();
				display_writeBufferToScreen();
				display_putString("    RECORDING!   ",3,0,System5x7);
				display_putString(" press select to ",6,0,System5x7);
				display_putString(" stop recording ",7,0,System5x7);
				display_writeBufferToScreen();

			}

			if(Dpad_CheckButton(Select)){
				rs232Recording = false;
				recording = false;
				display_clearBuffer();
				display_writeBufferToScreen();
				display_putString(" Done Recording! ",3,0,System5x7);
				display_putString("   Thank You!    ",4,0,System5x7);
				display_writeBufferToScreen();
				Sensors_ResetTemperatureBuffers();
				Sensors_ResetHumidityBuffers();
				Sensors_ResetPressureBuffers();
				Sensors_ResetMicrophoneBuffers();
				Sensors_ResetLightBuffers();

				SD_Close();
				_delay_ms(2000);
				Leds_Set(wifi_Green);
			

				while(true){
					if(SD_Inserted()){
						Leds_Set(sd_Green);
						Leds_Clear(sd_Red);
						display_writeBufferToScreen();
						display_putString(" Press Select To ",3,0,System5x7);
						display_putString("  Record Again   ",4,0,System5x7);
						display_writeBufferToScreen();
						_delay_ms(100);
						if(Dpad_CheckButton(Select)){
							break;
						}

					} else {
						Leds_Clear(sd_Green);
						Leds_Set(sd_Red);
						display_writeBufferToScreen();
						display_putString("  Please Replace ",3,0,System5x7);
						display_putString("  The uSD card   ",4,0,System5x7);
						display_writeBufferToScreen();
						_delay_ms(250);
						SD_Init();


					}
				}
				break;
			}
		}
	}


}



//******************************************************************* init functions *******************************************************************

void Clock_Init(void)
{
	OSC.XOSCCTRL = (uint8_t) OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_EXTCLK_gc;
	OSC.CTRL |=  OSC_XOSCEN_bm;
	while(!(OSC.STATUS & OSC_XOSCRDY_bm));
	uint8_t clkCtrl = ( CLK.CTRL & ~CLK_SCLKSEL_gm ) | CLK_SCLKSEL_XOSC_gc;
	CCPWrite( &CLK.CTRL, clkCtrl );
	OSC.CTRL &= ~OSC_RC2MEN_bm;
}

void Disable_JTAG(void){
	CCPWrite( &MCU.MCUCR, 1 );
}

void DMA_Init(void){
	DMA.CTRL |= DMA_ENABLE_bm; // enable DMA

	// wifi
	DMA.CH0.ADDRCTRL |= DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc	| DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;							
	DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_USARTE0_RXC_gc;
	DMA.CH0.TRFCNT = Wifi_BufferSize;	// 1024 bytes in block
	DMA.CH0.REPCNT  = 0;		// repeat forever

	DMA.CH0.SRCADDR0 = (((uint16_t)(&Wifi_Usart.DATA) >> 0) & 0xFF);
	DMA.CH0.SRCADDR1 = (((uint16_t)(&Wifi_Usart.DATA) >> 8) & 0xFF);
	DMA.CH0.SRCADDR2 = 0x00;
	
	DMA.CH0.DESTADDR0 = (((uint16_t)(&WifiBuffer[0]) >> 0) & 0xFF);
	DMA.CH0.DESTADDR1 = (((uint16_t)(&WifiBuffer[0]) >> 8) & 0xFF);
	DMA.CH0.DESTADDR2 = 0x00;
			
	DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm; 	
		
}

void Interrupt_Init(void)
{
	PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;	// enable low level interrupts
	sei();							// enable all interrupts
}

void SD_BackroundWriter_Init(void){

	// fclk = 14745600
	// div  = 64
	// per  = 2304
	// => 14745600/64/2304 => 100 samples per second

	// Set period/TOP value
	TCE1.PER = 2304;

	// Select clock source
	TCE1.CTRLA = (TCE1.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV64_gc;

	// Enable CCA interrupt
	TCE1.INTCTRLA = (TCE1.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_LO_gc;

}

ISR(TCE1_OVF_vect)
{

	if(okToSendMicrophoneBuffer1){
		SD_WriteMicrophoneBuffer(1);
		okToSendMicrophoneBuffer1  = false;
	} else if (okToSendMicrophoneBuffer2){
		SD_WriteMicrophoneBuffer(2);
		okToSendMicrophoneBuffer2 = false;
	}

	if(okToSendTemperatureBuffer1){
		Debug_SendString("T Buffer1",true);
		SD_WriteTemperatureBuffer(1);
		okToSendTemperatureBuffer1 = false;
	} else if (okToSendTemperatureBuffer2){
		Debug_SendString("T Buffer2",true);
		SD_WriteTemperatureBuffer(2);
		okToSendTemperatureBuffer2 = false;
	}

	if(okToSendHumidityBuffer1){
		Debug_SendString("H Buffer1",true);
		SD_WriteHumidityBuffer(1);
		okToSendHumidityBuffer1 = false;
	} else if (okToSendHumidityBuffer2){
		Debug_SendString("H Buffer2",true);
		SD_WriteHumidityBuffer(2);
		okToSendHumidityBuffer2 = false;
	}

	if(okToSendPressureBuffer1){
		Debug_SendString("P Buffer1",true);
		SD_WritePressureBuffer(1);
		okToSendPressureBuffer1 = false;
	} else if (okToSendPressureBuffer2){
		Debug_SendString("P Buffer2",true);
		SD_WritePressureBuffer(2);
		okToSendPressureBuffer2 = false;
	}

	if(okToSendLightBuffer1){
		Debug_SendString("L Buffer1",true);
		SD_WriteLightBuffer(1);
		okToSendLightBuffer1 = false;
	} else if (okToSendLightBuffer2){
		Debug_SendString("L Buffer2",true);
		SD_WriteLightBuffer(2);
		okToSendLightBuffer2 = false;
	}

	if(okToSendAirQuality && recording){
		Debug_SendString("A Buffer",true);
		SD_WriteAirSample();
		okToSendAirQuality = false;
	}

	if(okToSendRTCBlock){
		Debug_SendString("RTC Block: ",false);
		SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_time);
		Debug_SendString(ltoa(UNIX_time,temp,10),false);
		Debug_SendString(", ",false);
		Debug_SendString(ltoa(Time_Get32BitTimer(),temp,10),true);
		okToSendRTCBlock = false;
	}


}


void SD_WriteRTCBlock(uint32_t ticker, uint32_t time){
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number 
	SD_Write32(27);				// record size  
	SD_Write16(2); 		// record type  
	
														// payload
	
	SD_Write32(ticker);				// 32-bit counter
	SD_Write32(time);			    // UNIX time  (40bit)
	SD_Write8(0);
	SD_Write32(0);           // unix time nanoseconds
	SD_WriteCRC();			      // CRC			
		
	f_sync(&Log_File);

}


void SD_WriteTemperatureBuffer(uint8_t bufferNumber){
	
		
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number 
	SD_Write32(62);				// record size  
	SD_Write16(3); 		// record type  
	
														// payload
	if(bufferNumber == 1){
		SD_Write32(temperatureSampleStartTime1);					// time
	} else {
		SD_Write32(temperatureSampleStartTime2);					// time
	}
	SD_Write32(1843200);										// sample period (1hz)
	SD_Write32(10);												// number of samples
		
	SD_WriteString("Temperature");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	if(bufferNumber == 1){
		for(uint8_t i = 0; i < 10; i++){
			SD_Write16(temperatureBuffer1[i]);
		}
	} else {
		for(uint8_t i = 0; i < 10; i++){
			SD_Write16(temperatureBuffer2[i]);
		}
	}
	
	SD_WriteCRC();			// CRC			
		
	f_sync(&Log_File);

}

void SD_WriteHumidityBuffer(uint8_t bufferNumber){
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(48);				// record size
	SD_Write16(3); 		// record type

											// payload
	if(bufferNumber == 1){
		SD_Write32(humiditySampleStartTime1);					// time
	} else {
		SD_Write32(humiditySampleStartTime2);					// time
	}
	SD_Write32(1843200);										// sample period (1hz)
	SD_Write32(10);												// number of samples

	SD_WriteString("Humidity");
	SD_Write8(0x09);
	SD_WriteString("8");
	SD_Write8(0x0A);
	SD_Write8(0x00);

	if(bufferNumber == 1){
		for(uint8_t i = 0; i < 10; i++){
			SD_Write8(humidityBuffer1[i]);
		}
	} else {
		for(uint8_t i = 0; i < 10; i++){
			SD_Write8(humidityBuffer2[i]);
		}
	}

	SD_WriteCRC();			// CRC

	f_sync(&Log_File);

}


void SD_WritePressureBuffer(uint8_t bufferNumber){
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(48);				// record size
	SD_Write16(3); 		// record type

											// payload
	if(bufferNumber == 1){
		SD_Write32(pressureSampleStartTime1);					// time
	} else {
		SD_Write32(pressureSampleStartTime2);					// time
	}
	SD_Write32(1843200);										// sample period (1hz)
	SD_Write32(10);												// number of samples

	SD_WriteString("Pressure");
	SD_Write8(0x09);
	SD_WriteString("8");
	SD_Write8(0x0A);
	SD_Write8(0x00);

	if(bufferNumber == 1){
		for(uint8_t i = 0; i < 10; i++){
			SD_Write8(pressureBuffer1[i]);
		}
	} else {
		for(uint8_t i = 0; i < 10; i++){
			SD_Write8(pressureBuffer2[i]);
		}
	}

	SD_WriteCRC();			// CRC

	f_sync(&Log_File);

}

void SD_WriteMicrophoneBuffer(uint8_t bufferNumber){

	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(40+microphoneNumberOfSamples);		// record size
	SD_Write16(3); 									// record type

														// payload
	if(bufferNumber == 1){
		SD_Write32(microphoneSampleStartTime1);					// time
	} else {
		SD_Write32(microphoneSampleStartTime2);					// time
	}


	SD_Write32(256);								// sample period (7.2khz)



	SD_Write32(microphoneNumberOfSamples);			// number of samples

	SD_WriteString("Microphone");
	SD_Write8(0x09);
	SD_WriteString("8");
	SD_Write8(0x0A);
	SD_Write8(0x00);

	if(bufferNumber == 1){
		SD_WriteBuffer(microphoneBuffer1,microphoneNumberOfSamples);
	} else {
		SD_WriteBuffer(microphoneBuffer2,microphoneNumberOfSamples);
	}

	SD_WriteCRC();									// CRC

	f_sync(&Log_File);

}

void SD_WriteLightBuffer(uint8_t bufferNumber){


	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(244);				// record size
	SD_Write16(3); 		// record type

														// payload
	if(bufferNumber == 1){
		SD_Write32(lightSampleStartTime1);					// time
	} else {
		SD_Write32(lightSampleStartTime2);					// time
	}
	SD_Write32(1843200);										// sample period (1hz)
	SD_Write32(10);												// number of samples

	SD_WriteString("Light_Green");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Light_Red");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Light_Blue");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Light_Clear");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);

	SD_Write8(0x00);

	if(bufferNumber == 1){
		for(uint8_t i = 0; i < 40; i++){
			SD_Write32(lightBuffer1[i]);
		}
	} else {
		for(uint8_t i = 0; i < 40; i++){
			SD_Write32(lightBuffer2[i]);
		}
	}

	SD_WriteCRC();			// CRC

	f_sync(&Log_File);

}

void SD_WriteAirSample(void){


	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(61);				// record size
	SD_Write16(3); 		// record type

														// payload

	SD_Write32(airSampleTime);					// time

	SD_Write32(110592000);										// sample period (0.01667hz)
	SD_Write32(1);												// number of samples

	SD_WriteString("Air_Small");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_Large");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);

	SD_Write8(0x00);

	SD_Write32(smallCount);
	SD_Write32(largeCount);

	SD_WriteCRC();			// CRC

	f_sync(&Log_File);

}

void Read_config_file(void){

	while(!SD_Inserted()){
		Leds_Toggle(sd_Red);
		_delay_ms(500);
	}
	Leds_Clear(sd_Red);
	Leds_Set(sd_Green);
	_delay_ms(1000);
	

	SD_Open("/config.txt");

	while(true){
	  f_gets(temp,50,&Log_File);
	  if(temp[0] != 0){
	    if(strstr(temp,"ssid") != 0){
	      strtok(temp,"=");
	      strcat(ssid,strtok(NULL,"="));
	      ssidRead = true;
	    } else if(strstr(temp,"phrase") != 0){
	      strtok(temp,"=");
	      strcat(phrase,strtok(NULL,"="));
	      phraseRead = true;
	    } else if(strstr(temp,"port") != 0){
	      strtok(temp,"=");
	      strcat(port,strtok(NULL,"="));
	      portRead = true;
	    } else if(strstr(temp,"auth") != 0){
	      strtok(temp,"=");
	      strcat(auth,strtok(NULL,"="));
	      authRead = true;
	    }
	  } else {
	    break;
	  }
	}
	
	
}

void Config_Wifi(void){
	uint8_t col = 0;
	
	Wifi_ClearBuffer();



	Wifi_EnterCMDMode(1000);
	_delay_ms(1000);
	display_clearBuffer();
	display_writeBufferToScreen();
	

	if(Wifi_SendCommand("factory RESET","Set Factory Defaults","Set Factory Defaults",500)){
		display_putString("reset..........OK",col,0,System5x7);
	} else {
		display_putString("reset........FAIL",col,0,System5x7);
	}
	display_writeBufferToScreen();
	col++;
	
	_delay_ms(1000);
	
	Wifi_ClearBuffer();
	Wifi_SendCommand("set comm remote 0","AOK","AOK",500);
	_delay_ms(1000);

	Wifi_SendCommand("set comm open 0","AOK","AOK",500);	
	_delay_ms(1000);
	
	Wifi_SendCommand("set comm close AOK","AOK","AOK",500);		
	_delay_ms(1000);
	
	if(Wifi_SendCommand("set time enable 1","AOK","AOK",500)){
		display_putString("enable time....OK",col,0,System5x7);
	} else {
		display_putString("enable time..FAIL",col,0,System5x7);
	}
	display_writeBufferToScreen();
	_delay_ms(1000);
	col++;
	
	if(authRead){
		if(Wifi_SendCommand(auth,"AOK","AOK",500)){
			display_putString("encryption.....OK",col,0,System5x7);
		} else {
			display_putString("encryption...FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(1000);
		col++;
	}
	
	if(phraseRead){
		if(Wifi_SendCommand(phrase,"AOK","AOK",500)){
			display_putString("phrase.........OK",col,0,System5x7);
		} else {
			display_putString("phrase.......FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(1000);
		col++;
	}
	
	if(portRead){
		if(Wifi_SendCommand(port,"AOK","AOK",500)){
			display_putString("port...........OK",col,0,System5x7);
		} else {
			display_putString("port.........FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(1000);
		col++;
	}
	
	if(ssidRead){
		if(Wifi_SendCommand(ssid,"DeAut","Auto+",2000)){
			display_putString("ssid...........OK",col,0,System5x7);
		} else {
			display_putString("ssid.........FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(1000);
		col++;
	}
	
	
	Wifi_SendCommand("get time","TimeEna=1","TimeEna=1",500);
	_delay_ms(1000);
	
	Wifi_ExitCMDMode(500);
	
	_delay_ms(1000);
	
	Wifi_EnterCMDMode(500);
	_delay_ms(1000);
	if(Wifi_Connected(1000)){
		display_putString("connection.....OK",col,0,System5x7);
		connected = true;
	} else {
		display_putString("connection...FAIL",col,0,System5x7);
		connected = false;
	}
	display_writeBufferToScreen();
	_delay_ms(1000);
	col++;
	Wifi_ExitCMDMode(500);
	
	if(connected){
		_delay_ms(1000);
		Wifi_EnterCMDMode(500);
		if(Wifi_GetTime(1000)){
			display_putString("internet.......OK",col,0,System5x7);
			Leds_Set(wifi_Green);

		} else{
			display_putString("internet.....FAIL",col,0,System5x7);
			Leds_Set(wifi_Red);
			connected = false;
		}

		display_writeBufferToScreen();
		Wifi_GetMac(1000);
		Wifi_ExitCMDMode(1000);
	} else{
		display_putString("internet.....FAIL",col,0,System5x7);
		display_writeBufferToScreen();
		Leds_Set(wifi_Red);
	}

}


void Rs232_To_Debug(void){
	while(1){
		if(Rs232_CharReadyToRead()){
			Debug_SendByte(Rs232_GetByte(true));
		}		
	}
}


void Debug_To_Wifi(void){
	while(1){
		if(Debug_CharReadyToRead()){
			Wifi_SendByte(Debug_GetByte(true));
		}
		if(Wifi_CharReadyToRead()){
			Debug_SendByte(Wifi_GetByte(true));
		}	
	
	}
}

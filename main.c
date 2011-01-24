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
//				2.01 - 01/18/2011 - WEP128 supported
//				2.02 - 01/21/2011 - uses the wifi "connection status" pin for connection status
//								  - new GUI
//								  - time is resynced every time a new logfile is created
//								  - picoseconds per tick fixed in the SD_StartLogFile() function
//								  - changed the led pins to go better with the GUI
//								  - time zones supported (in US only), but no day light savings time
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
char key [50] = "set wlan key ";
char port [50] = "set ip localport ";
char ssid [50] = "join ";
char zone [10] = "GMT";

bool authRead = false;
bool phraseRead = false;
bool keyRead = false;
bool portRead = false;
bool ssidRead = false;
bool zoneChanged = false;

uint8_t timeZoneShift = 0;
uint8_t clockHour = 0;

volatile uint8_t currentMode      = 0;
uint8_t ssRefreshCounter = 0;
uint8_t signalStrength   = 0;
uint8_t uploadPercentBS  = 0;
uint8_t uploadPercentEXT = 0;

bool rtcSynced			 = false;

bool needToChangeBaud = true;

bool demoMode = false;
bool demoModeFirstFile = true;


	

// ********************************** Main Program *********************************


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


	Debug_SendString("booting up...",true);


	display_putString("   BaseStation   ",1,0,System5x7);
	strcat(temp,"  Hardware: v");
	strcat(temp,HardwareVersion);
	display_putString(temp,3,0,System5x7);
	temp[0] = 0;
	strcat(temp," Firmware: v");
	strcat(temp,FirmwareVersion);
	display_putString(temp,5,0,System5x7);
	display_writeBufferToScreen();
	if(demoMode){
		display_putString("    Demo Mode    ",7,0,System5x7);
	}
	display_writeBufferToScreen();
	_delay_ms(2000);
	



	while(!SD_Inserted()){
		Debug_SendString("SD?", true);
		Leds_Toggle(sd_Red);
		_delay_ms(500);
	}
	Leds_Clear(sd_Red);
	Leds_Set(sd_Green);
	_delay_ms(1000);
	
	
	Read_config_file();


	while(!connected && !demoMode){
		Config_Wifi();
		_delay_ms(500);
	}
	
	if(demoMode){
		Leds_Set(wifi_Green);
	}


	_delay_ms(500);

	
	
	


	display_clearBuffer();
	display_writeBufferToScreen();

StartRecording:
	SD_Init();
	Wifi_EnterCMDMode(500);
	if(!demoMode){
		Wifi_GetTime(1000);
	}else{
		if(demoModeFirstFile){
			demoModeFirstFile = false;
			UNIX_time = 1928383;
		}
	}
	Wifi_ExitCMDMode(500);
	sprintf(temp,"time: %lu", time_secs);
	Debug_SendString(temp,true);

	while(true){


		if(SD_StartLogFile(UNIX_time) == 0){								// open file

			Leds_Clear(sd_Green);
			Leds_Clear(wifi_Green);
			timeRecordingStarted = UNIX_time;

			Debug_SendString("RTC Block: ",false);							// send rtc block
			SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_time);
			Debug_SendString(ltoa(UNIX_time,temp,10),false);
			Debug_SendString(", ",false);
			Debug_SendString(ltoa(Time_Get32BitTimer(),temp,10),true);

			Rs232_ClearBuffer();
			rs232Recording = true;
			recording = true;





			while(true){

				if(ssRefreshCounter == 0){
					Wifi_EnterCMDMode(1000);
					signalStrength = Wifi_GetSignalStrength(1000);
					Wifi_ExitCMDMode(500);
				}
				ssRefreshCounter++;

				// controls


				if(currentMode == recordMode && Dpad_CheckButton(Up) && !recording){								// start recording

					display_putString("Recording      0m",0,0,System5x7);
					display_drawLine(1,60,7,60,true);		// up arrow
					display_drawPixel(2,59,true);
					display_drawPixel(3,58,true);
					display_drawPixel(2,61,true);
					display_drawPixel(3,62,true);

					display_writeBufferToScreen();

					Leds_Clear(sd_Green);
					Leds_Clear(sd_Red);
					Leds_Clear(wifi_Green);
					Leds_Clear(wifi_Red);
					Leds_Clear(ext_Green);
					Leds_Clear(ext_Red);

					_delay_ms(1000);
					goto StartRecording;
				} else if(currentMode == recordMode && Dpad_CheckButton(Up) && recording){							// pause recording

					rs232Recording = false;
					recording = false;

					Sensors_ResetTemperatureBuffers();
					Sensors_ResetHumidityBuffers();
					Sensors_ResetPressureBuffers();
					Sensors_ResetMicrophoneBuffers();
					Sensors_ResetLightBuffers();
					SD_Close();
					display_putString("Paused           ",0,0,System5x7);
					display_drawLine(1,60,7,60,true);		// up arrow
					display_drawPixel(2,59,true);
					display_drawPixel(3,58,true);
					display_drawPixel(2,61,true);
					display_drawPixel(3,62,true);
					display_writeBufferToScreen();


					if(demoMode){
						Leds_Set(wifi_Green);
					} else {
						Wifi_EnterCMDMode(500);
						if(Wifi_GetTime(500)){
							Leds_Set(wifi_Green);
						} else {
							Leds_Set(wifi_Red);
						}
						Wifi_ExitCMDMode(500);
					}
					if(SD_Inserted()){
						Leds_Set(sd_Green);
					} else {
						Leds_Set(sd_Red);
					}
					if(!chargeComplete && SD2_Inserted()){
						Leds_Set(ext_Red);
						Leds_Set(ext_Green);

					} else if(chargeComplete && SD2_Inserted()){
						Leds_Clear(ext_Red);
						Leds_Set(ext_Green);
					}



					_delay_ms(500);
				} else if(currentMode == recordMode && Dpad_CheckButton(Down)){										// go to sensorMode

					currentMode = sensorMode;
					display_clearBuffer();
					display_writeBufferToScreen();
				} else if(currentMode == sensorMode && Dpad_CheckButton(Up)){										// go to recordMode

					currentMode = recordMode;
					display_clearBuffer();
					display_writeBufferToScreen();
					_delay_ms(400);
				} else if(currentMode == recordMode && !recording && !SD_Inserted()){											// dont allow to start recording
					Leds_Set(sd_Red);
					Leds_Clear(sd_Green);
				} else if(currentMode == recordMode && !recording && SD_Inserted() && !Dpad_CheckButton(Up)){
					Leds_Clear(sd_Red);
					Leds_Set(sd_Green);
				}

				// load displays

				if(currentMode == recordMode){																		// show record screen

					if(recording){
						sprintf(temp, "Recording   %4lum", (UNIX_time - timeRecordingStarted)/60);		// load recording screen
						display_putString(temp,0,0,System5x7);
					} else {
						display_putString("Paused           ",0,0,System5x7);
					}

					display_drawLine(1,60,7,60,true);		// up arrow
					display_drawPixel(2,59,true);
					display_drawPixel(3,58,true);
					display_drawPixel(2,61,true);
					display_drawPixel(3,62,true);

					sprintf(temp, "Uploading    %3u", uploadPercentBS);
					strcat(temp,"%");
					display_putString(temp,1,0,System5x7);



					if(chargePercent == 100){
						chargeComplete = true;
						okToCharge  = false;
					}


					if(SD2_Inserted() && chargeComplete){
						display_putString("Ext Charged      ",3,0,System5x7);
						if(!recording){
							Leds_Clear(ext_Red);
							Leds_Set(ext_Green);
						}
					} else if(SD2_Inserted() && !chargeComplete){
						sprintf(temp, "Ext Charging  %2u",chargePercent);
						strcat(temp,"%");
						display_putString(temp,3,0,System5x7);
						okToCharge = true;
						if(!rtcSynced){
							Debug_SendString("Syncing RTC", true);
							RTC_init();
							RTC_setUTCSecs(UNIX_time);
							rtcSynced = true;
							Debug_SendString("RTC synced", true);
						}
						if(!recording){
							Leds_Set(ext_Red);
							Leds_Set(ext_Green);
						}
					}else{
						display_putString("Ext Removed      ",3,0,System5x7);
						chargePercent = 0;
						chargeComplete = false;
						rtcSynced = false;
						Leds_Clear(ext_Red);
						Leds_Clear(ext_Green);
					}





					sprintf(temp, "Uploading    %3u", uploadPercentEXT);
					strcat(temp,"%");
					display_putString(temp,4,0,System5x7);

					RTC_UTCSecsToTime(UNIX_time,&time);
					clockHour = time.Hour + 24;
					clockHour -= timeZoneShift;
					if(clockHour > 24){
						clockHour -= 24;
					}

					sprintf(temp,"Time %2u:%02u:%02u ", clockHour, time.Minute, time.Second);
					strcat(temp,zone);
					display_putString(temp,6,0,System5x7);

					sprintf(temp, "Wifi %3u",signalStrength);
					strcat(temp,"%   more");
					display_putString(temp,7,0,System5x7);


					display_drawLine(56,98,63,98,true);		// down arrow
					display_drawPixel(62,97,true);
					display_drawPixel(61,96,true);
					display_drawPixel(62,99,true);
					display_drawPixel(61,100,true);

					display_writeBufferToScreen();
					_delay_ms(50);





				} else if(currentMode == sensorMode){																// show sensor screen

					display_clearBuffer();
					display_putString("   Sensors  back",0,0,System5x7);
					display_drawLine(8, 15, 8,61,true);

					display_drawLine(1,99,7,99,true);		// up arrow
					display_drawPixel(2,98,true);
					display_drawPixel(3,97,true);
					display_drawPixel(2,100,true);
					display_drawPixel(3,101,true);


					sprintf(temp,"Temperature: %3uC", quickTemperature);
					display_putString(temp,2,0,System5x7);
					sprintf(temp,"Humidity:  %3u", quickHumidity);
					strcat(temp, "%RH");
					display_putString(temp,3,0,System5x7);
					sprintf(temp,"Pressure:  %3ukPa", quickPressure);
					display_putString(temp,4,0,System5x7);
					sprintf(temp,"Light:      %5u", quickLight);
					display_putString(temp,5,0,System5x7);
					sprintf(temp,"Air: %5lu, %5lu", quickSmall, quickLarge);
					display_putString(temp,6,0,System5x7);
					display_putString("Sound:           ",7,0,System5x7);
					display_drawRectangle(57,50,7,quickMic/2,true,false,true);
					display_writeBufferToScreen();
					_delay_ms(50);

					if(chargePercent == 100){
						chargeComplete = true;
						okToCharge  = false;
					}

					if(SD2_Inserted() && chargeComplete && !recording){
						Leds_Clear(ext_Red);
						Leds_Set(ext_Green);
					} else if(SD2_Inserted() && !chargeComplete){
						okToCharge = true;
						if(!rtcSynced){
							Debug_SendString("Syncing RTC", true);
							if(!demoMode){
								RTC_init();
								RTC_setUTCSecs(UNIX_time);
							}
							rtcSynced = true;
						}
						if(!recording){
							Leds_Set(ext_Red);
							Leds_Set(ext_Green);
						}

					}else{
						chargePercent = 0;
						chargeComplete = false;
						rtcSynced = false;
						Leds_Clear(ext_Red);
						Leds_Clear(ext_Green);
					}


				}












			}
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
	    } else if(strstr(temp,"key") != 0){
	      strtok(temp,"=");
	      strcat(key,strtok(NULL,"="));
	      keyRead = true;
	    } else if(strstr(temp,"port") != 0){
	      strtok(temp,"=");
	      strcat(port,strtok(NULL,"="));
	      portRead = true;
	    } else if(strstr(temp,"auth") != 0){
	      strtok(temp,"=");
	      strcat(auth,strtok(NULL,"="));
	      authRead = true;
	    } else if(strstr(temp,"zone") != 0){
	      strtok(temp,"=");
	      strcpy(zone,strtok(NULL,"="));
	      zoneChanged = true;
	      Debug_SendString("Time Zone changed to: ",false);
	      Debug_SendString(zone,true);
	      if(strcmp(zone,"EST") == 0){
	    	  timeZoneShift = 5;
	      } else if(strcmp(zone,"CST") == 0){
	    	  timeZoneShift = 6;
	      } else if(strcmp(zone,"MST") == 0){
	    	  timeZoneShift = 7;
	      } else if(strcmp(zone,"PST") == 0){
	    	  timeZoneShift = 8;
	      }
	      sprintf(temp,"shifted by %u",timeZoneShift);
	      Debug_SendString(temp,true);

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
	
	if(needToChangeBaud){
		Wifi_SendCommand("set sys iofunc 0x10","AOK","AOK",500);
		_delay_ms(1000);
		Wifi_SendCommand("save","Storing in config","Storing in config",500);
		_delay_ms(1000);
		Wifi_SendCommand("reboot","*Reboot*","*Reboot*",500);
		_delay_ms(1000);
		Wifi_ClearBuffer();


		Wifi_EnterCMDMode(1000);
		_delay_ms(1000);
		Wifi_SendCommand("set uart instant 115200","AOK","AOK",500);
		Wifi_Init(115200);
		_delay_ms(1000);
		needToChangeBaud = false;
	}

	Wifi_EnterCMDMode(1000);
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
	} else if(keyRead){
		if(Wifi_SendCommand(key,"AOK","AOK",500)){
			display_putString("key............OK",col,0,System5x7);
		} else {
			display_putString("key..........FAIL",col,0,System5x7);
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
	_delay_ms(500);
	
	Wifi_ExitCMDMode(500);

	_delay_ms(500);
	
	if(Wifi_Connected(1000)){
		display_putString("network........OK",col,0,System5x7);
		connected = true;
	} else {
		display_putString("network......FAIL",col,0,System5x7);
		connected = false;
	}
	display_writeBufferToScreen();
	col++;
	
	if(connected){
		_delay_ms(1000);
		Wifi_EnterCMDMode(500);
		if(Wifi_GetTime(1000)){
			display_putString("internet.......OK",col,0,System5x7);
			Time_Set(time_secs);
			Leds_Set(wifi_Green);
			Leds_Clear(wifi_Red);
		} else{
			display_putString("internet.....FAIL",col,0,System5x7);
			Leds_Set(wifi_Red);
			Leds_Clear(wifi_Green);
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

//___________________________________________
//				Joshua Schapiro
//					main.c
//			  created: 02/24/2011
//
//	Firmware:   1.00 - 02/24/2011 - Initial testing
//              1.01 - 03/08/2011 - l different points along the ekg circuit
//                                - device id added
//                                - accelerometer is now scaled and offset on server
//                                - red light goes on, and disables logging if rtc fails
//                                - green light blinks when recording
//                                - starts logging automatically
//              1.02 - 03/24/2011 - AREFA changed in hardware from VRef to VCC
//                                - the function to toggle the green led while recording was moved
//                                - Oscillator Changed to 14745600
//                                - if rtc stops working while recording, the time is no longer updated from it. The red light will turn on.
//                                - ekg circuit changed, and now works
//                                - respiration and ekg sampling at 300hz
//              3.00 - 06/30/2011 - First working version with hardware version 3
//
//___________________________________________

#include "avr_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configChestStrap.h"
#include "mainChestStrap.h"
#include "debug.c"
#include "leds.c"
#include "time.c"
#include "diskio.c"
#include "ff.c"
#include "sd.c"
#include "button.c"
#include "accel.c"
#include "sensorsCS.c"
#include "uploader.c"



// ********************************** Functions *********************************

void Clock_Init(void);
void Disable_JTAG(void);
void Interrupt_Init(void);
void getDeviceID(void);
void Charger_Init(void);
bool Charged(void);


void	GUI_Init(void);
void	SD_BackroundWriter_Init(void);
uint8_t SD_StartLogFile(uint32_t counter_32bits);
void	SD_WriteTemperatureBuffer(uint8_t bufferNumber);
void	SD_WriteHumidityBuffer(uint8_t bufferNumber);
void	SD_WriteEKGBuffer(uint8_t bufferNumber);
void	SD_WriteRespirationBuffer(uint8_t bufferNumber);
void	SD_WriteAccelBuffer(uint8_t bufferNumber);
void	SD_WriteRTCBlock(uint32_t ticker, uint32_t time);



// ********************************** Variables *********************************

char				temp [50];         // use in main loop

volatile bool		okToOpenLogFile = false;
volatile bool		okToWriteToLogFile = false;
volatile bool		okToCloseLogFile = false;

volatile bool		restartingFile     = false;

volatile uint32_t	timeToStopRecording = 0;


uint32_t			spaceRemainingOnDisk = 0;
uint32_t			totalDiskSpace = 0;
uint32_t			percentDiskUsed = 0;

volatile bool		okToGetRemainingSpace;


volatile uint16_t	syncCounter = 0;
volatile uint8_t	debounceTimer = 0;
volatile bool		debounceEnabled = false;


volatile bool batteryVoltageOk = true;

// ********************************** Main Program *********************************


int main(void){
	Clock_Init();
	Disable_JTAG();
	getDeviceID();
	Time_Init();
	Sensors_Init();
	Debug_Init(460800);
	Button_Init(Button_Pin);
	Accel_Init();
	Leds_Init(Green);
	Leds_Init(Red);
	GUI_Init();
	SD_BackroundWriter_Init();
    SD_Init();
	Interrupt_Init();
	
	if(Time_CheckVBatSystem() && (Time_Get() > 1000000)){				// grab time from rtc32 if enabled and valid
		timeIsValid = true;
		RTC32.INTCTRL = ( RTC32.INTCTRL & ~RTC32_COMPINTLVL_gm ) | RTC32_COMPINTLVL_LO_gc;
		UNIX_Time = Time_Get();
	} else {
		VBAT.CTRL = VBAT_ACCEN_bm;
		CCPWrite(&VBAT.CTRL, VBAT_RESET_bm);		// Reset battery backup
		RTC32.CTRL = 0;								// disable RTC32
		RTC32.INTCTRL = 0;
	}
	
	while(!SD_Inserted());
	_delay_ms(1000);
	
	SD_Read_config_file();
	
Reset:
	
	_delay_ms(500);
	connected = false;
	Uploader_connectToComputer();
	connected = true;
	
    while(true){
		if(!Uploader_Update()){
			goto Reset;
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


void Interrupt_Init(void)
{
	PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;	// enable low level interrupts
	sei();							// enable all interrupts
}

		   
void Charger_Init(void){
   Charger_Port.DIRCLR = 1 << Charger_Pin;
   Charger_Port.Charger_Pin_CNTL = PORT_OPC_WIREDANDPULL_gc;
}

bool Charged(void){
   if((Button_Port.IN & (1<<Charger_Pin)) >0 ){
	   return true;
   } else {
	   return false;
   }
}

		   
		   
		   
void GUI_Init(void){
	// fclk = 14745600
	// div  = 1024
	// per  = 1440
	// => 10 samples per second
	
	// Set period/TOP value
	GUI_Timer.PER = 1440;
	
	// Select clock source
	GUI_Timer.CTRLA = (GUI_Timer.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;
	
	// Enable CCA interrupt
	GUI_Timer.INTCTRLA = (GUI_Timer.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_LO_gc;
	
	
	
}

ISR(GUI_Timer_vect)                          // 10HZ
{
	
	
	if(recording){
		if(UNIX_Time > timeToStopRecording){
			restartingFile = true;
			recording = false;
			okToCloseLogFile = true;
			while(okToCloseLogFile);
			if(percentDiskUsed < 950){
				okToOpenLogFile = true;
				while(!recording);
			}
			restartingFile = false;
		}
	}
	
	if(timeIsValid){
		syncCounter++;
		if(syncCounter > 6000){
			syncCounter = 0;
			UNIX_Time = Time_Get();
		}
	}
	
	if(debounceEnabled){
		debounceTimer++;
		if(debounceTimer > 10){
			debounceTimer = 0;
			debounceEnabled = false;
		}
	}
		
	if(Button_Pressed(Button_Pin) && !recording && timeIsValid && SD_Inserted() && !debounceEnabled){			// start recording
		if(percentDiskUsed < 950){
			okToGetRemainingSpace = true;
			while(!okToGetRemainingSpace);
			
			okToOpenLogFile = true;
			while(okToOpenLogFile);
			debounceEnabled = true;
		}
	} else if((Button_Pressed(Button_Pin) || !SD_Inserted()) && recording && !debounceEnabled){		// stop recording
		recording = false;
		
		Sensors_ResetHumidityBuffers();
		Sensors_ResetTemperatureBuffers();
		Sensors_ResetRespirationBuffers();
		Sensors_ResetEKGBuffers();
		Sensors_ResetAccelBuffers();
		okToCloseLogFile = true;
		debounceEnabled = true;		
	} 	
	
	if(!SD_Inserted()){
		Leds_State(Red,on);
	} else if(!timeIsValid){
		Leds_State(Red,slow);
	} else if(!batteryVoltageOk){
		Leds_State(Red,fast);
	} else {
		Leds_State(Red,off);
	}
	
	if(recording){
		Leds_State(Green,on);
	} else if(connected && !Charged()){
		Leds_State(Green,slow);
	} else if(Charged()){
		Leds_State(Green,fast);
	} else {
		Leds_State(Green,off);
	}
	
	
	
	for(uint8_t i = 0; i < numberOfLeds; i++){
		ledCounter[i]++;
		if (ledState[i] == on) {
			Leds_Set(i);
		} else if (ledState[i] == slow) {
			if(ledCounter[i] > 10){
				Leds_Toggle(i);
				ledCounter[i] = 0;
			}
		} else if (ledState[i] == fast) {
			if(ledCounter[i] > 2){
				Leds_Toggle(i);
				ledCounter[i] = 0;
			}
		} else {
			Leds_Clear(i);
		}	
	}
}

void SD_BackroundWriter_Init(void){
	
	// fclk = 14745600
	// div  = 64
	// per  = 2304
	// => 14745600/64/2304 => 100 samples per second
	
	// Set period/TOP value
	SD_Writer_Timer.PER = 2304;
	
	// Select clock source
	SD_Writer_Timer.CTRLA = (SD_Writer_Timer.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV64_gc;
	
	// Enable CCA interrupt
	SD_Writer_Timer.INTCTRLA = (SD_Writer_Timer.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;
	
}

ISR(SD_Writer_Timer_vect)
{	
	
sdInterrupt:
	
	if(okToSendAccelBuffer1 && recording){
		SD_WriteAccelBuffer(1);
		okToSendAccelBuffer1 = false;
		goto sdInterrupt;
	} else if (okToSendAccelBuffer2 && recording){
		SD_WriteAccelBuffer(2);
		okToSendAccelBuffer2 = false;
		goto sdInterrupt;
	}
	
	if(okToSendTemperatureBuffer1 && recording){
		SD_WriteTemperatureBuffer(1);
		okToSendTemperatureBuffer1 = false;
		goto sdInterrupt;
	} else if (okToSendTemperatureBuffer2 && recording){
		SD_WriteTemperatureBuffer(2);
		okToSendTemperatureBuffer2 = false;
		goto sdInterrupt;
	}
	
	if(okToSendHumidityBuffer1 && recording){
		SD_WriteHumidityBuffer(1);
		okToSendHumidityBuffer1 = false;
		goto sdInterrupt;
	} else if (okToSendHumidityBuffer2 && recording){
		SD_WriteHumidityBuffer(2);
		okToSendHumidityBuffer2 = false;
		goto sdInterrupt;
	}
	
	if(okToSendRespirationBuffer1 && recording){
		SD_WriteRespirationBuffer(1);
		okToSendRespirationBuffer1 = false;
		goto sdInterrupt;
	} else if (okToSendRespirationBuffer2 && recording){
		SD_WriteRespirationBuffer(2);
		okToSendRespirationBuffer2 = false;
		goto sdInterrupt;
	}
	
	if(okToSendEKGBuffer1 && recording){
		SD_WriteEKGBuffer(1);
		okToSendEKGBuffer1 = false;
		goto sdInterrupt;
	} else if (okToSendEKGBuffer2 && recording){
		SD_WriteEKGBuffer(2);
		okToSendEKGBuffer2 = false;
		goto sdInterrupt;
	}
	
	if(okToSendRTCBlock && recording){
		SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_Time);
		okToSendRTCBlock = false;
		goto sdInterrupt;
	}
	
	if(okToOpenLogFile && (percentDiskUsed < 950)){
		if(SD_StartLogFile(UNIX_Time) == FR_OK){  // open file
			_delay_ms(100);
			
            timeToStopRecording = UNIX_Time + fileLengthInSeconds;
			
		    timeRecordingStarted = UNIX_Time;
		    SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_Time);
			
		    recording = true;
		    okToOpenLogFile = false;
		} else {
		    SD_Init();
		}
	}
	
	if(okToCloseLogFile){
		SD_Close();
		okToCloseLogFile = false;
		goto sdInterrupt;
	}
	
	if(okToOpenDirectory){
        f_opendir(&dir, "/");
        okToOpenDirectory = false;
	}
	
	if(okToGrabNextFileName){
        availableFileName[0] = 0;
        if((f_readdir(&dir, &fno) == FR_OK)){
            if(fno.fname[0] != 0){
                strcpy(availableFileName,fno.fname);
	        }
	        okToGrabNextFileName = false;
        } else if(SD_Inserted()){
            SD_Init();
            f_opendir(&dir, "/");
        }
	}
	
	if(okToOpenFileToUpload){
		f_stat(fileToUpload,&fno);
		uploadFileSize = fno.fsize;
		
		if(f_open(&Upload_File, fileToUpload, FA_READ | FA_WRITE | FA_OPEN_EXISTING) == FR_OK){
			fileExists = true;
		} else {
			fileExists = false;
		}
		f_lseek(&Upload_File, 0);
		uploadFileOpened = true;
		okToOpenFileToUpload = false;
		goto sdInterrupt;
	}
	
    if(okToFillUploadFileBuffer){
        uint16_t tmp;
        f_read(&Upload_File,&uploadFileBuffer,uploadChunkSize,&tmp);
        okToFillUploadFileBuffer = false;
        uploadFileBufferFull = true;
        goto sdInterrupt;
    }
	
	
	if(okToCloseUploadFile){
	    f_sync(&Upload_File);
	    f_close(&Upload_File);
	    strcpy(fileToUpload,"");
		okToCloseUploadFile = false;
		goto sdInterrupt;
	}
	
	if(okToGetRemainingSpace){
	    if(f_getfree("0:",&spaceRemainingOnDisk,&fs) != FR_OK){
			spaceRemainingOnDisk = 0;
	    }
	    totalDiskSpace = fs->max_clust;
		
        percentDiskUsed = totalDiskSpace - spaceRemainingOnDisk;
	    percentDiskUsed *= 1000;
	    percentDiskUsed /= totalDiskSpace;
		
	    okToGetRemainingSpace = false;
	    goto sdInterrupt;
	}
	
	if(okToEraseFile){
        eraseFileReturn = f_unlink(fileToErase);
        strcpy(fileToUpload,"");
        okToEraseFile = false;
	    goto sdInterrupt;
	}
}

uint8_t SD_StartLogFile(uint32_t time){
	uint8_t resp;
	
	SD_MakeFileName(time);
	resp = SD_Open(fileName);
	if(resp != FR_OK){
	    return resp;
	}
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);               // magic number
	SD_Write32(StartFileLength+strlen(deviceID));		    // record size
	SD_Write16(RTYPE_START_OF_FILE); 		// record type
	
	// payload
	SD_Write16(0x0100);				// protocol version
	SD_Write8(0x02);					// time protocol
	SD_Write32(Time_Get32BitTimer());					// time
	SD_Write32(542535);			// picoseconds per tick (48bit)
	SD_Write16(0);
	
	SD_WriteString("device_class");
	SD_Write8(0x09);
	SD_WriteString(DeviceClass);
	SD_Write8(0x0A);
	
	SD_WriteString("device_id");
	SD_Write8(0x09);
	SD_WriteString(deviceID);                                         
	SD_Write8(0x0A);
	
	SD_WriteString("firmware_version");
	SD_Write8(0x09);
	SD_WriteString(FirmwareVersion);
	SD_Write8(0x0A);
	
	SD_WriteString("hardware_version");
	SD_Write8(0x09);
	SD_WriteString(HardwareVersion);
	SD_Write8(0x0A);
	
	SD_WriteString("channel_specs");
	SD_Write8(0x09);
	SD_WriteString("{\"Temperature\":{\"units\": \"bits\", \"scale\": 1},");
	SD_WriteString("\"Respiration\":{\"units\": \"bits\", \"scale\": 1},");
	SD_WriteString("\"EKG\":{\"units\": \"bits\", \"scale\": 1},");
	SD_WriteString("\"Humidity\":{\"units\": \"%RH\", \"scale\": 0.1},");					   // 42
	SD_WriteString("\"Accel_X\":{\"units\": \"bits\", \"offset\": -1024, \"scale\": 0.004},"); // 61
	SD_WriteString("\"Accel_Y\":{\"units\": \"bits\", \"offset\": -1024, \"scale\": 0.004},"); // 61
	SD_WriteString("\"Accel_Z\":{\"units\": \"bits\", \"offset\": -1024, \"scale\": 0.004}}"); // 61
	
	SD_Write8(0x0A);
	
	SD_Write8(0x00);
	
	SD_WriteCRC();			// CRC			
	
	f_sync(&Log_File);
	
	return resp;
}


void SD_WriteRTCBlock(uint32_t ticker, uint32_t time){
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);		// magic number 
	SD_Write32(27);					// record size  
	SD_Write16(2);					// record type  
	
	// payload
	
	SD_Write32(ticker);				// 32-bit counter
	SD_Write32(time);			    // UNIX time  (40bit)
	SD_Write8(0);
	SD_Write32(0);					// unix time nanoseconds
	SD_WriteCRC();					// CRC			
	
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
	SD_Write32(59);				// record size
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
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	if(bufferNumber == 1){
		for(uint8_t i = 0; i < 10; i++){
			SD_Write16(humidityBuffer1[i]);
		}
	} else {
		for(uint8_t i = 0; i < 10; i++){
			SD_Write16(humidityBuffer2[i]);
		}
	}
	
	SD_WriteCRC();			// CRC
	
	f_sync(&Log_File);
	
}

void SD_WriteRespirationBuffer(uint8_t bufferNumber){
	
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(442);				// record size
	SD_Write16(3); 		// record type
	
	// payload
	if(bufferNumber == 1){
		SD_Write32(respirationSampleStartTime1);					// time
	} else {
		SD_Write32(respirationSampleStartTime2);					// time
	}
	SD_Write32(6144);										// sample period (300hz)
	SD_Write32(200);												// number of samples
	
	SD_WriteString("Respiration");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	if(bufferNumber == 1){
		for(uint16_t i = 0; i < 200; i++){
			SD_Write16(respirationBuffer1[i]);
		}
	} else {
		for(uint16_t i = 0; i < 200; i++){
			SD_Write16(respirationBuffer2[i]);
		}
	}
	
	SD_WriteCRC();			// CRC
	
	f_sync(&Log_File);
	
}



void SD_WriteEKGBuffer(uint8_t bufferNumber){
	
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(434);				// record size
	SD_Write16(3); 		// record type
	
	// payload
	if(bufferNumber == 1){
		SD_Write32(EKGSampleStartTime1);					// time
	} else {
		SD_Write32(EKGSampleStartTime2);					// time
	}
	SD_Write32(6144);										// sample period (300hz)
	SD_Write32(200);												// number of samples
	
	SD_WriteString("EKG");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	if(bufferNumber == 1){
		for(uint16_t i = 0; i < 200; i++){
			SD_Write16(EKGBuffer1[i]);
		}
	} else {
		for(uint16_t i = 0; i < 200; i++){
			SD_Write16(EKGBuffer2[i]);
		}
	}
	
	SD_WriteCRC();			// CRC
	
	f_sync(&Log_File);
	
}

void SD_WriteAccelBuffer(uint8_t bufferNumber){
	
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number
	SD_Write32(1260);				// record size
	SD_Write16(3); 		// record type
	
	// payload
	if(bufferNumber == 1){
		SD_Write32(accelSampleStartTime1);					// time
	} else {
		SD_Write32(accelSampleStartTime2);					// time
	}
	SD_Write32(5760);										// sample period (320hz)
	SD_Write32(accelNumberOfSamples);												// number of samples
	
	SD_WriteString("Accel_X");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_WriteString("Accel_Y");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_WriteString("Accel_Z");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	
	SD_Write8(0x00);
	
	if(bufferNumber == 1){
		for(uint16_t i = 0; i < accelNumberOfSamples*accelNumberOfChannels; i++){
			SD_Write16(accelBuffer1[i]);
		}
	} else {
		for(uint16_t i = 0; i < accelNumberOfSamples*accelNumberOfChannels; i++){
			SD_Write16(accelBuffer2[i]);
		}
	}
	
	SD_WriteCRC();			// CRC
	
	f_sync(&Log_File);
	
}

void getDeviceID(void){
	
	char tmp [5];
	
	strcpy(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM0_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM1_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM2_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM3_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM4_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+LOTNUM5_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+WAFNUM_offset) ,tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+COORDX0_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+COORDX1_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+COORDY0_offset),tmp,16));
	strcat(deviceID,itoa(SP_ReadCalibrationByte( PROD_SIGNATURES_START+COORDY1_offset),tmp,16));
}

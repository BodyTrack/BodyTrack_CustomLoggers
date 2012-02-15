//___________________________________________
//				Joshua Schapiro
//					mainBaseStation.c
//			  created: 08/27/2010
//
//	Firmware:   1.00 - 08/27/2010 - 08/27/2010 - Initial testing
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
//				2.03 - 02/22/2011 - automatically uploads to server when connected to internet
//                                - time zone bug fixed
//                                - daylight savings time supported through config file
//                                - space remaining on disk is shown on lcd
//                                - handshaking is supported for wifi usart
//                                - wifi baud rate automatically changes to 460.8k once connected to internet
//                                - UNIX time is incremented using 1hz sensor reader interrupt
//                                - all uSD actions take place in one interrupt
//                                - time syncing and getting wifi signal strength moved to main loop
//                                - max file length is ~15mins
//                                - CRC32 now used lookup table to speed it up
//                                - fixed "set comm remote error" bug
//                                - NTP server changed
//				2.04 - 03/18/2011 - supports different users
//                                - now logs failed uploads
//                                - cleans up user and nickname before using them
//                                - display is now software and hardware reset
//                                - improvements to RTC sync
//                                - day light savings bug fixed
//				2.05 - 03/18/2011 - uses processing to make a post
//                                - deviceID uses chip coordinates instead of mac address
//				2.06 - 04/19/2011 - supports posting through processing & wifi  (use config file to determine which)
//                                - demo mode is triggered using config file
//                                - wifly firmware needs to be version 2.23
//                                - time format from the wifly module changed with new firmware
//                                - baud rate change procedure changed with new wifly firmware
//                                - fixed bug in Wifi_SendCommand
//                                - handles wifi reset during upload
//                                - fixed bug for starting log file while uploading
//                                - delay places in Wifi_SendByte() to avoid missing bytes
//                                - crc errors fixed - need to speed up sd writes for microphone
//                                - fixed bug where logged data would go into the debug file
//                                - "set comm time" parameter changed from 1000ms to 10ms
//                                - fixed dylos bug
//                                - fixed the display of disk space used bug
//                                - changed how the 15 minutes file length cutoff is triggered
//                                - dynamically changes NTP server if it cannot connect
//                                - handles the card being full better
//				2.07 - 04/28/2011 - fixes display bug to show the reboot sequence after reset
//                                - fixes Wifi_GetTime() function that cause time to get out of sync
//				2.08 - 04/28/2011 - redesigned protocol for communicating with a computer for uploading
//                                - can now choose whether to record audio from config file
//				2.09 - 05/17/2011 - speed ups to logging process to avoid losing audio packets
//                                - speed ups to wifi setup
//                                - increased resolution on humidity and pressure sensors
//                                - dynamic dylos detection supports standard dylos, as well as custom (1hz refresh rate, 6 bins)
//				2.10 - 06/20/2011 - if incorrect number of bytes is read on when getting a command through debug, board resets
//                                - checks for errors while starting a log file. will re-init sd card and retry until successful
//                                - baud rate for upload through computer is raised to 460800
//				2.11 - 06/20/2011 - request for file name returns all of them
//                                - calibration bytes are used for ADC
//                                - improvements to sensor sampling equations
//                                - reduction in ram usage
//                                - crc32 is used on file upload
//                                - changes to the GUI
//                                - removed buffer from display driver
//				3.00 - 07/08/2011 - first working version with hardware version 3
//                                - included files are abstraced to the cheststrap can share them
//              3.01 - 07/28/2011 - changed timer PER registers
//								  - debug timeouts is RTS line is held high too long
//								  - device ID method updated
//								  - changing sensor buffer length in config file is supported
//								  - file length timer changed
//								  - holding the button down keeps the backlight lit until released + timeout
//								  - button presses are moved to interrupts
//              3.02 - 08/15/2011 - variable number of buffers per sensor supported
//								  - changed the way the sd card is detected
//								  - reworked the function to get the space remaining on disk
//              3.03 - 09/02/2011 - microphone input fixed
//								  - air quality on sensor screen shows while recording and not
//              3.04 - 09/06/2011 - timeout implemented in light sensor
//              3.05 - 09/14/2011 - Gap issue addressed. Buffers cannot overwrite before they are written. Buffers must be sent in the correct order.
//              3.06 - 09/28/2011 - logging 1hz sensors disabled in v3.05, now enabled.
//              3.07 - 10/03/2011 - changed mic buffer writing to speed it up
//                                - delay in record switch to avoid bouncing
//                                - update to light sensor display code. shows 32-bit value.
//              3.08 - 10/12/2011 - fix to speed mic buffer writing.
//              3.09 - 10/20/2011 - fix for truncated light sensor values
//                                - rejects invalid times
//              3.10 - 10/31/2011 - fix corrupted config data when dylos is connected
//                                - fix mislabeled dylos data
//                                - changed 6-channel dylos display on lcd
//              3.11 - 02/14/2012 - fixed display bug where "uploading" would appear on sensor parge
//                                - fixed 'garbage in config file' bug
//              3.12 - 02/15/2012 - changed dylos parsing, 6 channel appears as 2 channel less often
//
//
//___________________________________________



#define DeviceClass			"BaseStation"
#define FirmwareVersion		"3.12"
#define HardwareVersion		"3"



#include "avr_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configBaseStation.h"
#include "mainBaseStation.h"

#include "debug.c"
#include "rs232.c"

#include "time.c"
#include "diskio.c"
#include "ff.c"
#include "sd.c"

#include "display.c"
#include "light.c"
#include "button.c"
#include "sensorsBS.c"
#include "uploader.c"



// ********************************** Functions *********************************

void Clock_Init(void);
void Disable_JTAG(void);
void Interrupt_Init(void);
void getDeviceID(void);

void	Display_BackgroundWriter_Init(void);
void	SD_BackroundWriter_Init(void);
uint8_t SD_StartLogFile(uint32_t counter_32bits);
void	SD_WriteTemperatureBuffer(uint8_t bufferNumber);
void	SD_WriteHumidityBuffer(uint8_t bufferNumber);
void	SD_WritePressureBuffer(uint8_t bufferNumber);
void	SD_WriteMicrophoneBuffer(uint8_t bufferNumber);
void	SD_WriteLightBuffer(uint8_t bufferNumber);
void	SD_WriteAirSampleSecond(void);
void	SD_WriteAirSampleMinute(void);
void	SD_WriteRTCBlock(uint32_t ticker, uint32_t time);



// ********************************** Variables *********************************

char				temp [50];         // use in main loop
char				tempDisplay [50];        // use in display interrupt

volatile uint8_t	currentMode      = 0;

volatile bool		okToReadConfigFile		= false;
volatile bool		okToOpenLogFile			= false;
volatile bool		okToWriteToLogFile		= false;
volatile bool		okToCloseLogFile		= false;
volatile bool		okToDisplayGUI			= false;
volatile bool		okToGetRemainingSpace	= false;
volatile bool		justSwitchedStated		= false;
volatile bool		restartingFile			= false;

uint8_t				clockHour = 0;
volatile bool		displayAM = false;
volatile bool		displayPM = false;

volatile uint16_t	lengthOfCurrentFile = 0;
volatile uint8_t	spaceRemainingCounter = 0;
volatile uint8_t	backlight_Timer = 0;
volatile uint16_t	syncCounter = 0;

volatile uint8_t	numberOfBins = 1;



// ********************************** Main Program *********************************


int main(void){
	Clock_Init();
	Disable_JTAG();
	getDeviceID();
	display_init();
	Time_Init();
	Sensors_Init();
	Debug_Init(460800);
	Button_Init(Button_Pin,true,falling,0,high);
	Button_Init(Switch_Pin,true,both,1,high);
	Rs232_Init();
	Light_Init(LightAddress);
	Display_BackgroundWriter_Init();
	SD_BackroundWriter_Init();
    SD_Init();
	Interrupt_Init();
	
	if(Time_CheckVBatSystem() && (Time_Get() > 1260000000)){				// grab time from rtc32 if enabled and valid
		timeIsValid = true;
		RTC32.INTCTRL = ( RTC32.INTCTRL & ~RTC32_COMPINTLVL_gm ) | RTC32_COMPINTLVL_LO_gc;
		UNIX_Time = Time_Get();
	} else {
		
		VBAT.CTRL = VBAT_ACCEN_bm;
		CCPWrite(&VBAT.CTRL, VBAT_RESET_bm);		// Reset battery backup
		RTC32.CTRL = 0;								// disable RTC32
		RTC32.INTCTRL = 0;
	
	}
    
	display_showSplashScreen(false,false,false);
	_delay_ms(1000);
	
	while(!SD_Inserted()){
		SD_Init();		
		display_showSplashScreen(true,false,false);			// waiting for SD card
		_delay_ms(250);
	}
	
	_delay_ms(500);
	okToReadConfigFile = true;
	while(okToReadConfigFile);
	
    if(demoMode){
		display_showSplashScreen(false,false,true);
		_delay_ms(500);
		display_clearScreen();
		while(true);
	}
	

	_delay_ms(500);
	okToDisplayGUI = true;
	_delay_ms(100);	
	
	okToGetRemainingSpace = true;
	while(okToGetRemainingSpace);
	if(Button_Pressed(Switch_Pin) && timeIsValid){		// start recording
		if(percentDiskUsed < 950){
			okToOpenLogFile = true;
			_delay_ms(250);
		}
	}
 Reset: 
	uploadPercentBS = 0;
	connected = false;
	while(!Uploader_connectToComputer());
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


ISR(Button_IntVector){
	if(okToDisplayGUI){
		if(backLightIsOn){
			backlight_Timer = 0;
		} else {
			justSwitchedStated = true;
			if(currentMode == recordMode){													// switch to sensor screen
				currentMode = sensorMode;
				display_setBacklight(true);
				backlight_Timer = 0;
				display_clearScreen();
				_delay_ms(250);
			} else if(currentMode == sensorMode){											// switch to system screen
				currentMode = recordMode;
				display_setBacklight(true);
				backlight_Timer = 0;
				display_clearScreen();
				_delay_ms(250);	
			}
		}
	} else {
		backlight_Timer = 0;
		display_setBacklight(true);
		_delay_ms(250);
	}
}

ISR(Switch_IntVector){
	_delay_ms(50);
	if(Button_Pressed(Switch_Pin) && !okToOpenLogFile && !recording && timeIsValid && SD_Inserted()){		// start recording
		if(percentDiskUsed < 950){
			okToOpenLogFile = true;
		}
	} else if(!Button_Pressed(Switch_Pin) && recording && !okToCloseLogFile){			// close file
		recording = false;
		Sensors_ResetTemperatureBuffers();
		Sensors_ResetHumidityBuffers();
		Sensors_ResetPressureBuffers();
		Sensors_ResetMicrophoneBuffers();
		Sensors_ResetLightBuffers();
		okToCloseLogFile = true;
	}
}

void Display_BackgroundWriter_Init(void){
	// fclk = 14745600
	// div  = 1440
	// => 14745600/1440/1024 => 10 samples per second

	// Set period/TOP value
	Display_Writer_Timer.PER = 1440;

	// Select clock source
	Display_Writer_Timer.CTRLA = (Display_Writer_Timer.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;

	// Enable CCA interrupt
	Display_Writer_Timer.INTCTRLA = (Display_Writer_Timer.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_LO_gc;
}

ISR(Display_Writer_Timer_vect){
	if(backlight_Timer > 20){
		display_setBacklight(false);
	}
	backlight_Timer++;

	if(backLightIsOn && okToDisplayGUI && Button_Pressed(Button_Pin)){
		backlight_Timer = 0;
	}
	
	if(recording){
		lengthOfCurrentFile++;
		if(lengthOfCurrentFile > maxFileLength){
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
	
	if(timeIsValid && !okToSendRTCBlock){
		syncCounter++;
	   if(syncCounter > 6000){
		   syncCounter = 0;
		   UNIX_Time = Time_Get();
	   }
	}
	
	spaceRemainingCounter++;														// every 25 seconds, refresh space remaining on disk
	if(spaceRemainingCounter == 0){
		okToGetRemainingSpace = true;
		while(okToGetRemainingSpace);
	}
	
	if(okToDisplayGUI){
		
		if(Button_Pressed(Switch_Pin) && !okToOpenLogFile && !recording && timeIsValid && SD_Inserted()){		// start recording
			if(percentDiskUsed < 950){
				okToOpenLogFile = true;
			}
		}
		
		
		if(currentMode == recordMode){
			

			
			if(recording && !sdValid){														// write failed, so stop recording
				recording = false;
			}
			
			if(recording){
				sprintf(tempDisplay, "Recording   %4lum", (UNIX_Time - timeRecordingStarted)/60);		// load recording screen
				display_putString(tempDisplay,0,0,System5x7);
			} else {
				display_putString("Paused           ",0,0,System5x7);
			}


			sprintf(tempDisplay, "Uploading    %3u", uploadPercentBS);
			strcat(tempDisplay,"%");
			display_putString(tempDisplay,1,0,System5x7);
			if(SD_Inserted()){
				sprintf(tempDisplay,"Disk Used: %3lu.%lu",percentDiskUsed/10,percentDiskUsed%10);
				strcat(tempDisplay,"%");
				display_putString(tempDisplay,2,0,System5x7);
			} else {
				display_putString("Disk Used:  ??.?%",2,0,System5x7);
			}



			display_putString("                 ",3,0,System5x7);
			display_putString("                 ",4,0,System5x7);
			display_putString("                 ",5,0,System5x7);



			Time_UTCSecsToTime(UNIX_Time,&time);
			clockHour = time.Hour + 24;
			clockHour -= timeZoneShift;
			if(clockHour > 24){
				clockHour -= 24;
			}

			if(clockHour == 0){
				displayAM = true;
				displayPM = false;
				clockHour += 12;
			} else if(clockHour == 12){
				displayAM = false;
				displayPM = true;
			} else if(clockHour > 12){
				displayAM = false;
				displayPM = true;
				clockHour -= 12;
			}  else {
				displayAM = true;
				displayPM = false;
			}
			if(timeIsValid){
				
				
				sprintf(tempDisplay,"Time  %2u:%02u:%02u ", clockHour, time.Minute, time.Second);
				if(displayAM){
					strcat(tempDisplay,am);
				} else if(displayPM){
					strcat(tempDisplay,pm);
				}
			} else {
				strcpy(tempDisplay,"Time  ??:??:??   ");
			}
			display_putString(tempDisplay,6,0,System5x7);
			if(justSwitchedStated){
				justSwitchedStated = false;
				display_clearPage(7);
			}
			if(connected){
				display_putString("Host Connected   ",7,0,System5x7);
			} else {
				display_putString("Host Missing     ",7,0,System5x7);
			}


		} else if(currentMode == sensorMode){
			display_putString("     Sensors     ",0,0,System5x7);
			display_putString("                 ",1,0,System5x7);
			sprintf(tempDisplay,"Temperature: %3uC", quickTemperature);
			display_putString(tempDisplay,2,0,System5x7);
			sprintf(tempDisplay,"Humidity:  %3u", quickHumidity);
			strcat(tempDisplay, "%RH");
			display_putString(tempDisplay,3,0,System5x7);
			sprintf(tempDisplay,"Pressure:  %3ukPa", quickPressure);
			display_putString(tempDisplay,4,0,System5x7);
			sprintf(tempDisplay,"Light: %10lu", quickLight);
			display_putString(tempDisplay,5,0,System5x7);

			sprintf(tempDisplay,"Air: %5lu, %5lu", quickSmall, quickLarge);
			display_putString(tempDisplay,6,0,System5x7);
			display_putString("Sound:           ",7,0,System5x7);

			uint8_t i = 50;
			display_setCursor(7,i);
			while(i < 102){
				if(i < (quickMic/4 + 50)){
					display_sendData(0xFF);
				} else {
					display_sendData(0x00);
				}
				i++;
			}

			
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
	if(recording){
		
		for(uint8_t i = 0; i < microphoneNumberOfBuffers; i++){
			if(okToSendMicrophoneBuffer[i] && (lastMicrophoneBufferSent != i)){
				SD_WriteMicrophoneBuffer(i);
				lastMicrophoneBufferSent = i;
				okToSendMicrophoneBuffer[i] = false;
			}
		}
	}
	
	if(okToSendAirQuality && !restartingFile){
		
		numberOfBins = 1;
		for (uint8_t i = 0; i < 255; i++) {
			if(!Rs232_CharReadyToRead()){
				airQualityString[i] = 0;
				break;
			}
			airQualityString[i] = Rs232_GetByte(false);
			
			if(airQualityString[i] == 0x0A){
				airQualityString[i+1] = 0;
				break;	
			}
			
			if(airQualityString[i] == ','){
				numberOfBins++;
			}
		}
		
		
		
		if(strstr(airQualityString,"Dylos") == NULL){
			airSampleTime = Time_Get32BitTimer();
			airCount[0] = atol(strtok(airQualityString,","));
			if(numberOfBins == 2){
				airCount[1] = atol(strtok(NULL,","));
				quickSmall = airCount[0];
				quickLarge = airCount[1];
				if(recording){
					SD_WriteAirSampleMinute();
				}
				Debug_SendString(airQualityString, true);
			} else if(numberOfBins == 6){
				airCount[1] = atol(strtok(NULL,","));
				airCount[2] = atol(strtok(NULL,","));
				airCount[3] = atol(strtok(NULL,","));
				airCount[4] = atol(strtok(NULL,","));
				airCount[5] = atol(strtok(NULL,","));
				
				quickSmall = airCount[0];
				quickLarge = airCount[3];
				if(recording){
					SD_WriteAirSampleSecond();
				}
			} 
		} 
		
		okToSendAirQuality = false;
	}
	
	if(recording){
		for(uint8_t i = 0; i < temperatureNumberOfBuffers; i++){
			if(okToSendTemperatureBuffer[i] && (lastTemperatureBufferSent != i)){
				SD_WriteTemperatureBuffer(i);
				lastTemperatureBufferSent = i;
				okToSendTemperatureBuffer[i] = false;
			}
		}
		
		for(uint8_t i = 0; i < humidityNumberOfBuffers; i++){
			if(okToSendHumidityBuffer[i] && (lastHumidityBufferSent != i)){
				SD_WriteHumidityBuffer(i);
				lastHumidityBufferSent = i;
				okToSendHumidityBuffer[i] = false;
			}
		}
		
		for(uint8_t i = 0; i < pressureNumberOfBuffers; i++){
			if(okToSendPressureBuffer[i] && (lastPressureBufferSent != i)){
				SD_WritePressureBuffer(i);
				lastPressureBufferSent = i;
				okToSendPressureBuffer[i] = false;
			}
		}
		
		for(uint8_t i = 0; i < lightNumberOfBuffers; i++){
			if(okToSendLightBuffer[i] && (lastLightBufferSent != i)){
				SD_WriteLightBuffer(i);
				lastLightBufferSent = i;
				okToSendLightBuffer[i] = false;
			}
		}
		
		
		if(okToSendRTCBlock){
			UNIX_Time = Time_Get();
			SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_Time);
			okToSendRTCBlock = false;
		}
	}

	if(okToOpenLogFile){
		if(SD_StartLogFile(UNIX_Time) == FR_OK){  // open file
			_delay_ms(100);
			lengthOfCurrentFile = 0;	

		    timeRecordingStarted = UNIX_Time;
		    SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_Time);

		    recording = true;
		} else {
		    sdValid = false;
		}
		okToOpenLogFile = false;
	}
	
	if(okToCloseLogFile){
		SD_Close();
		okToCloseLogFile = false;
	}

	if(okToOpenDirectory){
        if(f_opendir(&dir, "/") != FR_OK){
			sdValid = false;
		}
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
	}

    if(okToFillUploadFileBuffer){
        uint16_t tmp;
        if(f_read(&Upload_File,&uploadFileBuffer,uploadChunkSize,&tmp) != FR_OK){
			sdValid = false;
		}
        okToFillUploadFileBuffer = false;
        uploadFileBufferFull = true;
    }
	

	if(okToCloseUploadFile){
	    if(f_sync(&Upload_File) != FR_OK){
			sdValid = false;
		}
	    f_close(&Upload_File);
	    strcpy(fileToUpload,"");
		okToCloseUploadFile = false;
	}

	if(okToGetRemainingSpace){
		SD_GetSpaceRemaining();
		okToGetRemainingSpace = false;
	}

	if(okToEraseFile){
        eraseFileReturn = f_unlink(fileToErase);
		if(eraseFileReturn != FR_OK){
			sdValid = false;
		}
        strcpy(fileToUpload,"");
        okToEraseFile = false;
	}
	
	if(okToReadConfigFile){
		SD_Read_config_file();
		okToReadConfigFile = false;
	}
}

uint8_t SD_StartLogFile(uint32_t time){
	uint8_t resp;
	uint16_t length;
	
	length = StartFileLength;
	length += strlen(DeviceClass);
	length += strlen(deviceID);
	length += strlen(FirmwareVersion);
	length += strlen(HardwareVersion);
	
	SD_MakeFileName(time);
	resp = SD_Open(fileName);
	if(resp != FR_OK){
		sdValid = false;
	    return resp;
	}
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);               // magic number
	SD_Write32(length);						// record size
	SD_Write16(RTYPE_START_OF_FILE); 		// record type
	
						// **** payload ****
	SD_Write16(0x0100);						// protocol version
	SD_Write8(0x02);						// time protocol
	SD_Write32(Time_Get32BitTimer());		// time
	SD_Write32(542535);						// picoseconds per tick (48bit) (truly is 542534.722)
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
	SD_WriteString("{\"Temperature\":{\"units\": \"deg C\", \"scale\": 0.1},");			
	SD_WriteString("\"Humidity\":{\"units\": \"%RH\", \"scale\": 0.1},");				
	SD_WriteString("\"Pressure\":{\"units\": \"kPa\", \"scale\": 0.1},");				
	SD_WriteString("\"Light_Green\":{\"units\": \"bits\", \"scale\": 1},");				
	SD_WriteString("\"Light_Red\":{\"units\": \"bits\", \"scale\": 1},");				
	SD_WriteString("\"Light_Blue\":{\"units\": \"bits\", \"scale\": 1},");				 
	SD_WriteString("\"Light_Clear\":{\"units\": \"bits\", \"scale\": 1},");				
	SD_WriteString("\"Air_1\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_2\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_3\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_4\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_5\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_6\":{\"units\": \"#particles\", \"scale\": 1},");				
	SD_WriteString("\"Air_Small\":{\"units\": \"#particles\", \"scale\": 1},");			
	SD_WriteString("\"Air_Large\":{\"units\": \"#particles\", \"scale\": 1},");			
	SD_WriteString("\"Microphone\":{\"units\": \"bits\", \"scale\": 1}}");				
	SD_Write8(0x0A);																	
	SD_Write8(0x00);																	
	
	SD_WriteCRC();																		
	
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
	return resp;
}


void SD_WriteRTCBlock(uint32_t ticker, uint32_t time){
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);					// magic number 
	SD_Write32(27);								// record size  
	SD_Write16(2);								// record type  
						// ***** payload *****
	SD_Write32(ticker);							// 32-bit counter
	SD_Write32(time);							// UNIX time  (40bit)
	SD_Write8(0);
	SD_Write32(0);								// unix time nanoseconds
	SD_WriteCRC();								// CRC			
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}

}


void SD_WriteTemperatureBuffer(uint8_t bufferNumber){
	uint16_t length;
	length = temperatureNumberOfSamples*2;
	length += 42;
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);									// magic number 
	SD_Write32(length);											// record size  
	SD_Write16(3);												// record type
	
	// ***** payload *****
	
	SD_Write32(temperatureSampleStartTime[bufferNumber]);		// time	
	SD_Write32(temperatureTicksPerSample);						// sample period 
	SD_Write32(temperatureNumberOfSamples);						// number of samples
	
	SD_WriteString("Temperature");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);	
	
	for(uint8_t i = 0; i < temperatureNumberOfSamples; i++){
		SD_Write16(temperatureBuffer[bufferNumber][i]);
	}
	
	SD_WriteCRC();					
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}

void SD_WriteHumidityBuffer(uint8_t bufferNumber){
	uint16_t length;
	length = humidityNumberOfSamples*2;
	length += 39;
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);									// magic number
	SD_Write32(length);											// record size
	SD_Write16(3);												// record type
	
	// ***** payload *****
	
	SD_Write32(humiditySampleStartTime[bufferNumber]);			// time
	SD_Write32(humidityTicksPerSample);							// sample period 
	SD_Write32(humidityNumberOfSamples);						// number of samples
	
	SD_WriteString("Humidity");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	
	for(uint8_t i = 0; i < humidityNumberOfSamples; i++){
		SD_Write16(humidityBuffer[bufferNumber][i]);
	}
	
	SD_WriteCRC();			// CRC
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}


void SD_WritePressureBuffer(uint8_t bufferNumber){
	uint16_t length;
	length = pressureNumberOfSamples*2;
	length += 39;
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);									// magic number
	SD_Write32(length);											// record size
	SD_Write16(3);												// record type

								// ***** payload *****

	SD_Write32(pressureSampleStartTime[bufferNumber]);			// time
	SD_Write32(pressureTicksPerSample);							// sample period (1hz)
	SD_Write32(pressureNumberOfSamples);						// number of samples

	SD_WriteString("Pressure");
	SD_Write8(0x09);
	SD_WriteString("16");
	SD_Write8(0x0A);
	SD_Write8(0x00);

	for(uint8_t i = 0; i < pressureNumberOfSamples; i++){
		SD_Write16(pressureBuffer[bufferNumber][i]);
	}
	
	SD_WriteCRC();			
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}

void SD_WriteMicrophoneBuffer(uint8_t bufferNumber){
	uint16_t length;
	length = 40+microphoneNumberOfSamples;

	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);									// magic number
	SD_Write32(length);											// record size
	SD_Write16(3);												// record type

								// ***** payload *****

	SD_Write32(microphoneSampleStartTime[bufferNumber]);		// time
	SD_Write32(microphoneTicksPerSample);						// sample period (7.2khz)
	SD_Write32(microphoneNumberOfSamples);						// number of samples

	SD_WriteString("Microphone");
	SD_Write8(0x09);
	SD_WriteString("8");
	SD_Write8(0x0A);
	SD_Write8(0x00);

	SD_WriteBuffer(&microphoneBuffer[bufferNumber][0], microphoneNumberOfSamples);
	
	SD_WriteCRC();									
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}

void SD_WriteLightBuffer(uint8_t bufferNumber){
	uint16_t length;
	length = lightNumberOfSamples*lightNumberOfChannels;
	length *= 4;
	length += 84; 

	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);									// magic number
	SD_Write32(length);											// record size
	SD_Write16(3);												// record type

								// ***** payload *****

	SD_Write32(lightSampleStartTime[bufferNumber]);				// time
	SD_Write32(lightTicksPerSample);							// sample period (1hz)
	SD_Write32(lightNumberOfSamples);							// number of samples

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

	for(uint8_t i = 0; i < (lightNumberOfSamples*lightNumberOfChannels); i++){
		SD_Write32(lightBuffer[bufferNumber][i]);
	}
	
	SD_WriteCRC();			
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}

void SD_WriteAirSampleSecond(void){
	
	uint16_t length;
	length = airNumberOfFastSamples * airNumberOfFastChannels;
	length *= 4;
	length += 81;
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);							// magic number
	SD_Write32(length);									// record size
	SD_Write16(3);										// record type
								// ***** payload *****
	SD_Write32(airSampleTime);							// time
	SD_Write32(airTicksPerSampleFastHZ);				// sample period (1hz)
	SD_Write32(airNumberOfFastSamples);					// number of samples
	
	SD_WriteString("Air_1");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_2");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_3");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_4");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_5");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_6");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	SD_Write32(airCount[0]);
	SD_Write32(airCount[1]);
	SD_Write32(airCount[2]);
	SD_Write32(airCount[3]);
	SD_Write32(airCount[4]);
	SD_Write32(airCount[5]);
	SD_WriteCRC();			
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
}

void SD_WriteAirSampleMinute(void){
	
	uint16_t length;
	length = airNumberOfSlowSamples * airNumberOfSlowChannels;
	length *= 4;
	length += 53;
	
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);							// magic number
	SD_Write32(length);									// record size
	SD_Write16(3);										// record type
								// ***** payload *****	
	SD_Write32(airSampleTime);							// time
	SD_Write32(airTicksPerSampleSlowHZ);				// sample period (0.01667hz)
	SD_Write32(airNumberOfSlowSamples);					// number of samples
	
	SD_WriteString("Air_Small");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_WriteString("Air_Large");
	SD_Write8(0x09);
	SD_WriteString("32");
	SD_Write8(0x0A);
	SD_Write8(0x00);
	
	SD_Write32(airCount[0]);
	SD_Write32(airCount[1]);
	SD_WriteCRC();			
	if(f_sync(&Log_File) != FR_OK){
		sdValid = false;
	}
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




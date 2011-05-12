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

void Display_BackgroundWriter_Init(void);
void SD_BackroundWriter_Init(void);
void SD_WriteTemperatureBuffer(uint8_t bufferNumber);
void SD_WriteHumidityBuffer(uint8_t bufferNumber);
void SD_WritePressureBuffer(uint8_t bufferNumber);
void SD_WriteMicrophoneBuffer(uint8_t bufferNumber);
void SD_WriteLightBuffer(uint8_t bufferNumber);
void SD_WriteAirSample(void);

void SD_WriteRTCBlock(uint32_t ticker, uint32_t time);


void getDeviceID(void);
uint8_t SP_ReadCalibrationByte( uint8_t index );

void connectToComputer(void);
bool getTime(void);
void sendSSID(void);
void sendAuthType(void);
void sendKey(void);
void sendUser(void);
void sendNickname(void);
void uploadFile(void);
void eraseFile(void);
void sendServer(void);
void sendPort(void);


// ********************************** Variables *********************************

#define uploadChunkSize 1000
char temp [50];         // use in main loop
char temp2 [50];        // use in display interrupt
char temp3 [50];        // use in sd interrupt
uint8_t tempStringFiller = 0;

char auth [50];
char phrase [50];
char key [50];
char ssid [50];
char nickname [50];
char serverOpenCommand [50] = "open ";
char port [50];
char server [50];
char zone [10] = "GMT";
char user [10];
char am [5] = "AM";
char pm [5] = "PM";
char daylightTime[10];
char uploaderToUse[10];
char demoModeString[10];
char microphoneString[10];



bool authRead = false;
bool phraseRead = false;
bool keyRead = false;
bool ssidRead = false;
bool zoneChanged = false;

uint8_t timeZoneShift = 0;
uint8_t clockHour = 0;

volatile uint8_t currentMode      = 0;
volatile uint16_t ssRefreshCounter = 0;
uint8_t signalStrength   = 0;
uint8_t uploadPercentBS  = 0;
uint8_t uploadPercentEXT = 0;

bool rtcSynced			 = false;




volatile bool okToOpenLogFile = false;
volatile bool okToWriteToLogFile = false;
volatile bool okToCloseLogFile = false;
volatile bool okToDisplayGUI = false;
volatile bool okToFindFileToUpload = false;
volatile bool okToReopenDirectory = false;
volatile bool directoryOpened = false;
volatile bool uploadFailed = false;
volatile bool okToCloseUploadFile = false;
volatile bool okToRenameUploadFile = false;
volatile bool okToEraseFile = false;

char fileToUpload[15];
char newFileName[15];
volatile bool okToUpload = false;
volatile bool uploading = false;
volatile bool doneUploading = false;
volatile bool okToOpenFileToUpload = false;
volatile bool uploadFileOpened = true;
volatile bool okToFillUploadFileBuffer = false;
volatile bool uploadFileBufferFull = false;
volatile bool restartingFile     = false;
volatile bool timeIsValid = false;
volatile bool displayAM = false;
volatile bool displayPM = false;
volatile uint32_t uploadFileSize = 0;
volatile uint32_t uploadHeaderSize = 0;

char uploadFileBuffer [ uploadChunkSize];
volatile uint32_t numberOfPacketsToUpload = 0;
volatile uint32_t  leftOverBytesToUpload = 0;

volatile uint8_t recordFileRestartCounter = 0;
volatile uint32_t timeToStopRecording = 0;

char httpResponse [10];
uint16_t lengthOfHttpResponse = 0;
volatile bool httpResponseReceived = false;
volatile bool connectionClosed = false;
uint8_t byteReceived;
uint32_t connectionTimeoutTimer = 0;
char failedBinaryRecordsString [10];
char successfulBinaryRecordsString [10];
volatile bool okToWriteUploaderLogFile = false;



uint32_t spaceRemainingOnDisk = 0;
uint32_t totalDiskSpace = 0;
uint32_t percentDiskUsed = 0;

volatile uint32_t uploadTimeStart;
volatile uint32_t uploadTimeStop;

volatile bool okToGetRemainingSpace;

bool demoModeFirstFile = true;

uint8_t timeServerAttempts = 0;

//volatile uint16_t timeOutCounter = 0;
volatile uint8_t command [50];
uint8_t commandCounter = '0';
uint32_t tempTime = 0;
char fileToErase [20];



// ********************************** Main Program *********************************


int main(void){
	_delay_ms(5);


	Clock_Init();
	Disable_JTAG();

	getDeviceID();
	display_init();

	Time_Init();
	Sensors_Init();
	Leds_Init();
	Dpad_Init();


	Rs232_Init();

	Light_Init(LightAddress);


	Display_BackgroundWriter_Init();
	SD_BackroundWriter_Init();
	DMA_Init();


	display_putString("   BaseStation   ",1,0,System5x7);
	strcat(temp,"  Hardware: v");
	strcat(temp,HardwareVersion);
	display_putString(temp,3,0,System5x7);
	temp[0] = 0;
	strcat(temp," Firmware: v");
	strcat(temp,FirmwareVersion);
	display_putString(temp,5,0,System5x7);
	display_writeBufferToScreen();
	_delay_ms(1000);


    SD_Init();
    Interrupt_Init();

	while(!SD_Inserted()){
		Leds_Toggle(sd_Red);
		_delay_ms(500);
	}
	Leds_Clear(sd_Red);
	Leds_Set(sd_Green);
	_delay_ms(1000);

	Read_config_file();

    if(demoMode){
		display_putString("    Demo Mode    ",7,0,System5x7);
	} else if(useWifiForUploading) {
	    display_putString("  Uploader: Wifi ",7,0,System5x7);
	} else {
	    display_putString("  Uploader: USB  ",7,0,System5x7);
	}
    display_writeBufferToScreen();

    if(useWifiForUploading){
	   Debug_Init(9600);
	} else {
	   Debug_Init(115200);
	}


    if(demoMode){                                   // go into demo mode
		Leds_Set(wifi_Green);
        display_clearBuffer();
	    display_writeBufferToScreen();
	    okToDisplayGUI = true;
        okToOpenLogFile = true;
	    while(!recording);
		while(true);
	} else {
	    Leds_Set(wifi_Red);
	}

    Reset:
    okToFindFileToUpload = false;
    connected = false;
    if(useWifiForUploading){                // connect to uploader  (wifi)
        okToDisplayGUI = false;
        Wifi_Init(9600);
        while(!connected && !demoMode){
            Config_Wifi();
       	    _delay_ms(500);
        }
    } else {                                // usb uploader
        connectToComputer();
       	_delay_ms(500);
       	connected = true;
        signalStrength = 100;
    }


  	Leds_Clear(wifi_Red);
	Leds_Set(wifi_Green);

	display_clearBuffer();
	display_writeBufferToScreen();

    okToDisplayGUI = true;

    if((!recording) && (!uploadTimedOut) && useWifiForUploading){
	    okToOpenLogFile = true;
	    while(!recording);
	} else {
	    Leds_Clear(wifi_Green);
	    Leds_Clear(wifi_Red);
	}

	_delay_ms(100);

	okToReopenDirectory = true;
	okToFindFileToUpload = true;

    Main:

    while(true){
        if(!useWifiForUploading){
            if(Debug_CharReadyToRead()){

			    timeOutCounter = 0;
			    command[0] = Debug_GetByte(false);

                if(command[0] == 'T'){                          // supply the time
            	    if(getTime()){
            	        timeIsValid = true;
                        okToDisplayGUI = true;
            		    if(!recording){
            		        okToOpenLogFile = true;
            	            while(!recording);
                        }
            	    } else {
                        timeIsValid = false;
			        }
    			} else if(command[0] == 'S'){                          // request SSID
                    sendSSID();
		        } else if(command[0] == 'A'){                          // request authorization type
                    sendAuthType();
    			} else if(command[0] == 'K'){                          // request authorisation key
                    sendKey();
                } else if(command[0] == 'U'){                          // request user
			        sendUser();
    			} else if(command[0] == 'N'){                   // request nickname
	    		    sendNickname();
    			} else if(command[0] == 'D'){                   // request data from file
    			    if(okToUpload){
    			        uploading = true;
    			        _delay_ms(100);
                        okToOpenFileToUpload = true;
                        while(!uploadFileOpened);

    			        _delay_ms(1000);
    			    }
                    uploadFile();
                } else if(command[0] == 'E'){                   // erase file
                    eraseFile();
                } else if(command[0] == 'V'){                   // request server for post
                    sendServer();
                } else if(command[0] == 'O'){                   // request port for post
                    sendPort();
    			}  else if(command[0] == 'R'){                   // reset
	    			_delay_ms(5);
		    		Debug_SendByte('R');
		    		Leds_Clear(wifi_Green);
			    	goto Reset;
			    }
		    }
		    _delay_ms(1);
		    timeOutCounter++;
		    if(timeOutCounter > 30000){
			    goto Reset;
		    }
        } else {
    	    if(ssRefreshCounter > 9000){
	            okToGetRemainingSpace = true;
	            while(!okToGetRemainingSpace);

		        if(useWifiForUploading){
		            Wifi_EnterCMDMode(1000);
		            signalStrength = Wifi_GetSignalStrength(1000);
		            if(Wifi_Connected(500)){
		                _delay_ms(500);
	                    if(Wifi_GetTime(500)){
		                    Time_Set(time_secs);
		                }
		                _delay_ms(500);
		            }
		            Wifi_ExitCMDMode(500);
    		    } else {
                    if(Debug_Connected(500)){
                        _delay_ms(500);
		                Time_Set(Debug_GetTime(1000));
		            }
		        }
		        ssRefreshCounter = 0;
	        }

            if(okToUpload && Wifi_Connected(500)){
                uploading = true;
                if(fileToUpload[0] != '/'){
			        strcpy(newFileName, "/");
			        strcat(newFileName, fileToUpload);
			        strcpy(fileToUpload, newFileName);
			    }
                if(useWifiForUploading){
			        Debug_SendString("",true);
                    Debug_SendString("_____________________________________________", true);
			        Debug_SendString("Uploading File: ", false);
			        Debug_SendString(fileToUpload, true);
			    }
                okToOpenFileToUpload = true;
                while(!uploadFileOpened);
                _delay_ms(2000);


                numberOfPacketsToUpload = uploadFileSize /  1000;
                leftOverBytesToUpload   = uploadFileSize %  1000;

                if(useWifiForUploading){
                    Debug_SendString("File Opened!", true);
                    sprintf(temp,"File Size: %lu",uploadFileSize);
                    Debug_SendString(temp, true);

                    Open_Connection:
                    Wifi_EnterCMDMode(500);
                    _delay_ms(1000);

                    if(!Wifi_SendCommand(serverOpenCommand,"Connect to","Connect to",500)){

                        _delay_ms(1000);
                        Wifi_ExitCMDMode(500);
                        _delay_ms(10000);
                        goto Open_Connection;
                    }
                    _delay_ms(4000);
                    _delay_ms(4000);

                    tempStringFiller = 0;
                    while(Wifi_CharReadyToRead()){
                        temp[tempStringFiller] = Wifi_GetByte(false);
                        tempStringFiller++;
                        if(tempStringFiller == 49){
                            break;
                        }
                    }
                    temp[tempStringFiller] = 0;


                    if(strstr(temp,"*OPEN*") != 0){                            // success
                        Debug_SendString("Connection Open!",true);
                        _delay_ms(1000);
                    } else if (strstr(temp,"ERR:Connected!")!=0){

                        if(!Wifi_Connected(500)){
                            Wifi_ExitCMDMode(500);
                            goto Main;
                        } else{
                            Debug_SendString("Let't retry connecting...",true);
                            Wifi_SendCommand("close","*CLOS*","*CLOS*",500);
                            _delay_ms(1000);
                            Wifi_ExitCMDMode(500);
                            goto Open_Connection;
                        }
                    } else{
                        if(!Wifi_Connected(500)){
                            Wifi_ExitCMDMode(500);
                            goto Main;
                        } else{
                            Debug_SendString("Other issues: ", true);
                            _delay_ms(1000);
                            Wifi_ExitCMDMode(500);
                            _delay_ms(10000);
                            goto Open_Connection;
                        }
                    }

                    Debug_SendString("Sending...", true);
                }

                uploadTimeStart = UNIX_time;
                uploadTimedOut = false;


                memmove(temp,strtok(fileToUpload,"/"),12);
                memmove(fileToUpload,temp,12);

                uploadHeaderSize = 177;
                uploadHeaderSize += strlen(user);
                uploadHeaderSize += strlen(fileToUpload);
                uploadHeaderSize += strlen(nickname);
                uploadHeaderSize += strlen(ltoa(uploadFileSize,temp,10));
                uploadHeaderSize += uploadFileSize;


                if(useWifiForUploading || Debug_TriggerUpload(uploadHeaderSize, 2000)){
                    Wifi_SendString("POST /users/",false);                              // 12
                    Wifi_SendString(user,false);
                    Wifi_SendString("/binupload?dev_nickname=",false);                  // 24
                    Wifi_SendString(nickname,false);
                    Wifi_SendString("&filename=",false);                                // 10
                    Wifi_SendString(fileToUpload, false);
                    Wifi_SendString(" HTTP/1.1",true);                                  // 11

                    Wifi_SendString("Host: bodytrack.org",true);                        // 21
                    Wifi_SendString("Content-Type: application/octet-stream",true);     // 40
                    Wifi_SendString("Content-Transfer-Encoding: binary",true);          // 35
                    sprintf(temp, "Content-Length: %lu",uploadFileSize);                    // 18
                    Wifi_SendString(temp,true);
                    Wifi_SendByte(0x0D);                                                // 1
                    Wifi_SendByte(0x0A);                                                // 1

                    for(uint32_t z = 0; z < numberOfPacketsToUpload; z++){
                        uploadFileBufferFull = false;
                        okToFillUploadFileBuffer = true;

                        uploadPercentBS = (z*100)/numberOfPacketsToUpload;
                        while(!uploadFileBufferFull);
                        for(uint16_t j = 0; j <  uploadChunkSize; j++){
                            Wifi_SendByte(uploadFileBuffer[j]);

                            if(Wifi_CharReadyToRead()){
                               Debug_SendByte(Wifi_GetByte(false));
                            }
                        }
                    }
                    uploadFileBufferFull = false;
                    okToFillUploadFileBuffer = true;
                    while(!uploadFileBufferFull);
                    for(uint16_t j = 0; j < leftOverBytesToUpload; j++){
                        Wifi_SendByte(uploadFileBuffer[j]);
                    }
                    Wifi_SendByte(0x0D);                                                // 4
                    Wifi_SendByte(0x0A);
                    Wifi_SendByte(0x0D);
                    Wifi_SendByte(0x0A);

                    uploadPercentBS = 100;
                    _delay_ms(1000);

              //Wait_For_Close:
                    httpResponseReceived = false;
                    connectionClosed = false;
                    if(useWifiForUploading){
                        Debug_SendString("Wait for connection to close",true);
                    }

                    httpResponse[0] = 0;
                    connectionTimeoutTimer = 0;
                    byteReceived = 0;
                    while(!connectionClosed){
                        if(Wifi_CharReadyToRead()){
                            byteReceived = Wifi_GetByte(false);
                        }
                        if(byteReceived == '*'){
                            byteReceived = Wifi_GetByte(true);
                            if(byteReceived == 'C'){
                                byteReceived = Wifi_GetByte(true);
                                if(byteReceived == 'L'){
                                    byteReceived = Wifi_GetByte(true);
                                    if(byteReceived == 'O'){
                                        byteReceived = Wifi_GetByte(true);
                                        if(byteReceived == 'S'){
                                            connectionClosed = true;
                                            break;
                                        }
                                    }
                                }
                            }

                        } else if(byteReceived == 'H'){
                            byteReceived = Wifi_GetByte(true);
                            if(byteReceived == 'T'){
                                byteReceived = Wifi_GetByte(true);
                                if(byteReceived == 'T'){
                                    byteReceived = Wifi_GetByte(true);
                                    if(byteReceived == 'P'){
                                        httpResponseReceived = true;
                                        _delay_ms(1000);
                                        tempStringFiller = 0;
                                        while(Wifi_CharReadyToRead()){
                                            temp[tempStringFiller] = Wifi_GetByte(false);
                                            tempStringFiller++;
                                            if(tempStringFiller == 8){
                                                break;
                                            }
                                        }
                                        temp[tempStringFiller] = 0;
                                        memcpy(httpResponse,temp+5,3);
                                        lengthOfHttpResponse = 0;
                                        _delay_ms(5000);
                                        while(Wifi_CharReadyToRead()){
                                            uploadFileBuffer[lengthOfHttpResponse] = Wifi_GetByte(false);
                                            lengthOfHttpResponse++;
                                            if(lengthOfHttpResponse > 999){
                                                break;
                                            }
                                        }
                                        uploadFileBuffer[lengthOfHttpResponse] = 0;
                                        if(strstr(uploadFileBuffer,"*CLOS") != 0){
                                            connectionClosed = true;
                                        }
                                    }
                                }
                            }
                        }
                        _delay_ms(1);
                        connectionTimeoutTimer++;
                        if(connectionTimeoutTimer > 120000){
                            connectionTimeoutTimer = 0;
                            break;
                        }
                    }
                }

                if(useWifiForUploading){
                   Debug_SendString("Connection Closed",true);
                }

                uploadTimeStop = UNIX_time;


                if((httpResponseReceived) & (strstr(httpResponse,"200") != 0)){
                    memcpy(successfulBinaryRecordsString,strtok((strstr(uploadFileBuffer,"\"successful_binrecs\":") + 21),","),5);
                    memcpy(failedBinaryRecordsString    ,strtok((strstr(uploadFileBuffer,"\"failed_binrecs\":") + 17),","),5);

                    if(useWifiForUploading){
                        Debug_SendString("Got a 200 back",true);
                        Debug_SendString("Successful: ",false);
                        Debug_SendString(successfulBinaryRecordsString,true);
                        Debug_SendString("Failed: ",false);
                        Debug_SendString(failedBinaryRecordsString,true);
                        sprintf(temp,"File TX took: %lu secs",uploadTimeStop - uploadTimeStart);
                        Debug_SendString(temp,true);
                        sprintf(temp,"TX speed: %lu kbps", uploadFileSize/(128*(uploadTimeStop - uploadTimeStart)));
                        Debug_SendString(temp,true);

                    }

                    okToCloseUploadFile = true;                   // flags it close file and to be to be renamed
                    _delay_ms(1000);
                    while(okToCloseUploadFile);
                    okToRenameUploadFile = true;
                    while(okToRenameUploadFile);
                    _delay_ms(1000);
                    okToWriteUploaderLogFile = true;               // adds entry to log file
                    while(okToWriteUploaderLogFile);

                } else {
                    if(useWifiForUploading){
                        Debug_SendString("File did not upload",true);
                            Debug_SendString("got: *",false);
                        Debug_SendString(httpResponse,false);
                        Debug_SendString("* back",true);
                    }


    			    okToCloseUploadFile = true;                    // flags it close file
                    _delay_ms(1000);
                    while(okToCloseUploadFile);
                    _delay_ms(1000);

			        uploadFailed = true;                             // adds error entry to log file
                    while(uploadFailed);
                }

                if(useWifiForUploading){
                  Debug_SendString("Done!", true);
                  Debug_SendString("_____________________________________________", true);
                }

		        uploading = false;
			    okToUpload = false;
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

	// Debug
	DMA.CH1.ADDRCTRL |= DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc	| DMA_CH_DESTRELOAD_BLOCK_gc | DMA_CH_DESTDIR_INC_gc;
    DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_USARTC0_RXC_gc;
    DMA.CH1.TRFCNT = Debug_BufferSize;	// 1024 bytes in block
    DMA.CH1.REPCNT  = 0;		// repeat forever

    DMA.CH1.SRCADDR0 = (((uint16_t)(&Debug_Usart.DATA) >> 0) & 0xFF);
    DMA.CH1.SRCADDR1 = (((uint16_t)(&Debug_Usart.DATA) >> 8) & 0xFF);
    DMA.CH1.SRCADDR2 = 0x00;

    DMA.CH1.DESTADDR0 = (((uint16_t)(&DebugBuffer[0]) >> 0) & 0xFF);
    DMA.CH1.DESTADDR1 = (((uint16_t)(&DebugBuffer[0]) >> 8) & 0xFF);
    DMA.CH1.DESTADDR2 = 0x00;

    DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;

}

void Interrupt_Init(void)
{
	PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;	// enable low level interrupts
	sei();							// enable all interrupts
}

void Display_BackgroundWriter_Init(void){
	// fclk = 14745600
	// div  = 1440
	// => 14745600/1440/1024 => 10 samples per second

	// Set period/TOP value
	TCD0.PER = 1440;

	// Select clock source
	TCD0.CTRLA = (TCD0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;

	// Enable CCA interrupt
	TCD0.INTCTRLA = (TCD0.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_LO_gc;
}

ISR(TCD0_OVF_vect){

    if(okToDisplayGUI){
        ssRefreshCounter++;

        if(recording){
            if(recordFileRestartCounter == 0){
                if(UNIX_time > timeToStopRecording){
	                restartingFile = true;
	                recordFileRestartCounter = 0;
		            rs232Recording = false;
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
	        recordFileRestartCounter++;
        }



	// controls


        if(currentMode == recordMode && Dpad_CheckButton(Down)){
		    currentMode = sensorMode;
		    display_clearBuffer();
		    display_writeBufferToScreen();
	    } else if(currentMode == sensorMode && Dpad_CheckButton(Up)){
		    currentMode = recordMode;
		    display_clearBuffer();
		    display_writeBufferToScreen();
		    _delay_ms(400);
	    } else if(currentMode == recordMode && !recording && SD_Inserted() && !Dpad_CheckButton(Up) && !restartingFile){					// waiting to start recording
		    Leds_Clear(sd_Red);
		    Leds_Set(sd_Green);
		    if(connected){
		        Leds_Clear(wifi_Red);
                Leds_Set(wifi_Green);
		    } else {
		        Leds_Clear(wifi_Green);
                Leds_Set(wifi_Red);
		    }
	    } else if(currentMode == recordMode && !recording && !SD_Inserted()){
		    Leds_Set(sd_Red);
		    Leds_Clear(sd_Green);
	    } else if(currentMode == recordMode && Dpad_CheckButton(Up) && !recording){
            if(percentDiskUsed < 950){
                display_putString("Recording      0m",0,0,System5x7);
                display_drawLine(1,60,7,60,true);		// up arrow
		        display_drawPixel(2,59,true);
		        display_drawPixel(3,58,true);
                display_drawPixel(2,61,true);
                display_drawPixel(3,62,true);
		        display_writeBufferToScreen();

                okToGetRemainingSpace = true;
                while(!okToGetRemainingSpace);

		        Leds_Clear(sd_Green);
		        Leds_Clear(sd_Red);
		        Leds_Clear(wifi_Green);
		        Leds_Clear(wifi_Red);
		        Leds_Clear(ext_Green);
		        Leds_Clear(ext_Red);

                okToOpenLogFile = true;
		        _delay_ms(50);
            } else {
                Leds_Clear(sd_Green);
		        Leds_Set(sd_Red);

            }


	    } else if(currentMode == recordMode && Dpad_CheckButton(Up) && recording){
		    rs232Recording = false;
		    recording = false;

		    Sensors_ResetTemperatureBuffers();
		    Sensors_ResetHumidityBuffers();
		    Sensors_ResetPressureBuffers();
		    Sensors_ResetMicrophoneBuffers();
		    Sensors_ResetLightBuffers();
		    okToCloseLogFile = true;


		    display_putString("Paused           ",0,0,System5x7);
		    display_drawLine(1,60,7,60,true);		// up arrow
		    display_drawPixel(2,59,true);
		    display_drawPixel(3,58,true);
		    display_drawPixel(2,61,true);
		    display_drawPixel(3,62,true);
		    display_writeBufferToScreen();

            if(timeIsValid || demoMode){
		        Leds_Set(wifi_Green);
		    } else {
		        Leds_Set(wifi_Red);
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
	    }

	// load displays

	    if(currentMode == recordMode){
		    if(recording){
			    sprintf(temp2, "Recording   %4lum", (UNIX_time - timeRecordingStarted)/60);		// load recording screen
			    display_putString(temp2,0,0,System5x7);
		    } else {
			    display_putString("Paused           ",0,0,System5x7);
		    }

		    display_drawLine(1,60,7,60,true);		// up arrow
		    display_drawPixel(2,59,true);
		    display_drawPixel(3,58,true);
		    display_drawPixel(2,61,true);
		    display_drawPixel(3,62,true);

		    sprintf(temp2, "Uploading    %3u", uploadPercentBS);
		    strcat(temp2,"%");
		    display_putString(temp2,1,0,System5x7);
            sprintf(temp2,"Disk Used: %3lu.%lu",percentDiskUsed/10,percentDiskUsed%10);
		    strcat(temp2,"%");
		    display_putString(temp2,2,0,System5x7);




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
			    sprintf(temp2, "Ext Charging  %2u",chargePercent);
			    strcat(temp2,"%");
			    display_putString(temp2,3,0,System5x7);
			    okToCharge = true;
			    if(!rtcSynced){
			        _delay_ms(500);
			        if(useWifiForUploading){
			            Debug_SendString("Syncing RTC", true);
			        }
				    RTC_init();
				    RTC_setUTCSecs(UNIX_time);
				    rtcSynced = true;
				    if(useWifiForUploading){
				        Debug_SendString("RTC synced", true);
				    }
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
			    //Leds_Clear(ext_Red);
			    //Leds_Clear(ext_Green);
		    }


		    sprintf(temp2, "Uploading    %3u", uploadPercentEXT);
		    strcat(temp2,"%");
		    display_putString(temp2,4,0,System5x7);


		    RTC_UTCSecsToTime(UNIX_time,&time);
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

            sprintf(temp2,"Time %2u:%02u:%02u ", clockHour, time.Minute, time.Second);
            if(displayAM){
                strcat(temp2,am);
            } else if(displayPM){
                strcat(temp2,pm);
            }
		    display_putString(temp2,6,0,System5x7);

		    sprintf(temp2, "Wifi %3u",signalStrength);
		    strcat(temp2,"%   more");
		    display_putString(temp2,7,0,System5x7);


		    display_drawLine(56,98,63,98,true);		// down arrow
		    display_drawPixel(62,97,true);
		    display_drawPixel(61,96,true);
		    display_drawPixel(62,99,true);
		    display_drawPixel(61,100,true);

		    display_writeBufferToScreen();



	    } else if(currentMode == sensorMode){
		    display_clearBuffer();
		    display_putString("   Sensors  back",0,0,System5x7);
		    display_drawLine(8, 15, 8,61,true);

		    display_drawLine(1,99,7,99,true);		// up arrow
		    display_drawPixel(2,98,true);
		    display_drawPixel(3,97,true);
		    display_drawPixel(2,100,true);
		    display_drawPixel(3,101,true);

            sprintf(temp2,"Temperature: %3uC", quickTemperature);
		    display_putString(temp2,2,0,System5x7);
            sprintf(temp2,"Humidity:  %3u", quickHumidity);
		    strcat(temp2, "%RH");
		    display_putString(temp2,3,0,System5x7);
		    sprintf(temp2,"Pressure:  %3ukPa", quickPressure);
		    display_putString(temp2,4,0,System5x7);
            sprintf(temp2,"Light:      %5u", quickLight);
		    display_putString(temp2,5,0,System5x7);
            sprintf(temp2,"Air: %5lu, %5lu", quickSmall, quickLarge);
		    display_putString(temp2,6,0,System5x7);
		    display_putString("Sound:           ",7,0,System5x7);
		    display_drawRectangle(57,50,7,quickMic/2,true,false,true);
		    display_writeBufferToScreen();

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
    				if(useWifiForUploading){
	    			    Debug_SendString("Syncing RTC", true);
		    		}
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
	TCE1.INTCTRLA = (TCE1.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;

}

ISR(TCE1_OVF_vect)
{

	if(okToSendMicrophoneBuffer1 && recording && !restartingFile){
		SD_WriteMicrophoneBuffer(1);
		okToSendMicrophoneBuffer1  = false;
	} else if (okToSendMicrophoneBuffer2 && recording && !restartingFile){
		SD_WriteMicrophoneBuffer(2);
		okToSendMicrophoneBuffer2 = false;
	}

	if(okToSendTemperatureBuffer1 && recording && !restartingFile){
		SD_WriteTemperatureBuffer(1);
		okToSendTemperatureBuffer1 = false;
	} else if (okToSendTemperatureBuffer2 && recording && !restartingFile){
		SD_WriteTemperatureBuffer(2);
		okToSendTemperatureBuffer2 = false;
	}

	if(okToSendHumidityBuffer1 && recording && !restartingFile){
		SD_WriteHumidityBuffer(1);
		okToSendHumidityBuffer1 = false;
	} else if (okToSendHumidityBuffer2 && recording && !restartingFile){
		SD_WriteHumidityBuffer(2);
		okToSendHumidityBuffer2 = false;
	}

	if(okToSendPressureBuffer1 && recording && !restartingFile){
		SD_WritePressureBuffer(1);
		okToSendPressureBuffer1 = false;
	} else if (okToSendPressureBuffer2 && recording && !restartingFile){
		SD_WritePressureBuffer(2);
		okToSendPressureBuffer2 = false;
	}

	if(okToSendLightBuffer1 && recording && !restartingFile){
		SD_WriteLightBuffer(1);
		okToSendLightBuffer1 = false;
	} else if (okToSendLightBuffer2 && recording && !restartingFile){
		SD_WriteLightBuffer(2);
		okToSendLightBuffer2 = false;
	}

	if(okToSendAirQuality && rs232Recording && !restartingFile){
		uint8_t counter = 0;
		while(Rs232_CharReadyToRead()){
			airQualityString[counter] = Rs232_GetByte(false);
			counter++;
		}
		if(strstr(airQualityString,"Dylos") == NULL){
			airSampleTime = Time_Get32BitTimer();
			smallCount = atol(strtok(airQualityString,","));
			largeCount = atol(strtok(NULL,","));

			quickSmall = smallCount;
			quickLarge = largeCount;
        }
        SD_WriteAirSample();
		okToSendAirQuality = false;
	}

	if(okToOpenLogFile && (percentDiskUsed < 950)){
		SD_StartLogFile(UNIX_time);								// open file
		_delay_ms(100);

        timeToStopRecording = UNIX_time + 900;

		Leds_Clear(sd_Green);
		Leds_Clear(wifi_Green);
		timeRecordingStarted = UNIX_time;
		SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_time);

		Rs232_ClearBuffer();
		rs232Recording = true;
		recording = true;
		okToOpenLogFile = false;
		directoryOpened = false;
		okToReopenDirectory = true;
	}

	if(okToCloseLogFile){
		SD_Close();
		okToCloseLogFile = false;
		directoryOpened = false;
		okToReopenDirectory = true;
	}

	if(okToSendRTCBlock && recording){
		SD_WriteRTCBlock(Time_Get32BitTimer(),UNIX_time);
		okToSendRTCBlock = false;
	}


	if(okToFindFileToUpload && SD_Inserted() && !uploading){
		if(okToReopenDirectory){
			if(f_opendir(&dir, "/") == FR_OK){
				directoryOpened = true;
				okToReopenDirectory = false;
			}
		}

		if(directoryOpened){
			if((f_readdir(&dir, &fno) == FR_OK)){
				if(fno.fname[0] == 0){
					directoryOpened = false;
					okToReopenDirectory = true;
				} else {
					if(recording){
						if((strcasecmp(currentLogFile,fno.fname)) > 0){		// file is NOT the current file
							if(strcasestr(fno.fname,".BT") != NULL){						// file has .bt extension
								if(strcasestr(fno.fname,".BTU") == NULL){
									strcpy(fileToUpload,fno.fname);
									okToUpload= true;
								}
							}
						}
					} else {
						if(strcasestr(fno.fname,".BT") != NULL){						// file has .bt extension
							if(strcasestr(fno.fname,".BTU") == NULL){
								strcpy(fileToUpload,fno.fname);
								okToUpload = true;
							}
						}
					}
				}
			}
		}
	}

	if(okToOpenFileToUpload){
	   f_stat(fileToUpload,&fno);
	   uploadFileSize = fno.fsize;
	   f_open(&Upload_File, fileToUpload, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
	   f_lseek(&Upload_File, 0);
	   uploadFileOpened = true;
	   okToOpenFileToUpload = false;
	}

    if(okToFillUploadFileBuffer){
        uint16_t tmp;
        f_read(&Upload_File,&uploadFileBuffer,uploadChunkSize,&tmp);
        okToFillUploadFileBuffer = false;
        uploadFileBufferFull = true;
    }


	if(okToCloseUploadFile){
	    f_sync(&Upload_File);
	    f_close(&Upload_File);
		okToCloseUploadFile = false;
	}

	if(okToRenameUploadFile){
	    strcpy(newFileName, fileToUpload);
        strcat(newFileName, "U");
	    f_rename(fileToUpload,newFileName);
	    okToRenameUploadFile = false;
	    f_sync(&Upload_File);
	}




	if(okToWriteUploaderLogFile){

        if(useWifiForUploading){
            Debug_SendString("Updating uploadLg File",true);
        }
        f_stat("/uploadLg.txt",&fno2);
        f_open(&Upload_File, "/uploadLg.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);

        if(fno2.fsize > 0){
           f_lseek(&Upload_File, fno2.fsize);
        }
        f_puts("File Name: ",&Upload_File);
        f_puts(fileToUpload,&Upload_File);
        f_puts(", Size: ",&Upload_File);
        f_puts(ltoa(uploadFileSize,temp3,10),&Upload_File);
        f_puts(" bytes, Response: ",&Upload_File);
        f_puts(httpResponse,&Upload_File);
        f_puts(", Successful Records: ",&Upload_File);
        f_puts(successfulBinaryRecordsString,&Upload_File);
        f_puts(", Failed Records: ",&Upload_File);
        f_puts(failedBinaryRecordsString,&Upload_File);
        f_puts(", Time to upload: ",&Upload_File);
        f_puts(ltoa(uploadTimeStop - uploadTimeStart,temp3,10),&Upload_File);
        f_puts(" secs, Speed of upload: ",&Upload_File);
        f_puts(ltoa(uploadFileSize/(128*(uploadTimeStop - uploadTimeStart)),temp3,10),&Upload_File);

        f_puts(" kbps",&Upload_File);
        f_putc(13,&Upload_File);
        f_sync(&Upload_File);
	    f_close(&Upload_File);
	    okToWriteUploaderLogFile = false;
	}

	if(uploadFailed){

        if(useWifiForUploading){
            Debug_SendString("Updating uploadLg File",true);
        }
        f_stat("/uploadLg.txt",&fno2);
        f_open(&Upload_File, "/uploadLg.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
        if(fno2.fsize > 0){
           f_lseek(&Upload_File, fno2.fsize);
        }
        f_puts("File Name: ",&Upload_File);
        f_puts(fileToUpload,&Upload_File);
        f_puts(", Size: ",&Upload_File);
        f_puts(ltoa(uploadFileSize,temp3,10),&Upload_File);
        f_puts(" bytes, uploadFailed failed...",&Upload_File);

        f_putc(13,&Upload_File);
        f_sync(&Upload_File);
	    f_close(&Upload_File);
	    uploadFailed = false;
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
	}

	if(okToEraseFile){
        f_unlink(fileToErase);
	    okToEraseFile = false;
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

	SD_Open("/config.txt");

	while(true){
	  f_gets(temp,50,&Log_File);
	  if(temp[0] != 0){
	    if(strstr(temp,"ssid") != 0){
	      strtok(temp,"=");
	      strcpy(ssid,strtok(NULL,"="));
	      ssidRead = true;
	    } else if(strstr(temp,"phrase") != 0){
	      strtok(temp,"=");
	      strcpy(phrase,strtok(NULL,"="));
	      phraseRead = true;
	    } else if(strstr(temp,"key") != 0){
	      strtok(temp,"=");
	      strcpy(key,strtok(NULL,"="));
	      keyRead = true;
	    } else if(strstr(temp,"port") != 0){
	      strtok(temp,"=");
	      strcpy(port,strtok(NULL,"="));
	    } else if(strstr(temp,"auth") != 0){
	      strtok(temp,"=");
	      strcpy(auth,strtok(NULL,"="));
	      authRead = true;
	     }else if(strstr(temp,"user") != 0){
	      strtok(temp,"=");
	      strcpy(user,strtok(NULL,"="));
	      for(uint8_t i = 0; i < strlen(user); i++){
            if(user[i] < ' '){
               user[i] = 0;
               break;
            }
          }
	    } else if(strstr(temp,"nickname") != 0){
	      strtok(temp,"=");
	      strcpy(nickname,strtok(NULL,"="));
          for(uint8_t i = 0; i < strlen(nickname); i++){
            if(nickname[i] < ' '){
               nickname[i] = 0;
               break;
            }
          }

	    } else if(strstr(temp,"server") != 0){
	      strtok(temp,"=");
	      strcpy(server,strtok(NULL,"="));
	    } else if(strstr(temp,"daylightTime") != 0){
	      strtok(temp,"=");
	      strcpy(daylightTime,strtok(NULL,"="));
        } else if(strstr(temp,"useWifi") != 0){
            strtok(temp,"=");
            strcpy(uploaderToUse,strtok(NULL,"="));
        } else if(strstr(temp,"demoMode") != 0){
            strtok(temp,"=");
            strcpy(demoModeString,strtok(NULL,"="));
        } else if(strstr(temp,"recordAudio") != 0){
            strtok(temp,"=");
            strcpy(microphoneString,strtok(NULL,"="));
	    } else if(strstr(temp,"zone") != 0){
	      strtok(temp,"=");
          memmove(zone,strtok(NULL,"="),3);
	      if(strcmp(zone,"EST") == 0){
	    	  timeZoneShift = 5;
	    	  zoneChanged = true;
	      } else if(strcmp(zone,"CST") == 0){
	    	  timeZoneShift = 6;
	    	  zoneChanged = true;
	      } else if(strcmp(zone,"MST") == 0){
	    	  timeZoneShift = 7;
	    	  zoneChanged = true;
	      } else if(strcmp(zone,"PST") == 0){
	    	  timeZoneShift = 8;
	    	  zoneChanged = true;
	      }

	    }



	  } else {
	    break;
	  }
	}
    if(server[strlen(server)-1] < 32){
        server[strlen(server)-1] = 0;
    }
	strcat(serverOpenCommand,server);
	strcat(serverOpenCommand," ");
	strcat(serverOpenCommand,port);

    if((strstr(daylightTime,"true") != 0) && (zoneChanged)){
       timeZoneShift--;
    }
    if((strstr(uploaderToUse,"true") != 0)){
       useWifiForUploading = true;
    }
    if((strstr(demoModeString,"true") != 0)){
       demoMode = true;
    }
    if((strstr(microphoneString,"true") != 0)){
       wantToRecordMicrophone = true;
    }
}

void Config_Wifi(void){
	uint8_t col = 0;



    _delay_ms(2000);

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



    Wifi_SendCommand("set sys iofunc 0x50","AOK","AOK",500);    // 0x10
    _delay_ms(1000);


    Wifi_SendCommand("set wlan join 0","AOK","AOK",500);
    _delay_ms(1000);

    Wifi_SendCommand("set uart flow 1","AOK","AOK",500);
    _delay_ms(500);

    Wifi_SendCommand("set comm size 1420","AOK","AOK",500);
	_delay_ms(500);

	Wifi_SendCommand("set comm time 10","AOK","AOK",500);
	_delay_ms(500);

    if(timeServerAttempts > 5){
        Wifi_SendCommand("set time address 195.43.74.3","AOK","AOK",500);
	    _delay_ms(500);
	} else if(timeServerAttempts > 10){
        Wifi_SendCommand("set time address 150.101.254.110","AOK","AOK",500);
	    _delay_ms(500);
    } else if(timeServerAttempts > 15){
            Wifi_SendCommand("set time address 193.171.23.163","AOK","AOK",500);
    	    _delay_ms(500);
    } else if(timeServerAttempts > 20){
        Wifi_SendCommand("set time address 83.64.124.251","AOK","AOK",500);
	    _delay_ms(500);
    } else if(timeServerAttempts > 25){
        Wifi_SendCommand("set time address 131.130.251.107","AOK","AOK",500);
	    _delay_ms(500);
	}
    timeServerAttempts++;

    Wifi_SendCommand("save","Storing in config","Storing in config",500);
	_delay_ms(1000);

	Wifi_SendCommand("reboot","*Reboot*","*Reboot*",500);
	_delay_ms(4000);


    Wifi_EnterCMDMode(1000);
    _delay_ms(1000);

	Wifi_SendCommand("set comm remote 0","AOK","AOK",500);
	_delay_ms(500);



	if(Wifi_SendCommand("set time enable 1","AOK","AOK",500)){
		display_putString("enable time....OK",col,0,System5x7);
	} else {
		display_putString("enable time..FAIL",col,0,System5x7);
	}
	display_writeBufferToScreen();
	_delay_ms(500);
	col++;
	
	if(authRead){
	    strcpy(temp,"set wlan auth ");
	    strcat(temp,auth);
		if(Wifi_SendCommand(temp,"AOK","AOK",500)){
			display_putString("encryption.....OK",col,0,System5x7);
		} else {
			display_putString("encryption...FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(500);
		col++;
	}
	
	if(phraseRead){
	    strcpy(temp,"set wlan phrase ");
	    strcat(temp,phrase);
		if(Wifi_SendCommand(temp,"AOK","AOK",500)){
			display_putString("phrase.........OK",col,0,System5x7);
		} else {
			display_putString("phrase.......FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(500);
		col++;
	} else if(keyRead){
	    strcpy(temp,"set wlan phrase ");
	    strcat(temp,key);
		if(Wifi_SendCommand(temp,"AOK","AOK",500)){
			display_putString("key............OK",col,0,System5x7);
		} else {
			display_putString("key..........FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(500);
		col++;
	}

	if(ssidRead){
	    strcpy(temp,"join ");
	    strcat(temp,ssid);
		if(Wifi_SendCommand(temp,"DeAut","Auto-",2000)){
			display_putString("ssid...........OK",col,0,System5x7);
		} else {
			display_putString("ssid.........FAIL",col,0,System5x7);
		}
		display_writeBufferToScreen();
		_delay_ms(1500);
		col++;
	}

	
	Wifi_SendCommand("get time","TimeEna=1","TimeEna=1",500);
	_delay_ms(500);

	Wifi_GetMac(1000);
	_delay_ms(500);
	Wifi_ExitCMDMode(500);

	_delay_ms(1000);
	
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
			Time_Set(time_secs);
		    timeIsValid = true;
			display_putString("internet.......OK",col,0,System5x7);
			signalStrength = Wifi_GetSignalStrength(1000);

	        Debug_SendString("Getting space remaining",true);
	        okToGetRemainingSpace = true;
	        while(!okToGetRemainingSpace);
            if(!recording){
                Leds_Set(wifi_Green);
			    Leds_Clear(wifi_Red);
            }
			_delay_ms(1000);
            Wifi_SendCommand("set uart instant 460800","AOK","AOK",5);
            Wifi_Init(460800);
	        _delay_ms(3000);
		} else{
			display_putString("internet.....FAIL",col,0,System5x7);
            if(!recording){
			Leds_Set(wifi_Red);
			Leds_Clear(wifi_Green);
            }
			connected = false;
			_delay_ms(500);

		}

		display_writeBufferToScreen();
        _delay_ms(1000);
		while(!Wifi_ExitCMDMode(1000)){
		   _delay_ms(1000);
		    Debug_SendString("Retrying EXIT CMD Mode",true);
		}

	} else{
		display_putString("internet.....FAIL",col,0,System5x7);
		display_writeBufferToScreen();
        if(!recording){
		    Leds_Set(wifi_Red);
        }
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
    Debug_SendString("Debug To Wifi", true);
	while(1){
		if(Debug_CharReadyToRead()){
		    while(!(Wifi_Usart.STATUS & USART_DREIF_bm));
            Wifi_Usart.DATA = Debug_GetByte(true);
		}
		if(Wifi_CharReadyToRead()){
			Debug_SendByte(Wifi_GetByte(true));
		}	
	
	}
}

void getDeviceID(void){
	deviceID[0] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM0_offset );
    deviceID[1] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM1_offset );
	deviceID[2] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM2_offset );
	deviceID[3] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM3_offset );
	deviceID[4] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM4_offset );
	deviceID[5] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + LOTNUM5_offset );
	deviceID[6] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + WAFNUM_offset );
	deviceID[7] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + COORDX0_offset );
	deviceID[8] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + COORDX1_offset );
	deviceID[9] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + COORDY0_offset );
	deviceID[10] = SP_ReadCalibrationByte( PROD_SIGNATURES_START + COORDY1_offset );
	deviceID[11] = '\0';
	for(uint8_t i = 0; i < 11; i++){
	    if(deviceID[i] == 0){
	       deviceID[i] += 1;
	    }
	}
}

uint8_t SP_ReadCalibrationByte( uint8_t index )
{
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
 	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
 	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return result;
}

void connectToComputer(void){
	uint16_t singCounter;
	uint8_t  char1;
	uint8_t  char2;

	connected = false;
	Debug_ClearBuffer();
	while(!connected){
		Debug_SendString("BS",false);
		if(!recording){
		    Leds_Set(wifi_Red);
		}
		singCounter = 750;
		while(singCounter > 0){
			if(Debug_CharReadyToRead()){
				char1 = Debug_GetByte(false);
				if(char1 == 'B'){
					_delay_ms(5);
					char2 = Debug_GetByte(false);
					if(char2 == 'T'){
						connected = true;
						Debug_SendString("BT",false);
						timeOutCounter = 0;
						Debug_ClearBuffer();
						break;
					}
				}
			}
			_delay_ms(1);
			singCounter--;
		}
	}
}

bool getTime(void){

    _delay_ms(100);

    command[1] = Debug_GetByte(false);
	command[2] = Debug_GetByte(false);
	command[3] = Debug_GetByte(false);
	command[4] = Debug_GetByte(false);

    tempTime = command[1];
    tempTime <<= 8;
    tempTime += command[2];
    tempTime <<= 8;
    tempTime += command[3];
    tempTime <<= 8;
    tempTime += command[4];
    Time_Set(tempTime);

    // echo back to cpu
    for(uint8_t j = 0; j < 5; j++){
        Debug_SendByte(command[j]);
    }

    return true;
}


void sendSSID(void){
   if(ssidRead){
       if(ssid[strlen(ssid)-1] < 32){
         ssid[strlen(ssid)-1] = 0;
       }
       Debug_SendByte('S');
       Debug_SendByte(strlen(ssid)+2);
       Debug_SendString(ssid,true);
   } else {
       Debug_SendByte('S');
       Debug_SendByte(0);
       Debug_SendString("",true);
   }
}

void sendAuthType(void){
   if(authRead){
       if(auth[strlen(auth)-1] < 32){
         auth[strlen(auth)-1] = 0;
       }
       Debug_SendByte('A');
       Debug_SendByte(strlen(auth)+2);
       Debug_SendString(auth,true);
   } else {
       Debug_SendByte('A');
       Debug_SendByte(0);
       Debug_SendString("",true);
   }
}

void sendKey(void){
   if(phraseRead){
       if(phrase[strlen(phrase)-1] < 32){
         phrase[strlen(phrase)-1] = 0;
       }
       Debug_SendByte('K');
       Debug_SendByte(strlen(phrase)+2);
       Debug_SendString(phrase,true);
   } else if(keyRead){
       if(key[strlen(key)-1] < 32){
         key[strlen(key)-1] = 0;
       }
       Debug_SendByte('K');
       Debug_SendByte(strlen(key)+2);
       Debug_SendString(key,true);
   } else {
       Debug_SendByte('K');
       Debug_SendByte(0);
       Debug_SendString("",true);
   }
}

void sendUser(void){
	Debug_SendByte('U');
	if(user[strlen(user)-1] < 32){
        user[strlen(user)-1] = 0;
    }
	Debug_SendByte(strlen(user)+2);
	Debug_SendString(user,true);
}

void sendNickname(void){
	Debug_SendByte('N');
	if(nickname[strlen(nickname)-1] < 32){
       nickname[strlen(nickname)-1] = 0;
    }
	Debug_SendByte(strlen(nickname)+2);
	Debug_SendString(nickname,true);
}

void uploadFile(void){
    if(!uploading){
        Debug_SendByte('D');
        Debug_SendByte(0);
        Debug_SendByte(0);
        Debug_SendByte(0);
        Debug_SendByte(0);
        return;
    }

    uint32_t lengthOfResp = 0;
    lengthOfResp = uploadFileSize + strlen(fileToUpload) + 2;
    Debug_SendByte('D');
    Debug_SendByte((lengthOfResp >> 24) & 0xFF);
    Debug_SendByte((lengthOfResp >> 16) & 0xFF);
    Debug_SendByte((lengthOfResp >>  8) & 0xFF);
    Debug_SendByte((lengthOfResp >>  0) & 0xFF);
    Debug_SendString(fileToUpload,true);

    numberOfPacketsToUpload = uploadFileSize /  1000;
    leftOverBytesToUpload   = uploadFileSize %  1000;

    for(uint32_t z = 0; z < numberOfPacketsToUpload; z++){
        uploadFileBufferFull = false;
        okToFillUploadFileBuffer = true;

        uploadPercentBS = (z*100)/numberOfPacketsToUpload;
        while(!uploadFileBufferFull);
            for(uint16_t j = 0; j <  uploadChunkSize; j++){
                Debug_SendByte(uploadFileBuffer[j]);
            }
    }
    uploadFileBufferFull = false;
    okToFillUploadFileBuffer = true;
    while(!uploadFileBufferFull);
    for(uint16_t j = 0; j < leftOverBytesToUpload; j++){
        Debug_SendByte(uploadFileBuffer[j]);
    }

    okToCloseUploadFile = true;
    while(okToCloseUploadFile);

    uploading = false;
    okToUpload = false;
}

void eraseFile(void){
    uint8_t timeOutCounter = 0;
    uint8_t commandCounter = 0;
    uint8_t numBytesToRead = 0;
    Debug_SendByte('E');
    _delay_ms(100);
    while(true){
        if(Debug_CharReadyToRead()){
            if(commandCounter == 0){
                numBytesToRead = Debug_GetByte(false);
                Debug_SendByte(numBytesToRead);
                commandCounter++;
            } else {
                fileToErase[commandCounter-1] = Debug_GetByte(false);
                Debug_SendByte(fileToErase[commandCounter-1]);
                commandCounter++;
                if(commandCounter == (numBytesToRead-1)){
                    fileToErase[numBytesToRead-1] = 0;
                    okToEraseFile = true;
                    while(okToEraseFile);
                    okToUpload = false;
                }
            }
        }
        _delay_ms(1);
        timeOutCounter++;
        if(timeOutCounter > 100){
            return;
        }
    }
}

void sendServer(void){
	Debug_SendByte('V');
	if(server[strlen(server)-1] < 32){
        server[strlen(server)-1] = 0;
    }
	Debug_SendByte(strlen(server)+2);
	Debug_SendString(server,true);
}

void sendPort(void){
	Debug_SendByte('O');
	if(port[strlen(port)-1] < 32){
        port[strlen(port)-1] = 0;
    }
	Debug_SendByte(strlen(port)+2);
	Debug_SendString(port,true);
}

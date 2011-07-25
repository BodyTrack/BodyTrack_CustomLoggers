/*
 *  uploader.c
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/8/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

#include "uploader.h"


#define uploadChunkSize		1000

char	uploadFileBuffer [uploadChunkSize];
char	fileToUpload[15];
char	availableFileName[15];
char	fileToErase [20];
char	command [50];

volatile uint32_t tempTime = 0;

uint8_t				commandCounter = '0';
volatile uint16_t	timeOutCounter = 0;
volatile uint32_t	uploadFileSize = 0;
volatile uint8_t	eraseFileReturn;
volatile uint8_t	uploadPercentBS  = 0;
volatile uint32_t	numberOfPacketsToUpload = 0;
volatile uint32_t	leftOverBytesToUpload = 0;
volatile uint32_t	uploadCRC;

volatile bool connected					= false;
volatile bool uploading					= false;
volatile bool okToOpenDirectory			= false;
volatile bool okToGrabNextFileName		= false;
volatile bool okToUpload				= false;
volatile bool doneUploading				= false;
volatile bool okToOpenFileToUpload		= false;
volatile bool uploadFileOpened			= false;
volatile bool okToFillUploadFileBuffer	= false;
volatile bool uploadFileBufferFull		= false;
volatile bool okToCloseUploadFile		= false;
volatile bool okToEraseFile				= false;
volatile bool fileExists				= false;
volatile bool timeIsValid				= false;


bool Uploader_Update(void){
	if(Debug_CharReadyToRead()){
		
		timeOutCounter = 0;
		command[0] = Debug_GetByte(false);
		
		if(command[0] == 'T'){                          // supply the time
			if(Uploader_getTime()){
				timeIsValid = true;
				UNIX_Time = Time_Get();
			} else {
				timeIsValid = false;
				return false;
			}
		} else if(command[0] == 'S'){                          // request SSID
			Uploader_sendSSID();
		} else if(command[0] == 'A'){                          // request authorization type
			Uploader_sendAuthType();
		} else if(command[0] == 'K'){                          // request authorisation key
			Uploader_sendKey();
		} else if(command[0] == 'U'){                          // request user
			Uploader_sendUser();
		} else if(command[0] == 'N'){                           // request nickname
			Uploader_sendNickname();
		} else if(command[0] == 'F'){                           // request filename
			Uploader_sendFilename();
		} else if(command[0] == 'D'){                           // request data from file
			if(!Uploader_uploadFile()){
				return false;
			}
		} else if(command[0] == 'E'){                   // erase file
			if(!Uploader_eraseFile()){
				return false;
			}
		} else if(command[0] == 'V'){                   // request server for post
			Uploader_sendServer();
		} else if(command[0] == 'O'){                   // request port for post
			Uploader_sendPort();
		}  else if(command[0] == 'R'){                   // reset
			_delay_ms(5);
			Debug_SendByte('R');
			return false;
		}
	}
	_delay_ms(1);
	timeOutCounter++;
	if(timeOutCounter > 30000){
		return false;
	}
	return true;
}

void Uploader_connectToComputer(void){
	uint16_t singCounter;
	uint8_t  char1;
	uint8_t  char2;
	
	connected = false;
	Debug_ClearBuffer();
	while(!connected){
		Debug_SendString("BS",false);
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

bool Uploader_getTime(void){
    uint32_t tempTime = 0;
    uint8_t timeOutCounter = 0;
    uint8_t commandCounter = 0;
    Debug_SendByte('T');
    while(true){
        if(Debug_CharReadyToRead()){
            command[commandCounter+1] = Debug_GetByte(false);
            Debug_SendByte(command[commandCounter+1]);
            commandCounter++;
            if(commandCounter == 4){
                tempTime = command[1];
                tempTime <<= 8;
                tempTime += command[2];
                tempTime <<= 8;
                tempTime += command[3];
                tempTime <<= 8;
                tempTime += command[4];
				
                Time_Set(tempTime);
                return true;
            }
        }
        _delay_ms(1);
        timeOutCounter++;
        if(timeOutCounter > 100){
            return false;
        }
    }
}


void Uploader_sendSSID(void){
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

void Uploader_sendAuthType(void){
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

void Uploader_sendKey(void){
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

void Uploader_sendUser(void){
	Debug_SendByte('U');
	if(user[strlen(user)-1] < 32){
        user[strlen(user)-1] = 0;
    }
	Debug_SendByte(strlen(user)+2);
	Debug_SendString(user,true);
}

void Uploader_sendNickname(void){
	Debug_SendByte('N');
	if(nickname[strlen(nickname)-1] < 32){
		nickname[strlen(nickname)-1] = 0;
    }
	Debug_SendByte(strlen(nickname)+2);
	Debug_SendString(nickname,true);
}

void Uploader_sendFilename(void){
    Debug_SendByte('F');
    okToOpenDirectory = true;
    while(okToOpenDirectory);
	
    while(true){
        okToGrabNextFileName = true;
        while(okToGrabNextFileName);
        if(availableFileName[0] == 0){
            Debug_SendString("",true);
            return;
        } else {
            if(recording){
				if((strcasecmp(currentLogFile,fno.fname)) != 0){		// file is NOT the current file
			    	if(strcasestr(fno.fname,".BT") != NULL){						// file has .bt extension
				    	if(strcasestr(fno.fname,".BTU") == NULL){
						    strcpy(availableFileName,fno.fname);
						    Debug_SendString(availableFileName,false);
						    Debug_SendByte(',');
						}
					}
				}
			} else {
				if(strcasestr(fno.fname,".BT") != NULL){						// file has .bt extension
					if(strcasestr(fno.fname,".BTU") == NULL){
						strcpy(availableFileName,fno.fname);
						Debug_SendString(availableFileName,false);
						Debug_SendByte(',');
					}
				}
			}
        }
    }
}



bool Uploader_uploadFile(void){
	
	
    uint8_t timeOutCounter = 0;
    uint8_t commandCounter = 0;
    uint8_t numBytesToRead = 0;
    uint32_t responseLength;
    bool gotFileName = false;
	
    uploading = true;
    _delay_ms(100);
	
    Debug_SendByte('D');
	
    while(!gotFileName){
        if(Debug_CharReadyToRead()){
            if(commandCounter == 0){
                numBytesToRead = Debug_GetByte(false);
                Debug_SendByte(numBytesToRead);
                commandCounter++;
            } else {
                fileToUpload[commandCounter-1] = Debug_GetByte(false);
                Debug_SendByte(fileToUpload[commandCounter-1]);
                commandCounter++;
                if(commandCounter == (numBytesToRead+1)){
                    fileToUpload[numBytesToRead+1] = 0;
                    gotFileName = true;
                }
            }
        }
        _delay_ms(1);
        timeOutCounter++;
        if(timeOutCounter > 100){
            return false;
        }
    }
	
    okToOpenFileToUpload = true;
    while(!uploadFileOpened);
	_delay_ms(1000);
	
	if(!fileExists){
        Debug_SendByte(0);
        Debug_SendByte(0);
        Debug_SendByte(0);
        Debug_SendByte(0);
        okToCloseUploadFile = true;
        while(okToCloseUploadFile);
        uploading = false;
        okToUpload = false;
        return true;
    }
	
    Uploader_ClearCRC();
    responseLength = uploadFileSize + 4;
    Debug_SendByte((responseLength >> 24) & 0xFF);
    Debug_SendByte((responseLength >> 16) & 0xFF);
    Debug_SendByte((responseLength >>  8) & 0xFF);
    Debug_SendByte((responseLength >>  0) & 0xFF);
	
    numberOfPacketsToUpload = uploadFileSize /  1000;
    leftOverBytesToUpload   = uploadFileSize %  1000;
	
    for(uint32_t z = 0; z < numberOfPacketsToUpload; z++){
        uploadFileBufferFull = false;
        okToFillUploadFileBuffer = true;
		
        uploadPercentBS = (z*100)/numberOfPacketsToUpload;
        while(!uploadFileBufferFull);
		for(uint16_t j = 0; j <  uploadChunkSize; j++){
			Debug_SendByte(uploadFileBuffer[j]);
			Uploader_UpdateCRC(uploadFileBuffer[j]);
			
			
		}
    }
    uploadFileBufferFull = false;
    okToFillUploadFileBuffer = true;
    while(!uploadFileBufferFull);
    for(uint16_t j = 0; j < leftOverBytesToUpload; j++){
        Debug_SendByte(uploadFileBuffer[j]);
        Uploader_UpdateCRC(uploadFileBuffer[j]);
    }
	
    Uploader_WriteCRC();
	
    okToCloseUploadFile = true;
    while(okToCloseUploadFile);
    uploading = false;
    okToUpload = false;
    uploadPercentBS = 100;
    return true;
}

bool Uploader_eraseFile(void){
    uint8_t timeOutCounter = 0;
    uint8_t commandCounter = 0;
    uint8_t numBytesToRead = 0;
    Debug_SendByte('E');
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
                if(commandCounter == (numBytesToRead+1)){
                    fileToErase[numBytesToRead+1] = 0;
                    okToEraseFile = true;
                    while(okToEraseFile);
                    if(eraseFileReturn == FR_OK){
                        Debug_SendByte('T');
                        return true;
                    } else {
                        Debug_SendByte('F');
                        return true;
                    }
                }
            }
        }
        _delay_ms(1);
        timeOutCounter++;
        if(timeOutCounter > 100){
            return false;
        }
    }
}

void Uploader_sendServer(void){
	Debug_SendByte('V');
	if(server[strlen(server)-1] < 32){
        server[strlen(server)-1] = 0;
    }
	Debug_SendByte(strlen(server)+2);
	Debug_SendString(server,true);
}

void Uploader_sendPort(void){
	Debug_SendByte('O');
	if(port[strlen(port)-1] < 32){
        port[strlen(port)-1] = 0;
    }
	Debug_SendByte(strlen(port)+2);
	Debug_SendString(port,true);
}


void Uploader_ClearCRC(void){
    uploadCRC = 0xFFFFFFFF;
}

void Uploader_WriteCRC(void){
    uint32_t tmpCRC = uploadCRC^0xFFFFFFFF;
    Debug_SendByte((tmpCRC >> 24) & 0xFF);
    Debug_SendByte((tmpCRC >> 16) & 0xFF);
    Debug_SendByte((tmpCRC >>  8) & 0xFF);
    Debug_SendByte((tmpCRC >>  0) & 0xFF);
}


void Uploader_UpdateCRC(uint8_t byte){
    uploadCRC = (uploadCRC >> 8) ^ crc_table[byte ^ (uploadCRC & 0xFF)];
}
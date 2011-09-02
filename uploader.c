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
		
		if(command[0] == 'T'){									// supply the time
			if(Uploader_getTime()){
				UNIX_Time = Time_Get();
				timeIsValid = true;				
			} else {
				timeIsValid = false;
				return false;
			}
		} else if(command[0] == 'S'){                          // request SSID
			if(!Uploader_sendSSID()){
				return false;
			}
		} else if(command[0] == 'A'){                          // request authorization type
			if(!Uploader_sendAuthType()){
				return false;
			}
		} else if(command[0] == 'K'){                          // request authorisation key
			if(!Uploader_sendKey()){
				return false;
			}
		} else if(command[0] == 'U'){                          // request user
			if(!Uploader_sendUser()){
				return false;
			}
		} else if(command[0] == 'N'){                           // request nickname
			if(!Uploader_sendNickname()){
				return false;
			}
		} else if(command[0] == 'F'){                           // request filename
			if(!Uploader_sendFilename()){
				return false;
			}
		} else if(command[0] == 'D'){                           // request data from file
			if(!Uploader_uploadFile()){
				return false;
			}
		} else if(command[0] == 'E'){							// erase file
			if(!Uploader_eraseFile()){
				return false;
			}
		} else if(command[0] == 'V'){							// request server for post
			if(!Uploader_sendServer()){
				return false;
			}
		} else if(command[0] == 'O'){							// request port for post
			if(!Uploader_sendPort()){
				return false;
			}
		}  else if(command[0] == 'R'){							// reset
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

bool Uploader_connectToComputer(void){
	uint16_t singCounter;
	uint8_t  char1;
	uint8_t  char2;
	
	connected = false;
	Debug_ClearBuffer();
	while(!connected){
		if(!Debug_SendString("BS",false)){
			return false;
		}
		singCounter = 750;
		while(singCounter > 0){
			if(Debug_CharReadyToRead()){
				char1 = Debug_GetByte(false);
				if(char1 == 'B'){
					_delay_ms(5);
					char2 = Debug_GetByte(false);
					if(char2 == 'T'){
						if(!Debug_SendString("BT",false)){
							return false;
						}
						
						connected = true;
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
	return true;
}

bool Uploader_getTime(void){
    uint32_t tempTime = 0;
    uint8_t timeOutCounter = 0;
    uint8_t commandCounter = 0;
    if(!Debug_SendByte('T')){
		return false;
	}
    while(true){
        if(Debug_CharReadyToRead()){
            command[commandCounter+1] = Debug_GetByte(false);
			if(!Debug_SendByte(command[commandCounter+1])){
				return false;
			}
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
	return true;
}


bool Uploader_sendSSID(void){
	if(ssidRead){
		if(ssid[strlen(ssid)-1] < 32){
			ssid[strlen(ssid)-1] = 0;
		}
		if(!Debug_SendByte('S')){
			return false;
		}
		if(!Debug_SendByte(strlen(ssid)+2)){
			return false;
		}
		if(!Debug_SendString(ssid,true)){
			return false;
		}
	} else {
		if(!Debug_SendByte('S')){
			return false;
		}
		if(!Debug_SendByte(0)){
			return false;
		}
		if(!Debug_SendString("",true)){
			return false;
		}
	}
	return true;
}

bool Uploader_sendAuthType(void){
	if(authRead){
		if(auth[strlen(auth)-1] < 32){
			auth[strlen(auth)-1] = 0;
		}
		if(!Debug_SendByte('A')){
			return false;
		}		
		if(!Debug_SendByte(strlen(auth)+2)){
			return false;
		}		
		if(!Debug_SendString(auth,true)){
			return false;
		}
	} else {
		if(!Debug_SendByte('A')){
			return false;
		}		
		if(!Debug_SendByte(0)){
			return false;
		}		
		if(!Debug_SendString("",true)){
			return false;
		}
	}
	return true;
}

bool Uploader_sendKey(void){
	if(phraseRead){
		if(phrase[strlen(phrase)-1] < 32){
			phrase[strlen(phrase)-1] = 0;
		}
		if(!Debug_SendByte('K')){
			return false;
		}		
		if(!Debug_SendByte(strlen(phrase)+2)){
			return false;
		}		
		if(!Debug_SendString(phrase,true)){
			return false;
		}
	} else if(keyRead){
		if(key[strlen(key)-1] < 32){
			key[strlen(key)-1] = 0;
		}
		if(!Debug_SendByte('K')){
			return false;
		}		
		if(!Debug_SendByte(strlen(key)+2)){
			return false;
		}		
		if(!Debug_SendString(key,true)){
			return false;
		}
	} else {
		if(!Debug_SendByte('K')){
			return false;
		}		
		if(!Debug_SendByte(0)){
			return false;
		}		
		if(!Debug_SendString("",true)){
			return false;
		}
	}
	return true;
}

bool Uploader_sendUser(void){
	if(!Debug_SendByte('U')){
		return false;
	}
	if(user[strlen(user)-1] < 32){
        user[strlen(user)-1] = 0;
    }
	if(!Debug_SendByte(strlen(user)+2)){
		return false;
	}
	if(!Debug_SendString(user,true)){
		return false;
	}
	return true;
}

bool Uploader_sendNickname(void){
	if(!Debug_SendByte('N')){
		return false;
	}
	if(nickname[strlen(nickname)-1] < 32){
		nickname[strlen(nickname)-1] = 0;
    }
	if(!Debug_SendByte(strlen(nickname)+2)){
		return false;
	}
	if(!Debug_SendString(nickname,true)){
		return false;
	}
	return true;
}

bool Uploader_sendFilename(void){
    if(!Debug_SendByte('F')){
		return false;
	}
	
    okToOpenDirectory = true;
    while(okToOpenDirectory);
	
    while(true){
        okToGrabNextFileName = true;
        while(okToGrabNextFileName);
		if(availableFileName[0] == 0){
            if(!Debug_SendString("",true)){
				return false;
			}
            return true;
        } else {
			if(recording){
				if((strcasecmp(currentLogFile,availableFileName)) != 0){		// file is NOT the current file
			    	if(strcasestr(availableFileName,".BT") != NULL){						// file has .bt extension
						if(!Debug_SendString(availableFileName,false)){
							return false;
						}
						if(!Debug_SendByte(',')){
							return false;
						}
					}
				}
			} else {
				if(strcasestr(availableFileName,".BT") != NULL){						// file has .bt extension
					if(!Debug_SendString(availableFileName,false)){
						return false;
					}
					if(!Debug_SendByte(',')){
						return false;
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
	
    if(!Debug_SendByte('D')){
		return false;
	}
	
    while(!gotFileName){
        if(Debug_CharReadyToRead()){
            if(commandCounter == 0){
                numBytesToRead = Debug_GetByte(false);
                if(!Debug_SendByte(numBytesToRead)){
					return false;
				}
                commandCounter++;
            } else {
                fileToUpload[commandCounter-1] = Debug_GetByte(false);
                if(!Debug_SendByte(fileToUpload[commandCounter-1])){
					return false;
				}
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
        if(!Debug_SendByte(0)){
			return false;
		}
        if(!Debug_SendByte(0)){
			return false;
		}
        if(!Debug_SendByte(0)){
			return false;
		}
        if(!Debug_SendByte(0)){
			return false;
		}
        okToCloseUploadFile = true;
        while(okToCloseUploadFile);
        uploading = false;
        okToUpload = false;
        return true;
    }
	
    Uploader_ClearCRC();
    responseLength = uploadFileSize + 4;
	if(!Debug_SendByte((responseLength >> 24) & 0xFF)){
	   return false;
	}
	if(!Debug_SendByte((responseLength >> 16) & 0xFF)){
		return false;
	}
	if(!Debug_SendByte((responseLength >>  8) & 0xFF)){
		return false;
	}
	if(!Debug_SendByte((responseLength >>  0) & 0xFF)){
	   return false;
	}
	
    numberOfPacketsToUpload = uploadFileSize /  1000;
    leftOverBytesToUpload   = uploadFileSize %  1000;
	
    for(uint32_t z = 0; z < numberOfPacketsToUpload; z++){
        uploadFileBufferFull = false;
        okToFillUploadFileBuffer = true;
		
        uploadPercentBS = (z*100)/numberOfPacketsToUpload;
        while(!uploadFileBufferFull);
		for(uint16_t j = 0; j <  uploadChunkSize; j++){
			if(!Debug_SendByte(uploadFileBuffer[j])){
				return false;
			}
			Uploader_UpdateCRC(uploadFileBuffer[j]);
			
			
		}
    }
    uploadFileBufferFull = false;
    okToFillUploadFileBuffer = true;
    while(!uploadFileBufferFull);
    for(uint16_t j = 0; j < leftOverBytesToUpload; j++){
        if(!Debug_SendByte(uploadFileBuffer[j])){
			return false;
		}
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
    if(!Debug_SendByte('E')){
		return false;
	}
    while(true){
        if(Debug_CharReadyToRead()){
            if(commandCounter == 0){
                numBytesToRead = Debug_GetByte(false);
                if(!Debug_SendByte(numBytesToRead)){
					return false;
				}
                commandCounter++;
            } else {
                fileToErase[commandCounter-1] = Debug_GetByte(false);
                if(!Debug_SendByte(fileToErase[commandCounter-1])){
					return false;
				}
                commandCounter++;
                if(commandCounter == (numBytesToRead+1)){
                    fileToErase[numBytesToRead+1] = 0;
                    okToEraseFile = true;
                    while(okToEraseFile);
                    if(eraseFileReturn == FR_OK){
                        if(!Debug_SendByte('T')){
							return false;
						}
                        return true;
                    } else {
                        if(!Debug_SendByte('F')){
							return false;
						}
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

bool Uploader_sendServer(void){
	if(!Debug_SendByte('V')){
		return false;
	}
	if(server[strlen(server)-1] < 32){
        server[strlen(server)-1] = 0;
    }
	if(!Debug_SendByte(strlen(server)+2)){
		return false;
	}
	if(!Debug_SendString(server,true)){
		return false;
	}
	return true;
}

bool Uploader_sendPort(void){
	if(!Debug_SendByte('O')){
		return false;
	}
	if(port[strlen(port)-1] < 32){
        port[strlen(port)-1] = 0;
    }
	   if(!Debug_SendByte(strlen(port)+2)){
		return false;
	}
	if(!Debug_SendString(port,true)){
		return false;
	}
	return true;
}


void Uploader_ClearCRC(void){
    uploadCRC = 0xFFFFFFFF;
}

bool Uploader_WriteCRC(void){
    uint32_t tmpCRC = uploadCRC^0xFFFFFFFF;
    if(!Debug_SendByte((tmpCRC >> 24) & 0xFF)){
		return false;
	}
    if(!Debug_SendByte((tmpCRC >> 16) & 0xFF)){
		return false;
	}
    if(!Debug_SendByte((tmpCRC >>  8) & 0xFF)){
		return false;
	}
    if(!Debug_SendByte((tmpCRC >>  0) & 0xFF)){
		return false;
	}
	return true;
}


void Uploader_UpdateCRC(uint8_t byte){
    uploadCRC = (uploadCRC >> 8) ^ crc_table[byte ^ (uploadCRC & 0xFF)];
}
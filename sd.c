//___________________________________________
//				Joshua Schapiro
//					  sd.c
//			  created: 08/27/2010
//			  updated: 10/12/2010
//___________________________________________

#include "sd.h"

#define MAGIC_NUMBER			0xb0de744c
#define RTYPE_START_OF_FILE		1
#define RTYPE_RTC				2
#define RTYPE_PERIODIC_DATA		3


static FATFS		fso0;
static FATFS		fso1;
FATFS				*fs;
FILINFO				fno,fno2;

DIR					dir;

FIL					Log_File, Upload_File;

char				fileName [20];
char				tmpString[10];

uint8_t				tmp8[1];
uint8_t				tmp16[2];
uint8_t				tmp32[4];

volatile uint32_t	CRC;

char				currentLogFile[15];

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
char demoModeString[10];
char fastString[10];

uint8_t timeZoneShift = 0;


bool authRead = false;
bool phraseRead = false;
bool keyRead = false;
bool ssidRead = false;
bool zoneChanged = false;

volatile bool sdValid = false;

uint32_t percentDiskUsed = 0;


const uint32_t crc_table[256] = {
0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
};


uint8_t SD_Init(void){
	uint8_t tmp;

	SD_CD_Port.DIRCLR = 1 << SD_CD;
	SD_Timer_Init();
	tmp = disk_initialize(0);
	f_mount(0, &fso0);
	f_mount(1, &fso1);
	if(tmp == FR_OK){
		sdValid = true;
		SD_GetSpaceRemaining();
	} else {
		sdValid = false;
	}
	Button_Port.PIN4CTRL =  PORT_OPC_WIREDORPULL_gc;
	return tmp;
}


uint8_t SD_Open(char string []){
	strcpy(currentLogFile,string);
	strupr(currentLogFile);
	for(uint8_t i = 1; i < strlen(currentLogFile); i++){
		currentLogFile[i-1] = currentLogFile[i];
	}
	currentLogFile[strlen(currentLogFile) - 1] = 0;

	return f_open(&Log_File, string, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
}

void SD_Close(void){
	f_close(&Log_File);
}

bool SD_Inserted(void){
	return sdValid;
}

void SD_Write8(uint8_t var){

	tmp8[0] = (uint8_t)var;
	SD_WriteBuffer(tmp8,1);
}

void SD_Write16(uint16_t var){
	tmp16[0] = (uint8_t)(var >> 0);
	tmp16[1] = (uint8_t)(var >> 8);
	SD_WriteBuffer(tmp16,2);
}

void SD_Write32(uint32_t var){
	tmp32[0] = (uint8_t)(var >> 0);
	tmp32[1] = (uint8_t)(var >> 8);
	tmp32[2] = (uint8_t)(var >> 16);
	tmp32[3] = (uint8_t)(var >> 24);
	SD_WriteBuffer(tmp32,4);
}

void SD_WriteString(char * string){
	f_puts (string, &Log_File);
    for (uint16_t i=0; i<strlen(string); i++){
        CRC = (CRC >> 8) ^ crc_table[string[i] ^ (CRC & 0xFF)];
    }
}

void SD_WriteBuffer(uint8_t * buffer, uint16_t length){
    uint16_t tmp;
    f_write (&Log_File, buffer, length, &tmp);

    for (uint16_t i=0; i<length; i++){
        CRC = (CRC >> 8) ^ crc_table[buffer[i] ^ (CRC & 0xFF)];
    }
}

void SD_ClearCRC(void){
	CRC = 0xFFFFFFFF;
}

void SD_WriteCRC(void){
	SD_Write32(CRC^0xFFFFFFFF);
}

void SD_MakeFileName(uint32_t var){
	fileName[0] = '/';
	ltoa(var, fileName+1, 16);
	strcat(fileName, ".bt");
}

void SD_Read_config_file(void){
	char temp [50];
	
	SD_Open("/config.txt");
    f_lseek(&Log_File, 0);
	
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
            } else if(strstr(temp,"user") != 0){
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
            } else if(strstr(temp,"demoMode") != 0){
                strtok(temp,"=");
                strcpy(demoModeString,strtok(NULL,"="));
            } else if(strstr(temp,"recordFast") != 0){
                strtok(temp,"=");
                strcpy(fastString,strtok(NULL,"="));
            } else if(strstr(temp,"zone") != 0){
				strtok(temp,"=");
                memmove(zone,strtok(NULL,"="),4);
                for(uint8_t i = 0;  i < strlen(zone); i++){
                    if(zone[i] < ' '){
                        zone[i] = 0;
                    }
                }
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
    if((strstr(demoModeString,"true") != 0)){
        demoMode = true;
    }
    if((strstr(fastString,"true") != 0)){
        wantToRecordFast = true;
    }
}


void SD_GetSpaceRemaining(void){
	uint32_t totalDiskSpace;
	uint32_t freeSpace;
	uint32_t percentUsed;
	
	f_getfree("0:",&freeSpace,&fs);
	totalDiskSpace = fs->max_clust;
	
	percentUsed = totalDiskSpace - freeSpace;
	percentUsed *= 1000;
	percentUsed /= totalDiskSpace;
	percentDiskUsed = percentUsed;
}




void SD_Timer_Init(void)			// Initialize 100 Hz timer needed for SD access 
{
	// fclk = 14745600
	// div  = 256
	// per  = 576
	// => 14745600/256/576 => 100 samples per second
	
	// Set period/TOP value
	SD_Timer.PER = 576;

	// Select clock source
	SD_Timer.CTRLA = (SD_Timer.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV256_gc;
	
	// Enable CCA interrupt
	SD_Timer.INTCTRLA = (SD_Timer.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;
	
}


// Called every 10 ms (100 Hz)
ISR(SD_Timer_vect)
{
	disk_timerproc();
	if(!sdValid){
		if((SD_CD_Port.IN & (1<<SD_CD)) == 0 ){
			SD_Init();
		}
	}
}



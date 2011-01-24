//___________________________________________
//				Joshua Schapiro
//					  sd.c
//			  created: 08/27/2010
//			  updated: 10/12/2010
//___________________________________________

#include "avr_compiler.h"
#include "sd.h"
#include "time.h"

#define MAGIC_NUMBER 0xb0de744c
#define RTYPE_START_OF_FILE 1
#define RTYPE_RTC 			2
#define RTYPE_PERIODIC_DATA 3

#define DeviceClass	"BaseStation"

#define StartFileLength 308

static FATFS file_system_object;
FIL Log_File;

char fileName [20];

uint8_t tmp8[1];
uint8_t tmp16[2];
uint8_t tmp32[4];
char tmpArray[20];

volatile uint32_t CRC;

uint8_t chargePercent    = 0;
uint16_t chargeCounter   = 0;
bool chargeComplete   	 = false;
bool okToCharge 		 = false;



void SD_Init(void){
	SD_CD_Port.SD_CD_CNTL = PORT_OPC_PULLUP_gc;
	SD_CD_Port.SD_CD2_CNTL = PORT_OPC_PULLUP_gc;
	SD_Timer_Init();
	disk_initialize(0);
	f_mount(0, &file_system_object);
}


uint8_t SD_Open(char string []){
	Debug_SendString("File Name= ",false);
	Debug_SendString(string,true);
	return f_open(&Log_File, string, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);	
}

void SD_Close(void){
	Debug_SendString("Closing File",true);
	f_close(&Log_File);
}

bool SD_Inserted(void){
	if((SD_CD_Port.IN & (1<<SD_CD)) > 0 ){
		return false;
	} else {
		return true;
	}
}

bool SD2_Inserted(void){
	if((SD_CD_Port.IN & (1<<SD_CD2)) > 0 ){
		return false;
	} else {
		return true;
	}
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
	
	for (uint8_t i=0; i<strlen(string); i++) 
      { 
      CRC = CRC ^ string[i] ; 
      for (uint8_t j=0; j<8; j++) 
         { 
            CRC = (CRC>>1) ^ (0xEDB88320 * (CRC & 1)) ; 
         } 
     } 
}

void SD_WriteBuffer(uint8_t * buffer, uint16_t length){
    uint16_t tmp;
    f_write (&Log_File, buffer, length, &tmp);

    for (uint16_t i=0; i<length; i++)
      { 
      CRC = CRC ^ buffer[i] ; 
      for (uint8_t j=0; j<8; j++) 
         { 
            CRC = (CRC>>1) ^ (0xEDB88320 * (CRC & 1)) ; 
         } 
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

uint8_t SD_StartLogFile(uint32_t time){
	uint8_t resp;

	Debug_SendString("Opening Log File",true);
		
	SD_MakeFileName(time);
	resp = SD_Open(fileName);
		
	SD_ClearCRC();
	SD_Write32(MAGIC_NUMBER);						// magic number 
	SD_Write32(StartFileLength+269);				// record size
	SD_Write16(RTYPE_START_OF_FILE); 		// record type  
	
														// payload
	SD_Write16(0x0100);				// protocol version
	SD_Write8(0x02);					// time protocol
	SD_Write32(Time_Get32BitTimer());					// time
	SD_Write32(542535);			// picoseconds per tick (48bit) (truly is 542534.722)
	SD_Write16(0);
		
	SD_WriteString("device_class");
	SD_Write8(0x09);
	SD_WriteString(DeviceClass);
	SD_Write8(0x0A);
		
	SD_WriteString("device_id");
	SD_Write8(0x09);
	SD_WriteString(macAddr);
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
	SD_WriteString("\"Humidity\":{\"units\": \"%RH\", \"scale\": 1},");
	SD_WriteString("\"Pressure\":{\"units\": \"kPa\", \"scale\": 1},");
	SD_WriteString("\"Light_Green\":{\"units\": \"bits\", \"scale\": 1},");				// 44
	SD_WriteString("\"Light_Red\":{\"units\": \"bits\", \"scale\": 1},");				// 42
	SD_WriteString("\"Light_Blue\":{\"units\": \"bits\", \"scale\": 1},");				// 43
	SD_WriteString("\"Light_Clear\":{\"units\": \"bits\", \"scale\": 1},");				// 44
	SD_WriteString("\"Air_Small\":{\"units\": \"#particles\", \"scale\": 1},");				// 48
	SD_WriteString("\"Air_Large\":{\"units\": \"#particles\", \"scale\": 1},");				// 48
	SD_WriteString("\"Microphone\":{\"units\": \"bits\", \"scale\": 1}}");
	SD_Write8(0x0A);
	
	SD_Write8(0x00);
		
	SD_WriteCRC();			// CRC			
		
	f_sync(&Log_File);

	return resp;
}

void SD_Timer_Init(void)			// Initialize 100 Hz timer needed for SD access 
{
	// fclk = 14745600
	// div  = 256
	// per  = 576
	// => 14745600/256/576 => 100 samples per second
	
	// Set period/TOP value
	TCE0.PER = 576;

	// Select clock source
	TCE0.CTRLA = (TCE0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV256_gc;
	
	// Enable CCA interrupt
	TCE0.INTCTRLA = (TCE0.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;
	
}


// Called every 10 ms (100 Hz)
ISR(TCE0_OVF_vect)
{
	disk_timerproc();


	if(okToCharge){
		chargeCounter++;
		if(chargeCounter >= 16200){
			chargeCounter=0;
			chargePercent++;
		}
	}
}



// From Application Note AVR1003
// Used to slow down clock in disk_initialize()
void CCPWrite( volatile uint8_t * address, uint8_t value ) {
    uint8_t volatile saved_sreg = SREG;
    cli();

#ifdef __ICCAVR__
	asm("movw r30, r16");
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm("ldi  r16,  0xD8 \n"
	    "out  0x34, r16  \n"
#if (__MEMORY_MODEL__ == 1)
	    "st     Z,  r17  \n");
#elif (__MEMORY_MODEL__ == 2)
	    "st     Z,  r18  \n");
#else /* (__MEMORY_MODEL__ == 3) || (__MEMORY_MODEL__ == 5) */
	    "st     Z,  r19  \n");
#endif /* __MEMORY_MODEL__ */

#elif defined __GNUC__
	volatile uint8_t * tmpAddr = address;
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm volatile(
		"movw r30,  %0"	      "\n\t"
		"ldi  r16,  %2"	      "\n\t"
		"out   %3, r16"	      "\n\t"
		"st     Z,  %1"       "\n\t"
		:
		: "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
		: "r16", "r30", "r31"
		);

#endif
	SREG = saved_sreg;
}

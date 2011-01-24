//___________________________________________
//				Joshua Schapiro
//					  sd.h
//			  created: 08/27/2010
//___________________________________________

// SPI pins for SD card
#define SD_PORT						PORTD // main
//#define SD_PORT						PORTE // acc
#define	SD_SPI						SPID  //main
//#define	SD_SPI						SPIE  // acc
#define SD_SPI_SS_bm        		PIN4_bm
#define SD_SPI_MOSI_bm      		PIN5_bm
#define SD_SPI_MISO_bm      		PIN6_bm
#define SD_SPI_SCK_bm       		PIN7_bm

#define SD_CD_Port					PORTF
#define SD_CD						0 // main
#define SD_CD2						1 // acc
#define SD_CD_CNTL					PIN0CTRL	// main
#define SD_CD2_CNTL					PIN1CTRL // acc


void SD_Init(void);
uint8_t SD_Open(char string []);
void SD_Close(void);
bool SD_Inserted(void);
bool SD2_Inserted(void);

void SD_Write8(uint8_t var);
void SD_Write16(uint16_t var);
void SD_Write32(uint32_t var);
void SD_WriteString(char * string);
void SD_WriteBuffer(uint8_t * buffer, uint16_t length);
void SD_ClearCRC(void);
void SD_WriteCRC(void);
void SD_MakeFileName(uint32_t var);
uint8_t SD_StartLogFile(uint32_t counter_32bits);



void SD_Timer_Init(void);
void CCPWrite(volatile uint8_t* address, uint8_t value);

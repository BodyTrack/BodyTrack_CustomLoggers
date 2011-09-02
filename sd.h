//___________________________________________
//				Joshua Schapiro
//					  sd.h
//			  created: 08/27/2010
//___________________________________________


uint8_t SD_Init(void);
uint8_t SD_Open(char string []);
void SD_Close(void);
bool SD_Inserted(void);

void SD_Write8(uint8_t var);
void SD_Write16(uint16_t var);
void SD_Write32(uint32_t var);
void SD_WriteString(char * string);
void SD_WriteBuffer(uint8_t * buffer, uint16_t length);
void SD_ClearCRC(void);
void SD_WriteCRC(void);
void SD_MakeFileName(uint32_t var);
void SD_Read_config_file(void);
void SD_GetSpaceRemaining(void);

void SD_Timer_Init(void);
void CCPWrite(volatile uint8_t* address, uint8_t value);

//___________________________________________
//				Joshua Schapiro
//					debug.h
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________


void Debug_Init(uint32_t baud);
void Debug_ClearBuffer(void);

bool Debug_CharReadyToRead(void);
uint8_t Debug_GetByte(bool blocking);

void Debug_SendByte(uint8_t data);
void Debug_SendString(char string [],bool CR);

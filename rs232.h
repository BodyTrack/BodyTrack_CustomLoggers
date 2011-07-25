//___________________________________________
//				Joshua Schapiro
//					rs232.h
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________


void Rs232_Init(void);

bool Rs232_CharReadyToRead(void);
uint8_t Rs232_GetByte(bool blocking);
void Rs232_ClearBuffer(void);

void Rs232_SendByte(uint8_t data);
void Rs232_SendString(char string [],bool CR);


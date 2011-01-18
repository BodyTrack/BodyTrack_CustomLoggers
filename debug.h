//___________________________________________
//				Joshua Schapiro
//					debug.h
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________

#define Debug_Port				PORTC
#define Debug_Usart				USARTC0
#define Debug_RX_pin_bm		PIN2_bm
#define Debug_TX_pin_bm		PIN3_bm


void Debug_Init(void);

bool Debug_CharReadyToRead(void);
uint8_t Debug_GetByte(bool blocking);

void Debug_SendByte(uint8_t data);
void Debug_SendString(char string [],bool CR);


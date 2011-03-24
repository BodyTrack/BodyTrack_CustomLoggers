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
#define Debug_Flow_Port          PORTD
#define Debug_RTS_pin            0
#define Debug_RTS_CNTL           PIN0CTRL


void Debug_Init(void);
void Debug_ClearBuffer(void);

bool Debug_CharReadyToRead(void);
uint8_t Debug_GetByte(bool blocking);

void Debug_SendByte(uint8_t data);
void Debug_SendString(char string [],bool CR);

uint32_t Debug_GetTime(uint16_t timeOut);
bool Debug_Connected(uint16_t timeOut);
bool Debug_TriggerUpload(uint32_t size, uint16_t timeOut);

//___________________________________________
//				Joshua Schapiro
//					wifi.h
//			  created: 08/27/2010
//			  updated: 09/21/2010
//
//___________________________________________

#define Wifi_Port			    PORTE
#define Wifi_Usart			  	USARTE0
#define Wifi_RX_pin_bm			PIN2_bm
#define Wifi_TX_pin_bm			PIN3_bm
#define Wifi_Connected_Port		PORTF
#define Wifi_Connected_pin		7
//#define Wifi_Connected_PinCTRL	PIN7CTRL


void Wifi_Init(uint32_t baud);
void Wifi_ClearBuffer(void);
bool Wifi_CharReadyToRead(void);
uint8_t Wifi_GetByte(bool blocking);

void Wifi_SendByte(uint8_t data);
void Wifi_SendString(char string [],bool CR);
bool Wifi_EnterCMDMode(uint16_t timeOut);
bool Wifi_ExitCMDMode(uint16_t timeOut);
bool Wifi_SendCommand(char toSend [], char ok [], char ok2 [], uint16_t timeOut);
bool Wifi_GetTime(uint16_t timeOut);
bool Wifi_Connected(uint16_t timeOut);
bool Wifi_GetMac(uint16_t timeOut);
uint8_t Wifi_GetSignalStrength(uint16_t timeOut);

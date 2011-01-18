//___________________________________________
//				Joshua Schapiro
//					depad.h
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________

#define Dpad_Port				PORTF

void Dpad_Init(void);
uint8_t Dpad_Read(void);
bool Dpad_CheckButton(uint8_t direction);


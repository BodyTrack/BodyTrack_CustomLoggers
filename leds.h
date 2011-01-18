//___________________________________________
//				Joshua Schapiro
//					leds.h
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________


#define Leds_Port				PORTB

void Leds_Init(void);
void Leds_Set(uint8_t led);
void Leds_Clear(uint8_t led);
void Leds_Toggle(uint8_t led);
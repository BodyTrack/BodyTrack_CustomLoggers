//___________________________________________
//				Joshua Schapiro
//					leds.c
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________

#include "avr_compiler.h"
#include "leds.h"

#define wifi_Green	0
#define wifi_Red 	1
#define sd_Green	4
#define sd_Red	 	5
#define ext_Green 	6
#define ext_Red		7



void Leds_Init(void){
	// make sure JTAGEN is not checked in the fuses!
	Leds_Port.DIRSET |= (1<<wifi_Green)|(1<<wifi_Red)|(1<<sd_Green)|(1<<sd_Red)|(1<<ext_Green)|(1<<ext_Red);
}

void Leds_Set(uint8_t led){
	Leds_Port.OUTSET = (1 << led);
}

void Leds_Clear(uint8_t led){
	Leds_Port.OUTCLR = (1 << led);
}

void Leds_Toggle(uint8_t led){
	Leds_Port.OUTTGL = (1 << led);
}

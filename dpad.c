//___________________________________________
//				Joshua Schapiro
//					dpad.c
//			  created: 08/27/2010
//			  updated:
//
//___________________________________________

#include "avr_compiler.h"
#include "dpad.h"

#define Up		2
#define Down 	3
#define Left 	4
#define Right 	5
#define Select 	6




void Dpad_Init(void){
	Dpad_Port.PIN2CTRL = PORT_OPC_PULLUP_gc;
	Dpad_Port.PIN3CTRL = PORT_OPC_PULLUP_gc;
	Dpad_Port.PIN4CTRL = PORT_OPC_PULLUP_gc;
	Dpad_Port.PIN5CTRL = PORT_OPC_PULLUP_gc;
	Dpad_Port.PIN6CTRL = PORT_OPC_PULLUP_gc;
	Dpad_Port.DIRCLR |= (1 << Up)|(1 << Down)|(1 << Left)|(1 << Right)|(1 << Select) ;
}

uint8_t Dpad_Read(void){
	return (Dpad_Port.IN & 0b01111100);
}

bool Dpad_CheckButton(uint8_t direction){
	if((Dpad_Read() & (1<<direction)) >0 ){
		return false;
	} else {
		return true;
	}
}



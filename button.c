//___________________________________________
//				Joshua Schapiro
//___________________________________________

#include "button.h"

void Button_Init(uint8_t button){
	Button_Port.DIRCLR = (1 << button);
	PORTCFG.MPCMASK = (1 << button);
	Button_Port.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
}


bool Button_Pressed(uint8_t button){
	if((Button_Port.IN & (1<<button)) >0 ){
		return false;
	} else {
		return true;
	}
} 



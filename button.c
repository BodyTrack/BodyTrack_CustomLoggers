//___________________________________________
//				Joshua Schapiro
//___________________________________________

#include "button.h"

void Button_Init(uint8_t button, bool enableInt, uint8_t edge, uint8_t intNumber,uint8_t intLevel){
	Button_Port.DIRCLR = (1 << button);
	
	PORTCFG.MPCMASK = (1 << button);
	Button_Port.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc | edge;
	if(enableInt){
		if(intNumber == 0){
			Button_Port.INT0MASK = (1<<button); 
			Button_Port.INTCTRL |= intLevel;
		} else if (intNumber == 1){
			Button_Port.INT1MASK = (1<<button); 
			Button_Port.INTCTRL |= (intLevel << 2);
		}
	}
}


bool Button_Pressed(uint8_t button){
	if((Button_Port.IN & (1<<button)) >0 ){
		return false;
	} else {
		return true;
	}
} 



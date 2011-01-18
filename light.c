//___________________________________________
//				Joshua Schapiro
//					light.c
//			  created: 09/07/2010
//			  updated:
//
//___________________________________________

#include "avr_compiler.h"
#include "light.h"


uint8_t	deviceAddress;
uint8_t writeAddress;
uint8_t commandCode;

uint16_t colors[4];
uint8_t  colors8[8];



void Light_Init(uint8_t address){
	deviceAddress = address;

	LightPort.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;

	//Set Master Read Acknowledge Action to ACK
	LightPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm;

	//Want 400 kHz I2C signal
	//14.7456Mhz/(2*400kHz) - 5 = 13
	LightPort.MASTER.BAUD = 13;

	LightPort.MASTER.STATUS =TWI_MASTER_BUSSTATE_IDLE_gc;

	gainSelector = 0;
	Light_setGain();
}


uint8_t Light_readByte(uint8_t location){
	uint8_t tmp;


	//Send start condition, and device address + write
	writeAddress = deviceAddress & ~0x01;
	commandCode = 0b10000000 | location;


	LightPort.MASTER.ADDR = writeAddress;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
				//Wait for write flag to turn on
	}

	LightPort.MASTER.DATA = commandCode;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
				//Wait for write flag to turn on
	}

	writeAddress = deviceAddress | 0x01;	// start with address & read
	LightPort.MASTER.ADDR = writeAddress;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_RIF_bm)) {
				//Wait for Read flag to come on
	}

	tmp = LightPort.MASTER.DATA;

	LightPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;	// stop

	return tmp;
}

void Light_readColors(void){

	while(Light_readByte(Control) && ADC_Valid_bm == 0);

	//Send start condition, and device address + write
	writeAddress = deviceAddress & ~0x01;
	commandCode = 0b10000000 | 0x10;

	LightPort.MASTER.ADDR = writeAddress;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
					//Wait for write flag to turn on
	}

	LightPort.MASTER.DATA = commandCode;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
					//Wait for write flag to turn on
	}

	writeAddress = deviceAddress | 0x01;	// start with address & read
	LightPort.MASTER.ADDR = writeAddress;


	for(uint8_t i = 0; i < 8; i++){
		while (!(LightPort.MASTER.STATUS & TWI_MASTER_RIF_bm)) {
				//Wait for Read flag to come on
		}

		colors8[i] = LightPort.MASTER.DATA;
		//If more bytes to read, send ACK and start byte read
		if(i < 7){
			LightPort.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
		}
	}

	LightPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;	// stop

	colors[green] = colors8[1]*256 + colors8[0];
	colors[red]   = colors8[3]*256 + colors8[2];
	colors[blue]  = colors8[5]*256 + colors8[4];
	colors[clear] = colors8[7]*256 + colors8[6];
}

uint16_t Light_returnColor(uint8_t color){
	return colors[color];
}

void Light_writeByte(uint8_t location, uint8_t toSend){
	//Send start condition, and device address + write
	writeAddress = deviceAddress & ~0x01;
	commandCode = 0b10000000 | location;

	LightPort.MASTER.ADDR = writeAddress;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
				//Wait for write flag to turn on
	}

	LightPort.MASTER.DATA = commandCode;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
				//Wait for write flag to turn on
	}

	LightPort.MASTER.DATA = toSend;

	while (!(LightPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
				//Wait for write flag to turn on
	}

	LightPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;	// stop

}


void Light_setGain(void){
	Light_writeByte(Control, Power_bm);
	Light_writeByte(Timing, (integrationTime[gainSelector] | Integ_Mode_Auto_bm));
	Light_writeByte(Gain, (gainSetting[gainSelector]));
	Light_writeByte(Control, (ADC_En_bm | Power_bm));
}


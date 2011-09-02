//___________________________________________
//				Joshua Schapiro
//					  accel.c
//			  created: 02/28/2011
//___________________________________________

#include "avr_compiler.h"
#include "accel.h"




void Accel_Init(void){

    Accel_PORT.DIRSET = Accel_MOSI_bm | Accel_SCK_bm | Accel_SS_bm;
    Accel_PORT.Accel_SS_CTRL = PORT_OPC_WIREDANDPULL_gc;
    Accel_deselectChip();

	  //Double clock, enable SPI, MSB sent first, set as master, mode 3, clk/16
  	Accel_SPI.CTRL |= SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_3_gc | SPI_PRESCALER_DIV4_gc;

    Accel_INT_Port.DIRCLR = (1 << Accel_INT1) | (1 << Accel_INT2);

    Accel_WriteToAddress(0x31,0x09);    // Data format:  4 wire spi, full resolution, right justified, 4 g's
    Accel_WriteToAddress(0x2C,0x0C);    // normal operation, rate code of 400
    Accel_WriteToAddress(0x2D,0x08);    // measure mode


    	// fclk = 14745600
    	// div  = 8
    	// per  = 5760 (remember to subtract 1)
    	//  => 320 samples per second

    	// Set period/TOP value
    Accel_Sample_Timer.PER = 5759;

    	// Select clock source
    Accel_Sample_Timer.CTRLA = (TCF0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;

    	// Enable CCA interrupt
    Accel_Sample_Timer.INTCTRLA = (TCF0.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;

	
	Sensors_ResetAccelBuffers();
}



void Accel_Write_Byte(uint8_t tmp){
	Accel_SPI.DATA = tmp;
	while(!(Accel_SPI.STATUS & SPI_IF_bm));
}

uint8_t Accel_Read_Byte(void){
	Accel_Write_Byte(0);
	return Accel_SPI.DATA;
}


void Accel_WriteToAddress(uint8_t addr, uint8_t byte){
    Accel_selectChip();
    Accel_Write_Byte(addr & 0b00111111);
    Accel_Write_Byte(byte);
    Accel_deselectChip();
}


uint8_t Accel_ReadFromAddress(uint8_t addr){
    uint8_t tmp;
    Accel_selectChip();
    Accel_Write_Byte(addr | 0b10000000);
    tmp = Accel_Read_Byte();
    Accel_deselectChip();
    return tmp;
}

void Accel_ReadResults(uint16_t  ** buffLocation, uint8_t bufferNumber, uint16_t bufferCounter){
    // 0 = x, 1 = y, 2 = z

    Accel_selectChip();
    Accel_Write_Byte(0x32 | 0b11000000);
    buffLocation[bufferNumber][bufferCounter] = Accel_Read_Byte();
    buffLocation[bufferNumber][bufferCounter] += Accel_Read_Byte() * 256;
    buffLocation[bufferNumber][bufferCounter] += 1024;
    buffLocation[bufferNumber][bufferCounter+1] = Accel_Read_Byte();
    buffLocation[bufferNumber][bufferCounter+1] += Accel_Read_Byte() * 256;
    buffLocation[bufferNumber][bufferCounter+1] += 1024;
    buffLocation[bufferNumber][bufferCounter+2] = Accel_Read_Byte();
    buffLocation[bufferNumber][bufferCounter+2] += Accel_Read_Byte() * 256;
    buffLocation[bufferNumber][bufferCounter+2] += 1024;
    Accel_deselectChip();
}

void Sensors_ResetAccelBuffers(void){
	accelBufferCounter = 0;
  	accelBufferToWriteTo = 0;
	for(uint8_t i = 0; i < accelNumberOfBuffers; i++){
		okToSendAccelBuffer[i] = false;
	}	
}


ISR(Accel_Sample_Timer_vect)
{
	if(recording && wantToRecordFast && !okToSendAccelBuffer[accelBufferToWriteTo]){ 
		if(accelBufferCounter == 0){
			accelSampleStartTime[accelBufferToWriteTo] = Time_Get32BitTimer();
		}
		
		
		//Accel_ReadResults(accelBuffer,accelBufferToWriteTo,accelBufferCounter);
		
		accelBufferCounter+=3;
		
		if(accelBufferCounter == accelNumberOfSamples*accelNumberOfChannels){
			accelBufferCounter=0;
			okToSendAccelBuffer[accelBufferToWriteTo] = true;
			accelBufferToWriteTo++;
			if(accelBufferToWriteTo == accelNumberOfBuffers){
				accelBufferToWriteTo = 0;
			}
		}
	}
}


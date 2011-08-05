/*
 *  sensorsCS.c
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

#include "sensorsCS.h"

volatile bool	okToSendRespirationBuffer1 = false;
volatile bool	okToSendRespirationBuffer2 = false;
uint8_t			respirationBufferToWriteTo = 1;
uint16_t		respirationBufferCounter = 0;

volatile bool	okToSendTemperatureBuffer1 = false;
volatile bool	okToSendTemperatureBuffer2 = false;
uint8_t			temperatureBufferToWriteTo = 1;
uint16_t		temperatureBufferCounter = 0;

volatile bool	okToSendEKGBuffer1 = false;
volatile bool	okToSendEKGBuffer2 = false;
uint8_t			EKGBufferToWriteTo = 1;
uint16_t		EKGBufferCounter = 0;

volatile bool	okToSendHumidityBuffer1 = false;
volatile bool	okToSendHumidityBuffer2 = false;
uint8_t			humidityBufferToWriteTo = 1;
uint16_t		humidityBufferCounter = 0;

volatile bool	okToSendRTCBlock = false;
uint8_t			rtcBlockCounter = 0;

uint16_t zeroOffsetA, zeroOffsetB;

uint16_t HZ_RefeshCounter = 0;

uint16_t ekgCounter = 0;


void Sensors_Init(void){
	
	
	ADCA.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
	ADCA.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );
	
	ADCB.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL0_offset );
	ADCB.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL1_offset );
	
    // Port A = ekg, temperature, humdity
	
    ADCA.EKGChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
    ADCA.EKGChannel.MUXCTRL = EKGMUXPos;
	
    ADCA.temperatureChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
    ADCA.temperatureChannel.MUXCTRL = temperatureMUXPos;
	
    ADCA.humidityChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
    ADCA.humidityChannel.MUXCTRL = humidityMUXPos;
	
	ADCA.groundChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
    ADCA.groundChannel.MUXCTRL = groundMUxPos;
	
    ADCA.PRESCALER = (ADCA.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV64_gc;
	
    ADCA.REFCTRL = ADC_REFSEL_AREFA_gc;
    ADCA.EVCTRL = (ADCA.EVCTRL & (~ADC_SWEEP_gm)) | ADC_SWEEP_0123_gc;
	
    ADCA.CTRLB |= ADC_FREERUN_bm; // free running mode
	
    ADCA.temperatureChannel.CTRL |= ADC_CH_START_bm;
    ADCA.humidityChannel.CTRL |= ADC_CH_START_bm;
    ADCA.EKGChannel.CTRL |= ADC_CH_START_bm;
	ADCA.groundChannel.CTRL |= ADC_CH_START_bm;
    ADCA.CTRLA = ADC_ENABLE_bm;							// enable adc w/o calibrating
	
	_delay_ms(10);
	zeroOffsetA = ADCA.groundResult;
	
	// Port B = Respiration
	
	ADCB.respirationChannel.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_8X_gc;	// set input mode
	ADCB.respirationChannel.MUXCTRL = respirationMUXPos | neg_resipirationMUXPos;
	
	ADCB.PRESCALER = (ADCB.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV64_gc;
	
	ADCB.REFCTRL = ADC_REFSEL_AREFA_gc;
	ADCB.EVCTRL = (ADCB.EVCTRL & (~ADC_SWEEP_gm)) | ADC_SWEEP_0123_gc;
	
	ADCB.CTRLB |= ADC_FREERUN_bm | ADC_CONMODE_bm; // free running mode & signed
	
	ADCB.respirationChannel.CTRL |= ADC_CH_START_bm;
	
	ADCB.CTRLA = ADC_ENABLE_bm;							// enable adc w/o calibrating
	
	_delay_ms(10);
	zeroOffsetB = ADCB.groundResult;
	
	respirationPort.DIRSET = 1<<respirationDriver;
	respirationPort.OUTSET = 1<<respirationDriver;
	
	
	// ekg/respiration @ 300hz
	// fclk = 14745600
	// div  = 64
	// per  = 768 (remember to subtract 1)
	// => 300 samples per second
	
	// Set period/TOP value
	Sensors_Timer_300HZ.PER = 767;
	
	// Select clock source
	Sensors_Timer_300HZ.CTRLA = (TCD1.CTRLA & ~TC0_CLKSEL_gm) |  TC_CLKSEL_DIV64_gc;
	
	// Enable CCA interrupt
	Sensors_Timer_300HZ.INTCTRLA = (TCD1.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;
	
	
}

uint16_t Sensors_ReadTemperature(void){
	return ADCA.temperatureResult;
}

uint16_t Sensors_ReadRespiration(void){
	return ADCB.respirationResult;
	//return Time_TimerLowCNT;

}

uint16_t Sensors_ReadEKG(void){
	//ekgCounter++;
	//return ekgCounter;
	return ADCA.EKGResult; 
}

uint16_t Sensors_ReadHumidity(void){
	uint32_t tmp =  ADCA.humidityResult - zeroOffsetA;
    uint16_t tmp2 = 0;
    uint32_t tmp3;
	
    tmp *= 19554;
    tmp -= 9756672;
    tmp += (2382*zeroOffsetA);
    tmp3 = tmp / (4096 - zeroOffsetA);       // %RH * 100
	tmp2 = tmp3/10;
	return tmp2;
}

void Sensors_ResetTemperatureBuffers(void){
	temperatureBufferCounter = 0;
  	temperatureBufferToWriteTo = 1;
	okToSendTemperatureBuffer1 = false;
	okToSendTemperatureBuffer2 = false;
}

void Sensors_ResetRespirationBuffers(void){
	respirationBufferCounter = 0;
  	respirationBufferToWriteTo = 1;
	okToSendRespirationBuffer1 = false;
	okToSendRespirationBuffer2 = false;
}

void Sensors_ResetEKGBuffers(void){
	EKGBufferCounter = 0;
  	EKGBufferToWriteTo = 1;
	okToSendEKGBuffer1 = false;
	okToSendEKGBuffer2 = false;
}

void Sensors_ResetHumidityBuffers(void){
	humidityBufferCounter = 0;
  	humidityBufferToWriteTo = 1;
	okToSendHumidityBuffer1 = false;
	okToSendHumidityBuffer2 = false;
}






ISR(Sensors_Timer_300HZ_vect)
{
	if(recording){
		if(wantToRecordRespiration){
			if((respirationBufferToWriteTo == 1) && !okToSendRespirationBuffer1){
				if(respirationBufferCounter == 0){
					respirationSampleStartTime1 = Time_Get32BitTimer();
				}
				respirationBuffer1[respirationBufferCounter] = Sensors_ReadRespiration();
				respirationBufferCounter++;
				if(respirationBufferCounter == respirationNumberOfSamples){
					respirationBufferCounter=0;
					respirationBufferToWriteTo = 2;
					okToSendRespirationBuffer1 = true;
				}
			} else if ((respirationBufferToWriteTo == 2) && !okToSendRespirationBuffer2){
				if(respirationBufferCounter == 0){
					respirationSampleStartTime2 = Time_Get32BitTimer();
				}
				respirationBuffer2[respirationBufferCounter] = Sensors_ReadRespiration();
				respirationBufferCounter++;
				if(respirationBufferCounter == respirationNumberOfSamples){
					respirationBufferCounter=0;
					respirationBufferToWriteTo = 1;
					okToSendRespirationBuffer2 = true;
				}
			}
		}
		
		if(wantToRecordEKG){
			if((EKGBufferToWriteTo == 1) && !okToSendEKGBuffer1){
				if(EKGBufferCounter == 0){
					EKGSampleStartTime1 = Time_Get32BitTimer();
				}
				EKGBuffer1[EKGBufferCounter] = Sensors_ReadEKG();
				EKGBufferCounter++;
				if(EKGBufferCounter == EKGNumberOfSamples){
					EKGBufferCounter=0;
					EKGBufferToWriteTo = 2;
					okToSendEKGBuffer1 = true;
				}
			} else if ((EKGBufferToWriteTo == 2) && !okToSendEKGBuffer1){
				if(EKGBufferCounter == 0){
					EKGSampleStartTime2 = Time_Get32BitTimer();
				}
				EKGBuffer2[EKGBufferCounter] = Sensors_ReadEKG();
				EKGBufferCounter++;
				if(EKGBufferCounter == EKGNumberOfSamples){
					EKGBufferCounter=0;
					EKGBufferToWriteTo = 1;
					okToSendEKGBuffer2 = true;
				}
			}
		}
		HZ_RefeshCounter++;
		if(HZ_RefeshCounter == 300){
			HZ_RefeshCounter = 0;
			
			UNIX_Time++;
			
			rtcBlockCounter++;
			if(rtcBlockCounter == 0){
				okToSendRTCBlock = true;
			}
			if(wantToRecordTemperature){
				if(temperatureBufferToWriteTo == 1){
					if(temperatureBufferCounter == 0){
						temperatureSampleStartTime1 = Time_Get32BitTimer();
					}
					temperatureBuffer1[temperatureBufferCounter] = Sensors_ReadTemperature();
					temperatureBufferCounter++;
					if(temperatureBufferCounter == temperatureNumberOfSamples){
						temperatureBufferCounter=0;
						temperatureBufferToWriteTo = 2;
						okToSendTemperatureBuffer1 = true;
					}
				} else if (temperatureBufferToWriteTo == 2){
					if(temperatureBufferCounter == 0){
						temperatureSampleStartTime2 = Time_Get32BitTimer();
					}
					temperatureBuffer2[temperatureBufferCounter] = Sensors_ReadTemperature();
					temperatureBufferCounter++;
					if(temperatureBufferCounter == temperatureNumberOfSamples){
						temperatureBufferCounter=0;
						temperatureBufferToWriteTo = 1;
						okToSendTemperatureBuffer2 = true;
					}
				}
			}
			
			if(wantToRecordHumidity){
				if(humidityBufferToWriteTo == 1){
					if(humidityBufferCounter == 0){
						humiditySampleStartTime1 = Time_Get32BitTimer();
					}
					humidityBuffer1[humidityBufferCounter] = Sensors_ReadHumidity();
					humidityBufferCounter++;
					if(humidityBufferCounter == humidityNumberOfSamples){
						humidityBufferCounter=0;
						humidityBufferToWriteTo = 2;
						okToSendHumidityBuffer1 = true;
					}
				} else if (humidityBufferToWriteTo == 2){
					if(humidityBufferCounter == 0){
						humiditySampleStartTime2 = Time_Get32BitTimer();
					}
					humidityBuffer2[humidityBufferCounter] = Sensors_ReadHumidity();
					humidityBufferCounter++;
					if(humidityBufferCounter == humidityNumberOfSamples){
						humidityBufferCounter=0;
						humidityBufferToWriteTo = 1;
						okToSendHumidityBuffer2 = true;
					}
				}	  
			}  
		}
	}
	
}

uint8_t SP_ReadCalibrationByte( uint8_t index )
{
	uint8_t result;
	
	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
 	result = pgm_read_byte(index);
	
	/* Clean up NVM Command register. */
 	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	
	return result;
}
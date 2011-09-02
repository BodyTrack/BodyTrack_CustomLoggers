/*
 *  sensorsCS.c
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

#include "sensorsCS.h"




uint16_t zeroOffsetA, zeroOffsetB;

volatile uint16_t HZ_RefeshCounter = 0;
volatile uint16_t HZ_RefeshCounter2 = 0;

volatile uint16_t ekgCounter = 0;


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
	
	
	// ekg @ 300hz
	// fclk = 14745600
	// div  = 64
	// per  = 768 (remember to subtract 1)
	// => 300 samples per second
	
	// Set period/TOP value
	Sensors_Timer_300HZ.PER = 767;
	//Sensors_Timer_300HZ.PER = 2303;		// 100 hz
	
	// Select clock source
	Sensors_Timer_300HZ.CTRLA = (TCD1.CTRLA & ~TC0_CLKSEL_gm) |  TC_CLKSEL_DIV64_gc;
	
	// Enable CCA interrupt
	Sensors_Timer_300HZ.INTCTRLA = (TCD1.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;
	
	Sensors_ResetTemperatureBuffers();
	Sensors_ResetRespirationBuffers();
	Sensors_ResetEKGBuffers();
	Sensors_ResetHumidityBuffers();
}

uint16_t Sensors_ReadTemperature(void){
	return ADCA.temperatureResult;
}

uint16_t Sensors_ReadRespiration(void){
	return ADCB.respirationResult;
}

uint16_t Sensors_ReadEKG(void){
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
  	temperatureBufferToWriteTo = 0;
	for(uint16_t i = 0; i < temperatureNumberOfBuffers; i++){
		okToSendTemperatureBuffer[i] = false;
	}	
}

void Sensors_ResetRespirationBuffers(void){
	respirationBufferCounter = 0;
  	respirationBufferToWriteTo = 0;
	for(uint16_t i = 0; i < respirationNumberOfBuffers; i++){
		okToSendRespirationBuffer[i] = false;
	}	
}

void Sensors_ResetEKGBuffers(void){
	EKGBufferCounter = 0;
  	EKGBufferToWriteTo = 0;
	for(uint16_t i = 0; i < EKGNumberOfBuffers; i++){
		okToSendEKGBuffer[i] = false;
	}	
}

void Sensors_ResetHumidityBuffers(void){
	humidityBufferCounter = 0;
  	humidityBufferToWriteTo = 0;
	for(uint16_t i = 0; i < humidityNumberOfBuffers; i++){
		okToSendHumidityBuffer[i] = false;
	}	
}






ISR(Sensors_Timer_300HZ_vect)
{
	if(recording){
		if(wantToRecordEKG && !okToSendEKGBuffer[EKGBufferToWriteTo]){ 
			if(EKGBufferCounter == 0){
				EKGSampleStartTime[EKGBufferToWriteTo] = Time_Get32BitTimer();
			}
			EKGBuffer[EKGBufferToWriteTo][EKGBufferCounter] = Sensors_ReadEKG();
			EKGBufferCounter++;
			if(EKGBufferCounter == EKGNumberOfSamples){
				EKGBufferCounter=0;
				okToSendEKGBuffer[EKGBufferToWriteTo] = true;
				EKGBufferToWriteTo++;
				if(EKGBufferToWriteTo == EKGNumberOfBuffers){
					EKGBufferToWriteTo = 0;
				}
			}
		}
		
		HZ_RefeshCounter2++;
		if(HZ_RefeshCounter2 == 6){
			HZ_RefeshCounter2 = 0;
			if(wantToRecordRespiration && !okToSendRespirationBuffer[respirationBufferToWriteTo]){ 
				if(respirationBufferCounter == 0){
					respirationSampleStartTime[respirationBufferToWriteTo] = Time_Get32BitTimer();
				}
				respirationBuffer[respirationBufferToWriteTo][respirationBufferCounter] = Sensors_ReadRespiration();
				respirationBufferCounter++;
				if(respirationBufferCounter == respirationNumberOfSamples){
					respirationBufferCounter=0;
					okToSendRespirationBuffer[respirationBufferToWriteTo] = true;
					respirationBufferToWriteTo++;
					if(respirationBufferToWriteTo == respirationNumberOfBuffers){
						respirationBufferToWriteTo = 0;
					}
				}
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
		
		if(recording){
			if(wantToRecordTemperature && !okToSendTemperatureBuffer[temperatureBufferToWriteTo]){ 
				if(temperatureBufferCounter == 0){
					temperatureSampleStartTime[temperatureBufferToWriteTo] = Time_Get32BitTimer();
				}
				temperatureBuffer[temperatureBufferToWriteTo][temperatureBufferCounter] = Sensors_ReadTemperature();
				temperatureBufferCounter++;
				if(temperatureBufferCounter == temperatureNumberOfSamples){
					temperatureBufferCounter=0;
					okToSendTemperatureBuffer[temperatureBufferToWriteTo] = true;
					temperatureBufferToWriteTo++;
					if(temperatureBufferToWriteTo == temperatureNumberOfBuffers){
						temperatureBufferToWriteTo = 0;
					}
				}
			}
			
			
			if(wantToRecordHumidity && !okToSendHumidityBuffer[humidityBufferToWriteTo]){ 
				if(humidityBufferCounter == 0){
					humiditySampleStartTime[humidityBufferToWriteTo] = Time_Get32BitTimer();
				}
				humidityBuffer[humidityBufferToWriteTo][humidityBufferCounter] = Sensors_ReadHumidity();
				humidityBufferCounter++;
				if(humidityBufferCounter == humidityNumberOfSamples){
					humidityBufferCounter=0;
					okToSendHumidityBuffer[humidityBufferToWriteTo] = true;
					humidityBufferToWriteTo++;
					if(humidityBufferToWriteTo == humidityNumberOfBuffers){
						humidityBufferToWriteTo = 0;
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
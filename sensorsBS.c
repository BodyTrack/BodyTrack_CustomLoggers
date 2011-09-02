//___________________________________________
//				Joshua Schapiro
//				   sensors.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________

#include "sensorsBS.h"


volatile uint8_t	quickTemperature = 0;
volatile uint8_t	quickHumidity = 0;
volatile uint8_t	quickPressure = 0;
volatile uint16_t	quickLight = 0;
volatile uint8_t	quickMic = 0;
volatile uint32_t	quickLarge = 0;
volatile uint32_t	quickSmall = 0;
volatile uint8_t	micSampleCounter = 0;

uint16_t zeroOffsetA, zeroOffsetB;

uint32_t tempLightResult[4];

volatile uint8_t micCounter = 0;

volatile uint8_t lastBufferIWroteTo = 0;



//char				tempDebug [50];      


void Sensors_Init(void){
	
	// ADCA
	
	ADCA.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
	ADCA.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );
	
	ADCA.groundChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCA.groundChannel.MUXCTRL = groundMUXPos;

	ADCA.powerChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCA.powerChannel.MUXCTRL = powerMUXPos;
	
	ADCA.temperatureChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCA.temperatureChannel.MUXCTRL = temperatureMUXPos;

	ADCA.humidityChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCA.humidityChannel.MUXCTRL = humidityMUXPos;
	
	ADCA.PRESCALER = (ADCA.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV64_gc;
	
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc;
	ADCA.EVCTRL = (ADCA.EVCTRL & (~ADC_SWEEP_gm)) | ADC_SWEEP_0123_gc;
	
	ADCA.CTRLB |= ADC_FREERUN_bm; // free running mode	
	
	
	ADCA.groundChannel.CTRL |= ADC_CH_START_bm;
	ADCA.powerChannel.CTRL |= ADC_CH_START_bm;
	ADCA.temperatureChannel.CTRL |= ADC_CH_START_bm;
	ADCA.humidityChannel.CTRL |= ADC_CH_START_bm;
	
	ADCA.CTRLA = ADC_ENABLE_bm;							// enable adc 
	
	_delay_ms(10);
	zeroOffsetA = ADCA.groundResult;
	
	// ADCB
	
	
	ADCB.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL0_offset );
	ADCB.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCBCAL1_offset );
	
	ADCB.groundChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCB.groundChannel.MUXCTRL = groundMUXPos;
	
	ADCB.pressureChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCB.pressureChannel.MUXCTRL = pressureMUXPos;
	
	ADCB.microphoneChannel.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// set input mode
	ADCB.microphoneChannel.MUXCTRL = microphoneMUXPos;

	ADCB.PRESCALER = (ADCB.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV64_gc;
	
	ADCB.REFCTRL = ADC_REFSEL_AREFA_gc;
	ADCB.EVCTRL = (ADCB.EVCTRL & (~ADC_SWEEP_gm)) | ADC_SWEEP_0123_gc;
	
	ADCB.CTRLB |= ADC_FREERUN_bm; // free running mode	
	
	ADCB.groundChannel.CTRL |= ADC_CH_START_bm;
	ADCB.microphoneChannel.CTRL |= ADC_CH_START_bm;
	ADCB.pressureChannel.CTRL |= ADC_CH_START_bm;

	ADCB.CTRLA = ADC_ENABLE_bm;							// enable adc 
	
	_delay_ms(10);
	zeroOffsetB = ADCB.groundResult;
	
	// general sensors @ 1hz
	// fclk = 14745600
	// div  = 1024
	// per  = 14400 (remember to subtract 1, so really 14399)
	// => 14745600/14400/1024 => 1 samples per second
	
	// Set period/TOP value
	Sensors_Timer_1HZ.PER = 14399;

	// Select clock source
	Sensors_Timer_1HZ.CTRLA = (Sensors_Timer_1HZ.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;
	
	// Enable CCA interrupt
	Sensors_Timer_1HZ.INTCTRLA = (Sensors_Timer_1HZ.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;
	
	// microphone @ 7.2khz
	// fclk = 14745600
	// div  = 8
	// per  = 256 (remember to subtract 1, so really 255)
	// => 14745600/8/256 => 7200 samples per second

	// Set period/TOP value

	Sensors_Timer_7200HZ.PER = 255; 				// 7.2khz

	// Select clock source
	Sensors_Timer_7200HZ.CTRLA = (Sensors_Timer_7200HZ.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;

	// Enable CCA interrupt
	Sensors_Timer_7200HZ.INTCTRLA = (Sensors_Timer_7200HZ.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;
	
	Sensors_ResetTemperatureBuffers();
	Sensors_ResetPressureBuffers();
	Sensors_ResetHumidityBuffers();
	Sensors_ResetLightBuffers();
	Sensors_ResetMicrophoneBuffers();

}


uint16_t Sensors_ReadPower(void){
	return ADCA.powerResult;
}


uint16_t Sensors_ReadTemperature(void){
	uint32_t tmp =  ADCA.temperatureResult - zeroOffsetA ;
	uint16_t tmp2 = 0;

	tmp *= 11784;
	tmp -= 3657728;
	tmp += (893*zeroOffsetA);
	tmp /= 10;
	tmp2 = tmp / (4096 - zeroOffsetA);

	return tmp2;
}

uint16_t Sensors_ReadHumidity(void){
    uint32_t tmp =  ADCA.humidityResult - zeroOffsetA;
    uint16_t tmp2 = 0;
    uint32_t tmp3, tmp4;

    tmp *= 19554;
    tmp -= 9756672;
    tmp += (2382*zeroOffsetA);
    tmp3 = tmp / (4096 - zeroOffsetA);       // %RH * 100
    tmp3 *= 10000;                          // %RH * 1000,000

    tmp4 = 216*quickTemperature;                                      // use temp to offset
    tmp4 = 105460 - tmp4;
    tmp2 = tmp3 / tmp4;

    return tmp2;
}

uint8_t Sensors_ReadMicrophone(void){
	return micCounter++;
	//return ADCB.microphoneResult/16;
}

uint16_t Sensors_ReadPressure(void){
	uint32_t tmp =  ADCB.pressureResult - zeroOffsetB;
    uint16_t tmp2 = 0;

    tmp *= 13889;
    tmp += 4325376;
    tmp -= (1056*zeroOffsetB);
    tmp /= 10;
    tmp2 = tmp / (4096 - zeroOffsetB);

    return tmp2;
}

void Sensors_ResetTemperatureBuffers(void){
	temperatureBufferCounter = 0;
  	temperatureBufferToWriteTo = 0;
	for(uint8_t i = 0; i < temperatureNumberOfBuffers; i++){
		okToSendTemperatureBuffer[i] = false;
	}	
}

void Sensors_ResetPressureBuffers(void){
	pressureBufferCounter = 0;
  	pressureBufferToWriteTo = 0;
	for(uint16_t i = 0; i < pressureNumberOfBuffers; i++){
		okToSendPressureBuffer[i] = false;
	}	
}

void Sensors_ResetHumidityBuffers(void){
	humidityBufferCounter = 0;
  	humidityBufferToWriteTo = 0;
	for(uint16_t i = 0; i < humidityNumberOfBuffers; i++){
		okToSendHumidityBuffer[i] = false;
	}	
}

void Sensors_ResetMicrophoneBuffers(void){
	microphoneBufferCounter = 0;
  	microphoneBufferToWriteTo = 0;
	for(uint16_t i = 0; i < microphoneNumberOfBuffers; i++){
		okToSendMicrophoneBuffer[i] = false;
	}	
}

void Sensors_ResetLightBuffers(void){
	lightBufferCounter = 0;
  	lightBufferToWriteTo = 0;
	for(uint16_t i = 0; i < lightNumberOfBuffers; i++){
		okToSendLightBuffer[i] = false;
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


ISR(Sensors_Timer_1HZ_vect)
{
	UNIX_Time++;
	
	if(recording){
		rtcBlockCounter++;
		if(rtcBlockCounter == 0){
			okToSendRTCBlock = true;
		}
		
		if(wantToRecordTemperature && !okToSendTemperatureBuffer[temperatureBufferToWriteTo]){ 
			if(temperatureBufferCounter == 0){
				temperatureSampleStartTime[temperatureBufferToWriteTo] = Time_Get32BitTimer();
			}
			temperatureBuffer[temperatureBufferToWriteTo][temperatureBufferCounter] = Sensors_ReadTemperature();
			quickTemperature = temperatureBuffer[temperatureBufferToWriteTo][temperatureBufferCounter]/10;
			temperatureBufferCounter++;
			if(temperatureBufferCounter == temperatureNumberOfSamples){
				temperatureBufferCounter=0;
				okToSendTemperatureBuffer[temperatureBufferToWriteTo] = true;
				temperatureBufferToWriteTo++;
				if(temperatureBufferToWriteTo == temperatureNumberOfBuffers){
					temperatureBufferToWriteTo = 0;
				}
			}
		} else {
			quickTemperature = Sensors_ReadTemperature()/10;
		}
		
		
		if(wantToRecordHumidity && !okToSendHumidityBuffer[humidityBufferToWriteTo]){ 
			if(humidityBufferCounter == 0){
				humiditySampleStartTime[humidityBufferToWriteTo] = Time_Get32BitTimer();
			}
			humidityBuffer[humidityBufferToWriteTo][humidityBufferCounter] = Sensors_ReadHumidity();
			quickHumidity = humidityBuffer[humidityBufferToWriteTo][humidityBufferCounter]/10;
			humidityBufferCounter++;
			if(humidityBufferCounter == humidityNumberOfSamples){
				humidityBufferCounter=0;
				okToSendHumidityBuffer[humidityBufferToWriteTo] = true;
				humidityBufferToWriteTo++;
				if(humidityBufferToWriteTo == humidityNumberOfBuffers){
					humidityBufferToWriteTo = 0;
				}
			}
		} else {
			quickHumidity = Sensors_ReadHumidity()/10;
		}
	  
		if(wantToRecordPressure && !okToSendPressureBuffer[pressureBufferToWriteTo]){ 
			if(pressureBufferCounter == 0){
				pressureSampleStartTime[pressureBufferToWriteTo] = Time_Get32BitTimer();
			}
			pressureBuffer[pressureBufferToWriteTo][pressureBufferCounter] = Sensors_ReadPressure();
			
			quickPressure = pressureBuffer[pressureBufferToWriteTo][pressureBufferCounter]/10;
			pressureBufferCounter++;
			if(pressureBufferCounter == pressureNumberOfSamples){
				pressureBufferCounter=0;
				okToSendPressureBuffer[pressureBufferToWriteTo] = true;
				pressureBufferToWriteTo++;
				if(pressureBufferToWriteTo == pressureNumberOfBuffers){
					pressureBufferToWriteTo = 0;
				}
			}
		} else {
			quickPressure = Sensors_ReadPressure()/10;
		}
		

		if(wantToRecordLight&& !okToSendLightBuffer[lightBufferToWriteTo]){

			Light_readColors();
			Light_readColors();
			Light_readColors();

			quickLight = Light_returnColor(clear);

			for(uint8_t j = 0; j < 4; j++){
				tempLightResult[j] = 0;
				for(uint32_t i = 0; i < 76800; i+=resultMultiplier[gainSelector]){
					tempLightResult[j] += Light_returnColor(j);
				}
			}
			if(lightBufferCounter == 0){
				lightSampleStartTime[lightBufferToWriteTo] = Time_Get32BitTimer();
			}
			for(uint8_t i = 0; i < lightNumberOfChannels; i++){
				
				lightBuffer[lightBufferToWriteTo][lightBufferCounter] = tempLightResult[i];
				lightBufferCounter++; 
				  
			}
			if(lightBufferCounter == (lightNumberOfSamples*lightNumberOfChannels)){
				lightBufferCounter=0;
				okToSendLightBuffer[lightBufferToWriteTo] = true;
				lightBufferToWriteTo++;
				if(lightBufferToWriteTo == lightNumberOfBuffers){
					lightBufferToWriteTo = 0;
				}
			}
		
			if((Light_returnColor(clear) > 49000) && (gainSelector!= 0)){			// saturated -> decrease gain
				gainSelector--;
				Light_setGain();
			} else if ((Light_returnColor(clear) < 16000) && (gainSelector!= 8)){			// too low  -> increase gain
				gainSelector++;
				Light_setGain();
			}

		} else {
			Light_readColors();
			quickLight = Light_returnColor(clear);
		}

	} else {
		quickTemperature = Sensors_ReadTemperature()/10;
		quickHumidity = Sensors_ReadHumidity()/10;
		quickPressure = Sensors_ReadPressure()/10;
		Light_readColors();
		quickLight = Light_returnColor(clear);

	}




}

ISR(Sensors_Timer_7200HZ_vect)
{
	
	/*if(okToSendMicrophoneBuffer[microphoneBufferToWriteTo]){
		Debug_SendByte(microphoneBufferToWriteTo + '0');
	}*/
	
	
	if(recording && wantToRecordFast && !okToSendMicrophoneBuffer[microphoneBufferToWriteTo]){
		if(microphoneBufferCounter == 0){
			microphoneSampleStartTime[microphoneBufferToWriteTo] = Time_Get32BitTimer();
			/*if(lastBufferIWroteTo == microphoneBufferToWriteTo){
				sprintf(tempDebug, "Possible Corruption At: %lu", UNIX_Time);		
				Debug_SendString(tempDebug, true);
			}*/
			lastBufferIWroteTo = microphoneBufferToWriteTo;
			
		}
		microphoneBuffer[microphoneBufferToWriteTo][microphoneBufferCounter] = Sensors_ReadMicrophone();
		
		micSampleCounter++;
		if(micSampleCounter > 10){
			micSampleCounter = 0;
			quickMic = microphoneBuffer[microphoneBufferToWriteTo][microphoneBufferCounter];
		}
		
		
		
		microphoneBufferCounter++;
		if(microphoneBufferCounter == microphoneNumberOfSamples){
			microphoneBufferCounter=0;
			okToSendMicrophoneBuffer[microphoneBufferToWriteTo] = true;
			microphoneBufferToWriteTo++;
			if(microphoneBufferToWriteTo == microphoneNumberOfBuffers){
				microphoneBufferToWriteTo = 0;
			}
		}

	} else {
		if(micSampleCounter == 0){
			quickMic = Sensors_ReadMicrophone();
		}
	}
	micSampleCounter++;
}

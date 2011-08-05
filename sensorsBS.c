//___________________________________________
//				Joshua Schapiro
//				   sensors.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________

#include "sensorsBS.h"


volatile bool	okToSendTemperatureBuffer1 = false;
volatile bool	okToSendTemperatureBuffer2 = false;
uint8_t			temperatureBufferToWriteTo = 1;
uint16_t		temperatureBufferCounter = 0;

volatile bool	okToSendPressureBuffer1 = false;
volatile bool	okToSendPressureBuffer2 = false;
uint8_t			pressureBufferToWriteTo = 1;
uint16_t		pressureBufferCounter = 0;

volatile bool	okToSendHumidityBuffer1 = false;
volatile bool	okToSendHumidityBuffer2 = false;
uint8_t			humidityBufferToWriteTo = 1;
uint16_t		humidityBufferCounter = 0;

volatile bool	okToSendMicrophoneBuffer1 = false;
volatile bool	okToSendMicrophoneBuffer2 = false;
uint8_t			microphoneBufferToWriteTo = 1;
uint16_t		microphoneBufferCounter = 0;

volatile bool	okToSendLightBuffer1 = false;
volatile bool	okToSendLightBuffer2 = false;
uint8_t			lightBufferToWriteTo = 1;
uint16_t		lightBufferCounter = 0;

volatile bool	okToSendRTCBlock = false;
uint8_t			rtcBlockCounter = 0;



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
	// per  = 14400 (remember to subtract 1)
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
	// per  = 256 (remember to subtract 1)
	// => 14745600/8/256 => 7200 samples per second

	// Set period/TOP value

	Sensors_Timer_7200HZ.PER = 255; 				// 7.2khz

	// Select clock source
	Sensors_Timer_7200HZ.CTRLA = (Sensors_Timer_7200HZ.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;

	// Enable CCA interrupt
	Sensors_Timer_7200HZ.INTCTRLA = (Sensors_Timer_7200HZ.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;

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
	return ADCB.microphoneResult/16;
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
  	temperatureBufferToWriteTo = 1;
	okToSendTemperatureBuffer1 = false;
	okToSendTemperatureBuffer2 = false;
}

void Sensors_ResetPressureBuffers(void){
	pressureBufferCounter = 0;
	pressureBufferToWriteTo = 1;
	okToSendPressureBuffer1 = false;
	okToSendPressureBuffer2 = false;
}

void Sensors_ResetHumidityBuffers(void){
	humidityBufferCounter = 0;
	humidityBufferToWriteTo = 1;
	okToSendHumidityBuffer1 = false;
	okToSendHumidityBuffer2 = false;
}

void Sensors_ResetMicrophoneBuffers(void){
	microphoneBufferCounter = 0;
	microphoneBufferToWriteTo = 1;
	okToSendMicrophoneBuffer1 = false;
	okToSendMicrophoneBuffer2 = false;
}

void Sensors_ResetLightBuffers(void){
	lightBufferCounter = 0;
	lightBufferToWriteTo = 1;
	okToSendLightBuffer1 = false;
	okToSendLightBuffer2 = false;
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

	  if(wantToRecordTemperature){
		  if(temperatureBufferToWriteTo == 1){
			  if(temperatureBufferCounter == 0){
				  temperatureSampleStartTime1 = Time_Get32BitTimer();
			  }
			  temperatureBuffer1[temperatureBufferCounter] = Sensors_ReadTemperature();
			  quickTemperature = temperatureBuffer1[temperatureBufferCounter]/10;
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
			  quickTemperature = temperatureBuffer2[temperatureBufferCounter]/10;
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
			  quickHumidity = humidityBuffer1[humidityBufferCounter]/10;
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
			  quickHumidity = humidityBuffer2[humidityBufferCounter]/10;
			  humidityBufferCounter++;
			  if(humidityBufferCounter == humidityNumberOfSamples){
				  humidityBufferCounter=0;
				  humidityBufferToWriteTo = 1;
				  okToSendHumidityBuffer2 = true;
			  }
	  	  }
	  }

	  if(wantToRecordPressure){
		  if(pressureBufferToWriteTo == 1){
			  if(pressureBufferCounter == 0){
				  pressureSampleStartTime1 = Time_Get32BitTimer();
			  }
			  pressureBuffer1[pressureBufferCounter] = Sensors_ReadPressure();
			  quickPressure = pressureBuffer1[pressureBufferCounter]/10;

			  pressureBufferCounter++;
			  if(pressureBufferCounter == pressureNumberOfSamples){
				  pressureBufferCounter=0;
				  pressureBufferToWriteTo = 2;
				  okToSendPressureBuffer1 = true;
			  }
		  } else if (pressureBufferToWriteTo == 2){
			  if(pressureBufferCounter == 0){
				  pressureSampleStartTime2 = Time_Get32BitTimer();
			  }
			  pressureBuffer2[pressureBufferCounter] = Sensors_ReadPressure();
			  quickPressure = pressureBuffer2[pressureBufferCounter]/10;
			  pressureBufferCounter++;
			if(pressureBufferCounter == pressureNumberOfSamples){
				pressureBufferCounter=0;
				pressureBufferToWriteTo = 1;
				okToSendPressureBuffer2 = true;
			}
		  }
	  }

	  if(wantToRecordLight){

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

		  if(lightBufferToWriteTo == 1){
			  if(lightBufferCounter == 0){
	  			  lightSampleStartTime1 = Time_Get32BitTimer();
			  }
			  lightBuffer1[lightBufferCounter] = tempLightResult[0];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = tempLightResult[1];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = tempLightResult[2];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = tempLightResult[3];
			  lightBufferCounter++;
			  if(lightBufferCounter == (lightNumberOfSamples*lightNumberOfChannels)){
				  lightBufferCounter=0;
				  lightBufferToWriteTo = 2;
				  okToSendLightBuffer1 = true;
			  }
		  } else if (lightBufferToWriteTo == 2){
			  if(lightBufferCounter == 0){
				  lightSampleStartTime2 = Time_Get32BitTimer();
			  }
			  lightBuffer2[lightBufferCounter] = tempLightResult[0];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = tempLightResult[1];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = tempLightResult[2];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = tempLightResult[3];
			  lightBufferCounter++;
			  if(lightBufferCounter == (lightNumberOfSamples*lightNumberOfChannels)){
				  lightBufferCounter=0;
				  lightBufferToWriteTo = 1;
				  okToSendLightBuffer2 = true;
			  }
		  }


		  if((Light_returnColor(clear) > 49000) && (gainSelector!= 0)){			// saturated -> decrease gain
			  gainSelector--;
			  Light_setGain();
		  } else if ((Light_returnColor(clear) < 16000) && (gainSelector!= 8)){			// too low  -> increase gain
			  gainSelector++;
			  Light_setGain();
		  }


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
	if(recording && wantToRecordFast){
	  if((microphoneBufferToWriteTo == 1) && !okToSendMicrophoneBuffer1){
	    if(microphoneBufferCounter == 0){
	    	microphoneSampleStartTime1 = Time_Get32BitTimer();
	    }
	    microphoneBuffer1[microphoneBufferCounter] = Sensors_ReadMicrophone();
	    if(micSampleCounter == 0){
	    	quickMic = microphoneBuffer1[microphoneBufferCounter];
	    }
	    microphoneBufferCounter++;
	    if(microphoneBufferCounter == microphoneNumberOfSamples){
	    	quickMic = microphoneBuffer1[0];
	    	microphoneBufferCounter=0;
	    	microphoneBufferToWriteTo = 2;
	    	okToSendMicrophoneBuffer1 = true;
	    }
	  } else if ((microphoneBufferToWriteTo == 2)&& !okToSendMicrophoneBuffer2){
	    if(microphoneBufferCounter == 0){
	    	microphoneSampleStartTime2 = Time_Get32BitTimer();
	    }
	    microphoneBuffer2[microphoneBufferCounter] = Sensors_ReadMicrophone();
	    if(micSampleCounter == 0){
	    	quickMic = microphoneBuffer2[microphoneBufferCounter];
	    }

	    microphoneBufferCounter++;
	    if(microphoneBufferCounter == microphoneNumberOfSamples){
	    	quickMic = microphoneBuffer2[0];
	    	microphoneBufferCounter=0;
	    	microphoneBufferToWriteTo = 1;
	    	okToSendMicrophoneBuffer2 = true;
	    }


	  }
	} else {
		if(micSampleCounter == 0){
			quickMic = Sensors_ReadMicrophone();
		}
	}
	micSampleCounter++;
}

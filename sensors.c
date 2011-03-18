//___________________________________________
//				Joshua Schapiro
//				   sensors.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________

#include "avr_compiler.h"
#include "sensors.h"
#include "sd.h"
#include "light.h"





volatile bool recording = false;
bool requestToStopRecording = false;

volatile bool okToSendTemperatureBuffer1 = false;
volatile bool okToSendTemperatureBuffer2 = false;
uint8_t temperatureBufferToWriteTo = 1;
uint8_t  temperatureBufferCounter = 0;

volatile bool okToSendPressureBuffer1 = false;
volatile bool okToSendPressureBuffer2 = false;
uint8_t  pressureBufferToWriteTo = 1;
uint8_t  pressureBufferCounter = 0;

volatile bool okToSendHumidityBuffer1 = false;
volatile bool okToSendHumidityBuffer2 = false;
uint8_t  humidityBufferToWriteTo = 1;
uint8_t  humidityBufferCounter = 0;

volatile bool okToSendMicrophoneBuffer1 = false;
volatile bool okToSendMicrophoneBuffer2 = false;
uint8_t microphoneBufferToWriteTo = 1;
uint16_t microphoneBufferCounter = 0;

volatile bool okToSendLightBuffer1 = false;
volatile bool okToSendLightBuffer2 = false;
uint8_t lightBufferToWriteTo = 1;
uint8_t lightBufferCounter = 0;


volatile bool okToSendRTCBlock = false;
uint8_t  rtcBlockCounter = 0;

bool wantToRecordTemperature 	= true;
bool wantToRecordPressure 		= true;
bool wantToRecordHumidity 		= true;
bool wantToRecordLight		 	= true;
bool wantToRecordAirQuality		= true;
bool wantToRecordMicrophone 	= false;


uint8_t quickTemperature = 0;
uint8_t quickHumidity = 0;
uint8_t quickPressure = 0;
uint16_t quickLight = 0;
uint8_t quickMic = 0;
uint8_t micSampleCounter = 0;


uint32_t result[4];

void Sensors_Init(void){
	
	ADCA.temperatureChannel.CTRL = ADC_CH_INPUTMODE_DIFF_gc;	// set input mode
	ADCA.temperatureChannel.MUXCTRL = temperatureMUXPos | ground_temperatureMUXPos;

	ADCA.humidityChannel.CTRL = ADC_CH_INPUTMODE_DIFF_gc;	// set input mode
	ADCA.humidityChannel.MUXCTRL = humidityMUXPos | ground_humidityMUXPos;

	ADCA.microphoneChannel.CTRL = ADC_CH_INPUTMODE_DIFF_gc;	// set input mode
	ADCA.microphoneChannel.MUXCTRL = microphoneMUXPos | ground_microphoneMUXPos;

	ADCA.pressureChannel.CTRL = ADC_CH_INPUTMODE_DIFF_gc;	// set input mode
	ADCA.pressureChannel.MUXCTRL = pressureMUXPos | ground_pressureMUXPos;

	ADCA.PRESCALER = (ADCA.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV64_gc;

	ADCA.REFCTRL = ADC_REFSEL_VCC_gc;
	ADCA.EVCTRL = (ADCA.EVCTRL & (~ADC_SWEEP_gm)) | ADC_SWEEP_0123_gc;

	ADCA.CTRLB |= ADC_FREERUN_bm | ADC_CONMODE_bm; // free running mode
	

	ADCA.temperatureChannel.CTRL |= ADC_CH_START_bm;
	ADCA.humidityChannel.CTRL |= ADC_CH_START_bm;
	ADCA.microphoneChannel.CTRL |= ADC_CH_START_bm;
	ADCA.pressureChannel.CTRL |= ADC_CH_START_bm;

	ADCA.CTRLA = ADC_ENABLE_bm;							// enable adc w/o calibrating
	
	// general sensors @ 1hz
	// fclk = 14745600
	// div  = 1024
	// per  = 15625
	// => 14745600/14400/1024 => 1 samples per second
	
	// Set period/TOP value
	TCD1.PER = 14400;

	// Select clock source
	TCD1.CTRLA = (TCD1.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;
	
	// Enable CCA interrupt
	TCD1.INTCTRLA = (TCD1.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;
	
	// microphone @ 8khz
	// fclk = 14745600
	// div  = 8
	// per  = 256
	// => 14745600/8/256 => 8000 samples per second

	// Set period/TOP value

	TCF0.PER = 256; 				// 7.2khz

	// Select clock source
	TCF0.CTRLA = (TCF0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;

	// Enable CCA interrupt
	TCF0.INTCTRLA = (TCF0.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_HI_gc;

}

uint16_t Sensors_ReadTemperature(void){
	uint32_t tmp =  ADCA.temperatureResult; //ADCA.groundResult;
	uint16_t tmp2 = 0;

	tmp *= 206250;
	tmp -= 31984375;
	tmp2 = tmp / 358225;

	return tmp2;
}

uint8_t Sensors_ReadHumidity(void){
	uint32_t tmp =  ADCA.humidityResult;
	uint8_t tmp2 = 0;

	tmp *= 7623;
	tmp -= 1776796;
	tmp2 = tmp / 81880;

	return tmp2;
}

uint8_t Sensors_ReadMicrophone(void){
	return ADCA.microphoneResult/16;
}

uint8_t Sensors_ReadPressure(void){
	uint32_t tmp =  ADCA.pressureResult;
	uint8_t tmp2 = 0;

	tmp *= 41250;
	tmp += 6417345;
	tmp2 = tmp / 607959;

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


ISR(TCD1_OVF_vect)
{
    UNIX_time++;
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
			  quickHumidity = humidityBuffer1[humidityBufferCounter];
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
			  quickHumidity = humidityBuffer2[humidityBufferCounter];
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
			  quickPressure = pressureBuffer1[pressureBufferCounter];

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
			  quickPressure = pressureBuffer2[pressureBufferCounter];
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
			  result[j] = 0;
			  for(uint32_t i = 0; i < 76800; i+=resultMultiplier[gainSelector]){
				  result[j] += Light_returnColor(j);
			  }
		  }

		  if(lightBufferToWriteTo == 1){
			  if(lightBufferCounter == 0){
	  			  lightSampleStartTime1 = Time_Get32BitTimer();
			  }
			  lightBuffer1[lightBufferCounter] = result[0];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = result[1];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = result[2];
			  lightBufferCounter++;
			  lightBuffer1[lightBufferCounter] = result[3];
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
			  lightBuffer2[lightBufferCounter] = result[0];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = result[1];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = result[2];
			  lightBufferCounter++;
			  lightBuffer2[lightBufferCounter] = result[3];
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
		quickHumidity = Sensors_ReadHumidity();
		quickPressure = Sensors_ReadPressure();
		Light_readColors();
		quickLight = Light_returnColor(clear);

	}




}

ISR(TCF0_OVF_vect)
{
	if(recording && wantToRecordMicrophone){
	  if(microphoneBufferToWriteTo == 1){
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
	  } else if (microphoneBufferToWriteTo == 2){
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

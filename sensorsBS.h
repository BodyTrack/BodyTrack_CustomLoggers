//___________________________________________
//				Joshua Schapiro
//				   sensors.h
//			  created: 10/12/2010
//			  updated: 
//___________________________________________


// Common
#define groundMUXPos                ADC_CH_MUXPOS_PIN1_gc
#define groundChannel	            CH0
#define	groundResult	            CH0RES


//ADCA

#define powerMUXPos					ADC_CH_MUXPOS_PIN3_gc
#define temperatureMUXPos			ADC_CH_MUXPOS_PIN4_gc
#define humidityMUXPos				ADC_CH_MUXPOS_PIN5_gc

#define powerChannel				CH1
#define temperatureChannel			CH2
#define humidityChannel				CH3


#define powerResult					CH1RES
#define	temperatureResult			CH2RES
#define	humidityResult				CH3RES


#define ADCACAL0_offset				0x20
#define ADCACAL1_offset				0x21


//ADCB
#define pressureMUXPos				ADC_CH_MUXPOS_PIN4_gc
#define microphoneMUXPos			ADC_CH_MUXPOS_PIN5_gc

#define pressureChannel				CH1
#define microphoneChannel			CH2

#define	pressureResult				CH1RES
#define	microphoneResult			CH2RES

#define ADCBCAL0_offset				0x24
#define ADCBCAL1_offset				0x25

void Sensors_Init(void);
uint16_t Sensors_ReadPower(void);
uint16_t Sensors_ReadTemperature(void);
uint16_t Sensors_ReadHumidity(void);
uint16_t Sensors_ReadPressure(void);
uint8_t Sensors_ReadMicrophone(void);

void Sensors_ResetTemperatureBuffers(void);
void Sensors_ResetPressureBuffers(void);
void Sensors_ResetHumidityBuffers(void);
void Sensors_ResetMicrophoneBuffers(void);
void Sensors_ResetLightBuffers(void);

uint8_t SP_ReadCalibrationByte( uint8_t index );

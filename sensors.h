//___________________________________________
//				Joshua Schapiro
//				   sensors.h
//			  created: 10/12/2010
//			  updated: 
//___________________________________________

#define microphoneMUXPos			ADC_CH_MUXPOS_PIN7_gc
#define pressureMUXPos				ADC_CH_MUXPOS_PIN6_gc
#define humidityMUXPos				ADC_CH_MUXPOS_PIN4_gc
#define temperatureMUXPos			ADC_CH_MUXPOS_PIN5_gc
#define ground_microphoneMUXPos		ADC_CH_MUXNEG_PIN0_gc
#define ground_pressureMUXPos		ADC_CH_MUXNEG_PIN1_gc
#define ground_temperatureMUXPos	ADC_CH_MUXNEG_PIN2_gc
#define ground_humidityMUXPos		ADC_CH_MUXNEG_PIN3_gc

#define temperatureChannel	CH0
#define humidityChannel		CH1
#define pressureChannel		CH2
#define microphoneChannel	CH3

#define	temperatureResult	CH0RES
#define	humidityResult		CH1RES
#define	pressureResult		CH2RES
#define	microphoneResult	CH3RES

void Sensors_Init(void);
uint16_t Sensors_ReadTemperature(void);
uint16_t Sensors_ReadHumidity(void);
uint16_t Sensors_ReadPressure(void);
uint8_t Sensors_ReadMicrophone(void);

void Sensors_ResetTemperatureBuffers(void);
void Sensors_ResetPressureBuffers(void);
void Sensors_ResetHumidityBuffers(void);
void Sensors_ResetMicrophoneBuffers(void);
void Sensors_ResetLightBuffers(void);

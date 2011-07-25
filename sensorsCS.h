/*
 *  sensorsCS.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

// ADCA

#define temperatureMUXPos			ADC_CH_MUXPOS_PIN5_gc
#define humidityMUXPos              ADC_CH_MUXPOS_PIN6_gc
#define EKGMUXPos                   ADC_CH_MUXPOS_PIN4_gc
#define groundMUxPos				ADC_CH_MUXPOS_PIN1_gc


#define EKGChannel	                CH0
#define temperatureChannel	        CH1
#define humidityChannel	            CH2
#define groundChannel				CH3


#define	EKGResult	                CH0RES
#define	temperatureResult           CH1RES
#define	humidityResult	            CH2RES
#define	groundResult	            CH3RES

#define ADCACAL0_offset  0x20
#define ADCACAL1_offset  0x21

// ADCB

#define respirationMUXPos           ADC_CH_MUXPOS_PIN1_gc
#define neg_resipirationMUXPos		ADC_CH_MUXNEG_PIN4_gc
#define respirationDriver           2

#define respirationChannel	        CH0
#define	respirationResult	        CH0RES

#define respirationPort				PORTB

#define ADCBCAL0_offset  0x24
#define ADCBCAL1_offset  0x25

void Sensors_Init(void);
uint16_t Sensors_ReadTemperature(void);
uint16_t Sensors_ReadRespiration(void);
uint16_t Sensors_ReadEKG(void);
uint16_t Sensors_ReadHumidity(void);
uint16_t Sensors_ReadGround(void);

void Sensors_ResetTemperatureBuffers(void);
void Sensors_ResetRespirationBuffers(void);
void Sensors_ResetEKGBuffers(void);
void Sensors_ResetHumidityBuffers(void);


uint8_t SP_ReadCalibrationByte( uint8_t index );

//___________________________________________
//				Joshua Schapiro
//				    light.h
//			  created: 09/07/2010
//			  updated:
//
//___________________________________________

#ifndef LIGHT_H_
#define LIGHT_H_

// Register Addresses
#define Control					0x00
#define Timing					0x01
#define Gain					0x07

// Control Register
#define ADC_Valid_bm			0b00010000
#define ADC_En_bm				0b00000010
#define Power_bm				0b00000001

// Timing Register
#define Integ_Mode_Auto_bm		0b00000000
#define Integ_Mode_Manual_bm	0b00010000
#define Integ_Mode_Sync_bm		0b00100000
#define Integ_Mode_Integrate_bm	0b00110000
#define	_12ms_bm				0b00000000
#define	_100ms_bm				0b00000001
#define	_400ms_bm				0b00000010

// Gain Register
#define Gain_1X					0b00000000
#define Gain_4X					0b00010000
#define Gain_16X				0b00100000
#define Gain_64X				0b00110000

#define green					0
#define red						1
#define	blue					2
#define clear					3

uint8_t gainSelector = 0;
uint8_t integrationTime[9]   = {_12ms_bm, _12ms_bm, _100ms_bm, _12ms_bm, _400ms_bm, _12ms_bm, _400ms_bm, _400ms_bm, _400ms_bm};
uint8_t gainSetting[9]	     = {Gain_1X, Gain_4X, Gain_1X, Gain_16X, Gain_1X, Gain_64X, Gain_4X, Gain_16X, Gain_64X};
uint32_t resultMultiplier[9] = {12,48,100,192,400,768,1600,6400,25600};

void Light_Init(uint8_t address);
uint8_t Light_readByte(uint8_t location);
void Light_readColors(void);
uint16_t Light_returnColor(uint8_t color);


void Light_writeByte(uint8_t location, uint8_t toSend);
void Light_setGain(void);

#endif /* LIGHT_H_ */

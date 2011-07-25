/*
 *  configBaseStation.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/7/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

bool wantToRecordTemperature 	= true;
bool wantToRecordPressure 		= true;
bool wantToRecordHumidity 		= true;
bool wantToRecordLight		 	= true;
bool wantToRecordAirQuality		= true;
bool wantToRecordFast			= false;

// Button
#define Button_Port				PORTF
#define Button_Pin				1
#define Switch_Pin				2


// Debug
#define Debug_Port				PORTC
#define Debug_Usart				USARTC0
#define Debug_RX_pin_bm		    PIN2_bm
#define Debug_TX_pin_bm		    PIN3_bm
#define Debug_Flow_Port         PORTC
#define Debug_RTS_pin           0
#define Debug_CTS_pin           1
#define Debug_DMA_Channel		CH1
#define Debug_BufferSize		100

// Display
#define Display_Port			PORTD 
#define Display_CDPort			PORTF 
#define Display_ResetPort		PORTF
#define Display_SPI				SPID  
#define	Backlight_Port			PORTF
#define	Backlight_Pin			0

#define Display_SS_bm           0x10 /*!< \brief Bit mask for the SS pin. */
#define Display_MOSI_bm         0x20 /*!< \brief Bit mask for the MOSI pin. */
#define Display_SCK_bm          0x80 /*!< \brief Bit mask for the SCK pin. */
#define Display_CD_bm			0x80 
#define Display_Reset_bm		0x40

#define Display_SS_CTRL			PIN4CTRL
#define Display_CD_CTRL			PIN7CTRL

// Light
#define LightPort				TWIE
#define LightAddress			0x72

// RS232 (Dlyos)
#define Rs232_Port				PORTD
#define Rs232_Usart				USARTD0
#define Rs232_RX_pin_bm			PIN2_bm
#define Rs232_TX_pin_bm			PIN3_bm
#define Rs232_BufferSize		1024
#define Rs232_RXC_vect			USARTD0_RXC_vect

// SD
#define SD_PORT					PORTC 
#define	SD_SPI					SPIC  
#define SD_SPI_SS_bm        	PIN4_bm
#define SD_SPI_MOSI_bm      	PIN5_bm
#define SD_SPI_MISO_bm      	PIN6_bm
#define SD_SPI_SCK_bm       	PIN7_bm
#define SD_CD_Port				PORTD
#define SD_CD					0 
#define SD_CD_CNTL				PIN0CTRL
#define DeviceClass				"BaseStation"
#define StartFileLength			839



// Timers
#define	Time_TimerHigh				TCC1
#define	Time_TimerHighCNT			TCC1_CNT
#define Time_TimerLow				TCC0
#define	Time_TimerLowCNT			TCC0_CNT
#define Time_EventChannelMux		CH0MUX
#define	Time_EventClockSource		TC_CLKSEL_EVCH0_gc
#define	Time_EventInput				EVSYS_CHMUX_TCC0_OVF_gc

#define Sensors_Timer_1HZ			TCD1
#define Sensors_Timer_1HZ_vect		TCD1_OVF_vect
#define Sensors_Timer_7200HZ		TCD0
#define Sensors_Timer_7200HZ_vect	TCD0_OVF_vect

#define	SD_Timer					TCE0
#define SD_Timer_vect				TCE0_OVF_vect

#define SD_Writer_Timer				TCE1
#define SD_Writer_Timer_vect		TCE1_OVF_vect

#define Display_Writer_Timer		TCF0
#define Display_Writer_Timer_vect	TCF0_OVF_vect




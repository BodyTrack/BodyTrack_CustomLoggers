/*
 *  configChestStrap.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

bool wantToRecordTemperature 	= true;
bool wantToRecordHumidity 		= true;
bool wantToRecordEKG		 	= true;
bool wantToRecordRespiration	= true;
bool wantToRecordFast			= false;



// Battery Charger				
#define Charger_Port			PORTD
#define Charger_Pin				3
#define Charger_Pin_CNTL		PIN3CTRL

// Button
#define Button_Port				PORTF
#define Button_Pin				6

// Leds
#define numberOfLeds			2
#define Leds_Port				PORTF
#define Red						0
#define Green					1

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

// SD
#define SD_PORT					PORTD 
#define	SD_SPI					SPID  
#define SD_SPI_SS_bm        	PIN4_bm
#define SD_SPI_MOSI_bm      	PIN5_bm
#define SD_SPI_MISO_bm      	PIN6_bm
#define SD_SPI_SCK_bm       	PIN7_bm
#define SD_CD_Port				PORTF
#define SD_CD					4 
#define SD_CD_CNTL				PIN4CTRL
#define DeviceClass				"ChestStrap"
#define StartFileLength			469

// Accel

#define Accel_PORT				PORTC
#define	Accel_SPI				SPIC
#define Accel_SS_bm        		PIN4_bm
#define Accel_MOSI_bm      		PIN5_bm
#define Accel_MISO_bm      		PIN6_bm
#define Accel_SCK_bm       		PIN7_bm
#define Accel_SS_CTRL           PIN4CTRL

#define Accel_INT_Port			PORTF
#define Accel_INT1				2
#define Accel_INT2				3


#define   Accel_selectChip(void) (Accel_PORT.OUTCLR = Accel_SS_bm);
#define Accel_deselectChip(void) (Accel_PORT.OUTSET = Accel_SS_bm);



// Timers
#define	Time_TimerHigh				TCC1
#define	Time_TimerHighCNT			TCC1_CNT
#define Time_TimerLow				TCC0
#define	Time_TimerLowCNT			TCC0_CNT
#define Time_EventChannelMux		CH0MUX
#define	Time_EventClockSource		TC_CLKSEL_EVCH0_gc
#define	Time_EventInput				EVSYS_CHMUX_TCC0_OVF_gc

#define GUI_Timer					TCD0
#define GUI_Timer_vect				TCD0_OVF_vect

#define Sensors_Timer_300HZ			TCD1
#define Sensors_Timer_300HZ_vect	TCD1_OVF_vect

#define	SD_Timer					TCE0
#define SD_Timer_vect				TCE0_OVF_vect

#define SD_Writer_Timer				TCE1
#define SD_Writer_Timer_vect		TCE1_OVF_vect

#define Accel_Sample_Timer			TCF0
#define Accel_Sample_Timer_vect		TCF0_OVF_vect




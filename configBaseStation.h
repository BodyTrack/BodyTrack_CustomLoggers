/*
 *  configBaseStation.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/7/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */



#define DeviceClass			"BaseStation"
#define FirmwareVersion		"3.07"
#define HardwareVersion		"3"


bool wantToRecordTemperature 	= true;
bool wantToRecordPressure 		= true;
bool wantToRecordHumidity 		= true;
bool wantToRecordLight		 	= true;
bool wantToRecordAirQuality		= true;
bool wantToRecordFast			= false;


#define maxFileLength					9000

#define temperatureNumberOfSamples   	10
#define humidityNumberOfSamples   		10
#define pressureNumberOfSamples   		10
#define microphoneNumberOfSamples   	2000
#define lightNumberOfSamples   			10
#define lightNumberOfChannels			4
#define airNumberOfFastSamples			1
#define airNumberOfFastChannels			6
#define airNumberOfSlowSamples			1
#define airNumberOfSlowChannels			2

#define temperatureNumberOfBuffers		2
#define pressureNumberOfBuffers			2
#define humidityNumberOfBuffers			2
#define lightNumberOfBuffers			2
#define microphoneNumberOfBuffers		3


#define temperatureTicksPerSample		1843200			// 1Hz
#define pressureTicksPerSample			1843200			// 1Hz
#define humidityTicksPerSample			1843200			// 1Hz
#define lightTicksPerSample				1843200			// 1Hz
#define microphoneTicksPerSample		256				// 7200Hz
#define airTicksPerSampleFastHZ			1843200			// 1Hz	
#define airTicksPerSampleSlowHZ			110592000		// 1/60Hz			


// Button
#define Button_Port				PORTF
#define Button_Pin				1
#define Switch_Pin				2
#define Button_IntVector		PORTF_INT0_vect
#define Switch_IntVector		PORTF_INT1_vect


// Debug
#define Debug_Port				PORTC
#define Debug_Usart				USARTC0
#define Debug_RX_pin_bm		    PIN2_bm
#define Debug_TX_pin_bm		    PIN3_bm
#define Debug_Flow_Port         PORTC
#define Debug_RTS_pin           0
#define Debug_CTS_pin           1
#define Debug_BufferSize		100
#define Debug_INTVector			USARTC0_RXC_vect
#define Debug_DMA_Channel		CH0



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
#define Rs232_BufferSize		100
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
#define StartFileLength			812



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




#include "avr_compiler.h"
#include "rtc.h"

/*
Years are expected to be offset from 1970 for conversion
Will be saved onto RTC accordingly (correct for this when printing out)

*/

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

// API starts months from 1, this array starts from 0
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; 


#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

#define  CalendarYrToTm(Y)   ((Y) - 1970)


//Global time variable
time_t time;

uint8_t initialRTCRegisters [17];

void RTC_init(void) {
	RTCPort.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
	//Don't need to set smart mode
	//TWIPort.CTRLB =

	//Set Master Read Acknowledge Action to ACK
	RTCPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm;

	//Want 400 kHz I2C signal
	//32Mhz/(2*400kHz) - 5 = 35
	//TWIPort.MASTER.BAUD = 35;
	RTCPort.MASTER.BAUD = 15;

	RTCPort.MASTER.STATUS =TWI_MASTER_BUSSTATE_IDLE_gc;


}

//Takes in UTC Epoch Time since 1970
//Returns calendar time in time_t struct
time_t * RTC_UTCSecsToTime(uint32_t UTCSecs, time_t * tm) {
	uint8_t year;
	uint8_t month, monthLength;
	unsigned long days;
	
	tm->Second = UTCSecs % 60;
	UTCSecs /= 60; // now it is minutes
	tm->Minute = UTCSecs % 60;
	UTCSecs /= 60; // now it is hours
	tm->Hour = UTCSecs % 24;
	UTCSecs /= 24; // now it is days
	tm->Wday = ((UTCSecs + 4) % 7) + 1;  // Sunday is day 1 
	
	year = 0;  
	days = 0;
	while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= UTCSecs) {
		year++;
	}
	
	tm->Year = year; // year is offset from 1970 

	
	days -= LEAP_YEAR(year) ? 366 : 365;
	UTCSecs  -= days; // now it is days in this year, starting at 0
	
	days=0;
	month=0;
	monthLength=0;
	for (month=0; month<12; month++) {
    	if (month==1) { // february
	      	if (LEAP_YEAR(year)) {
	       		monthLength=29;
	      	} else {
	        	monthLength=28;
	      	}
    	} else {
      		monthLength = monthDays[month];
    	}
    
    	if (UTCSecs >= monthLength) {
      		UTCSecs -= monthLength;
    	} else {
        	break;
  		}
	}
	tm->Month = month + 1;  // jan is month 1  
	tm->Day = UTCSecs + 1;     // day of month 
	
	return tm;	
}

uint32_t RTC_TimeToUTCSecs(time_t * tm) {
	uint32_t UTCSecs;
	int i;
	
 	// seconds from 1970 till 1 jan 00:00:00 of the given year
	UTCSecs= tm->Year*(SECS_PER_DAY * 365);
	for (i = 0; i < tm->Year; i++) {
    	if (LEAP_YEAR(i)) {
    		UTCSecs +=  SECS_PER_DAY;   // add extra days for leap years
    	}
  	}
  
  // add days for this year, months start from 1
	for (i = 1; i < tm->Month; i++) {
    	if ( (i == 2) && LEAP_YEAR(tm->Year)) { 
      		UTCSecs += SECS_PER_DAY * 29;
    	} else {
      		UTCSecs += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
    	}
  	}
	UTCSecs+= (tm->Day-1) * SECS_PER_DAY;
	UTCSecs+= tm->Hour * SECS_PER_HOUR;
	UTCSecs+= tm->Minute * SECS_PER_MIN;
	UTCSecs+= tm->Second;
 
	return UTCSecs; 
}



void RTC_setUTCSecs(uint32_t UTCSecs) {
	// break the given time_t into time components
// this is a more compact version of the C library localtime function
// note that year is offset from 1970 !!!
	
	RTC_UTCSecsToTime(UTCSecs,&time);
	RTC_setTime();
}


uint32_t RTC_getUTCSecs(void) {
	uint32_t UTCSecs;

	RTC_getTime();
	UTCSecs = RTC_TimeToUTCSecs(&time);
	  
 	return UTCSecs;
}

void RTC_getTime(void) {
	uint8_t timeData[8];
		
	//Write starting address
	timeData[0] = 0x00;
	RTC_sendBytes(1,timeData,RTCAddress);
	
	//Wait for data to come back
	RTC_receiveBytes(8,timeData,RTCAddress);
	

	time.Hundreths = 10*((timeData[0] & 0xF0) >> 4) + (timeData[0] & 0x0F);
	time.Second = 10*((timeData[1] & 0x70) >> 4) + (timeData[1] & 0x0F);
	time.Minute = 10*((timeData[2] & 0x70) >> 4) + (timeData[2] & 0x0F);
	time.Hour = 10*((timeData[3] & 0x30) >> 4) + (timeData[3] & 0x0F);
	time.Wday = timeData[4];
	time.Day = 10*((timeData[5] & 0x30) >> 4) + (timeData[5] & 0x0F);
	time.Month = 10*((timeData[6] & 0x10) >> 4) + (timeData[6] & 0x0F);
	time.Year = 100*((timeData[3] & 0x50) >> 6) + 10*((timeData[7] & 0xF0) >> 4) + (timeData[7] & 0x0F);


	//Convert to years since 1970 for time conversion
	time.Year += 30;




} 

void RTC_setTime(void) {
	uint8_t timeData[9];
	uint8_t years2000 = time.Year - 30; //Years since 2000	

	timeData[0] = 0x00; //Set starting write address
	timeData[1] = ((time.Hundreths/10) << 4) | (time.Hundreths % 10);
	timeData[2] = ((time.Second/10) << 4) | (time.Second % 10);
	timeData[3] = ((time.Minute/10) << 4) | (time.Minute % 10);
	//Write century and century enable bit
	timeData[4] = ((years2000 / 100) << 6) | (1 << 7) | ((time.Hour/10) << 4) | (time.Hour % 10);
	timeData[5] = time.Wday;
	timeData[6] = ((time.Day/10) << 4) | (time.Day % 10);
	timeData[7] = ((time.Month/10) << 4) | (time.Month % 10);
	timeData[8] = ((years2000/10) << 4) | (years2000 % 10);	

	RTC_sendBytes(9,timeData,RTCAddress);
}


void RTC_sendBytes(uint8_t numBytes, uint8_t* byteArray, uint8_t deviceAddress) {
	uint8_t i = 0;
	//R/~W is last bit on address
	uint8_t writeAddress = deviceAddress & ~0x01;
	RTCPort.MASTER.ADDR = writeAddress;

	while (i < numBytes) {

		while (!(RTCPort.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
			//Wait for write flag to turn on
		}

		//If NACK received (non-zero), warn user and cancel operation
		if (RTCPort.MASTER.STATUS & TWI_MASTER_RXACK_bm) {
			//USART_sendString("NACK RECEIVED",true);
			return;
		}

		//Send data
		RTCPort.MASTER.DATA = byteArray[i];

		i++;
	}

}

void RTC_receiveBytes(uint8_t numBytes, uint8_t* byteArray, uint8_t deviceAddress) {
	uint8_t i = 0;

	//Send start condition, and device address + Read
	uint8_t writeAddress = deviceAddress | 0x01;
	RTCPort.MASTER.ADDR = writeAddress;

	while (i < numBytes) {

		while (!(RTCPort.MASTER.STATUS & TWI_MASTER_RIF_bm)) {
			//Wait for Read flag to come on
		}
		byteArray[i] = RTCPort.MASTER.DATA;

		//If more bytes to read, send ACK and start byte read
		RTCPort.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
		i++;
	}

	//All values are read. Send STOP condition
	RTCPort.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;

}


 


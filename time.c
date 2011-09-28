//___________________________________________
//				Joshua Schapiro
//				    time.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________



typedef struct RTC32_struct2 {
	register8_t CTRL;
	register8_t SYNCCTRL;
	register8_t INTCTRL;
	register8_t INTFLAGS;
	_DWORDREGISTER(CNT);
	_DWORDREGISTER(PER);
	_DWORDREGISTER(COMP);
} RTC32_t2;

#undef RTC32

#define RTC32 (*(RTC32_t2 *)0x0420)

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

uint32_t UNIX_Time = 0;

void Time_Init(void)			 
{
	
	// 32 bit counter
  /* We need to set up the event user
   * Which event channel should be input for the peripheral, and what should be the action. */
  
	Time_TimerHigh.CTRLA = Time_EventClockSource; //Select event channel 0 as clock source for TCC1.

  /* For the event system, it must be selected what event is to be routed through the multiplexer for each
   * event channel used */
  
   /* Select TCC0 overflow as event channel 0 multiplexer input.
    * This is all code required to configure one event channel */
  
	EVSYS.Time_EventChannelMux = Time_EventInput;
    
  /* TCC0 is used as the 16 LSB for the timer. This runs from the normal (prescaled) Peripheral Clock */  
  
  //Select system clock divided by 8 as clock source for TCC0.
	Time_TimerLow.PER = 0xFFFF;
	Time_TimerLow.CTRLA = (Time_TimerLow.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;   // 1.8432 megehertz
	
	
}

void Time_Set(uint32_t time){
	cli();
	VBAT.CTRL = VBAT_ACCEN_bm;
	CCPWrite(&VBAT.CTRL, VBAT_RESET_bm);		// Reset battery backup
	VBAT.CTRL |= VBAT_XOSCFDEN_bm;				// enable oscillator failure detection
	VBAT.CTRL |= VBAT_XOSCEN_bm;				// enable 1hz output from oscillator
	
	/* Disable the RTC32 module before writing to it. Wait for synch. */
	RTC32.CTRL &= ~RTC32_ENABLE_bm;
	while (RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm);
	
	/* Write PER, COMP and CNT. */
	RTC32.PER = 0xFFFFFFFF - 1;
	RTC32.COMP = 0;
	RTC32.CNT = time;
	
	_delay_ms(10);
	/* Re-enable the RTC32 module, synchronize before returning. */
	RTC32.CTRL = RTC32_ENABLE_bm;
	while (RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm);
	
	RTC32.INTCTRL = ( RTC32.INTCTRL & ~RTC32_COMPINTLVL_gm ) | RTC32_COMPINTLVL_LO_gc;
	sei();
}

bool Time_CheckVBatSystem(void){
	if (VBAT.STATUS & VBAT_BBPWR_bm){
		return false;
	} else {
		if (VBAT.STATUS & VBAT_BBPORF_bm) {
			return false;
		} else if (VBAT.STATUS & VBAT_BBBORF_bm){
			return false;
		} else {
			VBAT.CTRL = VBAT_ACCEN_bm;
			if (VBAT.STATUS & VBAT_XOSCFAIL_bm){
				return false;
			} 
		}
	}
	return true;
}

uint32_t Time_Get(void){
	//cli();
	RTC32.SYNCCTRL |= RTC32_SYNCCNT_bm;
	while ( RTC32.SYNCCTRL & RTC32_SYNCCNT_bm );
	return RTC32.CNT;
	//sei();
}

uint32_t Time_Get32BitTimer(void){
  cli();
  uint16_t high = Time_TimerHighCNT;
  uint16_t low  = Time_TimerLowCNT;
  uint32_t result = 0;

  if(high != Time_TimerHighCNT){
	 high = Time_TimerHighCNT;
	 low  = Time_TimerLowCNT;
  }
  result = high * 65536 + low;
  sei();
  return result;
}


//Takes in UTC Epoch Time since 1970
//Returns calendar time in time_t struct
time_t * Time_UTCSecsToTime(uint32_t UTCSecs, time_t * tm) {
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


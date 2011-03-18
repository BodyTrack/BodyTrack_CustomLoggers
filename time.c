//___________________________________________
//				Joshua Schapiro
//				    time.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________


volatile uint32_t UNIX_time = 0;

void Time_Init(void)			 
{

	// internal rtc
	/* Turn on internal 32kHz. */

	//OSC.CTRL |= OSC_RC32KEN_bm;

	//do {
		/* Wait for the 32kHz oscillator to stabilize. */
//	} while ( ( OSC.STATUS & OSC_RC32KRDY_bm ) == 0);

	/* Set internal 32kHz oscillator as clock source for RTC. */
//	CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm;


//	do {
		/* Wait until RTC is not busy. */
//	} while (RTC.STATUS & RTC_SYNCBUSY_bm);


	/* Configure RTC period to 1 second. */
//	//RTC_Initialize( RTC_CYCLES_1S, 0, 0, RTC_PRESCALER_DIV1_gc );
//	RTC.PER = 1024 - 1;
//	RTC.CNT = 0;
//	RTC.COMP = 0;
//	RTC.CTRL = ( RTC.CTRL & ~RTC_PRESCALER_gm ) | RTC_PRESCALER_DIV1_gc;

	/* Enable overflow interrupt. */
	//RTC_SetIntLevels( RTC_OVFINTLVL_LO_gc, RTC_COMPINTLVL_OFF_gc );
//	RTC.INTCTRL = ( RTC.INTCTRL &
//	              ~( RTC_COMPINTLVL_gm | RTC_OVFINTLVL_gm ) ) |
//	              RTC_OVFINTLVL_MED_gc;
	
	// 32 bit counter
  /* We need to set up the event user
   * Which event channel should be input for the peripheral, and what should be the action. */
  
  TCC1.CTRLA = TC_CLKSEL_EVCH0_gc; //Select event channel 0 as clock source for TCC1.

  /* For the event system, it must be selected what event is to be routed through the multiplexer for each
   * event channel used */
  
   /* Select TCC0 overflow as event channel 0 multiplexer input.
    * This is all code required to configure one event channel */
  
  EVSYS.CH0MUX = EVSYS_CHMUX_TCC0_OVF_gc;
    
  /* TCC0 is used as the 16 LSB for the timer. This runs from the normal (prescaled) Peripheral Clock */  
  
  //Select system clock divided by 8 as clock source for TCC0.
  TCC0.PER = 0xFFFF;
  TCC0.CTRLA = (TCC0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV8_gc;   // 1.8432 megehertz
  
	
	
}

void Time_Set(uint32_t time){
  cli();
  UNIX_time = time;
  sei();
}

uint32_t Time_Get32BitTimer(void){
  cli();
  uint16_t high = TCC1_CNT;
  uint16_t low  = TCC0_CNT;
  uint32_t result = 0;

  if(high != TCC1_CNT){
	 high = TCC1_CNT;
	 low  = TCC0_CNT;
  }
  result = high * 65536 + low;
  sei();
  return result;
}


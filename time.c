//___________________________________________
//				Joshua Schapiro
//				    time.c
//			  created: 10/12/2010
//			  updated: 
//___________________________________________


volatile uint32_t UNIX_time = 0;

void Time_Init(void)			 
{
	// fclk = 14745600
	// div  = 14400
	// => 14745600/14400/1024 => 1 samples per second
	
	// Set period/TOP value
	TCD0.PER = 14400;

	// Select clock source
	TCD0.CTRLA = (TCD0.CTRLA & ~TC0_CLKSEL_gm) | TC_CLKSEL_DIV1024_gc;
	
	// Enable CCA interrupt
	TCD0.INTCTRLA = (TCD0.INTCTRLA & ~TC0_OVFINTLVL_gm) | TC_CCAINTLVL_MED_gc;
	
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
  UNIX_time = time;
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


ISR(TCD0_OVF_vect)
{
	UNIX_time++;
 }

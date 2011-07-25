//___________________________________________
//				Joshua Schapiro
//				    time.h
//			  created: 10/12/2010
//			  updated: 
//___________________________________________


void Time_Init(void);
uint32_t Time_Get(void);
uint32_t Time_Get32BitTimer(void);
void Time_Set(uint32_t time);
bool Time_CheckVBatSystem(void)


time_t * Time_UTCSecsToTime(uint32_t UTCSecs, time_t * tm);
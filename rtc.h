/*
RTC Driver code for STMicro RTC M41T81S
Written by Nolan Hergert

*/

#define RTCPort TWIE
#define RTCAddress 0xD0

typedef struct {
  uint8_t Hundreths;
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970;
} time_t;

void RTC_init(void);

time_t * RTC_UTCSecsToTime(uint32_t UTCSecs, time_t * tm);
uint32_t RTC_TimeToUTCSecs(time_t * tm); 

void RTC_setUTCSecs(uint32_t UTCTime);
uint32_t RTC_getUTCSecs(void);


void RTC_getTime(void);
void RTC_setTime(void);

void RTC_sendBytes(uint8_t numBytes, uint8_t* byteArray, uint8_t deviceAddress);
void RTC_receiveBytes(uint8_t numBytes, uint8_t* byteArray, uint8_t deviceAddress);









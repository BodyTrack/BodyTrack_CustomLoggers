

char deviceID   [50];

#define LOTNUM0_offset                  0x08
#define LOTNUM1_offset                  0x09
#define LOTNUM2_offset                  0x0A
#define LOTNUM3_offset                  0x0B
#define LOTNUM4_offset                  0x0C
#define LOTNUM5_offset                  0x0D
#define WAFNUM_offset                   0x10
#define COORDX0_offset                  0x12
#define COORDX1_offset                  0x13
#define COORDY0_offset                  0x14
#define COORDY1_offset                  0x15

#define recordMode						0
#define sensorMode						1

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


volatile bool recording = false;


uint32_t temperatureSampleStartTime1 = 0;
uint32_t temperatureSampleStartTime2 = 0;
uint16_t temperatureBuffer1 [temperatureNumberOfSamples];
uint16_t temperatureBuffer2 [temperatureNumberOfSamples];

uint32_t humiditySampleStartTime1 = 0;
uint32_t humiditySampleStartTime2 = 0;
uint16_t  humidityBuffer1 [humidityNumberOfSamples];
uint16_t  humidityBuffer2 [humidityNumberOfSamples];

uint32_t pressureSampleStartTime1 = 0;
uint32_t pressureSampleStartTime2 = 0;
uint16_t  pressureBuffer1 [pressureNumberOfSamples];
uint16_t  pressureBuffer2 [pressureNumberOfSamples];

uint32_t microphoneSampleStartTime1 = 0;
uint32_t microphoneSampleStartTime2 = 0;
uint8_t microphoneBuffer1 [microphoneNumberOfSamples];
uint8_t microphoneBuffer2 [microphoneNumberOfSamples];

uint32_t lightSampleStartTime1 = 0;
uint32_t lightSampleStartTime2 = 0;
uint32_t lightBuffer1 [lightNumberOfSamples * lightNumberOfChannels];
uint32_t lightBuffer2 [lightNumberOfSamples * lightNumberOfChannels];

uint32_t airSampleTime = 0;

uint32_t timeRecordingStarted = 0;


bool demoMode = false;
bool useWifiForUploading = false;

// From Application Note AVR1003
// Used to slow down clock in disk_initialize()
void CCPWrite( volatile uint8_t * address, uint8_t value ) {
    uint8_t volatile saved_sreg = SREG;
    cli();
	
#ifdef __ICCAVR__
	asm("movw r30, r16");
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm("ldi  r16,  0xD8 \n"
	    "out  0x34, r16  \n"
#if (__MEMORY_MODEL__ == 1)
	    "st     Z,  r17  \n");
#elif (__MEMORY_MODEL__ == 2)
	"st     Z,  r18  \n");
#else /* (__MEMORY_MODEL__ == 3) || (__MEMORY_MODEL__ == 5) */
	"st     Z,  r19  \n");
#endif /* __MEMORY_MODEL__ */
	
#elif defined __GNUC__
	volatile uint8_t * tmpAddr = address;
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm volatile(
				 "movw r30,  %0"	      "\n\t"
				 "ldi  r16,  %2"	      "\n\t"
				 "out   %3, r16"	      "\n\t"
				 "st     Z,  %1"       "\n\t"
				 :
				 : "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
				 : "r16", "r30", "r31"
				 );
	
#endif
	SREG = saved_sreg;
}

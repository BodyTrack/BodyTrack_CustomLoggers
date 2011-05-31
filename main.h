
#define FirmwareVersion "2.09"
#define HardwareVersion "2"
char deviceID   [20];



#define temperatureNumberOfSamples   	10
#define humidityNumberOfSamples   		10
#define pressureNumberOfSamples   		10
#define microphoneNumberOfSamples   	1000
#define lightNumberOfSamples   			10
#define lightNumberOfChannels			4

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
uint32_t lightBuffer1 [lightNumberOfSamples*lightNumberOfChannels];
uint32_t lightBuffer2 [lightNumberOfSamples*lightNumberOfChannels];

uint32_t airSampleTime = 0;

uint32_t timeRecordingStarted = 0;


bool demoMode = false;
bool useWifiForUploading = false;
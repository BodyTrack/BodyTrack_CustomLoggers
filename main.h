


#define temperatureNumberOfSamples   	10
#define humidityNumberOfSamples   		10
#define pressureNumberOfSamples   		10
#define microphoneNumberOfSamples   	1000
#define lightNumberOfSamples   			10
#define lightNumberOfChannels			4

uint32_t temperatureSampleStartTime1 = 0;
uint32_t temperatureSampleStartTime2 = 0;
uint16_t temperatureBuffer1 [temperatureNumberOfSamples];
uint16_t temperatureBuffer2 [temperatureNumberOfSamples];

uint32_t humiditySampleStartTime1 = 0;
uint32_t humiditySampleStartTime2 = 0;
uint8_t  humidityBuffer1 [humidityNumberOfSamples];
uint8_t  humidityBuffer2 [humidityNumberOfSamples];

uint32_t pressureSampleStartTime1 = 0;
uint32_t pressureSampleStartTime2 = 0;
uint8_t  pressureBuffer1 [pressureNumberOfSamples];
uint8_t  pressureBuffer2 [pressureNumberOfSamples];

uint32_t microphoneSampleStartTime1 = 0;
uint32_t microphoneSampleStartTime2 = 0;
uint8_t microphoneBuffer1 [microphoneNumberOfSamples];
uint8_t microphoneBuffer2 [microphoneNumberOfSamples];

uint32_t lightSampleStartTime1 = 0;
uint32_t lightSampleStartTime2 = 0;
uint32_t lightBuffer1 [lightNumberOfSamples*lightNumberOfChannels];
uint32_t lightBuffer2 [lightNumberOfSamples*lightNumberOfChannels];

uint32_t airSampleTime = 0;

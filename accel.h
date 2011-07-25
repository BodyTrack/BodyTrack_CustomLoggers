//___________________________________________
//				Joshua Schapiro
//					  accel.h
//			  created: 02/28/2011
//___________________________________________


void Accel_Init(void);
void Accel_WriteByte(uint8_t byte);
uint8_t Accel_ReadByte(void);

void Accel_WriteToAddress(uint8_t addr, uint8_t byte);
uint8_t Accel_ReadFromAddress(uint8_t addr);
void Accel_ReadResults(uint16_t * buffLocation, uint16_t buffCounter);
void Sensors_ResetAccelBuffers(void);
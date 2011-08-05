//___________________________________________
//				Joshua Schapiro
//
//___________________________________________

#define low			1
#define med			2
#define high		3

#define both		0
#define rising		1
#define falling		2

void Button_Init(uint8_t button, bool enableInt, uint8_t edge, uint8_t intNumber,uint8_t intLevel);
bool Button_Pressed(uint8_t button);


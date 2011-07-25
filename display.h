
/* Display Driver for BodyTrack Base Station
   Compiled and Written by Nolan Hergert
   
	-Currently, only writing strings at a time is allowed
	-Font styles:
   		--1: System 5X7 font
   		--2: Arial14 Variable Width Font
   		--3: Arial14 Bold
	-More fonts can be added using GLCDFontCreator2 under Windows
	-Drawing on the display is utilized by writing to the buffer and periodically
	updating using display_writeBufferToScreen();
	-Writing 0x01 to the first page and column on the screen corresponds to a dot at (0,0) (LSB order)

	-Non-straight lines are hard! For now just draw straight (same row or same column) ones. Something about floats that I'm missing...
*/

void display_init(void);
void display_showSplashScreen(bool showsd, bool showconnected, bool demo);
void display_setBacklight(bool state);
void display_toggleBacklight(void);
void display_sendCommand(uint8_t dataByte);
void display_sendData(uint8_t dataByte);
void display_setCursor(uint8_t page, uint8_t column);


//The following display commands write to the display buffer
//and need to be updated with display_writeBufferToScreen();
void display_clearPage(uint8_t page);
void display_clearScreen(void);
void display_putString(char* text, uint8_t page, uint8_t column, uint8_t* font);
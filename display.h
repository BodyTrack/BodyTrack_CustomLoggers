
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
#define Display_Port		PORTC //PORTE
#define Display_CDPort		PORTB //PORTE
#define Display_ResetPort	PORTD
#define Display_SPI			SPIC  //SPIE



#define Display_SS_bm           0x10 /*!< \brief Bit mask for the SS pin. */
#define Display_MOSI_bm         0x20 /*!< \brief Bit mask for the MOSI pin. */
#define Display_SCK_bm          0x80 /*!< \brief Bit mask for the SCK pin. */
#define Display_CD_bm			0x04 //0x01
#define Display_Reset_bm		0x01

#define Display_SS_CTRL			PIN4CTRL
#define Display_CD_CTRL			PIN3CTRL

void display_init(void);
void display_sendCommand(uint8_t dataByte);
void display_sendData(uint8_t dataByte);
void display_setCursor(uint8_t page, uint8_t column);
void display_writeBufferToScreen(void);


//The following display commands write to the display buffer
//and need to be updated with display_writeBufferToScreen();
void display_clearPage(uint8_t page);
void display_clearBuffer(void);
void display_putString(char* text, uint8_t page, uint8_t column, uint8_t* font);
void display_drawPixel(uint8_t row, uint8_t column, bool black);
void swap(uint8_t* val1, uint8_t* val2);
void display_drawLine(uint8_t row1, uint8_t column1, uint8_t row2, uint8_t column2, bool black);
void display_drawRectangle(uint8_t topRow, uint8_t leftColumn, uint8_t height, uint8_t width, bool filled, bool invertOrig, bool blackBorder);

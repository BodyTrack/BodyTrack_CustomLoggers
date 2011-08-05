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

#include "display.h"
#include "SystemFont5X7.h"

// Font Indices
#define FONT_LENGTH			0
#define FONT_WIDTH			2
#define FONT_HEIGHT			3
#define FONT_FIRST_CHAR		4
#define FONT_CHAR_COUNT		5
#define FONT_WIDTH_TABLE	6

#define DISPLAY_ROWS		64
#define DISPLAY_COLS		102
#define DISPLAY_PAGES		(DISPLAY_ROWS/8)

volatile bool backLightIsOn = false;

void display_init() {
	//Set SS to wired and pull up
	Display_Port.Display_SS_CTRL = PORT_OPC_WIREDANDPULL_gc;
	Display_CDPort.Display_CD_CTRL = PORT_OPC_WIREDANDPULL_gc;
	Display_Port.OUTSET = Display_SS_bm;
	
	Display_ResetPort.DIRSET = Display_Reset_bm;

	Display_ResetPort.OUTSET = Display_Reset_bm; 	// pull high

	_delay_ms(10);
	Display_ResetPort.OUTCLR = Display_Reset_bm; 	// reset

	_delay_ms(10);
	Display_ResetPort.OUTSET = Display_Reset_bm; 	// pull high

	_delay_ms(10);
	//Double clock, enable SPI, MSB sent first, set as master, mode 3, clk/16
	Display_SPI.CTRL = SPI_CLK2X_bm 
				| SPI_ENABLE_bm 
				| SPI_MASTER_bm 
				| SPI_MODE_3_gc 
				| SPI_PRESCALER_DIV16_gc; //Can be 4, 16, 64 or 128

	Display_Port.DIRSET = Display_MOSI_bm | Display_SCK_bm | Display_SS_bm;
	Display_CDPort.DIRSET = Display_CD_bm;

	//Enable display (write a 0 to SS)
	Display_Port.OUTCLR = Display_SS_bm;
	display_sendCommand(0xE2);	// 	reset
	display_sendCommand(0x40);	// 	scroll line
	display_sendCommand(0xA1);	//	seg direction
	display_sendCommand(0xC0);	//	com direction
	display_sendCommand(0xA4);	//	set all pixels on, show sram content
	display_sendCommand(0xA6);	//	set inverse display
	display_sendCommand(0x2F);	//  set power control, all on
	display_sendCommand(0x27);	//	set vlcd resistor ratio
	display_sendCommand(0xFA);	//	set advanced program control
	display_sendCommand(0x90);	//	temp comp = 1
	display_sendCommand(0x40);	//	scroll line

	_delay_ms(150);
	display_sendCommand(0xA2);	//	set lcd bias ratio

	display_sendCommand(0x81);	//	set electronic volume
	display_sendCommand(0x08);	//	PM = 8

	display_sendCommand(0xAF);	//	enable display

	display_clearScreen();
	
	Backlight_Port.DIRSET = 1 << Backlight_Pin;

}

void display_showSplashScreen(bool showsd, bool showconnected, bool demo){
	char tmp [50];
	display_putString("   BaseStation   ",1,0,System5x7);
	strcpy(tmp,"  Hardware: v");
	strcat(tmp,HardwareVersion);
	display_putString(tmp,3,0,System5x7);
	tmp[0] = 0;
	strcpy(tmp," Firmware: v");
	strcat(tmp,FirmwareVersion);
	display_putString(tmp,5,0,System5x7);
	
	if(showsd){
		display_putString("    SD Removed   ",7,0,System5x7);
	} else if(showconnected){
		display_putString("Waiting for sync ",7,0,System5x7);
	} else if(demo){
		display_putString("    Demo Mode    ",7,0,System5x7);
	} else {
		display_clearPage(7);
	}
}

void display_setBacklight(bool state){
	if(state){
		Backlight_Port.OUTSET = 1 << Backlight_Pin;
		backLightIsOn = true;
	} else {
		Backlight_Port.OUTCLR = 1 << Backlight_Pin;
		backLightIsOn = false;
	}
}

void display_toggleBacklight(void){
	Backlight_Port.OUTTGL = 1 << Backlight_Pin;
	if(backLightIsOn){
		backLightIsOn = false;
	} else {
		backLightIsOn = true;
	}
}

void display_sendCommand(uint8_t dataByte) {
	//Write 0 to CD
	Display_CDPort.OUTCLR = Display_CD_bm;
	Display_SPI.DATA = dataByte;
	while(!(Display_SPI.STATUS & SPI_IF_bm));
}

void display_sendData(uint8_t dataByte) {
	//Write 1 to CD
	Display_CDPort.OUTSET = Display_CD_bm;
	Display_SPI.DATA = dataByte;
	while(!(Display_SPI.STATUS & SPI_IF_bm));
}



void display_setCursor(uint8_t page, uint8_t column) {
	//Page
	display_sendCommand(0xB0 | page);
	//Column LSB
	display_sendCommand(0x00 | (column & 0x0F));
	//Column MSB
	display_sendCommand(0x10 | ((column >> 4) & 0x0F));
}



void display_clearPage(uint8_t page) {
	uint8_t j;
	for (j = 0; j < DISPLAY_COLS; j++) {
	    //displayBuffer[page][j] = 0x00;
	    display_setCursor(page,j);
 		display_sendData(0x00);
	}
}


void display_clearScreen() {
	uint8_t i, j;
	for (i = 0; i < DISPLAY_PAGES; i++) {
		for (j = 0; j < DISPLAY_COLS; j++) {
		    display_setCursor(i,j);
 			display_sendData(0);
		}
	}
}


void display_putString(char* text, uint8_t page, uint8_t column, uint8_t* fontTable) {
 uint8_t i,j,k, offset;
 uint8_t fontHeight = fontTable[FONT_HEIGHT];
 uint8_t firstChar = fontTable[FONT_FIRST_CHAR];
 uint8_t charCount = fontTable[FONT_CHAR_COUNT];
 uint8_t bytes = (fontHeight+7)/8;
 uint16_t charIndex;
 uint8_t charWidth, ch, data;


 i = 0;

 while (i*8 < fontHeight) {
	j = 0;
	offset = column;
 	//display_setCursor(page+i, column);
 	while (text[j] != '\0') {
 		ch = text[j] - firstChar;

 
  		charIndex = 0;
 		if (fontTable[FONT_LENGTH] == 0) {
 			charWidth = fontTable[FONT_WIDTH]; //Fixed width font
 			charIndex = ch * charWidth + FONT_WIDTH_TABLE;
 		} else {
 			charWidth = fontTable[ch+FONT_WIDTH_TABLE];
 			for (k = 0; k<ch; k++) {
 				charIndex += fontTable[k+FONT_WIDTH_TABLE]*bytes;
 			}
 			charIndex += charCount + FONT_WIDTH_TABLE + i*charWidth;
 		}
  		//Draw the appropriate portion of the character
 		for (k = 0; k < charWidth; k++) {
 			data = fontTable[charIndex+k];
 			
 			if (fontHeight > 8 && fontHeight < (i+1)*8) {
 				data >>= (i+1)*8-fontHeight;
 			}
 			display_setCursor(page+i,offset);
 			display_sendData(data);
 			//displayBuffer[page+i][offset] = data;
 			offset++;
		}

		//Add a 1px gap between characters
 		if(offset != 101){
 			display_setCursor(page+i,offset+1);
 			display_sendData(0x00);
 			//displayBuffer[page+i][offset+1] = 0x00;
 		}
		offset++;
 	
 		j++;
 	} 	
 	i++;
 }

}

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

#include "avr_compiler.h"
#include "display.h"
//#include "arial_bold_14.h"
//#include "Arial14.h"
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

volatile uint8_t displayBuffer[DISPLAY_PAGES][DISPLAY_COLS];


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

	display_clearBuffer();
	display_writeBufferToScreen();

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


void display_writeBufferToScreen(void) {
	uint8_t i, j;
	for (i = 0; i < DISPLAY_PAGES; i++) {
		display_setCursor(i,0);
		for (j = 0; j < DISPLAY_COLS; j++) {
			display_sendData(displayBuffer[i][j]);
		}
	}	
}


void display_clearPage(uint8_t page) {
	uint8_t j;
	for (j = 0; j < DISPLAY_COLS; j++) displayBuffer[page][j] = 0x0A;
}


void display_clearBuffer() {
	uint8_t i, j;
	for (i = 0; i < DISPLAY_PAGES; i++) {
		for (j = 0; j < DISPLAY_COLS; j++) {
			displayBuffer[i][j] = 0x00;
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
 			displayBuffer[page+i][offset] = data;
 			offset++;
		}

		//Add a 1px gap between characters
 		if(offset != 101){
 			displayBuffer[page+i][offset+1] = 0x00;
 		}
		offset++;
 	
 		j++;
 	} 	
 	i++;
 }

}

void display_drawPixel(uint8_t row, uint8_t column, bool black) {
	if (black) displayBuffer[row/DISPLAY_PAGES][column] |= (0x01 << row%8);
	else displayBuffer[row/DISPLAY_PAGES][column] &= ~(0x01 << row%8);
}

void swap(uint8_t* val1, uint8_t* val2) {
	uint8_t tempVal = *val2;
	*val2 = *val1;
	*val1 = tempVal;
}

void display_drawLine(uint8_t row1, uint8_t column1, uint8_t row2, uint8_t column2, bool black) {
	int8_t i,j;
	//float deltaError, sumErrors;
	int8_t step = 1;
	//int8_t deltaRow,deltaColumn;
	//bool steep = abs(row2 - row1) > abs(column2 - column1);
	//char debugMsg[50];


/*
//Wikipedia's code that is currently running on the micro
//I'm not sure what it is doing...so I might go back to my old code

	sprintf(debugMsg,"Before: %d, %d; %d, %d",row1,column1,row2,column2);
	USART_sendString(debugMsg,true);
	//Just to make things easier to work with, check wikipedia
	if (steep) {
		swap(&row1,&column1);
		swap(&row2,&column2);
	} 
	if (row1 > row2) {
		swap(&row1,&row2);
		swap(&column1, &column2);
	}
	sprintf(debugMsg,"After: %d, %d; %d, %d",row1,column1,row2,column2);
	USART_sendString(debugMsg,true);

	deltaRow = row2 - row1;
	deltaColumn = abs(column2 - column1);
	if (column2 < column1) step = -1;
	
	//deltaError = ((double)deltaColumn/(double)deltaRow);
//	sprintf(debugMsg,"%e",deltaError*10);
//	USART_sendString(debugMsg,true);
	sumErrors = deltaRow/2;
	j = column1;
	for (i = row1; i <= row2; i++) {
		sprintf(debugMsg,"%d,%d",i,j);
		USART_sendString(debugMsg,true);
		if (steep) display_drawPixel(j,i,black);
		else display_drawPixel(i,j,black);
		
		sumErrors -= deltaColumn;
		if (sumErrors < 0) {
			j += step;
			sumErrors--;
		}

	}
	*/

	if (row1 == row2) {
		//Draw horizontal line
		if (column1 > column2) step = -1;
		j = column1;
		while (j != column2) {
			display_drawPixel(row1,j,black);
			j += step;
		}
		display_drawPixel(row1,j,black);
	
	} else if (column1 == column2) {
		//Draw vertical line
		if (row1 > row2) step = -1;
		i = row1;
		while(i != row2) {
			display_drawPixel(i,column1,black);
			i += step;
		}
		display_drawPixel(i,column1,black);
	} else {
		
		/* No angles yet...complain to Nolan or write it yourself :)
		
		
		
		//Angled line :P
		//Check out http://www.cs.unc.edu/~mcmillan/comp136/Lecture6/Lines.html
		//and http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
		//for a great explanation of Bresenham's algorithm
		//Basically, we're incrementing error by deltaError (the slope)
		//until we hit a threshold of .5,
		//where we increment the non-incremented variable and decrement error
		
		
		
		
		//Check if abs(slope) is "steep"
		deltaRow = row2 - row1;
		deltaColumn = column2 - column1;
		sumErrors = 0;
		if (abs(deltaRow) < abs(deltaColumn)) { 	//Slope < 1
			//Iterate by columns
			deltaError = (float)((float)deltaColumn/(float)deltaRow);
			//Still having problems with this deltaError value not being a float >.<
			//Play around with it some more?
			if (deltaError > -1) {
				USART_sendString(">-1",true);
				
			} else {
				USART_sendString("NOT",true);
			}
			
			i = row1;
			//deltaRow*deltaError = deltaColumn;
			if (column1 > column2) step = -1;
			for (j = column1; j != column2; j+= step) {
				sumErrors += deltaError;
				if (sumErrors >= .5) {
					sumErrors--;
					i++;
				}
				display_drawPixel(i,j,black);
			}
		} else {					  	//Slope >= 1
			//Iterate by rows so that "slope" is less than 1
			deltaError = deltaRow/deltaColumn;
			if (row2 > row1) step = -1;
			
			
		}
		*/
	}
}


void display_drawRectangle(uint8_t topRow, uint8_t leftColumn, uint8_t height, uint8_t width, bool filled, bool invertOrig, bool blackBorder) {
	uint8_t i,j, remainder, bottom, right, byteOut, page;

	if (!filled) {
		display_drawLine(topRow,leftColumn,topRow,leftColumn+width,blackBorder); //Top
		display_drawLine(topRow+height,leftColumn,topRow+height,leftColumn+width,blackBorder); //Bottom
		display_drawLine(topRow,leftColumn,topRow+height,leftColumn,blackBorder); //Left
		display_drawLine(topRow,leftColumn+width,topRow+height,leftColumn+width,blackBorder); //Right
	} else {
		i = topRow;
		bottom = topRow + height;
		right = leftColumn + width;
		while (i <= topRow + height) {
			remainder = i%8;
			if (i+(7-remainder) <= bottom) {
				//Deals with first page and middle pages
				byteOut = 0xFF << remainder;
			} else {
				//Final page is a little tricky
				byteOut = 0xFF >> (7-(bottom-i));
			}
			page = i/8;
			for (j = leftColumn; j <= right; j++) {
				if (invertOrig) {
					displayBuffer[page][j] = ((~(displayBuffer[page][j] & byteOut)) & byteOut) | (displayBuffer[page][j] & ~byteOut);
				} else {
					displayBuffer[page][j] = byteOut;	
				}
			}
			i += (8 - remainder);	
		}
	
	}
			
}

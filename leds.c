/*
 *  leds.c
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

#include "leds.h"

volatile uint8_t ledState[numberOfLeds]; 
volatile uint8_t ledCounter[numberOfLeds];

void Leds_Init(uint8_t led){
	Leds_Port.DIRSET = (1<<led);
	Leds_State(led, off);
}

void Leds_State(uint8_t led, uint8_t state){
	ledState[led] = state;
}

void Leds_Set(uint8_t led){
	Leds_Port.OUTSET = (1<<led);
}


void Leds_Clear(uint8_t led){
	Leds_Port.OUTCLR = (1<<led);
}


void Leds_Toggle(uint8_t led){
	Leds_Port.OUTTGL = (1<<led);	
}
/*
 *  leds.h
 *  BaseStation
 *
 *  Created by Joshua Schapiro on 7/11/11.
 *  Copyright 2011 Carnegie Mellon. All rights reserved.
 *
 */

#define off		0
#define on		1
#define slow	2
#define	fast	3

void Leds_Init(uint8_t led);
void Leds_State(uint8_t led, uint8_t state);
void Leds_Set(uint8_t led);
void Leds_Clear(uint8_t led);
void Leds_Toggle(uint8_t led);
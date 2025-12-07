#include "wiring_private.h"
#include "pins_arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>
#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 

int myAnalogRead(uint8_t pin){
	// pin -=14;
	ADMUX = (ADMUX & 0xF0) | (pin & 0x07);
	ADCSRA |= (1<<ADSC);
	while (	ADCSRA & (1<<ADSC));
	return ADC; //return ADC
}

#include "wiring_private.h"
#include "pins_arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>
#ifndef LIBPRINTF_H
#define LIBPRINTF_H
#include <LibPrintf.h>
#endif 
#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 
#include "portAssignments.h"
#include "globals.h"

void initialize(){
  pinMode(pot, INPUT_PULLUP);
  // DDRD = B11100000;
  // DDRB = B00000111;
  // PORTD = B1110000;
  // PORTB = B0000111;
  // delay(1500);
  PORTD = B0000000;
  PORTB = B0000000;
  DDRD = B00000000;
  DDRB = B00000000;
  #ifdef invertedHighside
    phaseAHighDDR |= (B00000000 | 1 << shiftAHigh);
    phaseBHighDDR  |= (B00000000 | 1 << shiftBHigh);
    phaseCHighDDR  |= (B00000000 | 1 << shiftCHigh);
  #else
    phaseAHighDDR &= ~(B00000000 | 1 << shiftAHigh);
    phaseBHighDDR &= ~(B00000000 | 1 << shiftBHigh);
    phaseCHighDDR &= ~(B00000000 | 1 << shiftCHigh);
  #endif
  phaseALowDDR &= ~(B00000000 | 1 << shiftALow);
  phaseBLowDDR &= ~(B00000000 | 1 << shiftBLow);
  phaseCLowDDR &= ~(B00000000 | 1 << shiftCLow);
  Serial.begin(115200);
  val = 111;
  Serial.println("reset success ig");
  // put your setup code here, to run once:
  lastTime = millis();
  // pinMode(11,OUTPUT);
  // digitalWrite(11,HIGH);
  // analogWrite(11,85);
  blockNumber = 0;
  delay(1500);
}
/*
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define _SFR_MEM_ADDR(sfr) ((uint16_t) &(sfr))
#define _SFR_IO_ADDR(sfr) (_SFR_MEM_ADDR(sfr) - __SFR_OFFSET)
#define _SFR_IO_REG_P(sfr) (_SFR_MEM_ADDR(sfr) < 0x40 + __SFR_OFFSET)

#define _SFR_ADDR(sfr) _SFR_MEM_ADDR(sfr)
#endif 
// #define _SFR_BYTE(sfr) _MMIO_BYTE(_SFR_ADDR(sfr))
#define _SFR_WORD(sfr) _MMIO_WORD(_SFR_ADDR(sfr))
#define _SFR_DWORD(sfr) _MMIO_DWORD(_SFR_ADDR(sfr))
*/
// int myAnalogRead(uint8_t pin){
// 	pin -= 14; // allow for channel or pin numbers
//   	ADMUX = (1 << 6) | (pin & 0x07);
// 	// start the conversion
// 	// (_MMIO_BYTE(_SFR_ADDR(ADCSRA)) |= _BV(ADSC));
// 	// (*(volatile uint8_t *)(_SFR_ADDR(ADCSRA)) |= _BV(ADSC));
// 	// (*(volatile uint8_t *)(_SFR_ADDR(ADCSRA)) |= _BV(ADSC));
// 	// (*(volatile uint8_t *)((uint16_t) &(ADCSRA)) |= _BV(ADSC));
// 	// *(volatile uint8_t *)((uint16_t) &(ADCSRA)) |= (1 << (ADSC));
// 	//_SFR_MEM8(0x7A)
// 	//*(volatile uint8_t *)(mem_addr)
// 	// *(volatile uint8_t *)(&ADCSRA) |= (1 << (ADSC));
// 	// *(volatile uint8_t *)(0x7A) |= (1 << (ADSC));
// 	// *(volatile uint8_t *)(0x7A) |= (1 << (ADSC));
// 	ADCSRA |= (1<<ADSC);

// 	// ADSC is cleared when the conversion finishes
// 	while (
// 		// bit_is_set(ADCSRA, ADSC) 
// 		// *(volatile uint8_t *)(0x7A) & (1<<(ADSC))
// 		// *(volatile uint8_t *)(0x7A) & (1<<6)
// 		ADCSRA & (1<<ADSC)
// 	);
// 	// ADC macro takes care of reading ADC register.
// 	// avr-gcc implements the proper reading order: ADCL is read first.
// 	// return *(volatile uint16_t *)(0x78);;;;;;
// 	return (0x78); //return ADC
// }
int myAnalogRead(uint8_t pin){
	// pin -=14;
  	ADMUX = (1 << 6) | (pin & 0x07);
	ADCSRA |= (1<<ADSC);
	while (	ADCSRA & (1<<ADSC));
	return (0x78); //return ADC
}
#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 
#ifndef LIBPRINTF_H
#define LIBPRINTF_H
#include <LibPrintf.h>
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
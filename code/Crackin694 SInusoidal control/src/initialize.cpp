#pragma once
#include <LibPrintf.h>
#include <Arduino.h>
#include "portAssignments.h"
#include "globals.h"

void initialize();      
void initialize(){
  pinMode(pot, INPUT_PULLUP);
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
  // pinMode(9,OUTPUT);
  // analogWrite(9,85);
}
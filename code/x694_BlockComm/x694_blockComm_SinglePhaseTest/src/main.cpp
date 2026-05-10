#include "globals.h"
#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);
int longDelay = 200;
void setup() {
  // put your setup code here, to run once:
pinMode(phaseAHighPort,OUTPUT);
pinMode(phaseALowPort,OUTPUT);
pinMode(phaseBHighPort,OUTPUT);
pinMode(phaseBLowPort,OUTPUT);
pinMode(phaseCHighPort,OUTPUT);
pinMode(phaseCLowPort,OUTPUT);
//phase to set to low
digitalWrite(phaseCLowPort,1);
}
void loop() {
digitalWrite(phaseAHighPort,1);
delay(longDelay);
digitalWrite(phaseAHighPort,0);
delayMicroseconds(32);
digitalWrite(phaseALowPort,1);
delay(longDelay);
digitalWrite(phaseALowPort,0);
delayMicroseconds(32);
}

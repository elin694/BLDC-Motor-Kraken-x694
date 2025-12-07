#pragma once
#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 
#ifndef LIBPRINTF_H
#define LIBPRINTF_H
#include <LibPrintf.h>
#endif 
//automatic port assignments for direct PORT and DDR control
//sinusoidal control    
//============================Pin definition============================
#define phaseAHigh 10 //pin 19 on "oscilloscope"
#define phaseBHigh 9//pin 18 on "oscilloscope"
#define phaseCHigh 8//pin 17 on "oscilloscope"
#define phaseALow 7//pin 16 on "oscilloscope"
#define phaseBLow 6//pin 15 on "oscilloscope"
#define phaseCLow 5//pin 14 on "oscilloscope"
#define pot A0

#if (phaseAHigh >= 8 && phaseAHigh <= 13)
  #define phaseAHighPort PORTB
  #define shiftAHigh (phaseAHigh - 8)
  #define phaseAHighDDR DDRB
#elif (phaseAHigh >= 0 && phaseAHigh <= 7)
  #define phaseAHighPort PORTD
  #define shiftAHigh phaseAHigh
  #define phaseAHighDDR DDRD
#endif
//=======================================

#if (phaseALow >= 8 && phaseALow <= 13)
#define phaseALowPort PORTB
#define shiftALow (phaseALow - 8)
#define phaseALowDDR DDRB
#elif (phaseALow >= 0 && phaseALow <= 7)
#define phaseALowPort PORTD
#define shiftALow phaseALow
#define phaseALowDDR DDRD
#endif
//=======================================

#if (phaseBHigh >= 8 && phaseBHigh <= 13)
#define phaseBHighPort PORTB
#define shiftBHigh (phaseBHigh - 8)
#define phaseBHighDDR DDRB
#elif (phaseBHigh >= 0 && phaseBHigh <= 7)
#define phaseBHighPort PORTD
#define shiftBHigh phaseBHigh
#define phaseBHighDDR DDRD
#endif
//=======================================
#if (phaseBLow >= 8 && phaseBLow <= 13)
#define phaseBLowPort PORTB
#define shiftBLow (phaseBLow - 8)
#define phaseBLowDDR DDRB
#elif (phaseBLow >= 0 && phaseBLow <= 7)
#define phaseBLowPort PORTD
#define shiftBLow phaseBLow
#define phaseBLowDDR DDRD
#endif
//=======================================

#if (phaseCHigh >= 8 && phaseCHigh <= 13)
#define phaseCHighPort PORTB
#define shiftCHigh (phaseCHigh - 8)
#define phaseCHighDDR DDRB
#elif (phaseCHigh >= 0 && phaseCHigh <= 7)
#define phaseCHighPort PORTD
#define shiftCHigh phaseCHigh
#define phaseCHighDDR DDRD
#endif
//=======================================
#if (phaseCLow >= 8 && phaseCLow <= 13)
#define phaseCLowPort PORTB
    #define shiftCLow (phaseCLow - 8)
    #define phaseCLowDDR DDRB
#elif (phaseCLow >= 0 && phaseCLow <= 7)
  #define phaseCLowPort PORTD
    #define shiftCLow phaseCLow
    #define phaseCLowDDR DDRD
#endif
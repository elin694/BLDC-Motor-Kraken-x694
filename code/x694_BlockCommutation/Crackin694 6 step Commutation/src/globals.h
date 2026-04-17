#pragma once
#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 
#ifndef LIBPRINTF_H
#define LIBPRINTF_H
#include <LibPrintf.h>
#endif 
extern const int electricalCycles;
extern const long printPeriod;
extern uint64_t lastTime;
extern uint32_t val; //how long to delay every phase
extern uint32_t onTime; 
extern uint32_t deadTime; 
extern int blockNumber;
extern int steps[6][3];
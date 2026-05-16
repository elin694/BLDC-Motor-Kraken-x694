#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <iostream>
#include <cmath> 
#include <stdio.h>
#include <stdarg.h>
#include "rom/ets_sys.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/gpio_struct.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "driver/mcpwm_prelude.h"

#define GLOBALS_H
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14

#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16

#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32

#define dataPin GPIO_NUM_21
#define clockPin GPIO_NUM_22
#define pot GPIO_NUM_35 // or 35
// #define adcChannel 34
#define adcChannel ADC_CHANNEL_7 // diagonal pairing with physical placement

#define inlineShuntC 36 //Vp 
#define inlineShuntA 39 //Vn

inline int steps[6][3] ={  {1,0,-1},  {0,1,-1},  {-1,1,0},  {-1,0,1},  {0,-1,1},  {1,-1,0}  }; 
constexpr int electricalCycles= 3; //constexpr is defineable compile time costant 
extern const long printPeriod;

//changing during runtime
extern uint64_t lastTime;
extern uint32_t onTime;  
inline int blockNumber =0; //VARIABLE AND CHANGES
extern adc_oneshot_unit_handle_t adcHandle;
inline float duty = .5;
inline int dir = 1; //or 5 to go in reverse
inline int currentSector = 0;

//i2c
extern i2c_master_bus_config_t busSetup;
extern i2c_master_bus_handle_t busHandle;
extern i2c_device_config_t as5600Setup;
extern i2c_master_dev_handle_t as5600Handle;
//as5600
constexpr uint8_t as5600Register = 0x36;
constexpr uint8_t as5600TargetRegister = 0x0e;
constexpr size_t as5600WriteSize = 1;
inline uint8_t as5600RawDataBuf[2];
constexpr size_t as5600ReadSize = 2;
// #define as5600DirPinHigh


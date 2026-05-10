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

extern const int electricalCycles;
extern const long printPeriod;
extern uint64_t lastTime;
extern uint32_t val; //how long to delay every phase
extern uint32_t onTime; 
extern uint32_t deadTime; 
extern int blockNumber;
extern int steps[6][3];
extern adc_oneshot_unit_handle_t adcHandle;
//i2c
extern i2c_master_bus_config_t busSetup;
extern i2c_master_bus_handle_t busHandle;
extern i2c_device_config_t as5600Setup;
extern i2c_master_dev_handle_t as5600Handle;


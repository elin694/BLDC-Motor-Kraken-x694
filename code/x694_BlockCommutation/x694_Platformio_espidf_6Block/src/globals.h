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
// #include "hal/adc_types.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/gpio_struct.h"

#define led_14 GPIO_NUM_14
#define GLOBALS_H
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14
#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16
#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32
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


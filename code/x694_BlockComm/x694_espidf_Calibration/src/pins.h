#pragma once
#include <stdio.h>
#include <cmath> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "driver/mcpwm_prelude.h"
#include "soc/mcpwm_struct.h"
#include "esp_intr_alloc.h"
#include "esp_adc/adc_oneshot.h"
#include <string>
#include <cinttypes>
#include <atomic>
#include "ANSI.h" //========
#include "esp_log.h"
#include "esp_timer.h"
#include <iostream>
#include "rom/ets_sys.h"
#include <esp_rom_sys.h> 
#include "ANSI.h"
#include "esp_cpu.h"


#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14
#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16
#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32
#define freePort1 GPIO_NUM_18
#define freePort2 GPIO_NUM_19
#define clockPin GPIO_NUM_22
#define dataPin GPIO_NUM_21

gpio_num_t gateArray[6]= { phaseAHighPort, phaseALowPort, phaseBHighPort, phaseBLowPort, phaseCHighPort, phaseCLowPort};

void runOnESPTimerIntr(void * parameter);
void setup(void * parameter);
void initializeHighGate(uint32_t startingGateCmpValue);
void initializeLowGate();
void pinSetup();
void debug(void*parameter);
void as5600initialize(void * parameter) ;
void getSectorNumber(void*parameter);

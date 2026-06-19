#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <iostream>
#include <cmath> 
#include <stdio.h>
#include <stdarg.h>
#include "rom/ets_sys.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <esp_rom_sys.h> 

#include "driver/mcpwm_prelude.h"


//PORTS
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14

#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16

#define phaseCHighPort GPIO_NUM_26
// #define phaseCHighPort GPIO_NUM_19
#define phaseCLowPort GPIO_NUM_32
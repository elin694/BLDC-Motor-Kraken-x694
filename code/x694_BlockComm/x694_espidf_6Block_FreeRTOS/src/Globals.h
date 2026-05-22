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


//CHANGE ASSOCIATED PORT SET AND CLEAR
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14
#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16
#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32

volatile uint32_t *const PORT_SET[6]     =  { (volatile uint32_t *)&GPIO.out1_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out1_w1ts };
volatile uint32_t *const PORT_CLEAR[6] =  { (volatile uint32_t *)&GPIO.out1_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out1_w1tc};
constexpr uint32_t portShift[6] = { (1<<(phaseAHighPort-32)), (1<<phaseALowPort), (1<<phaseBHighPort), (1<<phaseBLowPort), (1<<phaseCHighPort), (1<<(phaseCLowPort-32))};
// *deref
#define dataPin GPIO_NUM_21
#define clockPin GPIO_NUM_22
#define pot GPIO_NUM_35 // or 35
// #define adcChannel 34
#define adcChannel ADC_CHANNEL_7 // diagonal pairing with physical placement

#define inlineShuntC 36 //Vp 
#define inlineShuntA 39 //Vn

#ifdef as5600DirPinHigh
#define as5600CalibratedOffset static_cast<uint16_t>(-(2107-(4095.0/3)) + 30.0 *(4095/3)/360);
 //2107 bit at c high a low (block 3#3 )with DIR  @5V
#else
 #define as5600CalibratedOffset static_cast<uint16_t>(-((4096-2107)-(4095.0/3)) + 30.0 *(4095/3)/360); 
#endif

constexpr gpio_num_t gateArray[6]= {phaseAHighPort, phaseALowPort, phaseBHighPort, phaseBLowPort, phaseCHighPort, phaseCLowPort};
constexpr int steps[6][3] ={ {-1,1,0}, {-1,0,1}, {0,-1,1}, {1,-1,0}, {1,0,-1}, {0,1,-1} }; 
// constexpr int gateLevel[6][3][2] = { //ah al bh bl ch cl
//     {{0,1}, {1,0}, {0,0}},
//     {{0,1}, {0,0}, {1,0}},
//     {{0,0}, {0,1}, {1,0}},
//     {{1,0}, {0,1}, {0,0}},
//     {{1,0}, {0,0}, {0,1}},
//     {{0,0}, {1,0}, {0,1}},
// };

constexpr int gateLevel[6][6] = { //ah al bh bl ch cl
    {0, 1, 1, 0, 0, 0},
    {0, 1, 0, 0, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 1},
    {0, 0, 1, 0, 0, 1},
};
constexpr int electricalCycles = 3; //constexpr is defineable compile time costant 

#define timerResolution  static_cast<uint32_t>(8e6) //125ns

//changing during runtime
extern adc_oneshot_unit_handle_t adcHandle;
extern uint64_t lastTime;
inline int blockNumber =0; //VARIABLE AND CHANGES
inline float duty = .5;
inline int dir = 1; //or 5 to go in reverse
inline int currentSector = 0;
// extern uint32_t onTime;  //in microseconds
inline uint32_t blockPeriod =  static_cast<uint32_t>(timerResolution/1000); //TBD
// 8 million ticks per second = 8000 ticks per period * 100 period
// timer rez = ticks per period * periods/second 

//i2c
extern i2c_master_bus_config_t busSetup;
extern i2c_master_bus_handle_t busHandle;
extern i2c_device_config_t as5600Setup;
extern i2c_master_dev_handle_t as5600Handle;
//as5600
constexpr uint8_t as5600Set = 0x36;
constexpr uint8_t as5600TargetRegister = 0x0e;
constexpr size_t as5600WriteSize = 1;
inline uint8_t as5600RawDataBuf[2];
constexpr size_t as5600ReadSize = 2;
// #define as5600DirPinHigh


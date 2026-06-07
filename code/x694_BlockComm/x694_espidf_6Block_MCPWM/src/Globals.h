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

#define ticksToµs static_cast<float>((1e6)/timerResolution)
#define µsToTicks static_cast<float>(timerResolution/1e6) //ontime * this = tick = 8
#define µsToTicksInt static_cast<int>(timerResolution/1e6) //ontime * this = tick

//====================FUNCTION DECLARATION =======================
 
#define black "\033[30m"
#define red "\033[31m"
#define green "\033[32m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[35m"
#define cyan "\033[36m"
#define white "\033[37m"
#define esc "\033[0m"

void readPotRepeat(void * parameter);
void readPotOnce(void * parameter);
const char* color(std::string str, std::string clr); 

inline int ledD =0;
//CHANGE ASSOCIATED PORT SET AND CLEAR
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14
// #define phaseBHighPort GPIO_NUM_2
#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_2
#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32

volatile uint32_t *const PORT_SET[6]     =  { (volatile uint32_t *)&GPIO.out1_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out1_w1ts };
volatile uint32_t *const PORT_CLEAR[6] =  { (volatile uint32_t *)&GPIO.out1_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out1_w1tc};
constexpr uint32_t portShift[6] = { (1<<(phaseAHighPort-32)), (1<<phaseALowPort), (1<<phaseBHighPort), (1<<phaseBLowPort), (1<<phaseCHighPort), (1<<(phaseCLowPort-32))};
// *deref
#define dataPin GPIO_NUM_21
#define clockPin GPIO_NUM_22
#define pot GPIO_NUM_35 // or 35
#define inlineShuntC 36 //Vp 
#define inlineShuntA 39 //Vn
// #define adcChannel 34
#define adcChannel ADC_CHANNEL_7 // diagonal pairing with physical placement

#ifdef as5600DirPinHigh
#define as5600CalibratedOffset static_cast<uint16_t>(-(2107-(4095.0/3)) + 30.0 *(4095/3)/360);
 //2107 bit at c high a low (block 3#3 )with DIR  @5V
#else
 #define as5600CalibratedOffset static_cast<uint16_t>(-((4096-2107)-(4095.0/3)) + 30.0 *(4095/3)/360); 
#endif

//++++++++++++++++++++++++++++++MCPWM++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++MCPWM++++++++++++++++++++++++++++++
   /*You can Probably Change*/
   #define estimatedI2CReadTimeInMicros 170
   #define timerResolution  static_cast<uint32_t>(1e5) //125ns , must not simple ratio
   #define activePwmPeriod static_cast<uint32_t>(timerResolution/10000)  //change to 20khz when high


    #define startingDuty .2
    #define minDutyHigh .05

    #define startingGateCmpValue static_cast<uint32_t>(startingDuty*activePwmPeriod) //High gate comparator's comparatorValue when ON; can be modified later
    #define offGateCmpValue static_cast<uint32_t>(minDutyHigh*activePwmPeriod) //comparatorValue when OFF, modify this when switching
    
    #define highDefaultPWMPeriod static_cast<uint32_t>(.05 * activePwmPeriod)
    //edit phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(); in gateControlCpp 

    /*DO NOT CHANGE VALUE*/
    constexpr gpio_num_t gateArray[6]= {phaseAHighPort, phaseALowPort, phaseBHighPort, phaseBLowPort, phaseCHighPort, phaseCLowPort};
    constexpr int electricalCycles = 3; //constexpr is defineable compile time costant 
    inline mcpwm_timer_handle_t blockTimer=NULL;
    inline mcpwm_timer_handle_t globalLowTimer =NULL;
    #define highSideGroup 1 //used in isr intr_source
    #define lowSideGroup 0
    constexpr int steps[6][3] ={ {-1,1,0}, {-1,0,1}, {0,-1,1}, {1,-1,0}, {1,0,-1}, {0,1,-1} }; 
    constexpr float lowGateLevelCycle[6] = {
        (float)(2/3.0), 1.0f, (float)(2/3.0), (float)(1/3.0), 0.0f, (float)(1/3.0) 
    };
    //given index of current sector, tells which phase is high
    constexpr int activeHighGate[6]= {1,2,2,0,0,1};
    // constexpr int activeLowGate[6]= {1,2,2,0,0,1};

    //in gateControl.cpp
    constexpr int gateLevelCycle[6][6] = { //ah al bh bl ch cl
        {0, 1, 1, 0, 0, 0}, //block 0,  HLHLHL
        {0, 1, 0, 0, 1, 0},
        {0, 0, 0, 1, 1, 0},
        {1, 0, 0, 1, 0, 0},
        {1, 0, 0, 0, 0, 1},
        {0, 0, 1, 0, 0, 1},
    };
    constexpr mcpwm_timer_direction_t LTimerDir[6] ={
        MCPWM_TIMER_DIRECTION_UP,
        MCPWM_TIMER_DIRECTION_UP, //NULL
        MCPWM_TIMER_DIRECTION_DOWN,
        MCPWM_TIMER_DIRECTION_DOWN,
        MCPWM_TIMER_DIRECTION_DOWN, //null
        MCPWM_TIMER_DIRECTION_UP,
    };
//+++++++++++++++++++++++++++++++++++RUNTIME VARIABLES+++++++++++++++++++++++++++++++++++
extern adc_oneshot_unit_handle_t adcHandle;
inline float duty = .5;
inline int dir = 1; //or 5 to go in reverse (preload dep on 5) (1 for half working AS5600)
inline bool newFrequency = false;
// timer rez = ticks per period * periods/second 

//Global Glabal Variables
typedef struct{
    float CMR_value_3[4];//impleemnt
    // ^^^^^^^^^^^^^^^^^^^^^
    volatile uint32_t oldSectorTarget =- 1010;
    volatile int sectorTarget = -1000; //for stator current vector
    volatile uint32_t blockPeriod= static_cast<uint32_t>((131072/2)/6); 
    //21845 is the maximum since 131073 is probably the maximum
    uint32_t BTimerPhaseShift;
} gVar_t;

inline gVar_t global;
inline volatile uint32_t counter =10;
inline volatile uint32_t isrCounter2 =10;
inline volatile uint32_t isrGroupCounter =10;

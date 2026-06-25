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
/*=============================DEBUG CONTROL PANEL=============================*/
// #define debug_testOnLED 
// #define debug_testBigBreadboardTestPins
// #define debug_fastPrints //isr indicator and BLOCK#
#define debug_printRPS 

/*IN MAIN.CPP DELAY, MOSTLY SPAM*/
// #define debug_spamPrintCounterStatus
// #define debug_spamDelay 2
// #define debug_spamPrintTimeISR1 //print how long it takes to do i2c transmit recieve+prelo8ad

#define debug_RPSprint_period (int)(1000) //affect mtr sim rate
// #define debug_dontReadVelocityPot 8000
// #define debug_useLookUpTableADC //commenout out to keep the Bperiod

/*=============================USER SETTING CONTROL PANEL=============================*/
#define enableReadPotRepeat
// #define as5600DirPinHigh
#define toggleTurnCW
#define i2cClockSpeed 800'000
#define velPotReadPeriod (int)(128) //set velocity via pot 1
#define SetAs5600PollPeriod 8000
#define preCompStartingTargetSector 1
#define timerResolution  static_cast<uint32_t>(16e7/40) //125ns , must not simple ratio
#define VTimerResolution  static_cast<uint32_t>(16e7/400) //125ns , must not simple ratio
/*ALSO CHANGE HARD CODED PRESCALERS*/
#define mcpwm_lowSideGroupPrescaler 40

inline bool motorStall =false;
inline DRAM_ATTR int isr2CurrentTime =0;
inline DRAM_ATTR int isr2CurrentCounter =0;
inline DRAM_ATTR bool isr2CurrentCounterCounted =0;
/*minimum and maximum RPS */
#define fMin static_cast<float>(VTimerResolution/(18.0f*65535)) 
#define fMax static_cast<float>(VTimerResolution/(18.0f*200))

//++++++++++++++++++++++++++++++MCPWM++++++++++++++++++++++++++++++
#define i2cWaitout 15//in ms
#define estimatedI2CReadTimeInMicros static_cast<uint32_t>(200)
#define estimatedI2CReadTimeInTicks static_cast<uint32_t>(ceil(estimatedI2CReadTimeInMicros/ticksToµs))
#define activePwmPeriod static_cast<uint32_t>(timerResolution/20000)  //change to 20khz when high
    #define startingDuty static_cast<float>(1- .3) //The Duty cycle is 1 - this.Value
    #define startingGateCmpValue static_cast<uint32_t>(startingDuty*activePwmPeriod/2.0) //High gate comparator's comparatorValue when ON; can be modified later

    #ifdef debug_testBigBreadboardTestPins
        #define phaseAHighPort GPIO_NUM_2
        #define phaseALowPort GPIO_NUM_4
        #define phaseBHighPort GPIO_NUM_16
        #define phaseBLowPort GPIO_NUM_17
        #define phaseCHighPort GPIO_NUM_18
        #define phaseCLowPort GPIO_NUM_19
    #else
    #if defined(debug_testOnLED)
        #define phaseAHighPort GPIO_NUM_14
        #define phaseALowPort GPIO_NUM_13
        #define phaseBHighPort GPIO_NUM_26
        #define phaseBLowPort GPIO_NUM_25
        #define phaseCHighPort GPIO_NUM_33
        #define phaseCLowPort GPIO_NUM_32
    #else //real PCB
        #define phaseAHighPort GPIO_NUM_33
        #define phaseALowPort GPIO_NUM_14
        #define phaseBHighPort GPIO_NUM_17
        #define phaseBLowPort GPIO_NUM_16
        #define phaseCHighPort GPIO_NUM_26
        #define phaseCLowPort GPIO_NUM_32

    //CHANGE ASSOCIATED PORT SET AND CLEAR
    // // volatile uint32_t *const PORT_SET[6]     =  { (volatile uint32_t *)&GPIO.out1_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts, (volatile uint32_t *)&GPIO.out_w1ts };
    // // volatile uint32_t *const PORT_CLEAR[6] =  { (volatile uint32_t *)&GPIO.out1_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc, (volatile uint32_t *)&GPIO.out_w1tc};
    // constexpr uint32_t portShift[6] = { (1<<(phaseAHighPort-32)), (1<<phaseALowPort), (1<<phaseBHighPort), (1<<phaseBLowPort), (1<<phaseCHighPort), (1<<(phaseCLowPort))};
    #endif
    #endif

//+++++++++++++++++++++++++++++++++++RUNTIME VARIABLES+++++++++++++++++++++++++++++++++++
typedef struct{
    // ^^^^^^^^^^^^^^^^^^^^^
    volatile uint32_t oldSectorTarget = preCompStartingTargetSector;
    volatile int sectorTarget =preCompStartingTargetSector; ; //for stator current vector
    // BLOCK CYCLING: / 0-RS, 1 BS, 2 RS, 3 RF), 4: BF, 5: RF
    // volatile uint32_t blockPeriod= static_cast<uint32_t>(((131072)/2)/6); 
    volatile uint32_t blockPeriod = 65535;
    volatile bool newVelPotValue = false;
    volatile bool newPhaseSwitchFlag = false;
    volatile bool readAS5600 = false;
    volatile uint32_t rotorVal =0;
} gVar_t;
extern adc_oneshot_unit_handle_t adcHandle;
inline portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;
// timer rez = ticks per period * periods/second 
DRAM_ATTR inline gVar_t global;


    /*DO NOT CHANGE VALUE*/
    #define electricalCycles 3 //constexpr is defineable compile time costant 
    inline mcpwm_timer_handle_t blockTimer=NULL;
    inline mcpwm_timer_handle_t globalLowTimer =NULL;
    inline mcpwm_timer_handle_t velocityTrackerTimer =NULL;
    #define highSideGroup 1 //used in isr intr_source
    #define lowSideGroup 0

    // constexpr int steps[6][3] ={ {-1,1,0}, {-1,0,1}, {0,-1,1}, {1,-1,0}, {1,0,-1}, {0,1,-1} }; 
    DRAM_ATTR constexpr uint32_t lowGateLevelCycle[6] = {
        // (float)(2/3.0), 1.0f, (float)(2/3.0), (float)(1/3.0), 0.0f, (float)(1/3.0) 
        2,3,2,1,0,1
    };
    constexpr int activeHighGate[6]= {1,2,2,0,0,1}; //given index of current sector, tells which phase is high
    // constexpr int activeLowGate[6]= {0,0,1,1,2,2}; //given index of current sector, tells which phase is high
    DRAM_ATTR constexpr int gateLevelCycle[6][6] = { //ah al bh bl ch cl
        {0, 1, 1, 0, 0, 0}, //block 0,  HLHLHL
        {0, 1, 0, 0, 1, 0},
        {0, 0, 0, 1, 1, 0},
        {1, 0, 0, 1, 0, 0},
        {1, 0, 0, 0, 0, 1},
        {0, 0, 1, 0, 0, 1}
    };
void readPotRepeat(void * parameter);
void readPotOnce(void * parameter);
void getTimerCountNow(const char* str);
void spamSearchCV(void *parameter);

constexpr gpio_num_t gateArray[6]= {phaseAHighPort, phaseALowPort, phaseBHighPort, phaseBLowPort, phaseCHighPort, phaseCLowPort};
#define dataPin GPIO_NUM_21 //i2c data yellow, 21 
#define clockPin GPIO_NUM_22 //i2c clock
#define pot GPIO_NUM_35 // or 35
#define inlineShuntC 36 //Vp 
#define inlineShuntA 39 //Vn
// #define adcChannel 34
#define adcChannel ADC_CHANNEL_7 // diagonal pairing with physical placement

//assume that calibrated value CHAL at dir Pin low give 2107
//top view of physical motor has ABC going ccw
#define as5600CalibrationRawValue (3388)
#define as5600CalibratedOffset static_cast<int>((4096.0)*(38.0/36.0) - (4096-as5600CalibrationRawValue) /*remove mutliples of 1 electrical cycle*/)  
#ifdef as5600DirPinHigh
#define getRotorValAdjusted(x) (as5600CalibratedOffset+x)
#else
#define getRotorValAdjusted(x) ((4096-x)+as5600CalibratedOffset)
#endif
#ifdef toggleTurnCW
inline int dir = 5; //or 5 to go in reverse (preload dep on 5) (1 for half working AS5600)
#else
inline int dir = 1; 
#endif
//====================FUNCTION DECLARATION =======================
inline TaskHandle_t setupTask= NULL;
inline TaskHandle_t getSectorNumberTask= NULL;
DRAM_ATTR constexpr const char * ghgl[6] = {"BAu2","CAd3","CBd2","ABd1","ACu0","BCu1"};
#define ticksToµs static_cast<float>((1e6)/timerResolution)
#define µsToTicks static_cast<float>(timerResolution/1e6) //ontime * this = tick = 8
#define µsToTicksInt static_cast<int>(timerResolution/1e6) //ontime * this = tick

#define black "\033[30m"
#define red "\033[31m"
#define green "\033[32m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[35m"
#define cyan "\033[36m"
#define white "\033[37m"
#define esc "\033[0m"

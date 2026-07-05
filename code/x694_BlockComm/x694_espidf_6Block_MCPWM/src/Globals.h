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
#include "ANSI_escape_sequences.h"
/*=============================DEBUG CONTROL PANEL=============================*/
// #define debug_testOnLED 
// #define debug_testBigBreadboardTestPins
// #define debug_fastPrints //isr indicator and BLOCK#
// #define debug_hyperFastPrints
#define debug_hyperFastPrintsWithPot //toggles on Blok Period printing
volatile inline DRAM_ATTR const char* darray[10000];
volatile inline DRAM_ATTR std::atomic<uint32_t> dindex[]={0,0}; //new, old
volatile inline DRAM_ATTR std::atomic<int> as5600BfieldVectorSector =0;

#define debug_printRPS 
#define velPotReadPeriod (int)(100) //set velocity via pot 1
/*IN MAIN.CPP DELAY, MOSTLY SPAM*/
// #define debug_spamPrintCounterStatus
// #define debug_spamDelay 2
#define debug_spamPrintTimeISR1 //print how long it takes to do i2c transmit recieve+prelo8ad

#define debug_RPSprint_period (int)(1000) //affect mtr sim rate
// #define debug_dontReadVelocityPot 22133 //affect block period
// #define debug_useLookUpTableADC //DEPRECATED
/*Current loop: 

initialize ... --> isr3--> isr1[pass,getSectorNumber] --> preloadGates] --> optimally minimal delay--> isr2[pass, when newPhaseSwitch flag -->executeGates ] 

*/
/*=============================USER SETTING CONTROL PANEL=============================*/
#define enableReadPotRepeat
// #define as5600DirPinHigh
#define startingDuty static_cast<float>(1- .9 ) //The Duty cycle is 1 - this.Value, normally .8
#define estimatedI2CReadTimeInMicros static_cast<uint32_t>(170)
// #define i2cClockSpeed 950000
#define i2cClockSpeed 1000000
// #define i2cClockSpeed 90000
#define i2cWaitout 1 //in ms
#define SetAs5600PollPeriod 1000 //period ticks
#if (estimatedI2CReadTimeInTicks > SetAs5600PollPeriod)
#warnings "SetAs5600PollPeriod too brief; shorter than i2c read time"
#endif
#define preCompStartingTargetSector 1
/*ALSO CHANGE HARD CODED PRESCALERS*/
#define mcpwm_lowSideGroupPrescaler 40
#define timerResolution  static_cast<uint32_t>(16e7/mcpwm_lowSideGroupPrescaler) //125ns , must not simple ratio
#define VTimerResolution  static_cast<uint32_t>(16e7/(mcpwm_lowSideGroupPrescaler*10)) //125ns , must not simple ratio

/*minimum and maximum RPS */
#define maxf_HTimerPeriod (1111) //200--> 111.11rps
#define minf_HTimerPeriod (uint32_t)(65535/2)
#define fMin static_cast<float>(VTimerResolution/(18.0f*-maxf_HTimerPeriod))
// #define fMin static_cast<float>(VTimerResolution/(18.0f*minf_HTimerPeriod))
#define fMax static_cast<float>(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 

#define aMin static_cast<float>(VTimerResolution/(18.0f*-maxf_HTimerPeriod))
#define aMax static_cast<float>(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 

#define pMin static_cast<float>(0)
#define pMax static_cast<float>(3*3.141592653/2)


// inline bool motorStall =false;
inline DRAM_ATTR int isr2CurrentTime =0; //t1
inline DRAM_ATTR int isr2CurrentTime2 =0; //t1
inline DRAM_ATTR int isr2CurrentCounter =0;
inline DRAM_ATTR bool isr2CurrentCounterCounted =0;
//++++++++++++++++++++++++++++++MCPWM++++++++++++++++++++++++++++++
#define estimatedI2CReadTimeInTicks static_cast<uint32_t>(ceil(estimatedI2CReadTimeInMicros/ticksToµs))
#define activePwmPeriod static_cast<uint32_t>(timerResolution/20000)  //change to 20khz when high
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
typedef enum {
    POSITION_CONTROL,
    VELOCITY_CONTROL,
    TORQUE_CONTROL
} control_type;

constexpr float kPID[3][3] = {
    { 1, 1, 1 }, /*Position*/
    { 1.1, 0,0 }, /*Velocity {kp, ki, kd}*/
    { 1, 1, 1 } /*Acceleration*/
};

typedef struct{
    uint32_t oldSectorTarget = preCompStartingTargetSector;
    int sectorTarget =preCompStartingTargetSector; ; //for stator current vector
    uint32_t blockPeriod = 65535;
    std::atomic<bool> newVelPotValue = false;
    std::atomic<bool> newPhaseSwitchFlag = false;
    std::atomic<bool> readAS5600 = false;
    std::atomic<bool> setMotorFreeSpin = false;
    std::atomic<bool> setMotorFreeTemporarily = false;
    float targetPosition =0; //target RPS
    float targetVelocity =0; //target RPS
    float targetAcceleration =0; //target RPS
    /*Measured Values*/
    uint32_t rotorVal =0;
    uint32_t measuredPositions[3] ={0,0,0}; //recent values at the front
    float measureVelocities[2] ={0,0};
    float measureAccelerations[1] ={0};
    control_type controlMethod = VELOCITY_CONTROL;
    int dir = 2; // 4=cw (-), 2 for ccw(+) (2 for half working AS5600)
} gVar_t;
volatile DRAM_ATTR inline gVar_t global;
inline uint32_t file1 =0;
extern adc_oneshot_unit_handle_t adcHandle;
inline portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;

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

//calibrated value CHAL at dir Pin low give 3388
//top view of physical motor has ABC going ccw, [-30 degrees, 30 degrees) = block 0
#define as5600CalibrationRawValue (3388) //38 not 37 because +0.5 and trucnate = round up,30degrees to sector_per_bits is only .5, not 1.
#define as5600CalibratedOffset static_cast<int>((4096.0)*(38.0/36.0) - (4096-as5600CalibrationRawValue) /*remove mutliples of 1 electrical cycle*/)  
#ifdef as5600DirPinHigh //not during calibration
#define getRotorValAdjusted(x) (as5600CalibratedOffset+x)
#else
#define getRotorValAdjusted(x) ((4096-x)+as5600CalibratedOffset)
#endif
//====================FUNCTION DECLARATION =======================
inline TaskHandle_t setupTask= NULL;
inline TaskHandle_t getSectorNumberTask= NULL;
// DRAM_ATTR constexpr const char* ghgl[6] = {"0BAu2","1CAd3","2CBd2","3ABd1","4ACu0","5BCu1"};
DRAM_ATTR constexpr const char* ghgl[6] = {"0BA ","1CA ","2CB ","3AB ","4AC ","5BC "}; //[-30,30) = block 0
#ifdef debug_hyperFastPrints
DRAM_ATTR constexpr const char* dgdir[6] = {"∅","D?","+","D?","NOT-","-"};
#endif
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

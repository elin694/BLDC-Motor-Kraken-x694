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

/*#################### TUNEABLES #################### */
#define ACCEPTABLE_I2C_READ_WINDOW 230
#define estimatedI2CReadTime_us (uint32_t)(200) //694
#define velPotReadPeriod (int)(20) //set velocity via pot 1
#define i2cClockSpeed 1250000
#define as5600CalibrationRawValue (1916) //38 not 37 because +0.5 and trucnate = round up,30degrees to sector_per_bits is only .5, not 1.
#define adcReadBufferSize 4
#define cBufSize 8



/* #################### MOTOR LIMITATIONS #################### */
/* ========================= MOTOR SOFTWARE LIMITS ========================= */
/*set to motor physical limits*/
#define maxDuty 0.95f
#define minDuty 0.03f


/* ========================= USER INPUT BOUNDS ========================= */
/*minimum and maximum RPS */
#define maxRPS (50)
#define maxf_HTimerPeriod (VTimerResolution/(maxRPS*18)) //200--> 111.11rps, 1111-->20rps
#define minf_HTimerPeriod (uint32_t)(65535/2)
// #define fMin (float)(VTimerResolution/(18.0f*minf_HTimerPeriod))
#define fMin (float)(VTimerResolution/(-18.0f*maxf_HTimerPeriod))
#define fMax (float)(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 
#define aMin (float)(VTimerResolution/(-18.0f*maxf_HTimerPeriod))
#define aMax (float)(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 
#define pMin (float)(0)
#define pMax (float)(3*3.141592653/2)



/* #################### INTERRUPT PRIORITY #################### */
#define MCPWM_HighsideIntrPriority 1 //tep,tez
#define MCPWM_LowsideIntrPriority 2 //tep,tez
#define runOnMCPWMIntrPriority ESP_INTR_FLAG_LEVEL2 //might be a bit long
#define i2c_intrPriority 3
/*esp timer intr : 1-3, (2)
freertos timer :lvl 1 or 3 (1)
watchdog and sys checks :4 or 5 */



/*#################### SHORTHANDS #################### */
/* ========================= FUNCTION SHORTHANDS ========================= */
#define SNAP() esp_timer_get_time()
#define time240() esp_cpu_get_cycle_count()
#define snap() time240()
#define print(x) esp_rom_printf(x)
#define CLEAR_ALL_NOTIFS(x) (ulTaskNotifyValueClear(x, 0xffffffff) ||  xTaskNotifyStateClear(x) )


/* ========================= CONSTANTS SHORTHANDS ========================= */
#define MCPWMx ((mcpwm_dev_t * ) &MCPWM0)
#define electricalCycles 18 //constexpr is defineable compile time costant 
#define SECTOR_PER_BITS (float)(electricalCycles / 4096.0f)
#define ExecuteGate_FreeSpin_NotifVal 0x0000FFFF
#define i2cWaitout 1 //in ms
#define initializationLatency pdMS_TO_TICKS(30)


/* ------------------------------ MCPWM SHORTHANDS------------------------------ */
#define mcpwm_lowSideGroupPrescaler 40
#define HighTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler)) 
#define activePwmPeriod (uint32_t)(HighTimerResolution/20000)  //change to 20khz when high
#define startingGateCmpValue (uint32_t)((1-startingDuty)*activePwmPeriod/2.0) //High gate comparator's comparatorValue when ON; can be modified later
// #if ((startingDuty < minDuty) || (startingDuty > maxDuty))
// #warning "DUTY out of bounds!!!!!!!!!!!!!!!!!!!!!!!!!"
// #endif
#define VTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler*10)) 
// constexpr int steps[6][3] ={ {-1,1,0}, {-1,0,1}, {0,-1,1}, {1,-1,0}, {1,0,-1}, {0,1,-1} }; 
// constexpr int activeLowGate[6]= {0,0,1,1,2,2}; //given index of current sector, tells which phase is high
// constexpr int activeHighGate[6]= {1,2,2,0,0,1}; //given index of current sector, tells which phase is high
DRAM_ATTR constexpr int gateLevelCycle[6][6] = { //ah al bh bl ch cl
    {0, 1, 1, 0, 0, 0}, //block 0,  HLHLHL
    {0, 1, 0, 0, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 1},
    {0, 0, 1, 0, 0, 1}
};

/* ========================= PIN (SHORTHANDS)========================= */
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14
#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16
#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32
constexpr gpio_num_t gateArray[6]= {phaseAHighPort, phaseALowPort, phaseBHighPort, phaseBLowPort, phaseCHighPort, phaseCLowPort};
#define dataPin GPIO_NUM_21 //i2c data yellow, 21 
#define clockPin GPIO_NUM_22 //i2c clock
#define pot GPIO_NUM_35 // or 35
#define inlineShuntC 36 //Vp 
#define inlineShuntA 39 //Vn
#define potL 34
#define adcChannel ADC_CHANNEL_7 // diagonal pairing with physical placement

/* 
####################
=========================
------------------------------

//tracking all interstate variables
tag - darray [dindex #W]  #W
    int oldSectorTarget = 0;
    int sectorTarget = 0; //for stator current vector
    std::atomic<uint32_t> blockPeriod = 10000.0; //6941
    std::atomic<uint32_t> tlog_readAS5600 = 0;
    std::atomic<bool> setMotorFreeSpin = false;
    std::atomic<bool> setMotorFreeTemporarily = false;
    int dir = 5; 
    control_type controlMethod = VELOCITY_CONTROL;
    int rotorVal =0; //needs to inversted
    float targetPosition =0; //target Position in bits
    float targetVelocity =0; //target RPS
    float targetAcceleration =0; //target RPS

runOnESPTimerIntr( ) --> getSectorNumberTask(
    --------------READ ONLY (CORE ONE)--------------
    global.dir #R
    --------------WRITE (CORE ONE)--------------
    as5600RawDataBuf #WR ++
    isr2i, #W ++
    
    global.oldSectorTarget #W
    global.sectorTarget #WR
    global.tlog_readAS5600 #W
    global.setMotorFreeTemporarily #W
    global.rotorVal #W
)

VTIMER TEZ --> VTimerCallback/runOnMCPWMIntr(tempStatusReg #WR) --> runActualISR( 
--------------READ ONLY--------------
    global.tlog_readAs5600, #R
)--> executeGatesTask(
    --------------READ ONLY--------------
    
    global.sectorTarget #R
    global.setMotorFreeSpin, #R
    global.setMotorFreeTemporarily, #R
)
//===============LOOPED TASKS####################==
debugMonitor(
--------------READ ONLY--------------
    isr2i ++
    rawData

    global.rotorVal
    global.blockPeriod
    global.setMotorFreeSpin
    global.setMotorFreeTemporarily
    global.targetVelocity
) #R 

readPotRepeat(
    --------------READ ONLY--------------
    global.controlMethod, #R
    --------------WRITE--------------
    rawData, #WR

    global.blockPeriod #WR
    global.setMotorFreeSpin #W
    global.dir #W
    global.targetVelocity #W
    global.targetAcceleration #W
    global.targetPosition #W
    
)
*/
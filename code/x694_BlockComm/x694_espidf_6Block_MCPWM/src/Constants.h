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


#define estimatedI2CReadTime_us (uint32_t)(200) //694
#define i2cClockSpeed 1250000
#define i2cWaitout 1 //in ms
#define mcpwm_lowSideGroupPrescaler 40
#define HighTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler)) //125ns , must not simple ratio
#define VTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler*10)) //125ns , must not simple ratio

// constexpr int steps[6][3] ={ {-1,1,0}, {-1,0,1}, {0,-1,1}, {1,-1,0}, {1,0,-1}, {0,1,-1} }; 
// constexpr int activeLowGate[6]= {0,0,1,1,2,2}; //given index of current sector, tells which phase is high
constexpr int activeHighGate[6]= {1,2,2,0,0,1}; //given index of current sector, tells which phase is high
DRAM_ATTR constexpr int gateLevelCycle[6][6] = { //ah al bh bl ch cl
    {0, 1, 1, 0, 0, 0}, //block 0,  HLHLHL
    {0, 1, 0, 0, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 1},
    {0, 0, 1, 0, 0, 1}
};
#define MCPWMx ((mcpwm_dev_t * ) &MCPWM0)
#define electricalCycles 18 //constexpr is defineable compile time costant 
#define SECTOR_PER_BITS (float)(electricalCycles / 4096.0f)
#define as5600CalibrationRawValue (1916) //38 not 37 because +0.5 and trucnate = round up,30degrees to sector_per_bits is only .5, not 1.
#define as5600CalibratedOffset (int)((4096.0) * (38.0 / 36.0) - (4096 - as5600CalibrationRawValue) )  
//top view of physical motor has ABC going ccw, [-30 degrees, 30 degrees) = block 0
//((4096-global.rotorVal)+(int)((4096.0)*(38.0/36.0) - (4096-(3388)) )) ==> (4096/18+3388-val)*18/4096==>>(7711.5-v)*0.00439453
#ifdef as5600DirPinHigh //When motor is running controller code
#define getRotorValAdjusted(x) (as5600CalibratedOffset + x) * SECTOR_PER_BITS
#else
#define getRotorValAdjusted(x) ((4096 - x) + as5600CalibratedOffset) * SECTOR_PER_BITS
#endif

DRAM_ATTR constexpr const char* ghgl[6] = {"0BA ","1CA ","2CB ","3AB ","4AC ","5BC "}; //[-30,30) = block 0
DRAM_ATTR constexpr const char* dgdir[6] = {"∅","D?","+","D?","NOT-","-"};
//=======================================PINS================================
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

#define SNAP() esp_timer_get_time()
#define time240() esp_cpu_get_cycle_count()
#define print(x) esp_rom_printf(x)
#define CLEAR_ALL_NOTIFS(x) (ulTaskNotifyValueClear(x, 0xffffffff) ||  xTaskNotifyStateClear(x) )

/* //tracking all interstate variables
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
//===============LOOPED TASKS===========================
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
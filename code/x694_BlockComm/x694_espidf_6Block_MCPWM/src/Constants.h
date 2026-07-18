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
#define cBufSize 8                                          /*For storing measured/calculated motor values*/



/*#################### SHORTHANDS #################### */
/* ========================= FUNCTION SHORTHANDS ========================= */
#define SNAP() esp_timer_get_time()
#define time240() esp_cpu_get_cycle_count()
#define snap() time240()
#define print(x) esp_rom_printf(x)
#define CLEAR_ALL_NOTIFS(x) (ulTaskNotifyValueClear(x, 0xffffffff) ||  xTaskNotifyStateClear(x) )
#define TICKS_TO_REAL_VELOCITY(x)    ( VTIMER_CLOCK / ( BLOCKSF_PER_ROTATION * x ))
#define INPUT_TO_REAL_VELOCITY(x)   TICKS_TO_REAL_VELOCITY( (uint32_t)(VTIMER_CLOCK / (x * BLOCKS_PER_ROTATION)) )
#define LMAP(alpha, x1, y1, x2, y2) (((y1 - y2) / (x1 - x2)) * (alpha - x2) + y2)  //linear mapping. Ensure that at least one of {Point 1, Point 2} has float
/*. lmap input fromLow toLow    fromHigh toHigh*/


/* ========================= CONSTANTS SHORTHANDS ========================= */
#define BLOCKSF_PER_ROTATION (18.0f) //constexpr is defineable compile time costant 
#define BLOCKS_PER_ROTATION (18) 
#define BITS_TO_ROTATIONS (1/4096.0)
#define ROTATIONS_TO_BITS (4096.0)
#define VTICKS_PER_BLOCK (VTIMER_CLOCK / BLOCKS_PER_ROTATIO)N /* VP = VTIMER_CLOCK / 18  */
#define VTICKSF_PER_BLOCK (VTIMER_CLOCK / BLOCKSF_PER_ROTATION) /* VP = VTIMER_CLOCK / 18  */

#define MCPWMx ((mcpwm_dev_t * ) &MCPWM0)
#define SECTOR_PER_BITS (float)( BLOCKS_PER_ROTATION / 4096.0f)
#define ExecuteGate_FreeSpin_NotifVal (0x0000FFFF)
#define i2cWaitout (1) //in ms
#define initializationLatency pdMS_TO_TICKS(30)
#define MAX_MCPWM_TIMER_PERIOD (65535)

/* ------------------------------ MCPWM SHORTHANDS------------------------------ */
#define mcpwm_lowSideGroupPrescaler (40)
#define HighTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler)) 
#define activePwmPeriod (uint32_t)(HighTimerResolution/20000)  //change to 20khz when high
// #if ((startingDuty < minDuty) || (startingDuty > maxDuty))
// #warning "DUTY out of bounds!!!!!!!!!!!!!!!!!!!!!!!!!"
// #endif
#define VTIMER_CLOCK  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler*10)) 
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



/* #################### MOTOR LIMITATIONS #################### */
/* ========================= MOTOR HARDWARE LIMITS ========================= */
/*  Unsigned values set to motor's physical limits (MOTOR_SPEC). Magnitudes only. DO NOT CHANGE. */
#define maxDuty 0.97f
#define minDuty 0.03f
// #define MOTOR_SPEC_MAX_VELOCITY (float)(50.0f) /* Unit: RPS */
#define MOTOR_SPEC_MAX_VELOCITY (INPUT_TO_REAL_VELOCITY( 50 )) /* Unit: RPS */
#define MOTOR_SPEC_MIN_VELOCITY (TICKS_TO_REAL_VELOCITY( MAX_MCPWM_TIMER_PERIOD )) /* Unit: RPS */
#define MOTOR_SPEC_MAX_TORQUE (maxDuty)
#define MOTOR_SPEC_MIN_TORQUE (minDuty)

/* ========================= MOTOR SOFTWARE LIMITS ========================= */
/* Unsigned values set to within motor's physical limits (MOTOR_SPEC). Magnitudes only. User can edit. */
#define SL_MAX_VELOCITY     (MOTOR_SPEC_MAX_VELOCITY)     /* Unit: RPS */
#define SL_MIN_VELOCITY     (TICKS_TO_REAL_VELOCITY( MAX_MCPWM_TIMER_PERIOD / 2 ))     /* Unit: RPS */

#define SL_MAX_TORQUE       (MOTOR_SPEC_MAX_TORQUE)
#define SL_MIN_TORQUE       (MOTOR_SPEC_MIN_TORQUE)

//save time by calculating software bounds beforehand
#define SL_MAX_VELOCITY_PERIOD_TICKS (uint32_t)(VTICKSF_PER_BLOCK / (SL_MAX_VELOCITY)) //200--> 111.11rps, 1111-->20rps
#define SL_MIN_VELOCITY_PERIOD_TICKS (uint32_t)(VTICKSF_PER_BLOCK / (SL_MIN_VELOCITY)) 

/* ========================= USER TARGET INPUT BOUNDS ========================= */
/* Signed minimum and maximum target settings. MEANT TO BE USER CHANGED*/
#define TARGET_POSITION_UB        (uint32_t) (0)        /* Unit: LSB */
#define TARGET_POSITION_LB        (uint32_t) (4096)    /* Unit: LSB */

#define TARGET_VELOCITY_UB     (float) (SL_MAX_VELOCITY)     /* Upper bound of target velocity. Unit: RPS */
#define TARGET_VELOCITY_LB      (float) (-1.0 * SL_MAX_VELOCITY)     /* Lower bound of target velocity. Unit: RPS */
// #define TARGET_VELOCITY_LB      (float) (SL_MIN_VELOCITY)     /* Lower bound of target velocity. Unit: RPS */
#define TARGET_TORQUE_UB        (float) (SL_MAX_TORQUE)
#define TARGET_TORQUE_LB        (float) (-1.0 *SL_MAX_TORQUE)


/* #################### INTERRUPT PRIORITY #################### */
#define MCPWM_HighsideIntrPriority 1 //tep,tez
#define MCPWM_LowsideIntrPriority 2 //tep,tez
#define runOnMCPWMIntrPriority ESP_INTR_FLAG_LEVEL2 //might be a bit long
#define i2c_intrPriority 3
/*esp timer intr : 1-3, (2)
freertos timer :lvl 1 or 3 (1)
watchdog and sys checks :4 or 5 */



/* ========================= PREPROCESSOR DIRECTIVE RULES ========================= */
/* 
ADDITION / SUBTRACTION
(1.0+50) eval to 51.0f
int - int eval to int

DIVISION / MULTIPLICATION
float * int 
- 1 * int does not carry sign (treated as uint32_t)
- integers don't carry sign; they wrap around and cant be forced ((-50) wraps around)

-1. * int does carry sign (treated as uint32_t)
uint32_t/uint32_t truncates
follows Order of operations (ex 400000 / 18 * 100 = 2222200 )
*/
/* 
####################
=========================
------------------------------


//tracking all interstate variables
tag - darray [dindex #W]  #W

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
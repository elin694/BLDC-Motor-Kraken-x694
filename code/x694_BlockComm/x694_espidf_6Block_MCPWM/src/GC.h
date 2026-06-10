#pragma once 
#include "Globals.h"

#define isrTickDeadTime static_cast<uint32_t>(timerResolution/1e6 *.9) //isr 700ns responds time
#define relativeDeadTime 5
#define syncTickBeforeCMPRThreshold -2

// gpio 19- miso, b High side is tx2

uint32_t CMRA0Threshold;
intr_handle_t sixBlockISR = NULL;
intr_handle_t oneBlockISR = NULL;
inline phaseMcpwm motorH[3];
inline phaseMcpwm motorL[3];

void initializeSyncs();
void initializeInterruptEnablePin();
void activateAllSyncs();
void setCountValueAndPeriod(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function1);
void synchr(mcpwm_sync_handle_t handle, std::string name);
void synchrISR(mcpwm_sync_handle_t handle, const char* name);

inline void blinkDebugLed(int delay){
    for(int i= 1000; i>0 && (ledD !=0); i--){
        GPIO.out_w1ts |= 1<<2;
        vTaskDelay(delay);
        GPIO.out_w1tc |= 1<<2;
        vTaskDelay(delay);
    }
}
mcpwm_timer_config_t phaseTimerSetupHigh = { //Grass with peaks
    .group_id = highSideGroup,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = timerResolution,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    // .intr_priority = 1,
    .flags = {
        .update_period_on_empty = 0,
        .update_period_on_sync = 1 
    }
};

mcpwm_timer_config_t blockTimerSetup = { //onces per step/block
    .group_id = lowSideGroup,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = timerResolution,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    // .intr_priority = 1,
    .flags = {
        .update_period_on_empty = 0,
        .update_period_on_sync = 1
    }
};
mcpwm_timer_config_t globalTimerSetupLow = { //Grass with peaks
    .group_id = lowSideGroup,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = timerResolution,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    // .intr_priority = 1,
    .flags = {
        .update_period_on_empty = 0,
        .update_period_on_sync = 1 
    }
};
//+===================
//+===================
//+===================
//+===================
const mcpwm_dead_time_config_t highGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};
const mcpwm_operator_config_t operatorSetupHigh = {
    .group_id = highSideGroup,
    // .intr_priority = 0,
    .flags = {
        .update_gen_action_on_tez = 0,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 1,
        .update_dead_time_on_tez = 1,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 1,
    },
};

const mcpwm_dead_time_config_t lowGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime + relativeDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};
const mcpwm_operator_config_t operatorSetupLow = {
    .group_id = lowSideGroup,
    // .intr_priority = 0,
    .flags = {
        .update_gen_action_on_tez = 0,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 1,
        .update_dead_time_on_tez = 0,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 1,
    },
};

//=========================================== SYNC =======================================================
//syncs block timer
#define sacrificialUniversalGpio 27
#define SET_SUG_REGISTER (uint32_t *)(&GPIO.out_w1ts)
#define CLEAR_SUG_REGISTER (uint32_t *)(&GPIO.out_w1tc)

mcpwm_soft_sync_config_t tripleHighTriggerSetup ={};
mcpwm_sync_handle_t tripleHighTrigger[3]; //CONTROLS ALL 3 HIGH TIMERS
mcpwm_timer_sync_phase_config_t tripleHighOnSync = { 
    .count_value = 10000, 
    .direction = MCPWM_TIMER_DIRECTION_UP,
};


mcpwm_soft_sync_config_t BTimerTriggerSetup = {};
mcpwm_sync_handle_t BTimerTrigger;
mcpwm_timer_sync_phase_config_t BTimerOnSync = { 
    .sync_src = BTimerTrigger, //assign to a syn src
    .count_value = 10000, 
    .direction = MCPWM_TIMER_DIRECTION_UP,
};//active Btimer sync

mcpwm_soft_sync_config_t LTimerTriggerSetup = {};
mcpwm_sync_handle_t LTimerTrigger;
mcpwm_timer_sync_phase_config_t LTimerOnSync = { 
    .sync_src = LTimerTrigger, //assign to a syn src
    .count_value = 10000, 
    .direction = MCPWM_TIMER_DIRECTION_UP, 
    // Only one that would be modified ^^^^
};
/*
Hardware prioriy:
- Fault/Brake
- TEZ/TEP
- CMRA/CMRB
*/
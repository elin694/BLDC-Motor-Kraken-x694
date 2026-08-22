#pragma once 
#include "Globals.h"
// #include "driver/gpio.h"
#define isrTickDeadTime (uint32_t)0 /*in ns*/
#define relativeDeadTime 50 /*in ns*/
// gpio 19- miso, b High side is tx2

inline DRAM_ATTR phaseMcpwm motorH[3];
inline DRAM_ATTR phaseMcpwm motorL[3];

/* #################### MCPWM TIMERS #################### */
mcpwm_timer_config_t HTimerSetup = { //Grass with peaks
    .group_id = MCPWM_HIGHSIDE_GROUP,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = HighTimerResolution,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =activePwmPeriod,
    .intr_priority = MCPWM_HighsideIntrPriority,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0 
    }
};
mcpwm_timer_config_t VTimerSetup= { //onces per step/block
    .group_id = MCPWM_LOWSIDE_GROUP,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = VTIMER_CLOCK,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .intr_priority=MCPWM_LowsideIntrPriority,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 1,
    }
};
#ifdef lastResort
bool VTimerCallback(mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t *edata, void *user_ctx);
constexpr mcpwm_timer_event_callbacks_t callbackFamily = {
   .on_empty = VTimerCallback
};
#endif

/* #################### MCPWM OPERATORS #################### */
const mcpwm_operator_config_t HOperatorSetup = {
    .group_id = MCPWM_HIGHSIDE_GROUP,
    .intr_priority = MCPWM_HighsideIntrPriority,
    .flags = {
        .update_gen_action_on_tez = 1,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 0,
        .update_dead_time_on_tez = 1,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 0,
    },
};
const mcpwm_operator_config_t LOperatorSetup = {
    .group_id = MCPWM_LOWSIDE_GROUP,
    .intr_priority = MCPWM_LowsideIntrPriority,
    .flags = {
        .update_gen_action_on_tez = 0,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 1,
        .update_dead_time_on_tez = 0,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 1,
    },
};

/* #################### MCPWM OPERATOR SUBMODULES #################### */
inline mcpwm_comparator_config_t HComparatorSetup = {
    .intr_priority = MCPWM_HighsideIntrPriority,
    .flags ={
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};
inline mcpwm_generator_config_t HPWMSetup = {
    .flags = {
        .invert_pwm = false
    }
};

#define DT_SCALER (HighTimerResolution * 1.0 / 1e9)
const mcpwm_dead_time_config_t highGateDeadTimeSetup = {
    .posedge_delay_ticks = ceil(isrTickDeadTime * DT_SCALER),
    .negedge_delay_ticks = ceil(isrTickDeadTime * DT_SCALER),
    .flags = {
        // invert_output = 1;
    }
};
const mcpwm_dead_time_config_t lowGateDeadTimeSetup = {
    .posedge_delay_ticks = ceil( (isrTickDeadTime + relativeDeadTime) * DT_SCALER),
    .negedge_delay_ticks = ceil( isrTickDeadTime * DT_SCALER),
    .flags = {
        // invert_output = 1;
    }
};

/* #################### MCPWM SYNCS #################### */
mcpwm_soft_sync_config_t tripleHighTriggerSetup ={};
mcpwm_sync_handle_t tripleHighTrigger[3]; //CONTROLS ALL 3 HIGH TIMERS
mcpwm_timer_sync_phase_config_t tripleHighOnSync = { 
    .direction = MCPWM_TIMER_DIRECTION_UP,
};

mcpwm_soft_sync_config_t VTimerTriggerSetup = {};
mcpwm_sync_handle_t VTimerTrigger;
mcpwm_timer_sync_phase_config_t VTimerOnSync = { 
    .sync_src = VTimerTrigger, //assign to a syn src
    .direction = MCPWM_TIMER_DIRECTION_UP, 
};
/*
Hardware prioriy:
- Fault/Brake
- TEZ/TEP
- CMRA/CMRB
*/

/* #################### MCPWM INITALIZE FUNCTIONS  #################### */
void mcpwmSetup();
void setCountValueAndPeriod();
void initializeHighGate( uint32_t comparatorOff_Duty);
void initializeLowGate();
void initializeTimer();
void initializeISR();
void runOnMCPWMIntr(void *returnValue);
void executeGates(void * parameter);

#pragma once 
#include "Globals.h"
// #include "driver/gpio.h"
//timer setups, operator , syncs
//comparator in gateControl.h
#define isrTickDeadTime (uint32_t)0 //isr 700ns responds time
#define relativeDeadTime 5
#define highSideGroup 1 
#define lowSideGroup 0


// gpio 19- miso, b High side is tx2

phaseMcpwm motorH[3];
phaseMcpwm motorL[3];

void setCountValueAndPeriod();

mcpwm_timer_config_t HTimerSetup = { //Grass with peaks
    .group_id = highSideGroup,
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
    .group_id = lowSideGroup,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = VTimerResolution,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .intr_priority=MCPWM_LowsideIntrPriority,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 1,
    }
};
//+===================

//+===================
const mcpwm_dead_time_config_t highGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};
const mcpwm_operator_config_t HOperatorSetup = {
    .group_id = highSideGroup,
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
    .group_id = lowSideGroup,
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


const mcpwm_dead_time_config_t lowGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime + relativeDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};

//=========================================== SYNC =======================================================
//syncs block timer

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
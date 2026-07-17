#pragma once 
#include "Globals.h"

inline mcpwm_comparator_config_t phaseComparatorSetup = {
    .intr_priority = MCPWM_HighsideIntrPriority,
    .flags ={
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 1
    }
};
inline mcpwm_generator_config_t phasePwmSetup = {
    .flags = {
        .invert_pwm = false,
        // .io_loop_back = 0,
        // .io_od_mode = 0, //pull low or float only
        // .pull_up = 0,
        // .pull_down= 1
    }
};
typedef struct {
    int node; //a,b,c
    mcpwm_timer_config_t timerConfig;
    mcpwm_operator_config_t opConfig;
    mcpwm_comparator_config_t compConfig = phaseComparatorSetup;
    mcpwm_generator_config_t pwmConfig = phasePwmSetup;

    mcpwm_timer_handle_t timer = NULL;
    mcpwm_oper_handle_t operatorModule= NULL;
    mcpwm_cmpr_handle_t comparator0 = NULL;
    mcpwm_cmpr_handle_t comparator1 = NULL; //null for high
    mcpwm_gen_handle_t pwmGate0 = NULL;
    mcpwm_gen_handle_t pwmGate1 = NULL;// stays null
    //shoutout gemini for suggest changing countval
} phaseMcpwm;

//initialization functions
void mcpwmSetup();
void initializeHighGate( uint32_t comparatorOff_Duty);
void initializeLowGate();
void initializeTimer();
void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm  * motorLow);
void initializeISR();
bool VTimerCallback(mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t *edata, void *user_ctx);

void preloadGates();
void executeGates(bool freeSpin);

#ifdef lastResort
constexpr mcpwm_timer_event_callbacks_t callbackFamily = {
   .on_empty = VTimerCallback
};
#endif

/*
Hardware prioriy:
- Fault/Brake
- TEZ/TEP
- CMRA/CMRB
*/
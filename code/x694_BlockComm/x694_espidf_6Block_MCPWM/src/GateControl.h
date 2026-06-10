#pragma once 
#include "Globals.h"

inline mcpwm_comparator_config_t phaseComparatorSetup = {
    // .intr_priority = 0,
    .flags ={
        .update_cmp_on_tez = 0,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 1
    }
};
inline mcpwm_generator_config_t phasePwmSetup = {
    // .gen_gpio_num =19;
    .flags = {
        .invert_pwm = false,
        .io_loop_back = 0,
        .io_od_mode = 0, //pull low or float only
        .pull_up = 0,
        .pull_down= 1
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
    // uint32_t phaseShift= 0; //tack on with an add later
    //shoutout gemini for suggest changing countval
} phaseMcpwm;

//initialization functions
void mcpwmSetup(int targetSectorNumber, volatile  uint32_t * blockPeriod_f);
void initializeHighGate(int startingTargetSector, uint32_t blockPeriod_f);
void initializeLowGate(int startingTargetSector, float threshold_thirds[]);
void configureLowGateEvents();
void initializeTimer(int startingTargetSector, uint32_t blockPeriod_f);
void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm  * motorLow, int startingTargetSector, uint32_t blockPeriod_f);
void initializeISR();

//Loop
void preloadGates(int previousState, int nextState, uint32_t blockPeriod_f, mcpwm_dev_t * mcpwm, uint32_t clearMask);
void executeGates(mcpwm_int_clr_reg_t* clearRegister, mcpwm_dev_t * mcpwm);

#if (lowSideGroup == 1)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM1)
#elif (lowSideGroup == 0)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM0)
#endif
/*
Hardware prioriy:
- Fault/Brake
- TEZ/TEP
- CMRA/CMRB
*/
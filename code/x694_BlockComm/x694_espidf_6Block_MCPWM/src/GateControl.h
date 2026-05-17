#pragma once 
#include "Globals.h"

/*
Hardware prioriy:
- Fault/Brake
- TEZ/TEP
- CMRA/CMRB
*/
constexpr  int pwmControllerGroupID =  SOC_MCPWM_GROUPS-1;
inline constexpr mcpwm_dead_time_config_t lowGateDeadTimeSetup = {
    .posedge_delay_ticks = 4,
    // .negedge_delay_ticks = 2,
    .flags = {
        // invert_output = 1;
    }
};

inline constexpr mcpwm_operator_config_t phaseOperatorSetup = {
    .group_id = pwmControllerGroupID,
    .intr_priority = 0,
    .flags = {
        .update_gen_action_on_tez = 0,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 1,
        .update_dead_time_on_tez = 0,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 1,
    },
};

typedef struct {
    mcpwm_timer_config_t timerConfig;
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_operator_config_t opConfig = phaseOperatorSetup;
    mcpwm_oper_handle_t operatorModule = NULL;
    mcpwm_comparator_config_t compConfig;
    mcpwm_cmpr_handle_t comparatorHigh = NULL;
    mcpwm_cmpr_handle_t comparatorLow = NULL;

    mcpwm_generator_config_t pwmConfigHigh;
    mcpwm_generator_config_t pwmConfigLow;
    mcpwm_gen_handle_t pwmHighGate = NULL;
    mcpwm_gen_handle_t pwmLowGate = NULL;

    mcpwm_dead_time_config_t deadTime = lowGateDeadTimeSetup;
    mcpwm_timer_sync_phase_config_t syncConfig = {
        //config to use sync to sync timers
        // .sync_src = , //assign to a syn src
        .count_value = 0, //assign phase
        .direction = MCPWM_TIMER_DIRECTION_UP,
    };
    float phaseShift= 0.0f; //tack on with an add later
    //shoutout gemini for suggest changing countval
} phaseMcpwm;



extern void mcpwmSetup();
extern void phaseSwitching();
extern void executeGate(phaseMcpwm phase, int state, int previousState);
extern void preloadNextBlock(phaseMcpwm phase, int previousState, int nextState);

/* DESCRIPTION
every 100ms, Pot will send new updated value to analog read pin
After reading the pin, Esp32 will adjust timers by calling updatePwms(int phase, int duty )

@param *** frequency will be cosntnant adn defined by RC time cosntant
each MCPWM module has 3 operators, each operator can make 2 PWM waves
Low gate pwm will be adjusted with 

3 timers  --> 3 operators, each generator a high and low gate signal for their corresponding 3 phases
+1 timer with block frequency
at every blockTimer tick, (the 4th timer)
- if High gate needs to be set high, the corresponding timer frequency is increase to PWM frequency until
- If high gate switching is finished
- set low gate deadtime on operators's 2nd pwm wave

*/
/// UNUSED CODE
/*
    // ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(genHandle,
    //     MCPWM_GEN_TIMER_EVENT_ACTION(
    //         MCPWM_TIMER_DIRECTION_UP, //up or down
    //         MCPWM_TIMER_EVENT_EMPTY, // timer to 0, peak, or timer invalid event
    //         MCPWM_GEN_ACTION_LOW // set to same level, low/high level, or toggle
    //     ),
    //     MCPWM_GEN_TIMER_EVENT_ACTION_END()
    // ));
*/
/*
mcpwm_capture_channel_config_t triggerChannelSetup = {
    .gpio_num = captureGPIO,
    .intr_priority = 0,
    .prescale = 2,
    .flags = {
        .pos_edge = 1,
        .neg_edge = 0,
        .pull_up = 1,
        .pull_down = 0,
        .invert_cap_signal = 0,
        .io_loop_back = 0 ,
        .keep_io_conf_at_exit =1,
    }
};
mcpwm_cap_channel_handle_t triggerChannelHandle;
// mcpwm_capture_timer_sync_phase_config_t 
mcpwm_capture_timer_config_t triggerSetup = {
    .group_id = id,
    .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    .resolution_hz = 1052631,
};
mcpwm_cap_timer_handle_t triggerHandle;

mcpwm_capture_channel_enable()
mcpwm_capture_channel_register_event_callbacks(),
mcpwm_capture_channel_disable()
mcpwm_capture_timer_enable() 
mcpwm_capture_timer_start() //START
*/
#include "globals.h"
//bldc first:

int id=  SOC_MCPWM_GROUPS-1;

mcpwm_timer_config_t timerSetup = {
    .group_id = id,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = static_cast<uint32_t>(10e6),
    .period_ticks =1000, //
    .intr_priority = 0,
    .flags = {
        .update_period_on_empty = 1, //when should period change if you want to change it
        .update_period_on_sync = 0 //these 2 determine when set_period takes effect
    }
};
static mcpwm_timer_handle_t timerHandle;
//Register Timer Event Callbacks

mcpwm_operator_config_t operatorSetup = {
    .group_id = id,
    .intr_priority = 0,
    .flags = {
        //when to update the generator pin output level
        .update_gen_action_on_tez = 1, 
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 0,
        .update_dead_time_on_tez = 0,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 0,
    },
};
mcpwm_oper_handle_t operatorHandle;

const mcpwm_comparator_config_t comparatorSetup = {
    .intr_priority = 0,
    .flags ={
        //when to adjust comparator threshold
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};
mcpwm_cmpr_handle_t comparatorHandle;

mcpwm_generator_config_t genSetup = {
    .gen_gpio_num = 2,
    .flags = {
        .invert_pwm = false,
        .io_loop_back = 1,
        .io_od_mode = 1,
        .pull_up = 1,
        .pull_down= 0
    }
};
mcpwm_gen_handle_t genHandle;

mcpwm_capture_timer_config_t triggerSetup = {
    .group_id = id,
    .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    .resolution_hz = 2000000,
};
mcpwm_cap_timer_handle_t triggerHandle;

mcpwm_capture_channel_config_t triggerChannelSetup = {
    .gpio_num = 22,
    .intr_priority = 0,
    .prescale = 2,
    .flags = {
        .pos_edge = 1,
        .neg_edge = 0,
        .pull_up = 0,
        .pull_down = 1,
        .invert_cap_signal = 1,
        .io_loop_back = 1,
        .keep_io_conf_at_exit =1,
    }
};
mcpwm_cap_channel_handle_t triggerChannelHandle;

extern void setupMCPWM(){
    ESP_ERROR_CHECK(mcpwm_new_timer(&timerSetup, &timerHandle));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operatorSetup, &operatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_comparator(operatorHandle, &comparatorSetup, &comparatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_generator(operatorHandle, &genSetup, &genHandle));
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&triggerSetup, &triggerHandle));
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(triggerHandle, &triggerChannelSetup, &triggerChannelHandle));


    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operatorHandle, timerHandle)); //--
    ESP_ERROR_CHECK(mcpwm_timer_enable(timerHandle));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timerHandle, MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparatorHandle,500)); 
}


/*RUNTIME FUNCTIONS*/

/*
mcpwm_timer_set_period() 
mcpwm_comparator_register_event_callbacks()-- comparator can be used to trigger event when comparator reaches threshold
    - update time  set by Config: update_cmp_on_tez, update_cmp_on_tep, or update_cmp
mcpwm_generator_set_actions_on_timer_event()- One generator can set multiple actions on different timer events
    - EX: when generator a's timer has a rising edge that reaches 0, set generator A output low
mcpwm_generator_set_actions_on_compare_event()
    - EX:When genA's timer has a rising edge that meets cmpa's threshold, set genA output high,
            then low on the falling edge
mcpwm_generator_set_action_on_sync_event()- sync base trigger event,  MCPWM_GEN_SYNC_EVENT_ACTION
    - doesn't have variadic function 
    mcpwm_generator_set_dead_time()
*/
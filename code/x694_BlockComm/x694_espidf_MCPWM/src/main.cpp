#include "headers.h"
#define generatorGPIO phaseCHighPort
//b HIGH SIDE tx2
#define phaseLowGate phaseALowPort
// #define generatorGPIO GPIO_NUM_2 //b HIGH SIDE tx2
#define captureGPIO GPIO_NUM_19 //miso


//in
#define timerPeriod 65535 //2e16
// #define timerPeriod (65535+1)/16 //2e16
// #define countingFrequency (1048576*64)
#define countingFrequency (104876)
#define dutyCycle 50

uint32_t compareValue = dutyCycle*.005*timerPeriod;
int id =  1;

void groundSetup(){
    gpio_num_t gateArray[6]= {
      phaseAHighPort,
      phaseALowPort,
      phaseBHighPort,
      phaseBLowPort,
      phaseCHighPort,
      phaseCLowPort,
   };
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      ets_delay_us(10);
      gpio_set_pull_mode(gate, GPIO_PULLDOWN_ONLY);
      gpio_set_level(gate, 0);
   }
    gpio_reset_pin(phaseLowGate);
    gpio_set_direction(static_cast<gpio_num_t>(phaseLowGate),GPIO_MODE_OUTPUT);
    gpio_set_level(phaseLowGate, 1);
}

mcpwm_timer_config_t timerSetup = {
    .group_id = id,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = static_cast<uint32_t>(countingFrequency),
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =static_cast<uint32_t>(timerPeriod),//
    // .intr_priority = 1,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0 //these 2 determine when set_period takes effect
    }
};
static mcpwm_timer_handle_t timerHandle;
//Register Timer Event Callbacks

mcpwm_operator_config_t operatorSetup = {
    .group_id = id,
    // .intr_priority = 0,
    .flags = {
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
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};
mcpwm_cmpr_handle_t comparatorHandle;

mcpwm_generator_config_t genSetup = {
    .gen_gpio_num = generatorGPIO,
    .flags = {
        .invert_pwm = false,
        .io_loop_back = 0,
        .io_od_mode = 0, //pull low or float only
        .pull_up = 0,
        .pull_down= 1
    }
};
mcpwm_gen_handle_t genHandle;

// mcpwm_capture_timer_config_t triggerSetup = {
//     .group_id = id,
//     .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
//     .resolution_hz = 1052631,
// };
// mcpwm_cap_timer_handle_t triggerHandle;

// mcpwm_capture_channel_config_t triggerChannelSetup = {
//     .gpio_num = captureGPIO,
//     .intr_priority = 0,
//     .prescale = 2,
//     .flags = {
//         .pos_edge = 1,
//         .neg_edge = 0,
//         .pull_up = 1,
//         .pull_down = 0,
//         .invert_cap_signal = 0,
//         .io_loop_back = 0 ,
//         .keep_io_conf_at_exit =1,
//     }
// };
// mcpwm_cap_channel_handle_t triggerChannelHandle;

void setupMCPWM(){
    ets_delay_us(10000);
    groundSetup();
    ESP_ERROR_CHECK(mcpwm_new_timer(&timerSetup, &timerHandle));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operatorSetup, &operatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_comparator(operatorHandle, &comparatorSetup, &comparatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_generator(operatorHandle, &genSetup, &genHandle));
    // ESP_ERROR_CHECK(mcpwm_new_capture_timer(&triggerSetup, &triggerHandle));
    // ESP_ERROR_CHECK(mcpwm_new_capture_channel(triggerHandle, &triggerChannelSetup, &triggerChannelHandle));
}

extern "C" {
    void app_main() {
        setupMCPWM();
        
        /*RUNTIME FUNCTIONS*/
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(genHandle, 0, true)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operatorHandle, timerHandle)); //--
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparatorHandle,compareValue)); 
        
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(genHandle,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                comparatorHandle,
                MCPWM_GEN_ACTION_HIGH
            ),
              MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_DOWN,
                comparatorHandle,
                MCPWM_GEN_ACTION_LOW
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));
        // ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(genHandle,
        //     MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_EVENT_FULL, MCPWM_GEN_ACTION_HIGH),
        //     MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_LOW),
        //     MCPWM_GEN_TIMER_EVENT_ACTION_END())
        // );

        ESP_ERROR_CHECK(mcpwm_timer_enable(timerHandle));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timerHandle, MCPWM_TIMER_START_NO_STOP));
        for(;;){
            vTaskDelay(pdMS_TO_TICKS(100000));
        }       
    } 
} 
    //timer - clock
    //operator -supermodule
    // comparator- compares to threshold value. Compare event
    //                     when = to threshold, MCPWM updates level
// generator - makes pair of pwm waves (complementary or not)
//                    based on event triggers from other submodules (Timer, Comparator)
// fault - detect external fault (ex. gpio) and set to predefined state
// sync - sync timers so PWM from different generators have fixed phase difference
//            signal routed from gpio or timer event
// deadtime
// carrier - chopper
// brake - configure brake when fault condition
//UPDATE+ CHANGE
//*******PWM CAPTURE**** */
// 1 dedicated timer
// many indep. channels
// channel connected to GPIO, 
//     ---   pulse on gpio --> capture  us timer + isr event
//     --- measure pulse width
//.    --- can sync via MCPWM sync

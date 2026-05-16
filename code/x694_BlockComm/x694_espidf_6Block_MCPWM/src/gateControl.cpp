#include "Globals.h"
#include "GateControl.h"
//b HIGH SIDE tx2
// #define captureGPIO GPIO_NUM_19 //miso

#define timerResolution  static_cast<uint32_t>(8e6) //125ns
// #define blockPeriod 65535 //2e16

const uint16_t pwmPeriod = timerResolution/20000;  //change to 20khz when high
//temp gateControl Runtime var
uint32_t blockPeriod = static_cast<uint32_t>(timerResolution/1000);  //1000 Hz, period of a single block

uint32_t compareValue = static_cast<uint32_t>(blockPeriod*duty);
//&^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^6

phaseMcpwm phaseSignals[3];  
gpio_num_t gateArray[6]= {
    phaseAHighPort,
    phaseALowPort,
    phaseBHighPort,
    phaseBLowPort,
    phaseCHighPort,
    phaseCLowPort,
};
extern void groundSetup(){
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      ets_delay_us(1000);
      gpio_set_pull_mode(gate, GPIO_PULLUP_ONLY);
      gpio_set_level(gate, 0);
   }
}

mcpwm_timer_config_t blockTimerSetup = { //6 times per electric cycle
    .group_id = !(pwmControllerGroupID),
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz =timerResolution,
    // .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks =static_cast<uint32_t>(blockPeriod), //1 phase every change int
    // .intr_priority = 1,
    .flags = {
        // .update_period_on_empty = 1,
        .update_period_on_sync = 1
    }
};
mcpwm_timer_handle_t blockTimer;
mcpwm_timer_config_t phaseTimerSetup = {
    .group_id = pwmControllerGroupID,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = static_cast<uint32_t>(timerResolution),
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =static_cast<uint32_t>(6*blockPeriod), //approximate sine wave with a triangle
    // .intr_priority = 1,
    .flags = {
    //     .update_period_on_empty = 1,
        .update_period_on_sync = 1 
    }
};
mcpwm_comparator_config_t phaseComparatorSetup = {
    .intr_priority = 0,
    .flags ={
        .update_cmp_on_tez = 0,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 1
    }
};
mcpwm_generator_config_t phasePwmSetup = {
    // .gen_gpio_num =19;
    .flags = {
        .invert_pwm = false,
        .io_loop_back = 0,
        .io_od_mode = 0, //pull low or float only
        .pull_up = 0,
        .pull_down= 1
    }
};

//========================================================================================================
//========================================================================================================
/*
mcpwm_generator_set_action_on_sync_event();
*/
mcpwm_gpio_sync_src_config_t gpioMultiSyncSetup = {
    .group_id = pwmControllerGroupID,
    .gpio_num = 17,
    .flags = {
        .pull_down =1
    }
};
mcpwm_sync_handle_t gpioMultiSync;

mcpwm_soft_sync_config_t softSyncSetup = {};
mcpwm_sync_handle_t softSync;

mcpwm_timer_sync_src_config_t masterTimerSyncSetup = { //sets a timer as sync trigger
    .timer_event = MCPWM_TIMER_EVENT_EMPTY,
    .flags = {
        .propagate_input_sync = 1,
    }
};
mcpwm_sync_handle_t masterTimerSync;

mcpwm_timer_sync_phase_config_t syncState1 = { //config to use sync to sync timers
    .sync_src = gpioMultiSync, //assign to a syn src
    .count_value = 600, //assign phase
    .direction = MCPWM_TIMER_DIRECTION_UP,
};
//========================================================================================================
//========================================================================================================

extern void mcpwmSetup(){
    groundSetup();
    int i = 0;
    for (phaseMcpwm phase: phaseSignals){
        phasePwmSetup.gen_gpio_num = gateArray[i];
        phase = {
            .timerConfig = phaseTimerSetup,
            // .opConfig = phaseOperatorSetup,
            .compConfig = phaseComparatorSetup,
            .pwmConfigHigh = phasePwmSetup
        };
        i++;
        phasePwmSetup.gen_gpio_num = gateArray[i];
        phase.pwmConfigLow = phasePwmSetup;

        ESP_ERROR_CHECK(mcpwm_new_timer(&phase.timerConfig, &phase.timer));
        ESP_ERROR_CHECK(mcpwm_new_operator(&phase.opConfig, &phase.operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_comparator(phase.operatorModule, &phaseComparatorSetup, &phase.comparatorHigh));
        ESP_ERROR_CHECK(mcpwm_new_comparator(phase.operatorModule, &phaseComparatorSetup, &phase.comparatorLow));
        ESP_ERROR_CHECK(mcpwm_new_generator(phase.operatorModule, &phase.pwmConfigHigh, &phase.pwmHighGate));
        ESP_ERROR_CHECK(mcpwm_new_generator(phase.operatorModule, &phase.pwmConfigLow, &phase.pwmLowGate));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(phase.pwmLowGate, phase.pwmLowGate, &lowGateDeadTimeSetup));
        // ESP_ERROR_CHECK(mcpwm_new_capture_timer(&triggerSetup, &triggerHandle));
        // ESP_ERROR_CHECK(mcpwm_new_capture_channel(triggerHandle, &triggerChannelSetup, &triggerChannelHandle));

        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(phase.operatorModule, phase.timer)); //--
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(phase.comparatorHigh,compareValue)); 
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(phase.comparatorLow, static_cast<uint32_t>(blockPeriod/3))); //one tme run
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, 0, true)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, 0, true)); // Force low until ready
    }
        ets_delay_us(1000);
             //Start block timer
    ESP_ERROR_CHECK(mcpwm_new_timer(&blockTimerSetup, &blockTimer));
    ESP_ERROR_CHECK(mcpwm_new_timer_sync_src(blockTimer, &masterTimerSyncSetup, &masterTimerSync));
    
    ESP_ERROR_CHECK(mcpwm_new_gpio_sync_src(&gpioMultiSyncSetup, &gpioMultiSync)); //cross module timer sync 
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&softSyncSetup, &softSync));

    for (phaseMcpwm phase: phaseSignals){
     
    }
    for (phaseMcpwm phase: phaseSignals){
        //putting command of setting lowGate Low (by comparator action event) into buffer
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(phase.pwmLowGate,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_DOWN, //up or down
                phase.comparatorLow,
                MCPWM_GEN_ACTION_HIGH  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP, //up or down
                phase.comparatorLow,
                MCPWM_GEN_ACTION_LOW  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));
    }
    // ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(genHandle,
    //     MCPWM_GEN_TIMER_EVENT_ACTION(
    //         MCPWM_TIMER_DIRECTION_UP, //up or down
    //         MCPWM_TIMER_EVENT_EMPTY, // timer to 0, peak, or timer invalid event
    //         MCPWM_GEN_ACTION_LOW // set to same level, low/high level, or toggle
    //     ),
    //     MCPWM_GEN_TIMER_EVENT_ACTION_END()
    // ));
     for (phaseMcpwm phase: phaseSignals){
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, -1, false)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, -1, false)); // Force low until ready

        ESP_ERROR_CHECK(mcpwm_timer_enable(phase.timer));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(phase.timer, MCPWM_TIMER_START_NO_STOP));
     }
}
//for swithcing: ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(phase[1].timer, syncState1));

// [1][2][3] = arr[2][3]
// [4][5][6]
void phaseSwitching(int currentBlockTarget, int blockMap[6][3], int blockNumber, uint32_t blockFrequency){
//REMOVE USELESS VAR PASS
    int previousBlock= currentBlockTarget - 1;
    if(previousBlock <0){
        previousBlock = 5;
    }
    int nextBlock= currentBlockTarget + 1;
    if(nextBlock > 5){
        nextBlock = 0;
    }
    for(phaseMcpwm phase: phaseSignals){
        executeGate(phase, currentBlockTarget, previousBlock);
        //sync here
        preloadNextBlock(phase, currentBlockTarget, nextBlock);
        
    }
}

/*
    only pulldowns highGate when lowGatePWm is High
    or lowGate when timerFrequency is at 20kHz for highGatePWM
*/
//check as5600 to amke sure itdidn;'t double jump
void executeGate(phaseMcpwm phase, int state, int previousState){
    switch (state) {
        case -1: //sink current
            if ( -1 != previousState){
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, 0, true)); //150-300ns
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, -1, false));
            }
            break;
        case 0: //float
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, -1, false));
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, -1, false));
            break;
        case 1: //source current
            if (1 != previousState){
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, -1, false));
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, 0, true));
            }
            break;
        default:
        ESP_LOGE("GATE Execution",":THIS STATE DEOSN'T EXIST %d", state);
    }
    //sync here
}
void preloadNextBlock(phaseMcpwm phase, int previousState, int nextState){
    switch (nextState) {
        case -1: //sink current //keep case to avoid default
            // if ( -1 != previousState){ //if the next state doesn't remain as sink
            //     // ESP_ERROR_CHECK(mcpwm_set_compare_valie(pwmPeriod*duty));
            /*Not setting comparator value because we are forcing generator Low instead*/
            // }
            break;
        case 0: //float
            if(previousState = 1){
                ESP_ERROR_CHECK(mcpwm_timer_set_period(phase.timer, blockPeriod));
            }
            break;
        case 1: //source current
            if (1 != previousState){
                ESP_ERROR_CHECK(mcpwm_timer_set_period(phase.timer, pwmPeriod));
            }
            break;
        default:
        ESP_LOGE("GATE PRELOAD",":THIS STATE DEOSN'T EXIST %d", nextState);
    }
}
/*
    mcpwm_timer_set_period() ;
    **** mcpwm_timer_register_event_callbacks() : timer can generate different events at runtime.
            - call befor timer enable
    **** mcpwm_comparator_register_event_callbacks()-  comparator can be used to trigger event when comparator reaches threshold
    **** mcpwm_generator_set_action_on_sync_event()- sync base trigger event,  MCPWM_GEN_SYNC_EVENT_ACTION
        - doesn't have variadic function 
        mcpwm_generator_set_dead_time()
*/

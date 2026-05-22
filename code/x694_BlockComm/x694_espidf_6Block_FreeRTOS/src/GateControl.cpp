#include "Globals.h"
#include "GateControl.h"
// gpio 19- miso, b High side is tx2
#define syncTickBeforeCMPRThreshold -2
#define GPIO

const uint16_t pwmPeriod = timerResolution/20000;  //change to 20khz when high
//temp gateControl Runtime var
uint32_t compareValue = static_cast<uint32_t>(blockPeriod*duty);
//&^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
phaseMcpwm phaseSignals[3];  
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
//=========================================== SYNC =======================================================
mcpwm_soft_sync_config_t softSyncSetup = {};
mcpwm_sync_handle_t softSync;
mcpwm_timer_sync_src_config_t masterTimerSyncSetup = { //sets a timer as sync trigger
    .timer_event = MCPWM_TIMER_EVENT_EMPTY,
    //getts associated witha. timer
    .flags = {
        .propagate_input_sync = 1,
    }
};
mcpwm_sync_handle_t masterTimerSync;
mcpwm_gpio_sync_src_config_t gpioMultiSyncSetup = {
    .group_id = pwmControllerGroupID,
    .gpio_num = 17,
    .flags = {
        .pull_down =1
    }
};
mcpwm_sync_handle_t gpioMultiSync;
//based on the peak of sineA
mcpwm_timer_sync_phase_config_t syncState = { //config to use sync to sync timers
    .sync_src = gpioMultiSync, //assign to a syn src
    .count_value = 600, //assign phase
    .direction = MCPWM_TIMER_DIRECTION_UP,
};
//========================================================================================================
extern void mcpwmSetup(int startingTargetSector){
    groundSetup();
    int i = 0;
    phaseSignals[0].index = 0;
    phaseSignals[1].index = 1;
    phaseSignals[2].index = 2;
    phaseSignals[0].phaseShift = 5; //A doesn't matter- the block  number chnages direction
    phaseSignals[1].phaseShift = 3; //B -need to set gpio level immediately after sync //2 
    //we need to undo gpio_force and write directly to gpio register to set to assumed level
    phaseSignals[2].phaseShift = 1; //C
    //Start block timer and sync sources
    //startingTargetSector = 0 -> [5/6][3][1]
    //startingTargetSector = 1 -> [0/6][4][2]
    //startingTargetSector = 2 -> [1/6][5][3]
    //startingTargetSector = 3 -> [2/6][0][4]
    //startingTargetSector = 4 -> [3/6][1][5]
    //startingTargetSector = 5 -> [4/6][2][0]
    //
    ESP_ERROR_CHECK(mcpwm_new_timer(&blockTimerSetup, &blockTimer));
    ESP_ERROR_CHECK(mcpwm_new_timer_sync_src(blockTimer, &masterTimerSyncSetup, &masterTimerSync));
    ESP_ERROR_CHECK(mcpwm_new_gpio_sync_src(&gpioMultiSyncSetup, &gpioMultiSync)); //cross module timer sync 
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&softSyncSetup, &softSync));
    
    //making new new componenets and setting compare Thresholds 
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
        phase.syncConfig.count_value = mod6(phase.phaseShift +startingTargetSector)*blockPeriod/3 + syncTickBeforeCMPRThreshold;

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
    
    //putting command of setting lowGate Low and high gate High (by comparator action event) into buffer
    for (phaseMcpwm phase: phaseSignals){
        //Low gate (CMP_LOW)
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
        //High gate (CMP_HIGH)
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(phase.pwmHighGate,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP, //up or down
                phase.comparatorHigh,
                MCPWM_GEN_ACTION_HIGH  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_DOWN, //up or down
                phase.comparatorHigh,
                MCPWM_GEN_ACTION_LOW  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));
        
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(phase.timer, &phase.syncConfig)); 
        // When i get a sync from syncConfig, I will pahse shift phase.timer
    }

    preloadGates(phaseSignals, startingTargetSector, mod6(startingTargetSector+1 ));
     for (phaseMcpwm phase: phaseSignals){
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmHighGate, -1, false)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase.pwmLowGate, -1, false)); // Force low until ready

        ESP_ERROR_CHECK(mcpwm_timer_enable(phase.timer));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(phase.timer, MCPWM_TIMER_START_NO_STOP));
     }
}
void phaseSwitching(int currentBlockTarget, int blockNumber, uint32_t blockFrequency){
    //REMOVE USELESS VAR PASS
// [1][2][3] = arr[2][3]
// [4][5][6]
    int previousBlock= currentBlockTarget - 1;
    if(previousBlock <0){
        previousBlock = 5;
    }
    int nextBlock= currentBlockTarget + 1;
    if(nextBlock > 5){
        nextBlock = 0;
    }
    executeGates(phaseSignals, currentBlockTarget, previousBlock);
    mcpwm_soft_sync_activate(softSync);
    preloadGates(phaseSignals, currentBlockTarget, nextBlock);
    //for swithcing: ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(phase[1].timer, syncState1));
}
void executeGate(phaseMcpwm* phase, int state, int previousState){
    for(int i =0; i <3; i++){
        /*
            only pulldowns highGate when lowGatePWm is High
            or lowGate when timerFrequency is at 20kHz for highGatePWM
        */
        switch (state) {
            case -1: //sink current
                if ( -1 != previousState){
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmHighGate, 0, true)); //150-300ns
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmLowGate, -1, false));
                    //set level after to avoid conflicting force
                    *(PORT_SET[2*i]) |= (1<<(portShift[2*i+1])); //set low gate  HLHLHL -> 012345
                }
                break;
            case 0: //float 
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmHighGate, -1, false));
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmLowGate, -1, false));
                *(PORT_CLEAR[2*i+1]) |= (1<<(portShift[2*i]));
                *(PORT_CLEAR[2*i+1]) |= (1<<(portShift[2*i+1]));
                break;
            case 1: //source current
                if (1 != previousState){
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmHighGate, -1, false));
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmLowGate, 0, true));
                    //get force set level sets other to 0
                    *(PORT_SET[2*i + 1]) |= (1<<(portShift[2*i]));
                }
                break;
            default:
            ESP_LOGE("GATE Execution",":THIS STATE DEOSN'T EXIST %d", state);
        }
    //sync here
    }
}   
void preloadNextBlock(phaseMcpwm* phase, int previousState, int nextState){
    for(int i =0; i <3; i++){
        switch (nextState) {
            //loads next timer periods and the sync timer values
            case -1: //sink current //keep case to avoid default
                // if ( -1 != previousState){ //if the next state doesn't remain as sink
                //     // ESP_ERROR_CHECK(mcpwm_set_compare_valie(pwmPeriod*duty));
                /*Not setting comparator value because we are forcing generator Low instead*/
                // }
                break;
            case 0: //float
                if(previousState = 1){
                    ESP_ERROR_CHECK(mcpwm_timer_set_period(phase[i].timer, blockPeriod));
                }
                break;
            case 1: //source current
                if (1 != previousState){
                    ESP_ERROR_CHECK(mcpwm_timer_set_period(phase[i].timer, pwmPeriod));
                }
                break;
            default:
            ESP_LOGE("GATE PRELOAD",":THIS STATE DEOSN'T EXIST %d", nextState);
        }
        phase[i].syncConfig.count_value= static_cast<uint32_t>(mod6(phase[i].phaseShift + nextState)*blockPeriod/3 + syncTickBeforeCMPRThreshold);
    }
}
/*;
    **** mcpwm_timer_register_event_callbacks() : timer can generate different events at runtime.
            - call befor timer enable
    **** mcpwm_comparator_register_event_callbacks()-  comparator can be used to trigger event when comparator reaches threshold
    **** mcpwm_generator_set_action_on_sync_event()- sync base trigger event,  MCPWM_GEN_SYNC_EVENT_ACTION
        - doesn't have variadic function 
*/

extern int mod6 (int value){ //for single add
    if(value > 5){
        value = 0;
    } else if(value < 0){
        value = 5;
    }
    return value;
}
#include "Globals.h"
#include "GateControl.h"
#include "Initialize.h"
#include "GC.h"
 // Required for PRIu32
#define syncTickBeforeCMPRThreshold -2
// gpio 19- miso, b High side is tx2


//temp gateControl Runtime var
uint32_t CMRA0Threshold;
intr_handle_t sixBlockISR = NULL;
intr_handle_t oneBlockISR = NULL;

inline phaseMcpwm motorH[3];  
inline phaseMcpwm motorL[3];  

void mcpwmSetup(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function){
    // adding tolerance so we definitely won't trigger ETS_PWM0_INTR_SOURCE and then the phaseSwotch/gitpush ISR
    tripleHighOnSync.count_value = 1; //Practically never Changes
    BTimerOnSync.count_value = static_cast<uint32_t>(*bPeriod_pass_by_function-(estimatedI2CReadTimeInMicros*µsToTicksInt)); //is the starting phaseOffset
    ESP_LOGW("mcpwmSetup", "RRRRRRRR countval: %" PRIu32 ", GlobalLowTiemr Period %u  \n", static_cast<uint32_t>(BTimerOnSync.count_value), blockTimerSetup.period_ticks);
    LTimerOnSync.count_value = static_cast<uint32_t>(lowGateLevelCycle[startingTargetSector] *(*bPeriod_pass_by_function)/2)+2; 

    //SET TIMER PERIODS
    CMRA0Threshold = static_cast<uint32_t>((*bPeriod_pass_by_function)*startingDutyHigh);
    ESP_LOGW("mcpwmSetup", "highGate Timer Period (in Ticks): %" PRIu32 ", activePwmPeriod: %d, timerResolution: %d"  ,highDefaultPWMPeriod, (int)activePwmPeriod, (int)timerResolution);
    phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(highDefaultPWMPeriod);
    blockTimerSetup.period_ticks =static_cast<uint32_t>(*bPeriod_pass_by_function); //1 phase every change int
    globalTimerSetupLow.period_ticks =static_cast<uint32_t>(*bPeriod_pass_by_function);
    
    //Setting everything up, but not activating or executing any planned actions
    initializeHighGate(startingTargetSector, CMRA0Threshold); //suppress Hgate to OFF
    initializeLowGate(startingTargetSector, global.CMR_value_3); // suppress Lgate to OFF
    configureLowGateEvents();
    initializeISRsAndSyncs();
    
    //Start and wait out first block to trigger ISR1
    initializeTimer(startingTargetSector, *bPeriod_pass_by_function); //actualy starts all 5 timers
    ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(BTimerTrigger)); // trigger getSector to get another read,
    //sync BTimer to trigger ISR. No harm in syncing tripleHighTimers as well
    
    //Disable suppression so preload can set the timers ot High
    for (int i= 0; i<3; i++){ 
       ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, -1, false)); 
    }
    firstPreload(motorH, motorL, startingTargetSector, *bPeriod_pass_by_function); //Turnsthe correct HighSide Gate ON

    //start 6 block ISR so we can wait for when an interrupt triggers phaseSwitch ISR and enter the cycle
    // ESP_ERROR_CHECK(esp_intr_enable(sixBlockISR)); //disabled when ISr1 and ISR2 merged
   for (int i= 0; i<3; i++){ 
       ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, -1, false)); 
    }
    //NOTE: Only adjust LTimer with sync if we stay in the same block, or blockFrequency updates 
}
void initializeHighGate(int startingTargetSector, uint32_t comparatorOff_Duty){
    for (int i = 2; i >=0 ; i--){
        motorH[i] = {
            .timerConfig = phaseTimerSetupHigh,
            .opConfig = operatorSetupHigh,
        };
        motorH[i].compConfig.flags.update_cmp_on_tez =1; //allow duty cycle adjustment
        motorH[i].pwmConfig.gen_gpio_num = gateArray[2*i];
        ESP_ERROR_CHECK(mcpwm_new_timer(&motorH[i].timerConfig, &motorH[i].timer));
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorH[i].opConfig, &motorH[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_comparator(motorH[i].operatorModule, &motorH[i].compConfig, &motorH[i].comparator0)); //igh needs only 1 gen and cmra
        ESP_ERROR_CHECK(mcpwm_new_generator(motorH[i].operatorModule, &motorH[i].pwmConfig, &motorH[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorH[i].pwmGate0, motorH[i].pwmGate0, &highGateDeadTimeSetup));
        
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorH[i].operatorModule, motorH[i].timer)); //--
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true)); // Force low until ready
    }
    //putting command of setting lowGate Low and high gate High (by comparator action event) into buffer
    for (int i = 2; i >=0 ; i--){
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, //up or down
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_HIGH  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, //up or down
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_LOW  // set to same level, low/high level, or toggle
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event( motorH[i].pwmGate0,
                MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_LOW),
                MCPWM_GEN_TIMER_EVENT_ACTION_END()
            )
        );
    }
}
void initializeLowGate(int startingTargetSector, float threshold_thirds[]){
    for (int i = 2; i >=0 ; i--){
        motorL[i] = {
            //timer defined by globalLow timer
            .opConfig = operatorSetupLow,
        };
        motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
    }
    ESP_ERROR_CHECK(mcpwm_new_timer(&blockTimerSetup, &blockTimer)); //TIMER 0 = BTimer
    ESP_ERROR_CHECK(mcpwm_new_timer(&globalTimerSetupLow, &globalLowTimer)); //TIMER 1= LTimer
    for (int i = 2; i >=0 ; i--){
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); //--

        ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[i].operatorModule, &motorL[i].compConfig, &motorL[i].comparator0)); //igh needs only 1 gen and cmra
        ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));
        // ESP_LOGW("initializeLow Gate", "RRRRRRRR 2 threshold_thirds: %" PRIu32 ", GlobalLowTiemr Period %u , bP: %u \n", static_cast<uint32_t>(threshold_thirds[2]), globalTimerSetupLow.period_ticks, static_cast<int>(global.blockPeriod));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[i].comparator0, 
            static_cast<uint32_t>(threshold_thirds[2])
        )); //all low generators need it
        /*
        */
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true)); // Force low until ready
    }
    #define phaseA_gen_one_third 0 //the index of the Low Generator that has to Compare with 1/
    ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[phaseA_gen_one_third].operatorModule, &motorL[phaseA_gen_one_third].compConfig, &motorL[phaseA_gen_one_third].comparator1)); 
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[phaseA_gen_one_third].comparator1, static_cast<uint32_t>(threshold_thirds[1]))); 
}
void initializeISRsAndSyncs(){
    // ESP_ERROR_CHECK(esp_intr_alloc(
    //         ETS_PWM0_INTR_SOURCE,
    //         ESP_INTR_FLAG_LEVEL3 
    //         |ESP_INTR_FLAG_IRAM
    //         ,
    //         phaseSwitching,
    //         (void *)&global,
    //         &sixBlockISR
    //     )
    // );

    ESP_ERROR_CHECK(esp_intr_alloc(
            ETS_PWM0_INTR_SOURCE,
            ESP_INTR_FLAG_LEVEL3 
            | ESP_INTR_FLAG_IRAM
            ,
            getSectorNumber,
            (void *)&global,
            &oneBlockISR
        )  
    );
    // ESP_ERROR_CHECK(mcpwm_new_gpio_sync_src(&tripleHighSyncSourceSetup, &tripleHighSyncSource)); //cross module timer sync 
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[0]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[1]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[2]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&BTimerTriggerSetup, &BTimerTrigger));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&LTimerTriggerSetup, &LTimerTrigger));
    BTimerOnSync.sync_src = BTimerTrigger; 
    LTimerOnSync.sync_src = LTimerTrigger;
    
    for(int i= 0; i<3; i++){
        tripleHighOnSync.sync_src = tripleHighTrigger[i]; 
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(motorH[i].timer, &tripleHighOnSync));
    }
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
    ESP_LOGW("initializeISRS", "RRRRRRRR countval: %" PRIu32 ", GlobalLowTiemr Period %u  \n", static_cast<uint32_t>(BTimerOnSync.count_value), blockTimerSetup.period_ticks);
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync));
}

void configureLowGateEvents(){
    mcpwm_timer_direction_t dir[2] = {MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_DIRECTION_UP};
    mcpwm_timer_event_t timerEmpty =  MCPWM_TIMER_EVENT_EMPTY;
    mcpwm_generator_action_t action[2] = {MCPWM_GEN_ACTION_LOW, MCPWM_GEN_ACTION_HIGH} ;
    int i =0;
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[0].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[1], motorL[0].comparator0, action[1]), //up , then down
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[0].comparator0, action[0]),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
    //=================
    i=1;
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[i].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[i].comparator0, action[1]),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
     ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event( motorL[i].pwmGate0,
            // MCPWM_GEN_TIMER_EVENT_ACTION(dir[0], timerEmpty, action[0]),
            MCPWM_GEN_TIMER_EVENT_ACTION(dir[1], timerEmpty, action[0]),
            MCPWM_GEN_TIMER_EVENT_ACTION_END()
        )
    );
    //=================
    i=2;
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event( motorL[i].pwmGate0,
            // MCPWM_GEN_TIMER_EVENT_ACTION(dir[0], timerEmpty, action[1]),
            MCPWM_GEN_TIMER_EVENT_ACTION(dir[1], timerEmpty, action[1]),
            MCPWM_GEN_TIMER_EVENT_ACTION_END()
        )
    );
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[i].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[1], motorL[i].comparator0, action[0]),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
}
void initializeTimer(int startingTargetSector, uint32_t bPeriod_pass_by_function){
     for (int i= 0; i<3; i++){
        ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[i].timer));
     }
    ESP_ERROR_CHECK(mcpwm_timer_enable(globalLowTimer));
    ESP_ERROR_CHECK(mcpwm_timer_enable(blockTimer));
    //ACTIVATE ALLLLLLLLLLL
    for (int i= 0; i<3; i++){
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[i].timer, MCPWM_TIMER_START_NO_STOP));
    }
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(globalLowTimer, MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(blockTimer, MCPWM_TIMER_START_NO_STOP));
}
void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm * motorLow, int startingTargetSector, uint32_t bPeriod_pass_by_function){
    //motor is currently not runing, good time to set phase, 5 timers need ot be syncs
    int index= -1;
    for (int i =0; i=0; i--){
    // for (int i =2; i>=0; i--){
        if (gateLevelCycle[startingTargetSector][2*i] == 1) {
            ESP_ERROR_CHECK(mcpwm_timer_set_period(motorH[i].timer, activePwmPeriod));
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, startingGateCmpValue)); 
        } else{
            // ESP_LOGW("firstPreload", " offGateCmpValue" PRIu32 ", offGateCmpValue" PRIu32 "\n", offGateCmpValue, offGateCmpValue); //i didn't know lol
            ESP_LOGW("firstPreload", " offGateCmpValue: %" PRIu32 "\n", offGateCmpValue);
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, offGateCmpValue)); 
        }
    }
    for(int i =2; i>=0; i--){
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(tripleHighTrigger[i]));
    }
}

void IRAM_ATTR preloadGates(phaseMcpwm* highSide, phaseMcpwm* lowSide, int previousState, int nextState, uint32_t bPeriod_pass_by_function){
    if (global.sectorTarget == global.oldSectorTarget) {
        // Low Gates- Precalcuate values to Resync timers to previous Block
        BTimerOnSync.count_value = global.blockPeriod-(estimatedI2CReadTimeInMicros*µsToTicks);
        // tripleHighTrigger.count_value;
        LTimerOnSync.count_value = lowGateLevelCycle[global.sectorTarget] * global.blockPeriod; //n/3 fraction
        if(global.sectorTarget != 1 && global.oldSectorTarget != 4){
            LTimerOnSync.direction = LTimerDir[global.sectorTarget];
        }
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync));
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
        
        // Gpio level remains set by dt_config
        // Set w/ gpio.out1_w1ts for safety (test/calibr) 
    } else {
        if(global.sectorTarget % 2){
            //don't modify low gate timers 
            //Comaprator 0 is that one tha actually can produce rising edges
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[activeHighGate[global.oldSectorTarget]].comparator0, idleHighGateCmpVal));
            ESP_ERROR_CHECK(mcpwm_timer_set_period(motorH[activeHighGate[global.oldSectorTarget]].timer, highDefaultPWMPeriod));
            
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[activeHighGate[global.sectorTarget]].comparator0, startingGateCmpValue));
            ESP_ERROR_CHECK(mcpwm_timer_set_period(motorH[activeHighGate[global.sectorTarget]].timer, activePwmPeriod)); //turn on
        }
        
        // for(int i =0; i <3; i++){
        //     switch (nextState) {
        //         //loads next timer periods and the sync timer values
        //         case -1: //sink current //keep case to avoid default
        //             break;
        //         case 0: //float
        //             if(previousState == 1){
        //                 ESP_ERROR_CHECK(mcpwm_timer_set_period(phase[i].timer, global.blockPeriod));
        //             }
        //             break;
        //         case 1: //source current
        //             if (1 != previousState){
        //                 ESP_ERROR_CHECK(mcpwm_timer_set_period(phase[i].timer, activePwmPeriod));
        //             }
        //             break;
        //         default:
        //         ESP_LOGE("GATE PRELOAD",":THIS STATE DEOSN'T EXIST %d", nextState);
        //     }
        //     phase[i].syncConfig.count_value= static_cast<uint32_t>(mod6(phase[i].phaseShift + nextState)*global.blockPeriod/3 + syncTickBeforeCMPRThreshold);
        // }
    }

}



void IRAM_ATTR phaseSwitching(mcpwm_int_clr_reg_t* clearRegister, mcpwm_dev_t * mcpwm){

    /*
   mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };
   //checks to see if any of ther flags are triggered
    #if (phaseA_gen_one_third == 0) //index that has the 1/3 BP comparatorB
        if(tempStatusReg.timer0_tez_int_st ||
            tempStatusReg.timer0_tep_int_st ||
            tempStatusReg.op0_tea_int_st ||
            tempStatusReg.op0_teb_int_st){ //L TIMER = id1, SO WE USE TIMER 0
            mcpwm_int_clr_reg_t tempClearReg = { .val = 0b0};
            tempClearReg.op0_tea_int_clr = 1;
            tempClearReg.op0_teb_int_clr = 1;
    // #elif (phaseA_gen_one_third == 1)
    //     if(tempStatusReg.timer0_tez_int_st ||
    //         tempStatusReg.timer0_tep_int_st ||
    //         tempStatusReg.op1_tea_int_st ||
    //         tempStatusReg.op1_teb_st){ 
    //         mcpwm_int_clr_reg_t tempClearReg = { .val = 0b0};
    //         tempClearReg.op1_tea_int_clr = 1;
    //         tempClearReg.op1_teb_int_clr = 1;
    
    */
// #endif 
        executeGates(tripleHighTrigger, (size_t)3);
        (mcpwm)->int_clr.val = clearRegister->val;
//    }
}
void IRAM_ATTR executeGates(mcpwm_sync_handle_t * triggers, size_t arraySize){
    for(size_t i =arraySize; i>0; i--){
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate((triggers[i])));
    }
    // for(int i =0; i <3; i++){
    //     switch (state) {
    //         case -1: //sink current
    //             if ( -1 != previousState){
    //                 ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmLowGate, -1, false));//150-300ns
    //                 //set level after to avoid conflicting force
    //                 *(PORT_SET[2*i+1]) |= (1<<(portShift[2*i+1])); //set low gate  HLHLHL -> 012345
    //             }
    //             break;
    //         case 0: //float 
    //             ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmHighGate, -1, false));
    //             ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmLowGate, -1, false));
    //             *(PORT_CLEAR[2*i]) |= (1<<(portShift[2*i]));
    //             *(PORT_CLEAR[2*i+1]) |= (1<<(portShift[2*i+1]));
    //             break;
    //         case 1: //source current
    //             if (1 != previousState){
    //                 ESP_ERROR_CHECK(mcpwm_generator_set_force_level(phase[i].pwmHighGate, -1, false));
    //                 //get force set level sets other to 0
    //                 *(PORT_SET[2*i]) |= (1<<(portShift[2*i]));
    //             }
    //             break;
    //         default:
    //         ESP_LOGE("GATE Execution",":THIS STATE DEOSN'T EXIST %d", state);
    //     }
    // //sync here
    // }
}  

int mod6 (int value){ //for single add
    if(value > 5){
        value = 0;
    } else if(value < 0){
        value = 5;
    }
    return value;
}
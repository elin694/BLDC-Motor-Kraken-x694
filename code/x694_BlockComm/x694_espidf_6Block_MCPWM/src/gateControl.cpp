// TIMER 0 = BTimer
#include "GateControl.h"
#include "Initialize.h"
#include "GC.h"
void mcpwmSetup(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function){
    ESP_LOGW("debug", magenta " -------------bPeriod %d \n",  (int)*bPeriod_pass_by_function);
    setCountValueAndPeriod(startingTargetSector, bPeriod_pass_by_function);
    // initializeHighGate(startingTargetSector, CMRA0Threshold); //suppress Hgate to OFF, CMRA0 NEVER actually used
    initializeLowGate(startingTargetSector, global.CMR_value_3); // suppress Lgate to OFF
    configureLowGateEvents();
    //LIGHT THE FUSE:::::::::::::::✨✨✨✨✨:Start and wait out first block to trigger ISR1
    // initializeISR();
    initializeSyncs();//un comment all high gate events past here
    initializeTimer(startingTargetSector, *bPeriod_pass_by_function); 
    // initializeInterruptEnablePin(); //after isr init 
    // ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(BTimerTrigger));
    ESP_LOGI( "GC6.5", "====Activate BtimeTrigger getSector to get another read and ISR");
    for (int i= 0; i<3; i++){ 
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, -1, false)); 
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, -1, false)); 
    }
    firstPreload(motorH, motorL, startingTargetSector, *bPeriod_pass_by_function);
    activateAllSyncs();
    //start 6 block ISR so we can wait for when an interrupt triggers phaseSwitch ISR and enter the cycle
    // ESP_ERROR_CHECK(esp_intr_enable(sixBlockISR)); //disabled when ISr1 and ISR2 merged
    //NOTE: Only adjust LTimer with sync if we stay in the same block, or blockFrequency updates 
}

void setCountValueAndPeriod(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function1){
      // adding tolerance so we definitely won't trigger ETS_PWM0_INTR_SOURCE and then the phaseSwotch/gitpush ISR
    tripleHighOnSync.count_value = 1; //Practically does not need to be changed
    BTimerOnSync.count_value = static_cast<uint32_t>(*bPeriod_pass_by_function1-(estimatedI2CReadTimeInMicros*µsToTicksInt)); //is the starting phaseOffset

    ESP_LOGW("debug", magenta " -------------stgts:%d, LGLC %f, bPeriod %d \n", startingTargetSector, lowGateLevelCycle[startingTargetSector], (int)(*bPeriod_pass_by_function1));
    LTimerOnSync.count_value = static_cast<uint32_t>(lowGateLevelCycle[startingTargetSector] *(*bPeriod_pass_by_function1)/2)+2;
    ESP_LOGW("GC1 OnSyncValues", "Btimer count_val: %" PRIu32 ", LowTimer count_val %d \n", BTimerOnSync.count_value, (int)LTimerOnSync.count_value);

    CMRA0Threshold = static_cast<uint32_t>((*bPeriod_pass_by_function1)*startingDuty);
    // ESP_LOGW("highTimer", "Period (in ticks): %" PRIu32 ", Period_when_on: %d, timerResolution: %d"  ,highDefaultPWMPeriod, (int)activePwmPeriod, (int)timerResolution);
    phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(highDefaultPWMPeriod);
    blockTimerSetup.period_ticks =static_cast<uint32_t>(*bPeriod_pass_by_function1); //1 phase every change int
    globalTimerSetupLow.period_ticks =static_cast<uint32_t>(6*(*bPeriod_pass_by_function1));
    ESP_LOGW(blue "GC2 Group 1 Periods (ticks)", blue "Ltimer Period: %d , Btimer Period: %d ", (int) globalTimerSetupLow.period_ticks, blockTimerSetup.period_ticks);
    ESP_LOGI( "GC 00", "====SET ONSYNC count value and Period ticks fo rall timers");
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
    ESP_LOGI("======high waves component linked to handles. C0 and T_low set action onto G0", " ");
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
        ESP_LOGW("initLG", "RRRRRRRR 2 threshold_thirds: %" PRIu32 ", LowTimerPeriod %u , bP: %u \n", 
            static_cast<uint32_t>(threshold_thirds[2]), globalTimerSetupLow.period_ticks, static_cast<int>(global.blockPeriod));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[i].comparator0, 
            static_cast<uint32_t>(threshold_thirds[2])
        )); //all low generators need it
        /*
        */
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true)); // Force low until ready
    }
    #define phaseA_gen_one_third 0 //the index of the Low Generator that has to Compare with 1/
    ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[phaseA_gen_one_third].operatorModule,
        &motorL[phaseA_gen_one_third].compConfig,
        &motorL[phaseA_gen_one_third].comparator1)); 
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[phaseA_gen_one_third].comparator1, static_cast<uint32_t>(threshold_thirds[1]))); 
    ESP_LOGI("GC3", "=====block + low gate timer started. all LOW waves have tocgd. Operator cnnct to timer. C0's set to 2/3 Block period, o0c1 to 1/3.===== ");
}

void initializeISR(){
    /*
    /////Solution2 : using callback evt
    // mcpwm_comparator_register_event_callbacks(comparator, );
    // // mcpwm_timer_event_callbacks_t timer_isr = {

    // // };
    // mcpwm_comparator_event_callbacks_t = {
    //     .onreach
    // }
    // mcpwm_compare_event_cb_t
    // mcpwm_compare_event_data_t = {
    //     .compare_ticks = int,
    //     .direction =
    // }
    */
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
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00))};
    MCPWMx->int_clr.val=  clearReg.val;
    MCPWMx->int_ena.timer0_tez_int_ena = 0;
    MCPWMx->int_ena.timer1_tez_int_ena = 0;
    MCPWMx->int_ena.timer1_tep_int_ena = 0;
    MCPWMx->int_ena.op0_tea_int_ena = 0;
    MCPWMx->int_ena.op0_teb_int_ena = 0;
    ESP_LOGI("======esp_alloc_intr used to allocate ISR line before flushing all related intr_ena bits", " ");
}

void initializeInterruptEnablePin(){
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00))};
    MCPWMx->int_clr.val=  clearReg.val;
    MCPWMx->int_ena.timer0_tez_int_ena = 1;
    MCPWMx->int_ena.timer1_tez_int_ena = 1;
    MCPWMx->int_ena.timer1_tep_int_ena = 1;
    MCPWMx->int_ena.op0_tea_int_ena = 1;
    MCPWMx->int_ena.op0_teb_int_ena = 1;
    ESP_LOGI("======CLEARing INTR REGISTER AND PUSHing INTR_ENABLE BITS", " ");
}

void initializeSyncs(){
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[0]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[1]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[2]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&BTimerTriggerSetup, &BTimerTrigger));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&LTimerTriggerSetup, &LTimerTrigger));
    BTimerOnSync.sync_src = BTimerTrigger; 
    LTimerOnSync.sync_src = LTimerTrigger;
    
    for(int i= 0; i<3; i++){
        tripleHighOnSync.sync_src = tripleHighTrigger[i]; 
        // ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(motorH[i].timer, &tripleHighOnSync));
    }
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
    // ESP_LOGW("initializeISRS", "RRRRRRRR countval: %" PRIu32 ", LowTiemr Period %u  \n", BTimerOnSync.count_value, blockTimerSetup.period_ticks);
    ESP_LOGI("init syncs", "Block timer count val %d",(int)(BTimerOnSync.count_value));
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync));

    ESP_LOGI("GC5", "======Sync handle, phase shift, and Sync sources linked");
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
    ESP_LOGI("GC4", "======All low gate actions have been set ");
}

void initializeTimer(int startingTargetSector, uint32_t bPeriod_pass_by_function){
     for (int i= 0; i<3; i++){
        // ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[i].timer));
     }
     ESP_ERROR_CHECK(mcpwm_timer_enable(globalLowTimer));
     ESP_ERROR_CHECK(mcpwm_timer_enable(blockTimer));
     //ACTIVATE ALLLLLLLLLLL
     for (int i= 0; i<3; i++){
         // ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[i].timer, MCPWM_TIMER_START_NO_STOP));
        }
        ESP_LOGW(blue "aa", blue "Starting block timer nxtl" red);
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(blockTimer, MCPWM_TIMER_START_NO_STOP));
        ESP_LOGW(blue "aa", blue "Starting Low timer nxtl" red);
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(globalLowTimer, MCPWM_TIMER_START_NO_STOP));
    ESP_LOGI("GC6", "======ENABLES AND STARTS counting on all 5 timers ");
}

void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm * motorLow, int startingTargetSector, uint32_t bPeriod_pass_by_function){
    //motor is currently not runing, good time to set phase, 5 timers need ot be syncs

    // ESP_LOGI("FIRSST PRELOAD", "ACTIVE PWM PERIOD %d",(int)(activePwmPeriod));
    // #define apple
    #ifdef apple
    for (int i =0; i<3; i++){
    // for (int i =2; i>=0; i--){
        if (gateLevelCycle[startingTargetSector][2*i] == 1) {
            ESP_ERROR_CHECK(mcpwm_timer_set_period(motorH[i].timer, activePwmPeriod));
            // ESP_LOGE("FIRST PRELOAD: ", "startingHighGateCMPVal %u, offgate %u, period range: %u", startingGateCmpValue, offGateCmpValue, activePwmPeriod);
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, startingGateCmpValue)); 
        } else{
            // ESP_LOGW("firstPreload", " offGateCmpValue" PRIu32 ", offGateCmpValue" PRIu32 "\n", offGateCmpValue, offGateCmpValue); //i didn't know lol
            // ESP_LOGW("firstPreload", " offGateCmpValue: %" PRIu32 "\n", offGateCmpValue);
            // ESP_LOGE("FIRST PRELOAD: ", "offPeriod offgate %u", offGateCmpValue, );
            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, offGateCmpValue)); 
        }
    }
    #endif
    ESP_LOGI("GC7", "======Set Compare Values for each High Side");
}

void activateAllSyncs(){
      for(int i =2; i>=0; i--){
        // ESP_ERROR_CHECK(mcpwm_soft_sync_activate(tripleHighTrigger[i]));
    }
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(BTimerTrigger));
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(LTimerTrigger));
    ESP_LOGI("GC8", "======Pushes compare values by activating all syncs");
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
        // executeGates(tripleHighTrigger, (size_t)3);
        (mcpwm)->int_clr.val = clearRegister->val;
//    }
}
void IRAM_ATTR executeGates(mcpwm_sync_handle_t * triggers, size_t arraySize){
    for(size_t i =arraySize; i>0; i--){
        esp_rom_printf(yellow "itarting at indexi");
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate((triggers[0])));
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

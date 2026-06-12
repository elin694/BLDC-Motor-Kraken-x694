// TIMER 0 = BTimer
#include "GateControl.h"
#include "Initialize.h"
#include "GC.h"
void mcpwmSetup(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function){
    setCountValueAndPeriod(startingTargetSector, bPeriod_pass_by_function);
    initializeLowGate(startingTargetSector, global.CMR_value_3); // suppress Lgate to OFF
    configureLowGateEvents();
    // initializeHighGate(startingTargetSector, CMRA0Threshold); //suppress Hgate to OFF, CMRA0 NEVER actually used
    initializeSyncs();
    initializeISR();
    
    initializeTimer(startingTargetSector, *bPeriod_pass_by_function); 
    esp_rom_delay_us(1*ticksToµs+1); 
    synchrISR(BTimerTrigger, "Pre-intr_ena BT Sync"); //start after to mimic phase offset wave
    synchrISR(LTimerTrigger, "Pre-intr_ena LT Sync"); //start after to mimic phase offset wave
    
    int bt12 = MCPWM0.timer[0].timer_status.timer_value;
    int lt12 = MCPWM0.timer[1].timer_status.timer_value;
    esp_rom_printf(red "Bc %5d Lc %5d, (ot,nt): %d, %d \n", bt12, lt12, global.oldSectorTarget, global.sectorTarget);
    synchrISR(BTimerTrigger, "Post-intr_ena B   T Sync"); //0-1us
    synchrISR(LTimerTrigger, "Post-intr_ena LT Sync"); //0-1us 
    esp_rom_delay_us(ticksToµs+1); //calib minimum

    initializeInterruptEnablePin(); //after isr init  and L sync 
    // ESP_LOGI( "GC6.5", "====activate BtimeTrigger getSector to get another read and ISR");
    firstPreload(motorH, motorL, startingTargetSector, *bPeriod_pass_by_function);

    // activateAllSyncs();
    /*
        //start 6 block ISR so we can wait for when an interrupt triggers phaseSwitch ISR and enter the cycle
        // ESP_ERROR_CHECK(esp_intr_enable(sixBlockISR)); //disabled when ISr1 and ISR2 merged
        //NOTE: Only adjust LTimer with sync if we stay in the same block, or blockFrequency updates 
    */
}

void setCountValueAndPeriod(int startingTargetSector, volatile uint32_t * bPeriod_pass_by_function1){
    // adding tolerance so we definitely won't trigger ETS_PWM0_INTR_SOURCE and then the phaseSwotch/gitpush ISR
   phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(activePwmPeriod);
   CMRA0Threshold = static_cast<uint32_t>(startingGateCmpValue);
   //SET PERIOD TICKS
    blockTimerSetup.period_ticks =static_cast<uint32_t>(*bPeriod_pass_by_function1); //1 phase every change int
    globalTimerSetupLow.period_ticks =static_cast<uint32_t>(6*(*bPeriod_pass_by_function1));
    ESP_LOGW(blue "GC1  Periods (in ticks)", blue "Btimer Period: %d , ltimer Period: %d ", (int)blockTimerSetup.period_ticks, (int) globalTimerSetupLow.period_ticks);

    tripleHighOnSync.count_value = 1; //Practically does not need to be changed
    BTimerOnSync.count_value = estimatedI2CReadTimeInTicks; //is the starting phaseOffset

    int excess = 0;
    // if (LTimerDir[startingTargetSector] == MCPWM_TIMER_DIRECTION_DOWN){ excess = -3;} else{ excess = 3;}
    
    LTimerOnSync.direction = LTimerDir[startingTargetSector];
    LTimerOnSync.count_value = static_cast<uint32_t>(lowGateLevelCycle[startingTargetSector] *(*bPeriod_pass_by_function1) + excess);

    ESP_LOGW(magenta "GC 1.5 OnSyncValues", "Btimer c_v: %d, LTimer c_v %d, excess: %d", (int)BTimerOnSync.count_value, (int)LTimerOnSync.count_value, excess);
}

void initializeLowGate(int startingTargetSector, float threshold_thirds[]){
    for (int i = 0; i <3; i++){
        motorL[i] = { .opConfig = operatorSetupLow};
        ESP_LOGE("initLG: first set", "i: %d, first actual pin number %d",i, motorL[i].pwmConfig.gen_gpio_num);
        motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
    }
    //TIMER 0 = BTimer
    ESP_LOGI("initLG"," first set");
    ESP_ERROR_CHECK(mcpwm_new_timer(&blockTimerSetup, &blockTimer));
    ESP_LOGW("initLG"," first set");

    //TIMER 1= LTimer
    ESP_ERROR_CHECK(mcpwm_new_timer(&globalTimerSetupLow, &globalLowTimer)); 
    ESP_LOGW("      initLG", "2/3 peak : %d, BTimer Period: %u, LTimer Period %u \n",  
    (int)(threshold_thirds[2]), static_cast<int>(global.blockPeriod),  globalTimerSetupLow.period_ticks);
    for (int i = 0; i <3; i++){
        //AL op0, BL op1, CL op2
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[i].operatorModule, &motorL[i].compConfig, &motorL[i].comparator0)); //igh needs only 1 gen and cmra
        ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[i].operatorModule, &motorL[i].compConfig, &motorL[i].comparator1)); //igh needs only 1 gen and cmra
        ESP_LOGE("initLG: immediately b4 init", "i: %d, first actual pin number %d",i, motorL[i].pwmConfig.gen_gpio_num);
        ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
        // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));

        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); 
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[i].comparator0, 
            static_cast<uint32_t>(threshold_thirds[2])
        )); //all low generators need it
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true)); // Force low until ready
    }
    #define phaseA_gen_one_third 0 //the index of the Low Generator that has to Compare with 1/
    // ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[phaseA_gen_one_third].operatorModule,
    //     &motorL[phaseA_gen_one_third].compConfig,
    //     &motorL[phaseA_gen_one_third].comparator1)); 
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[phaseA_gen_one_third].comparator1, static_cast<uint32_t>(threshold_thirds[1]))); 
    ESP_LOGW("GC3", "=====block + LTimer started. all LOW waves have tocgd. Operator cnnct to timer. C0's set to 2/3 Block period, o0c1 to 1/3.===== ");
    
}

void configureLowGateEvents(){
    mcpwm_timer_direction_t dir[2] = {MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_DIRECTION_UP};
    mcpwm_generator_action_t action[2] = {MCPWM_GEN_ACTION_LOW, MCPWM_GEN_ACTION_HIGH} ;
    mcpwm_timer_event_t timerEmpty =  MCPWM_TIMER_EVENT_EMPTY;
    int index1 =0;


    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[0].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[1], motorL[index1].comparator0, action[1]), //up , then down
        // MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[index1].comparator0, MCPWM_GEN_ACTION_TOGGLE),
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[index1].comparator0, action[0]),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
    /* =================*/
    index1=1;
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[index1].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[index1].comparator0, action[1]),
        // MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[index1].comparator0, MCPWM_GEN_ACTION_TOGGLE),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
     ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event( motorL[index1].pwmGate0,
            MCPWM_GEN_TIMER_EVENT_ACTION(dir[1], timerEmpty, action[0]),
            MCPWM_GEN_TIMER_EVENT_ACTION_END()
        )
    );
    /* =================*/
    index1=2;
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event( motorL[index1].pwmGate0,
            MCPWM_GEN_TIMER_EVENT_ACTION(dir[1], timerEmpty, action[1]),
            // MCPWM_GEN_COMPARE_EVENT_ACTION(dir[0], motorL[index1].comparator0, MCPWM_GEN_ACTION_TOGGLE),
            MCPWM_GEN_TIMER_EVENT_ACTION_END()
        )
    );
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorL[index1].pwmGate0,
        MCPWM_GEN_COMPARE_EVENT_ACTION(dir[1], motorL[index1].comparator0, action[0]),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
    ESP_LOGI("GC4", "======All low gate actions have been set ");
}

void initializeHighGate(int staartingTargetSector, uint32_t comparatorOff_Duty){
    for (int i = 0; i <3 ; i++){
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
        // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorH[i].pwmGate0, motorH[i].pwmGate0, &highGateDeadTimeSetup));
        
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorH[i].operatorModule, motorH[i].timer)); 
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true)); // Force low until ready
    }
    //putting command of setting lowGate Low and high gate High (by comparator action event) into buffer
    for (int i = 0; i <3; i++){
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

void initializeISR(){
    /*
        ///Solution2 : using callback evt
        mcpwm_comparator_register_event_callbacks(comparator, );
        mcpwm_timer_event_callbacks_t timer_isr = {};
        mcpwm_comparator_event_callbacks_t = {}
        mcpwm_compare_event_cb_t
        mcpwm_compare_event_data_t = {}
    */
   mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val= 0xFFFFFFFF;
    ESP_ERROR_CHECK(esp_intr_alloc(
        ETS_PWM0_INTR_SOURCE,
        ESP_INTR_FLAG_LEVEL3 
        | ESP_INTR_FLAG_IRAM
        ,
        getSectorNumber,
        (void *)&global,
        &oneBlockISR
    ));
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_LOGI("======esp_alloc_intr used to allocate ISR line before flushing all related intr_ena bits", " ");
}

void initializeInterruptEnablePin(){
    // ------#ifdef phaseA_gen_one_third 
    // ------&motorL[phaseA_gen_one_third].comparator1
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_clr.val=  clearReg.val;
    
    //block timer = 0
    MCPWMx->int_ena.timer0_tez_int_ena = 1; // //timer 0= BTimer
    /*all in lowside, and in accordance to phaseA_gen_one_third*/
    MCPWMx->int_ena.timer1_tez_int_ena = 1; //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
    MCPWMx->int_ena.timer1_tep_int_ena = 1; //timer 1= LTimer, (ie change from block 0-1), 2^7 = 128
    MCPWMx->int_ena.op0_tea_int_ena = 1; // op0 = phase A lowside, (ie change from block 5-0 or 1-2), 2^15 = 32765
    MCPWMx->int_ena.op0_teb_int_ena = 1; // timer, (ie change from block 2-3 or 4-5), 2^18 = 262144
    ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
    // ESP_LOGW(cyan "initIEP esp_rom time", "%d", (int) esp_timer_get_time());
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
    ESP_LOGI(magenta "          init syncs", "Block timer count val: %d",(int)(BTimerOnSync.count_value));
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync));

    ESP_LOGI(magenta "GC5","======Sync handle, phase shift, and Sync sources linked");
}

void initializeTimer(int startingTargetSector, uint32_t bPeriod_pass_by_function){
     for (int i= 0; i<3; i++){
        // ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[i].timer));
     }
     ESP_ERROR_CHECK(mcpwm_timer_enable(globalLowTimer));
     ESP_ERROR_CHECK(mcpwm_timer_enable(blockTimer));
     ESP_LOGI(blue "GC6", "======ENABLES AND STARTS counting on all 5 timers ");
     for (int i= 0; i<3; i++){
         // ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[i].timer, MCPWM_TIMER_START_NO_STOP));
        }
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(globalLowTimer, MCPWM_TIMER_START_NO_STOP));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(blockTimer, MCPWM_TIMER_START_NO_STOP));
}

void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm * motorLow, int startingTargetSector, uint32_t bPeriod_pass_by_function){
    //motor is currently not runing, good time to set phase, 5 timers need ot be syncs

    for (int i= 0; i<3; i++){ 
        //     // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, -1, false)); 
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, -1, true)); 
    }
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
    for(int i= 0; i< 3; i++){
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 1, false));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, gateLevelCycle[global.sectorTarget][2*i+1], false));
    }
    ESP_LOGI("GC7", "======Set Compare Values for each High Side \n \n");
}

void activateAllSyncs(){
      for(int i =0; i < 3; i++){
        // synchr(tripleHighTrigger[i], "triple high triggers");
    }
    ////////////// synchr(BTimerTrigger, "BTimerSync");
    synchr(LTimerTrigger, "LTimerSync");
    ESP_LOGI("GC8", "======Pushes compare values by actively syncing");
}

void synchr(mcpwm_sync_handle_t handle, std::string name){
    ESP_LOGI(white "              ", "syncing %s", name.c_str());
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(handle));
}

void IRAM_ATTR synchrISR(mcpwm_sync_handle_t handle, const char* name){ 
    //if code only activites sync, execution time <1us
    // esp_rom_printf(white "         syncing %s \n", name); //109us/2 faster than synchr, still 194us/2
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(handle)); //0-1us
}

void IRAM_ATTR getTimerCountNow(const char* str){
    int bt1 = MCPWM0.timer[0].timer_status.timer_value;
    int lt1 = MCPWM0.timer[1].timer_status.timer_value;
    esp_rom_printf(green "%s Bc %5d Lc %5d\n", str, bt1, lt1);
}





//for isr1
void IRAM_ATTR preloadGates(int previousState, int nextState, uint32_t bPeriod_pass_by_function, mcpwm_dev_t * mcpwm, uint32_t clearMask){
    if (global.sectorTarget == global.oldSectorTarget) { //Motor stalling case
        //The phase that is high doesn't change
        // Low Gates- Precalcuate values to Resync timers to previous Block
        BTimerOnSync.count_value = estimatedI2CReadTimeInTicks; //Test if needed
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync)); //Test if needed
        
        LTimerOnSync.count_value = lowGateLevelCycle[global.sectorTarget] * global.blockPeriod; //n/3 fraction
        LTimerOnSync.direction = LTimerDir[global.sectorTarget];
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
        esp_rom_printf(green " pg SYNC BCV %d, LCV %d \n", (int)BTimerOnSync.count_value, (int)LTimerOnSync.count_value);

        // Gpio level remains set by dt_config
        // Set w/ gpio.out1_w1ts for safety (test/calibr) 
    } 
    // else if (global.sectorTarget % 2){ //the case where High Phase switches(ex. 0->1, 2->3, 4->5) 
        //don't modify low gate timers (they'll always be the same when high gates swap)
        // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[activeHighGate[global.sectorTarget]].comparator0, startingGateCmpValue));
        // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[activeHighGate[global.oldSectorTarget]].comparator0, idleHighGateCmpVal));
        
    // } //extra case of 1->2, 3->4, 5->0, but high gate is the same and comparatorActions wills set LGates low --> no code needed
    mcpwm-> int_clr.val = clearMask;

    // esp_rom_printf(white "ISR2E gpiolvl: %d\n",gpio_get_level(digitalReadPin));
    // MCPWM0.timer[0].timer_status.timer_value; //block tiemr count
    // esp_rom_printf(magenta "iR:%d, TC: %d", (int)(mcpwm-> int_st.val), MCPWM0.timer[1].timer_status.

}
///////////////BETWEEN THIS DELAY IS WHEN THE LED TURNS OFF CODEBLUE
void IRAM_ATTR executeGates(mcpwm_int_clr_reg_t* clearRegister, mcpwm_dev_t * mcpwm){
    for(int i =0; i<3; i++){
        // synchrISR(tripleHighTrigger[i], "a"); //v3.14 - updates cmp value only
    }
    //code below is not outdateed
    // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[activeHighGate[global.oldSectorTarget]].comparator0, idleHighGateCmpVal)); //can be done in a normal fucntion
    // if(global.oldSectorTarget != global.sectorTarget){//order with its own sync doesn't matter
    //     ESP_ERROR_CHECK(mcpwm_generator_set_ force_level (motorH[activeHighGate[global.oldSectorTarget]].pwmGate0, 0 , true)); //turn off old one by setting to 0!
    //     ESP_ERROR_CHECK(mcpwm_generator_set_ force_level (motorH[activeHighGate[global.sectorTarget]].pwmGate0, -1, true)); //turn off old one by setting to 0!
    // }
    #define del (ticksToµs+1)
    
    if(global.sectorTarget == global.oldSectorTarget){
        int t1= esp_timer_get_time();
        synchrISR(LTimerTrigger, "Ltimer On!"); //can'ts wap for some reason
        int a1= global.sectorTarget/2;
        int a2 = gateLevelCycle[global.sectorTarget][2*global.sectorTarget/2];
        
        esp_rom_printf("p%d lvl%d \n", a1, a2);
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(
            // motorL[global.sectorTarget/2].pwmGate0,
            // gateLevelCycle[global.sectorTarget][2* global.sectorTarget/2],
            motorL[a1].pwmGate0,a2,
            true
        ));
        
        int nST = mod6(global.sectorTarget+dir);
        a1= nST/2;
        a2 = gateLevelCycle[nST][2*nST/2];
        esp_rom_printf("p%d lvl%d \n", a1, a2);
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(
            // motorL[nST/2].pwmGate0,
            // gateLevelCycle[nST][2*nST/2],
            motorL[a1].pwmGate0, a2,
            true
        ));
        p_stalled = true;
        // t1= esp_timer_get_time() - t1;esp_rom_printf("t1: %d\n", t1);    
            
        } else if(p_stalled){
        esp_rom_printf("yeeerrt \n");
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(
            motorL[global.oldSectorTarget/2].pwmGate0,
            gateLevelCycle[global.oldSectorTarget][2*global.oldSectorTarget/2],
            false
        ));
        int nST = global.sectorTarget;
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(
            motorL[nST/2].pwmGate0,
            gateLevelCycle[nST][nST/2],
            false
        ));
        p_stalled=false;
    }
    synchrISR(BTimerTrigger, "Btimer");
    esp_rom_delay_us(del); //delay between 2 syncs, needs to be long enough for the first sync to trigger the timer event and execute the generator action that turns on the gate, but short enough to not cause a noticeable delay in the waveform.

    // getTimerCountNow("\n");
    // esp_rom_printf(white "GPIOLVL abc: %d, %d, %d\n \n",intermediateGpioLvl[0], intermediateGpioLvl[1], intermediateGpioLvl[2]);

    /* 
        tea, tep, tea,teb,tez,teb
        apa,bzb
        log_2 : 15, 7, 15,  18, 4, 18 [0-5]
        32768, 128, 32768, 262144, 16, 262144
        3,1,3,2,1,2
    */
    mcpwm->int_clr.val = clearRegister->val;
}
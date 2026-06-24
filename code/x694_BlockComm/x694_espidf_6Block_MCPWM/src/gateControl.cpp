// TIMER 0 = BTimer
#include "GateControl.h"
#include "Initialize.h"
#include "GC.h"
void mcpwmSetup(int startingTargetSector){
    int tVal[3] ={0,0,0};
    setCountValueAndPeriod(startingTargetSector);
    initializeLowGate(startingTargetSector, global.CMR_value_3); // suppress Lgate to OFF
    configureLowGateEvents();
    initializeHighGate(startingTargetSector, CMRA0Threshold); //suppress Hgate to OFF, CMRA0 NEVER actually used
    initializeSyncs();
    initializeISR();
    initializeTimer(startingTargetSector); 
    
    firstPreload(motorH, motorL, startingTargetSector);
     for(int i =0; i < 3; i++){
        synchr(tripleHighTrigger[i], "triple high triggers");
    }
    synchrISR(BTimerTrigger, ""); //0-1us, Post-intr_ena BTSync
    synchrISR(LTimerTrigger, ""); //0-1us, Post-intr_ena LT Sync
    synchrISR(VTimerTrigger, ""); //0-1us ,Post-intr_ena VT Sync
    for(int i =0; i < 3; i++){
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, -1, true));
    }
    tVal[0] =MCPWM0.timer[0].timer_status.timer_value; //block
    tVal[1] =MCPWM0.timer[1].timer_status.timer_value;  //low
    tVal[2] =MCPWM0.timer[2].timer_status.timer_value;  //velecoty
    esp_rom_printf(red "Bc %5d Lc %5d, Vc %5d (ot,nt): %d, %d \n", tVal[0], tVal[1], tVal[2], global.oldSectorTarget, global.sectorTarget);
    
    esp_rom_delay_us(ticksToµs); //Vtimer tick may not have passed
    initializeInterruptEnablePin(); //after isr init  and L sync 
}

void setCountValueAndPeriod(int startingTargetSector){
    // adding tolerance so we definitely won't trigger ETS_PWM0_INTR_SOURCE and then the phaseSwotch/gitpush ISR
    CMRA0Threshold = static_cast<uint32_t>(startingGateCmpValue);

   phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(activePwmPeriod);
   //SET PERIOD TICKS
    I2CReadTimerSetup.period_ticks =static_cast<uint32_t>(SetAs5600PollPeriod); //1 phase every change int
    globalTimerSetupLow.period_ticks =static_cast<uint32_t>(SetAs5600PollPeriod);
    velocityTrackerTimerSetup.period_ticks = global.blockPeriod;

    tripleHighOnSync.count_value = 0; //Practically does not need to be changed
    int excess = 0;
    // if (LTimerDir[startingTargetSector] == MCPWM_TIMER_DIRECTION_DOWN){ excess = -3;} else{ excess = 3;}
    // LTimerOnSync.direction = LTimerDir[startingTargetSector];
    // LTimerOnSync.count_value = static_cast<uint32_t>((*SetAs5600PollPeriod) + excess);
    VTimerOnSync.count_value = 0;
    BTimerOnSync.count_value = estimatedI2CReadTimeInTicks; //is the starting phaseOffset
    LTimerOnSync.count_value = static_cast<uint32_t>(0 + excess);

    ESP_LOGW(white "GC 1.5 ON_SYNC_VALUES-setCountValueAndPeriod", "\n BTIMER_CV_OS: %d\n LTIMER_CV_OS %d\nVTIMER_CV_OS %d\n excess: %d\n", (int)BTimerOnSync.count_value, (int)LTimerOnSync.count_value, (int)VTimerOnSync.count_value, excess);
    ESP_LOGW("GC 1.75 PERIODS-setCountValueAndPeriod", "\n BTIMER: %d\n LTIMER %d\nVTIMER %d\n", (int)I2CReadTimerSetup.period_ticks, (int)globalTimerSetupLow.period_ticks, (int)velocityTrackerTimerSetup.period_ticks);
}

void initializeLowGate(int startingTargetSector, float threshold_thirds[]){
    for (int i = 0; i <3; i++){
        motorL[i] = { .opConfig = operatorSetupLow};
        motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
    }
    //TIMER 0 = BTimer
    ESP_ERROR_CHECK(mcpwm_new_timer(&I2CReadTimerSetup, &blockTimer));
    //TIMER 1= LTimer
    ESP_ERROR_CHECK(mcpwm_new_timer(&globalTimerSetupLow, &globalLowTimer)); 

    velocityTrackerTimerSetup.resolution_hz = timerResolution;
    ESP_ERROR_CHECK(mcpwm_new_timer(&velocityTrackerTimerSetup, &velocityTrackerTimer)); 
    MCPWM0.clk_cfg.clk_prescale = mcpwm_lowSideGroupPrescale-1;
    MCPWM0.timer[0].timer_cfg0.timer_prescale= 160e6/timerResolution/mcpwm_lowSideGroupPrescale-1;
    MCPWM0.timer[1].timer_cfg0.timer_prescale= 160e6/timerResolution/mcpwm_lowSideGroupPrescale -1;
    MCPWM0.timer[2].timer_cfg0.timer_prescale= 160e6/VTimerResolution/mcpwm_lowSideGroupPrescale-1;
    ESP_LOGW("GC timerPrescalers", "%d %d %d", MCPWM0.timer[0].timer_cfg0.timer_prescale,MCPWM0.timer[1].timer_cfg0.timer_prescale,MCPWM0.timer[2].timer_cfg0.timer_prescale);
    

    // ESP_LOGW(white "initLG", "2/3 peak: %d, \n BTIMER PERIOD: %u, \n LTIMER_PERIOD %u \n \n",  
    // (int)(threshold_thirds[2]), static_cast<int>(global.blockPeriod),  globalTimerSetupLow.period_ticks);
    for (int i = 0; i <3; i++){
        //AL op0, BL op1, CL op2
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
        // ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[i].operatorModule, &motorL[i].compConfig, &motorL[i].comparator0)); //igh needs only 1 gen and cmra
        // ESP_ERROR_CHECK(mcpwm_new_comparator(motorL[i].operatorModule, &motorL[i].compConfig, &motorL[i].comparator1)); //igh needs only 1 gen and cmra

        ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));

        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); 
        // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[i].comparator0, 
            // static_cast<uint32_t>(threshold_thirds[2])
        // )); //all low generators need it
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true));
    }
    #define phaseA_gen_one_third 0
    ESP_LOGW("GC3", "=====block + LTimer started. all LOW waves have tocgd. Operator cnnct to timer. C0's set to 2/3 Block period, o0c1 to 1/3.===== ");
}

void configureLowGateEvents(){
    mcpwm_timer_direction_t dir[2] = {MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_DIRECTION_UP};
    mcpwm_generator_action_t action[2] = {MCPWM_GEN_ACTION_LOW, MCPWM_GEN_ACTION_HIGH} ;
    mcpwm_timer_event_t timerEmpty =  MCPWM_TIMER_EVENT_EMPTY;
    int index1 =0;
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
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorH[i].pwmGate0, motorH[i].pwmGate0, &highGateDeadTimeSetup));
        
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorH[i].operatorModule, motorH[i].timer)); 
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, comparatorOff_Duty)); //set to max to be off
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true)); 
    }
    //putting command of setting lowGate Low and high gate High (by comparator action event) into buffer
    for (int i = 0; i <3; i++){
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_HIGH 
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_LOW
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
        runOnMCPWMIntr,
        (void *)&global,
        &oneBlockISR
    ));
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_LOGI("======esp_alloc_intr used to allocate ISR line before flushing all related intr_ena bits", " ");
}

void initializeInterruptEnablePin(){
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_clr.val=  clearReg.val;
    
    //block timer = 0
    MCPWMx->int_ena.timer0_tez_int_ena = 1; // //timer 0= BTimer
    MCPWMx->int_ena.timer1_tez_int_ena = 1; //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
    // MCPWMx->int_ena.timer1_tep_int_ena = 1; //timer 1= LTimer, (ie change from block 0-1), 2^7 = 128
    // MCPWMx->int_ena.op0_tea_int_ena = 1; // op0 = phase A lowside, (ie change from block 5-0 or 1-2), 2^15 = 32765
    // MCPWMx->int_ena.op0_teb_int_ena = 1; // timer, (ie change from block 2-3 or 4-5), 2^18 = 262144

    MCPWMx->int_ena.timer2_tez_int_ena = 1; // //timer 0= BTimer
    ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
    ESP_LOGI("======CLEARing INTR REGISTER AND PUSHing INTR_ENABLE BITS", "\n\n\n ");
}

void initializeSyncs(){
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[0]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[1]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[2]));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&BTimerTriggerSetup, &BTimerTrigger));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&LTimerTriggerSetup, &LTimerTrigger));
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&VTimerTriggerSetup, &VTimerTrigger));
    BTimerOnSync.sync_src = BTimerTrigger; 
    LTimerOnSync.sync_src = LTimerTrigger;
    VTimerOnSync.sync_src = VTimerTrigger;
    
    for(int i= 0; i<3; i++){
        tripleHighOnSync.sync_src = tripleHighTrigger[i]; 
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(motorH[i].timer, &tripleHighOnSync));
    }
    ESP_LOGI(white "initSyncs", "\n BTIMER Count Val: %d",(int)(BTimerOnSync.count_value));
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(blockTimer, &BTimerOnSync));
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(velocityTrackerTimer, &VTimerOnSync));
    ESP_LOGI(magenta "GC5","======Sync handle, phase shift, and Sync sources linked");
}

void initializeTimer(int startingTargetSector){
     for (int i= 0; i<3; i++){
        ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[i].timer));
     }
     ESP_ERROR_CHECK(mcpwm_timer_enable(globalLowTimer));
     ESP_ERROR_CHECK(mcpwm_timer_enable(blockTimer));
     ESP_ERROR_CHECK(mcpwm_timer_enable(velocityTrackerTimer));

     for (int i= 0; i<3; i++){
         ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[i].timer, MCPWM_TIMER_START_NO_STOP));
    }
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(globalLowTimer, MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(blockTimer, MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(velocityTrackerTimer, MCPWM_TIMER_START_NO_STOP));
    ESP_LOGI(blue "GC6", "======ENABLES AND STARTS counting on all 5 timers ");
}

void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm * motorLow, int startingTargetSector){
    for (int i= 0; i<5; i+= 2){ 
        if(gateLevelCycle[global.sectorTarget][i]==1){
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, -1, true));
        }else{
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, 0, true));
        }
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
        // esp_rom_printf("glvl, %d, st: %d, 2i+1: %d, phase: %d \n",gateLevelCycle[global.sectorTarget][i+1], global.sectorTarget, i+1, i/2);
    }
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
    int vt1 = MCPWM0.timer[2].timer_status.timer_value;
    esp_rom_printf(magenta"%s (%5d,%5d,%5d)\n", str, bt1, lt1, vt1);
}
/*==============================================================================================*/
//for isr1
void preloadGates(){
    // if (global.sectorTarget == global.oldSectorTarget || global.newVelPotValue) { //Motor stalling case
        if(global.newVelPotValue){
            //change period of all timers and the compare values
            ESP_ERROR_CHECK(mcpwm_timer_set_period(globalLowTimer, static_cast<uint32_t>(global.blockPeriod) ));
            ESP_ERROR_CHECK(mcpwm_timer_set_period(blockTimer, static_cast<uint32_t>(global.blockPeriod) ));
            // ESP_ERROR_CHECK(mcpwm_timer_set_period(velocityTrackerTimer, static_cast<uint32_t>(global.blockPeriod) ));

            /*
                // for(int i =0; i<3; i++){
                //     ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[i].comparator0, 
                //         2*global.blockPeriod
                //     ));
                // }
                // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorL[phaseA_gen_one_third].comparator1, 
                //     global.blockPeriod
                // ));
            */     
            //prepare  so esp32 runs on the start of the previous block at sync
            // global.newVelPotValue=false;//keep, disable in execute gates
            /*
            taskENTER_CRITICAL_ISR(&stepPeriodMux);
            int bp =global.blockPeriod;
            taskEXIT_CRITICAL_ISR(&stepPeriodMux);
            LTimerOnSync.count_value = lowGateLevelCycle[global.sectorTarget] * bp; //n/3 fraction
            LTimerOnSync.direction = LTimerDir[global.sectorTarget];
            ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(globalLowTimer, &LTimerOnSync));
            */
        }
        // esp_rom_printf(green " pg SYNC BCV %d, LCV %d \n", (int)BTimerOnSync.count_value, (int)LTimerOnSync.count_value);
    // } 

    /*
        else if (global.sectorTarget % 2){ //the case where High Phase switches(ex. 0->1, 2->3, 4->5) 
        don't modify low gate timers (they'll always be the same when high gates swap)
        } //extra case of 1->2, 3->4, 5->0, but high gate is the same and comparatorActions wills set LGates low --> no code needed
    */
}

void IRAM_ATTR executeGates(mcpwm_dev_t * mcpwm){
    //normal operating conidtions, suppresss the right pins
    if(global.newPhaseSwitchFlag){
        for(int i =0; i<5; i+=2){
            if(gateLevelCycle[global.sectorTarget][i]==1){
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, -1, true));
            }else{
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, 0, true));
            }
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
        }
        global.newPhaseSwitchFlag = false;
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(VTimerTrigger)); //push new duty cycles. 
        // esp_rom_printf("lcoked int loop1");
    }

    if(global.newVelPotValue){ 
        // for(int i=0; i<3; i++){
        //     ESP_ERROR_CHECK(mcpwm_soft_sync_activate(tripleHighTrigger[i])); //push new duty cycles
        // }
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(BTimerTrigger)); //push new duty cycles
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(LTimerTrigger)); //push new duty cycles. 
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(VTimerTrigger)); //push new duty cycles. 
        global.newVelPotValue=false;
        esp_rom_delay_us(ticksToµs+1);
        esp_rom_printf("lcoked int loop2");
    }
    // if(global.sectorTarget == global.oldSectorTarget){
    //     esp_rom_printf("&tall");
    // }
    #ifdef debug_fastPrints
        esp_rom_printf(white "|%d,%s" red "|L\n", global.sectorTarget, ghgl[global.sectorTarget]);
    #endif
}
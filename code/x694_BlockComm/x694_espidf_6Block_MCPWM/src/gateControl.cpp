#include "GateControl.h"
#include "GC.h"
intr_handle_t oneBlockISR = NULL;

void mcpwmSetup(){
    int tVal[3] ={0,0,0};
    setCountValueAndPeriod();
    initializeLowGate(); // suppress Lgate to OFF
    initializeHighGate( startingGateCmpValue ); //suppress Hgate to OFF, CMRA0 NEVER actually used
    #ifndef lastResort 
    initializeISR();
    #endif
    initializeTimer();  //sets and starts B and L timer
    
    tVal[0] =MCPWM0.timer[0].timer_status.timer_value; //block
    esp_rom_printf(cyan "VTIMER%d\nOldSector: %d NewSector %d\n", tVal[0], global.oldSectorTarget, global.sectorTarget);
    ESP_LOGW("gcc"," maximum target RPs; %6.3f, minimum target RPS: %6.3f",fMin, fMax);
}

void setCountValueAndPeriod(){
    VTimerSetup.period_ticks = global.blockPeriod;
    tripleHighOnSync.count_value = 1; 
    VTimerOnSync.count_value = 1;
    ESP_LOGW(white "GC 1.5 ON_SYNC_VALUES-setCountValueAndPeriod", "\n VTIMER_CV_OS %d",(int)VTimerOnSync.count_value);
    ESP_LOGW("GC 1.75 PERIODS-setCountValueAndPeriod", "VTIMER %d\n", (int)VTimerSetup.period_ticks);
}

void initializeLowGate(){
    for (int i = 0; i <3; i++){
        motorL[i] = { .opConfig = operatorSetupLow};
        motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
    }
    VTimerSetup.resolution_hz = VTimerResolution;
    ESP_ERROR_CHECK(mcpwm_new_timer(&VTimerSetup, &VTimer)); 
    // MCPWM0.clk_cfg.clk_prescale = mcpwm_lowSideGroupPrescaler-1;
    // MCPWM0.timer[0].timer_cfg0.timer_prescale= 160e6/VTimerResolution/mcpwm_lowSideGroupPrescaler-1;
    // ESP_LOGW("GC VTimerPrescaler", "%d| GroupPrescaler %d", MCPWM0.timer[0].timer_cfg0.timer_prescale ,mcpwm_lowSideGroupPrescaler);

    for (int i = 0; i <3; i++){
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));

        // ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); 
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true));
    }

    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&VTimerTriggerSetup, &VTimerTrigger));
    VTimerOnSync.sync_src = VTimerTrigger;    
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(VTimer, &VTimerOnSync));
}

void initializeHighGate(uint32_t comparatorOff_Duty){
    ESP_LOGI("High Gate CMP Value","%d ", comparatorOff_Duty);
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
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_HIGH
            )
        ));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_LOW
            )
        ));

        ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&tripleHighTriggerSetup, &tripleHighTrigger[i]));
        tripleHighOnSync.sync_src = tripleHighTrigger[i]; 
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(motorH[i].timer, &tripleHighOnSync));
    }
}
  
void initializeISR(){
mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_ERROR_CHECK(esp_intr_alloc(
        ETS_PWM0_INTR_SOURCE,
        runOnMCPWMIntrPriority | ESP_INTR_FLAG_IRAM,
        runOnMCPWMIntr,
        (void*)&global,
        &oneBlockISR
    ));
}

void initializeTimer(){
    for (int i= 0; i<3; i++){
        ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[i].timer));
    }
    #ifdef lastResort
    ESP_ERROR_CHECK(mcpwm_timer_register_event_callbacks(VTimer, &callbackFamily, (void *)&global));
    #endif
    ESP_ERROR_CHECK(mcpwm_timer_enable(VTimer));

    for (int i= 0; i<3; i++){
         ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[i].timer, MCPWM_TIMER_START_NO_STOP));
    }
    ESP_LOGI(blue "GC6", "======ENABLES AND STARTS counting on 4 timers ");
}

void IRAM_ATTR tag(const char* tag){
    #ifdef debug_fastPrints
    esp_rom_printf(tag);
    #elif defined(debug_hyperFastPrints)
    darray[dindex[0].fetch_add(1)]= tag;
    #endif
}

void tagFlag(bool start,int time){
    taskENTER_CRITICAL(&stepPeriodMux);
    // int bp = global.blockPeriod;
    int currentTargetSector = global.sectorTarget;
    // bool readPotFlag = global.newVelPotValue.load(std::memory_order::relaxed);
    bool i2cfailed= global.setMotorFreeTemporarily.load(std::memory_order::relaxed);
    bool setMotorCoast = global.setMotorFreeSpin.load(std::memory_order::relaxed);
    taskEXIT_CRITICAL(&stepPeriodMux);
     if(start){
        esp_rom_printf("<%d%d%d%d",
            // readPotFlag,
            i2cfailed,
            setMotorCoast
        );
    }else {
        esp_rom_printf("^%d%d%d%d>%d\n",
            // readPotFlag,
            i2cfailed,
            setMotorCoast,
            currentTargetSector
            // time
        );
    }
    
    // taskENTER_CRITICAL(&stepPeriodMux);
    // // int bp = global.blockPeriod;
    // int previousRotorVal = global.rotorVal;
    // int previousTargetSector = global.oldSectorTarget;
    // int currentTargetSector = global.sectorTarget;
    // taskEXIT_CRITICAL(&stepPeriodMux);
    //  if(start){
    // }else {
    //     esp_rom_printf("%d, %d\n",
    //         previousTargetSector,
    //         currentTargetSector
    //         // time
    //     );
    // }
}

void IRAM_ATTR getTimerCountNow(const char* str){
    int vt1 = MCPWM0.timer[0].timer_status.timer_value;
    esp_rom_printf(magenta"%s V:%5d\n", str, vt1);
}

void IRAM_ATTR executeGates(bool freeSpin){
    
    if(freeSpin){
        tag(yellow "EgTfre ");
        for(int i =2; i>-1; i--){
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true));
            // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 1, true));
        }
    }else{

        if(global.setMotorFreeTemporarily.load() ||global.setMotorFreeSpin.load()){ //don't esrase these valeus
            tag(yellow "EgFTFree ");
            for(int i =2; i>-1; i--){
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true));
                // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 1, true));
            }

        } else {  //not freespinning = active control
            #define snap() time240()
            int temp[7];
            int in =0;
            tag(yellow "EgFFSw ");
            temp[0] = snap();
            //when motor is off (freespinnig), nPSF still runs, but no changes are made
            for(int i =4; i>-1; i-=2){
                int hlvl;
                if(gateLevelCycle[global.sectorTarget][i] == 1){
                    hlvl =-1;
                }else{
                    hlvl =0;
                }
                temp[++in] =  snap()-temp[0];
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, hlvl, true));
                temp[++in] =  snap()-temp[0];
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
            }
            esp_rom_printf("#%d$%d$%d\n%d$%d$%d\n%d",temp[0], temp[1], temp[2], temp[3],temp[4], temp[5], temp[6]);
        }
    }

    #ifdef debug_fastPrints
    esp_rom_printf(white "|%s,%d" red "|L\n" ghgl[global.sectorTarget],global.dir);
    #elif defined(debug_hyperFastPrints)
    // int t1= esp_timer_get_time();
    tag(red);
    tag(ghgl[global.sectorTarget]);
    tag(dgdir[global.dir]);
    
    for(int hfp = dindex[1];hfp<dindex[0];hfp++ ){
        esp_rom_printf("%s",darray[hfp]);
    }
    #ifdef debug_hyperFastPrintsWithPot
    int bp = global.blockPeriod;
    esp_rom_printf(" p%d\n", bp);
    #else
    esp_rom_printf("\n"); 
    #endif
    
    // t1= esp_timer_get_time() - t1;esp_rom_printf("T1: %d\n",t1);
    dindex[1].fetch_and(0x00);
    dindex[0].fetch_and(0x00);
    #endif
    
}
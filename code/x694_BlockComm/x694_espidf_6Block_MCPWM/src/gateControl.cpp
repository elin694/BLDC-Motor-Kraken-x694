#include "GateControl.h"

#include "GC.h"
void mcpwmSetup(int startingTargetSector){
    int tVal[3] ={0,0,0};
    setCountValueAndPeriod(startingTargetSector);
    initializeLowGate(startingTargetSector); // suppress Lgate to OFF
    initializeHighGate(startingTargetSector, startingGateCmpValue); //suppress Hgate to OFF, CMRA0 NEVER actually used
    initializeSyncs();
    initializeISR();
    initializeTimer(startingTargetSector);  //sets and starts B and L timer
    
    // synchrISR(BTimerTrigger, ""); //0-1us, Post-intr_ena BTSync
    // synchrISR(LTimerTrigger, ""); //0-1us, Post-intr_ena LT Sync
    // synchrISR(VTimerTrigger, ""); //0-1us ,Post-intr_ena VT Sync
    tVal[0] =MCPWM0.timer[0].timer_status.timer_value; //block
    tVal[1] =MCPWM0.timer[1].timer_status.timer_value;  //low
    // tVal[2] =MCPWM0.timer[2].timer_status.timer_value;  //velecoty
    esp_rom_printf(red "Bc %5d Lc %5d (ot,nt): %d, %d \n", tVal[0], tVal[1], global.oldSectorTarget, global.sectorTarget);
    ESP_LOGW("gcc"," maximum target RPs; %6.3f, minimum target RPS: %6.3f \n\n",fMin, fMax);
}

void setCountValueAndPeriod(int startingTargetSector){
//SET PERIOD TICKS
//    phaseTimerSetupHigh.period_ticks =(uint32_t)(activePwmPeriod);
    I2CReadTimerSetup.period_ticks =(uint32_t)(SetLTimerPollPeriod); //1 phase every change int
    globalTimerSetupLow.period_ticks =(uint32_t)(SetLTimerPollPeriod);//causes execute gate isr
    velocityTrackerTimerSetup.period_ticks = global.blockPeriod;

    tripleHighOnSync.count_value = 1; 
    int excess = 0;
    VTimerOnSync.count_value = 1;
    BTimerOnSync.count_value = estimatedI2CReadTimeInTicks; //is the starting phaseOffset
    LTimerOnSync.count_value = excess;
    ESP_LOGW(white "GC 1.5 ON_SYNC_VALUES-setCountValueAndPeriod", "\n BTIMER_CV_OS: %d\n LTIMER_CV_OS %d\n VTIMER_CV_OS %d\n excess: %d", (int)BTimerOnSync.count_value, (int)LTimerOnSync.count_value, (int)VTimerOnSync.count_value, excess);
    ESP_LOGW("GC 1.75 PERIODS-setCountValueAndPeriod", "\n BTIMER: %d\n LTIMER %d\n VTIMER %d\n", (int)I2CReadTimerSetup.period_ticks, (int)globalTimerSetupLow.period_ticks, (int)velocityTrackerTimerSetup.period_ticks);
}

void initializeLowGate(int startingTargetSector){
    for (int i = 0; i <3; i++){
        motorL[i] = { .opConfig = operatorSetupLow};
        motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
    }
    ESP_ERROR_CHECK(mcpwm_new_timer(&I2CReadTimerSetup, &blockTimer));
    ESP_ERROR_CHECK(mcpwm_new_timer(&globalTimerSetupLow, &globalLowTimer)); 
    velocityTrackerTimerSetup.resolution_hz = timerResolution;
    ESP_ERROR_CHECK(mcpwm_new_timer(&velocityTrackerTimerSetup, &velocityTrackerTimer)); 
    MCPWM0.clk_cfg.clk_prescale = mcpwm_lowSideGroupPrescaler-1;
    MCPWM0.timer[0].timer_cfg0.timer_prescale= 160e6/timerResolution/mcpwm_lowSideGroupPrescaler-1;
    MCPWM0.timer[1].timer_cfg0.timer_prescale= 160e6/timerResolution/mcpwm_lowSideGroupPrescaler -1;
    MCPWM0.timer[2].timer_cfg0.timer_prescale= 160e6/VTimerResolution/mcpwm_lowSideGroupPrescaler-1;
    ESP_LOGW("GC timerPrescalers", "%d %d %d| GroupPrescaler %d", MCPWM0.timer[0].timer_cfg0.timer_prescale,MCPWM0.timer[1].timer_cfg0.timer_prescale,MCPWM0.timer[2].timer_cfg0.timer_prescale,mcpwm_lowSideGroupPrescaler);

    for (int i = 0; i <3; i++){
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));

        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); 
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true));
    }
    #define phaseA_gen_one_third 0
    ESP_LOGW("GC3", "=====Linked all Low timer Submodules===== ");
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
    ESP_LOGI(blue "GC6", "======ENABLES AND STARTS counting on 4 timers ");
}

void firstPreload(phaseMcpwm * motorHigh, phaseMcpwm * motorLow, int startingTargetSector){
    for (int i= 0; i<5; i+= 2){ 
        int hlvl;
        if(gateLevelCycle[global.sectorTarget][i]==1){
            hlvl= -1;
        }else{
            hlvl= 0;
        }
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, hlvl, true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
    }
}

void IRAM_ATTR synchrISR(mcpwm_sync_handle_t handle, const char* name){ 
    //if code only activites sync, execution time <1us
    ESP_ERROR_CHECK(mcpwm_soft_sync_activate(handle)); //0-1us
}
void IRAM_ATTR tag(const char* tag){
    #ifdef debug_fastPrints
    esp_rom_printf(tag);
    #elif defined(debug_hyperFastPrints)
    darray[dindex[0].fetch_add(1)]= tag;
    #endif
}

void tagFlag(bool start){
    #ifdef debug_hyperFastPrints
    taskENTER_CRITICAL(&stepPeriodMux);
    int bp = global.blockPeriod;
    bool readPotFlag = global.newVelPotValue;
    bool phaseSwitchFlag = global.newPhaseSwitchFlag;
    bool finishedAs5600 = global.readAS5600;
    bool i2cfailed= global.setMotorFreeTemporarily;
    bool setMotorCoast = global.setMotorFreeSpin;
    taskEXIT_CRITICAL(&stepPeriodMux);
     if(start){
        esp_rom_printf("<%d_%d%d%d%d%d\n",
            bp,
            readPotFlag,
            phaseSwitchFlag,
            finishedAs5600,
            i2cfailed,
            setMotorCoast
        );
    }else {
        esp_rom_printf("%d_%d%d%d%d%d>\n",
            bp,
            readPotFlag,
            phaseSwitchFlag,
            finishedAs5600,
            i2cfailed,
            setMotorCoast
        );
    }
    #else
    #endif
}

void IRAM_ATTR getTimerCountNow(const char* str){
    int bt1 = MCPWM0.timer[0].timer_status.timer_value;
    int lt1 = MCPWM0.timer[1].timer_status.timer_value;
    int vt1 = MCPWM0.timer[2].timer_status.timer_value;
    esp_rom_printf(magenta"%s (%5d,%5d,%5d)\n", str, bt1, lt1, vt1);
}

/*==============================================================================================*/
void IRAM_ATTR preloadGates(){
    if(global.newVelPotValue.exchange(false)){
    tag("PgTVf");
        if(global.blockPeriod <minf_HTimerPeriod){ 
            ESP_ERROR_CHECK(mcpwm_timer_set_period(velocityTrackerTimer, global.blockPeriod));   
        }
    }
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
        }else {  //not freespinning = active control
            tag(yellow "EgFFSw ");
            //when motor is off (freespinnig), nPSF still runs, but no changes are made
            for(int i =0; i<5; i+=2){
                int hlvl;
                if(gateLevelCycle[global.sectorTarget][i]==1){
                    hlvl =-1;
                }else{
                    hlvl =0;
                }
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, hlvl, true));
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
            }
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
    // esp_rom_printf(" p%d:%d,%d,%d,%d\n", bp,rA[0],rA[1],rA[2],rA[3]);
    // for(int i=0; i<4;i++){
    //     rA[i]=0;
    // }
    // rindx.store(0);
    #else
    esp_rom_printf("\n"); 
    #endif
    
    // t1= esp_timer_get_time() - t1;esp_rom_printf("T1: %d\n",t1);
    dindex[1].fetch_and(0x00);
    dindex[0].fetch_and(0x00);
    #endif
    
}
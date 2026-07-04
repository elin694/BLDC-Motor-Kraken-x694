#include "GateControl.h"
#include "Initialize.h"
#include "GC.h"
void mcpwmSetup(int startingTargetSector){
    int tVal[3] ={0,0,0};
    setCountValueAndPeriod(startingTargetSector);
    initializeLowGate(startingTargetSector); // suppress Lgate to OFF
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
    ESP_LOGW("mcwpmSetup finished ","\n\n");
}

void setCountValueAndPeriod(int startingTargetSector){
    CMRA0Threshold = static_cast<uint32_t>(startingGateCmpValue);

    //SET PERIOD TICKS
   phaseTimerSetupHigh.period_ticks =static_cast<uint32_t>(activePwmPeriod);
    I2CReadTimerSetup.period_ticks =static_cast<uint32_t>(SetAs5600PollPeriod); //1 phase every change int
    globalTimerSetupLow.period_ticks =static_cast<uint32_t>(SetAs5600PollPeriod);//causes execute gate isr
    velocityTrackerTimerSetup.period_ticks = global.blockPeriod;

    tripleHighOnSync.count_value = 0; 
    int excess = 0;
    VTimerOnSync.count_value = 0;
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
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_ERROR_CHECK(esp_intr_alloc(
        ETS_PWM0_INTR_SOURCE,
        ESP_INTR_FLAG_LEVEL3 
        | ESP_INTR_FLAG_IRAM
        ,
        runOnMCPWMIntr,
        (void *)&global,
        &oneBlockISR
    ));
}

void initializeInterruptEnablePin(){
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_clr.val=  clearReg.val;
    
    //block timer = 0
    MCPWMx->int_ena.timer0_tez_int_ena = 1; // //timer 0= BTimer
    MCPWMx->int_ena.timer1_tez_int_ena = 1; //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
    MCPWMx->int_ena.timer2_tez_int_ena = 1; // //timer 0= BTimer
    ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
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
void preloadGates(){
    if(global.newVelPotValue){
        /*
        */
       #ifdef debug_fastPrints
        esp_rom_printf("PgV ");
        #elif defined(debug_hyperFastPrints)
        darray[dindex[0]++]= yellow "PgV ";
        #endif
        if(global.blockPeriod <minf_HTimerPeriod){ 
            ESP_ERROR_CHECK(mcpwm_timer_set_period(velocityTrackerTimer, global.blockPeriod));
        }
        else{ //case when bp is bigger than mcpwm can allow, aka too slow Frequency
            ESP_ERROR_CHECK(mcpwm_timer_set_period(velocityTrackerTimer, minf_HTimerPeriod));
            global.dir= 0;
            global.setMotorFreeSpin = true;
        }
    }
}

void IRAM_ATTR executeGates(mcpwm_dev_t * mcpwm){
    if(global.newVelPotValue){  //potentiometer  moved (ie motor velocity target changed)
        #ifdef debug_fastPrints
        esp_rom_printf("EgV ");
        #elif defined(debug_hyperFastPrints)
        darray[dindex[0]++]= "EgV ";
        #endif
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(BTimerTrigger)); 
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(LTimerTrigger));
        ESP_ERROR_CHECK(mcpwm_soft_sync_activate(VTimerTrigger)); //push new duty cycles. 
        global.newVelPotValue=false;  
        // esp_rom_delay_us(ticksToµs+1);
    }

    //normal operating conidtions, suppresss the right pins
    if(global.newPhaseSwitchFlag){ 
        #ifdef debug_fastPrints
        esp_rom_printf("EgPh ");
        #elif defined(debug_hyperFastPrints)
        darray[dindex[0]++]= green "EgPh ";
        #endif
        //when motor is off (dir=0), nPSF still runs, but no changes are made
        if(!global.setMotorFreeSpin){ //not freespinning = active control
            for(int i =0; i<5; i+=2){
                if(gateLevelCycle[global.sectorTarget][i]==1){
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, -1, true));
                }else{
                    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i/2].pwmGate0, 0, true));
                }
                ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i/2].pwmGate0, gateLevelCycle[global.sectorTarget][i+1], true));
            }
        }
            global.newPhaseSwitchFlag = false;
    }

    if(global.setMotorFreeSpin){
        #ifdef debug_fastPrints
        esp_rom_printf("EgPh ");
        #elif defined(debug_hyperFastPrints)
        darray[dindex[0]++]= cyan "EgFree ";
        #endif
        for(int i =2; i>-1; i--){
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true));
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true));
        }
        global.setMotorFreeSpin=false;
    }



    #ifdef debug_fastPrints
    esp_rom_printf(white "|%d,%s,%d" red "|L\n", global.sectorTarget, ghgl[global.sectorTarget],global.dir);
    #elif defined(debug_hyperFastPrints)
    // int t1= esp_timer_get_time();
    darray[dindex[0]++] = red;
    darray[dindex[0]++] = ghgl[global.sectorTarget];
    darray[dindex[0]++] = dgdir[global.dir];
    
    for(int hfp = dindex[1];hfp<dindex[0];hfp++ ){
        esp_rom_printf("%s",darray[hfp]);
    }
    #ifdef debug_hyperFastPrintsWithPot
    int bp = global.blockPeriod;
    esp_rom_printf(" p%d\n", bp );
    #endif
    
    // t1= esp_timer_get_time() - t1;esp_rom_printf("T1: %d\n",t1);
    dindex[1]=dindex[0]=0;
    #endif
    
}
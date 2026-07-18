#include "Initialize.h"
#include "GateControl.h"

#define isMinutelyCheckup(x) ((x % 1024) == 15)
TickType_t synchronizedTime;
TaskHandle_t initializeI2CTask= NULL;
void initialize(void * parameter){   
   pinSetup();
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
   xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
   mcpwmSetup(); 
   ESP_ERROR_CHECK(esp_timer_create(&gsnTimerSetup, &gsnTimerHandle));
   int b = global.blockPeriod.load(std::memory_order::relaxed);
   ESP_LOGI("init.cpp ","blockPeriod %d", b);//nti
   xTaskCreatePinnedToCore(executeGates, ".exe", 3000, NULL,  15, &executeGatesTask, 0);

   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   synchronizedTime = xTaskGetTickCount();
   int now1 = SNAP();
   xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &synchronizedTime,  15, &getSectorNumberTask, 1);
   xTaskCreatePinnedToCore(debugMonitor, "debugLog", 5000, &synchronizedTime, 3, NULL, 0);
   xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 2000, &synchronizedTime, 6, NULL,0);
   xTaskCreatePinnedToCore(initializeInterruptEnablePin, "startVtimer", 2000, &synchronizedTime, 6, NULL, 0);
   esp_intr_dump(stdout);
   esp_timer_dump(stdout);
   esp_err_t probeCheck = i2c_master_probe(busHandle, as5600Address, 1);
   int now2 = SNAP()-now1;
   ESP_LOGI("init", "TaskCreation(us): %d, Probe Check %d", now2, probeCheck);
   vTaskDelete(NULL);
}
// xTaskCreatePinnedToCore(mathItOut, "mathItOut", 10000, &synchronizedTime, (int)(thisTaskPriority)+3, &mathItOutTask, 0);

void pinSetup(){
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
      gpio_set_level(gateArray[i], 0);
   }
}

void initializeInterruptEnablePin(void * startTick6){ 
   TickType_t startTick = *(TickType_t*)startTick6;
   ESP_LOGI(blue "init.cpp", "=====starttimer==== ");
   xTaskDelayUntil(&startTick,initializationLatency);
   ESP_ERROR_CHECK(mcpwm_timer_start_stop(VTimer, MCPWM_TIMER_START_NO_STOP));
   #ifndef lastResort
   mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
   MCPWMx->int_clr.val=  clearReg.val;
   MCPWMx->int_ena.timer0_tez_int_ena = 1; 
   #endif
   vTaskDelete(NULL);
}

void IRAM_ATTR runOnESPTimerIntr (void * globe) { /*intrpt*/
   BaseType_t xHigherPriorityTaskWoken;
   vTaskNotifyGiveFromISR(getSectorNumberTask, &xHigherPriorityTaskWoken);
   if(xHigherPriorityTaskWoken == pdTRUE){
      xHigherPriorityTaskWoken =pdFALSE;
      esp_timer_isr_dispatch_need_yield();
   }
}

#ifdef lastResort
bool IRAM_ATTR VTimerCallback (mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t *edata, void *user_ctx) { /*intrpt*/
   return runActualISR(user_ctx);
}
#else
void IRAM_ATTR runOnMCPWMIntr (void * user_ctx) { /*intrpt*/
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts
      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS VTIMER
         runActualISR(user_ctx);
         MCPWMx->int_clr.val = tempClearR1.val;
         return;
      } 
   }  
}
#endif

// volatile std::atomic<int> oneTimeFlag = 0;
bool runActualISR(void * data){
   gVar_t *masterVar = (gVar_t*)data;
   BaseType_t xHigherPriorityTaskWoken2;
   int timeNow = SNAP();
   if((timeNow - masterVar->tlog_readAS5600.load()) < ACCEPTABLE_I2C_READ_WINDOW ){
      tag(cyan "V1");
      // if(oneTimeFlag.fetch_add(1,std::memory_order::relaxed) <240000){
         xTaskNotifyFromISR(executeGatesTask, 0, eIncrement, &xHigherPriorityTaskWoken2);
      // }
   }
   //  else { //COMING SOON!
   //    tag(cyan "V2");
   //    /*execute gates only if we have a valid i2c*/;
   //    #define FAST_BLOCK_RPS (4096.0f/(5*18*estimatedI2CReadTime_us)) //bits/s, whree 5 = min # samples. 3.33k RPM @ 200us
   //    if(global.measuredVel > FAST_BLOCK_RPS) {
   //       xTaskNotifyFromISR(executeGatesTask, ExecuteGate_FreeSpin_NotifVal, eSetValueWithOverwrite, &xHigherPriorityTaskWoken2);
   //    }
   // }
   if(xHigherPriorityTaskWoken2 == pdTRUE) {
      xHigherPriorityTaskWoken2 = pdFALSE;
      portYIELD_FROM_ISR();
      return true;
   }
   return false;
}

void as5600initialize(void * parameter) {
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));

   int startWatch  =SNAP();
   ESP_ERROR_CHECK(i2c_master_transmit_receive( as5600Handle, fthRegister, 1, fthRegisterData, 1, -1 ) ); //read current settings
   /* (dev_handle), pointer-pointed addr to start on, # bytes to write, saved data, # bytes to save, waitout */
   
   int lapWatch =SNAP()-startWatch;
   fthRegister[1]= (fthRegisterData[0] & fth_sf_clear_mask) | fth_sf_set_mask; //rese
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 2, fthRegisterData, 1, -1));
   int lapWatch2 =SNAP()-startWatch;
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 1, fthRegisterData, 1, -1));
   int lapWatch3 =SNAP()-startWatch;
   ESP_LOGI(magenta "init.cpp", "\nas5600 Fast Fillter Threshold Set to %d\n1st REG read time:%4d \nSF-FTH write time:%4d REG_Check time:%4d ", 
      (int)fthRegisterData[0],
      lapWatch,
      lapWatch2,
      lapWatch3
   );
   
   xTaskNotifyGive(setupTask);
   vTaskDelete(NULL);
}

void IRAM_ATTR getSectorNumber (void * startTick1){ /*GSNG*/
   CLEAR_ALL_NOTIFS(NULL);
   TickType_t startTick = *(TickType_t*)startTick1;
   int lap1 =0;
   int startTime =0;
   uint32_t printCounter=0;

   ESP_ERROR_CHECK(esp_timer_start_periodic(gsnTimerHandle, estimatedI2CReadTime_us));
   xTaskDelayUntil(&startTick,initializationLatency);
   while(1){
      uint32_t file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(1));
      // xTaskNotifyWaitIndexed(0, ULONG_MAX,ULONG_MAX, &file1, pdMS_TO_TICKS(1000));

      #if (defined(debug_i2cTransmitTime) || defined(debug_useTagFlag))
      if(isMinutelyCheckup(++printCounter)){ 
         startTime= SNAP(); 
         
         #ifdef debug_useTagFlag
         tagFlag(true,0); //tags before and after transmit
         #endif
      }
      #endif
      esp_err_t valRequestStatus= i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, 1, as5600RawDataBuf, 2, i2cWaitout);
      #if (defined(debug_i2cTransmitTime) || defined(debug_useTagFlag))
      if(isMinutelyCheckup(printCounter)){ 
         lap1 = SNAP() - startTime;
         
         #if (!defined(debug_i2cTransmitTime) && defined(debug_useTagFlag))
         tagFlag(false, lap1); //tags before and after transmit
         #endif
      }
      #endif

      if(valRequestStatus == ESP_OK){
         isr2i.fetch_add(1,std::memory_order::relaxed);
         global.oldSectorTarget = global.sectorTarget;
         
         uint32_t reading = (as5600RawDataBuf[0]<<8) | as5600RawDataBuf[1]; 
         global.rotorVal = reading;
         global.sectorTarget = (uint32_t)(getRotorValAdjusted(reading) + global.dir) % 6; //0- bitsPerSector --> smaller sector
         global.setMotorFreeTemporarily.store(false, std::memory_order::relaxed);
         uint32_t tlog = SNAP();
         global.tlog_readAS5600.store(tlog);
      } else{
         global.oldSectorTarget = global.sectorTarget;
         global.setMotorFreeTemporarily.store(true, std::memory_order::relaxed);
         tag("#F ");
      }

      #if defined(debug_i2cTransmitTime)
      if(isMinutelyCheckup(printCounter)){ 
         startTime = SNAP() - startTime;      esp_rom_printf("@%d+%d,%d \n"  , lap1,startTime,file1);
      }
      #endif
   }
   
}
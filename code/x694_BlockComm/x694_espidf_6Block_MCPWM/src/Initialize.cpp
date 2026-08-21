#include "Initialize.h"
#include "Controller.h"
#define isMinutelyCheckup(x) ((x % 1024) == 15)
TickType_t synchronizedTime;
TaskHandle_t initializeI2CTask= NULL;
DRAM_ATTR uint32_t megaTimerResolution = 0;
void initialize(void * parameter){   
   pinSetup();
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
   
   /*higher priority but runs on differnet core*/
   xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
   mcpwmSetup(); 
   #ifndef useGPTimerOverESP32Timer
   ESP_ERROR_CHECK(esp_timer_create(&gsnTimerSetup, &gsnTimerHandle));
   #endif
   int b = global.blockPeriod.load(std::memory_order::relaxed);
   ESP_LOGI("init.cpp ","blockPeriod %d", b);//nti
   xTaskCreatePinnedToCore(executeGates, ".exe", 3000, NULL,  15, &executeGatesTask, 0);
   xTaskCreatePinnedToCore(torqueControlLoop, "Controller-tqLoop", 3000, (void*) &global,  14, &torqueControlLoopTask, 0);
   xTaskCreatePinnedToCore(velocityControlLoop, "Controller-vlLoop", 3000, (void*) &global,  13, &velocityControlLoopTask, 0);
   xTaskCreatePinnedToCore(positionControlLoop, "Controller-psLoop", 3000, (void*) &global,  12, &positionControlLoopTask, 0);
   xTaskCreatePinnedToCore(stableLoopCheck, "Controller-IntegrationFlagLoop", 2000, (void*) &global,  5, &stableLoopCheckTask, 0);
   
   /*Wait for as5600 initialize to ping this task*/
   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   synchronizedTime = xTaskGetTickCount();
   int now1 = SNAP();
   
   /*high priority but runs on differnet core*/
   xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &synchronizedTime,  15, &getSectorNumberTask, 1);
   xTaskCreatePinnedToCore(debugMonitor, "debugLog", 5000, &synchronizedTime, 3, NULL, 0);
   xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 2000, &synchronizedTime, 6, NULL,0); //not as important as contorl loops
   xTaskCreatePinnedToCore(startAllTimersAndInterrupts, "Controller-startVtimer", 2000, &synchronizedTime, 7, NULL, 0);
   #ifdef DEBUG_ALLOW_ONE_TIME_DUMPING
   esp_intr_dump(stdout);
   #endif
   #ifndef useGPTimerOverESP32Timer
   // esp_timer_dump(stdout);
   #endif
   esp_err_t probeCheck = i2c_master_probe(busHandle, as5600Address, 2);
   int now2 = SNAP()-now1;
   ESP_LOGI("init", "TaskCreation(us): %d, Probe Check %d", now2, probeCheck);
   vTaskDelete(NULL);
}


void pinSetup(){
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
      gpio_set_level(gateArray[i], 0);
   }
}

#ifdef useGPTimerOverESP32Timer
DRAM_ATTR std::atomic <uint32_t> ct =0;
DRAM_ATTR std::atomic <int> lct =0;
DRAM_ATTR BaseType_t xHigherPriorityTaskWoken = pdFALSE;
bool IRAM_ATTR runOnMegaTimerIntr (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
   vTaskNotifyGiveFromISR(getSectorNumberTask, &xHigherPriorityTaskWoken);
   // if((ct.fetch_add(1, std::memory_order::relaxed) % 1024) == 200){
   //    int now= SNAP();
   //    int dt= now - lct.load(std::memory_order::relaxed);
   //    esp_rom_printf("yeet %d \n", dt);
   //    lct.store(now, std::memory_order::relaxed);
   // }
   return ((bool) xHigherPriorityTaskWoken);
}
#else
void IRAM_ATTR runOnESPTimerIntr (void * globe) { /*intrpt*/
   BaseType_t xHigherPriorityTaskWoken;
   vTaskNotifyGiveFromISR(getSectorNumberTask, &xHigherPriorityTaskWoken);
   if(xHigherPriorityTaskWoken == pdTRUE){
      xHigherPriorityTaskWoken =pdFALSE;
      esp_timer_isr_dispatch_need_yield();
   }
}
#endif


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
bool IRAM_ATTR runActualISR(void * data){
   gVar_t *masterVar = (gVar_t*)data;
   BaseType_t xHigherPriorityTaskWoken2;
   int timeNow = SNAP();
   taskENTER_CRITICAL( &sensorMux );
   uint32_t tlog_sensor = masterVar->tlog_readAS5600.load();
   taskEXIT_CRITICAL( &sensorMux );
   if((timeNow - tlog_sensor) < ACCEPTABLE_I2C_READ_WINDOW ){
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
   ESP_ERROR_CHECK(i2c_master_transmit_receive( as5600Handle, fthRegister, 1, fthRegisterData, 2, -1 ) ); //read current settings
   /* (dev_handle), pointer-pointed addr to start on, # bytes to write, saved data, # bytes to save, waitout */
   
   int lapWatch =SNAP()-startWatch;
   fthRegister[1]= ( (fthRegisterData[0] & fth_sf_clear_mask) | fth_sf_set_mask); //read and set the fth and sf bits
   fthRegister[2]= ( (fthRegisterData[1] & power_clear_mask) | power_set_mask); //read and set the pm  bits to 00
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 3, fthRegisterData, 1, -1));
   int lapWatch2 =SNAP()-startWatch;
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 1, fthRegisterData, 2, -1));
   int lapWatch3 =SNAP()-startWatch;
   fthRegisterData[1] = (fthRegisterData[1] && (~power_clear_mask));
   ESP_LOGI(magenta "init.cpp", "\nas5600 FTH & SF + PM Registers set to %d+%d\n1st REG read time:%4d \nSF-FTH write time:%4d REG_Check time:%4d ", 
      (int)fthRegisterData[0],
      (int)fthRegisterData[1],
      lapWatch,
      lapWatch2,
      lapWatch3
   );
   
   #ifdef useGPTimerOverESP32Timer
   ESP_ERROR_CHECK(gptimer_new_timer(&megaTimerSetup, &megaTimer));
   ESP_ERROR_CHECK(gptimer_set_alarm_action(megaTimer, &megaTimerAlarmSetup));
   ESP_ERROR_CHECK(gptimer_register_event_callbacks(megaTimer, &megaTimerCallback, NULL));
   ESP_ERROR_CHECK(gptimer_enable(megaTimer));
   ESP_ERROR_CHECK(gptimer_get_resolution(megaTimer, &megaTimerResolution));
   float mperiod = (float)ALARM_VAL/ megaTimerResolution;
   ESP_LOGI(magenta, "mega timer period: %8.5f" , mperiod);
   #endif

   xTaskNotifyGive(setupTask);
   vTaskDelete(NULL);
}

#define FAIL_SLOTS (4)
#define FAIL_THRESHOLD_TIME (FAIL_SLOTS * 1e6) /*in us. Larger catches more*/
#define MAX_BUS_RESET_ATTEMTPS 3

#define JAILBREAK_SLOTS (2)
#define JAILBREAK_THRESHOLD_TIME (JAILBREAK_SLOTS * 1e6 ) /*in us*/
void IRAM_ATTR getSectorNumber (void * startTick1){ /*GSNG*/
   CLEAR_ALL_NOTIFS(NULL);
   TickType_t startTick = *(TickType_t*)startTick1;
   #if (defined(debug_i2cTransmitTime) || defined(debug_useTagFlag))
   int lap1 =0;
   int startTime =0;
   uint32_t printCounter=0;
   #endif
   #ifdef useGPTimerOverESP32Timer
   ESP_ERROR_CHECK(gptimer_start ( megaTimer ) );
   #else
   ESP_ERROR_CHECK(esp_timer_start_periodic(gsnTimerHandle, estimatedI2CReadTime_us));
   #endif
   #ifdef ENABLE_GAMBLING_ON_I2C
   uint32_t failHistory[FAIL_SLOTS];
   uint32_t failIndex = 0;
   uint32_t jailHistory[JAILBREAK_SLOTS];
   uint32_t jailCell = 0;
   #endif

   xTaskDelayUntil(&startTick,initializationLatency);
   while(1){
      uint32_t file1 = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000000));
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
         uint32_t reading = (as5600RawDataBuf[0]<<8) | as5600RawDataBuf[1]; 
         uint32_t tlog = SNAP();
         global.oldSectorTarget = global.sectorTarget;
         
         global.rotorVal = reading;
         global.sectorTarget = (uint32_t)(getRotorValAdjusted(reading) + global.dir) % 6; //0- bitsPerSector --> smaller sector
         global.setMotorFreeTemporarily.store(false, std::memory_order::relaxed);
         taskENTER_CRITICAL( &sensorMux );
         global.tlog_trailingReadAS5600.store(global.tlog_readAS5600.load()); /*ensure happens on same core as mathLoop*/
         global.tlog_readAS5600.store(tlog);
         taskEXIT_CRITICAL( &sensorMux );

      } else{
         global.oldSectorTarget = global.sectorTarget;
         global.setMotorFreeTemporarily.store(true, std::memory_order::relaxed);

         #ifdef ENABLE_GAMBLING_ON_I2C
         int timeNow = SNAP();
         /* If the time between thsi failure and the trailing failure in the queue is less than threshold, Call an demergency*/
         if ((timeNow - failHistory[failIndex % FAIL_SLOTS] <= FAIL_THRESHOLD_TIME ) && (failIndex >= FAIL_SLOTS)){
            int resetAttemptsLeft;
            for( resetAttemptsLeft = MAX_BUS_RESET_ATTEMTPS ; resetAttemptsLeft > 0; resetAttemptsLeft--){
               if(i2c_master_bus_reset(busHandle) == ESP_OK){
                  failIndex = 0;
                  /*================================= JUDGEMENT DAY ====================================*/
                  if ((timeNow - jailHistory[jailCell % JAILBREAK_SLOTS] <= JAILBREAK_THRESHOLD_TIME ) && (jailCell >= JAILBREAK_SLOTS)){
                     /* You had One too many chances */
                     abort();
                  } else {
                     jailHistory[jailCell++ % JAILBREAK_SLOTS] = timeNow;
                  }
                  /*================================= JUDGEMENT DAY ====================================*/
                  ESP_LOGW("JAILBREAK!","\n");
                  break;
                  resetAttemptsLeft = -1;
               }  else {
                  /* Resetting Failed*/
                  esp_rom_printf("Roblox police on me! \n");
               }
            }
            if( resetAttemptsLeft == 0){
               abort();
            } else { /* sucess! */
               failIndex = 0;
               // ulTaskNotifyValueClear(NULL, UINT_MAX);
            }
         } else {
            failHistory[failIndex++ % FAIL_SLOTS] = timeNow;
         }
         #endif
         tag("#F ");
      }

      #if defined(debug_i2cTransmitTime)
      if(isMinutelyCheckup(printCounter)){ 
         startTime = SNAP() - startTime;      esp_rom_printf("@%d+%d,%d \n"  , lap1,startTime,file1);
      }
      #endif
   }
   
}
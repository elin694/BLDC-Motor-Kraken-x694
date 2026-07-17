#include "Initialize.h"
#include "GateControl.h"
#define isMinutelyCheckup(x) ((x % 1024) == 0)
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
BaseType_t xHigherPriorityTaskWoken2 = pdFALSE; 
TickType_t synchronizedTime;
// UBaseType_t thisTaskPriority;
TaskHandle_t initializeI2CTask= NULL;
void initialize(void * parameter){   
   pinSetup();
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));

   ulTaskNotifyValueClear(NULL, 0xffffffff);
   xTaskNotifyStateClear(NULL);
   xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
   mcpwmSetup(); 
   ESP_ERROR_CHECK(esp_timer_create(&gsnTimerSetup, &gsnTimerHandle));

   int b = global.blockPeriod.load(std::memory_order::relaxed);
   int c=global.newVelPotValue.load(std::memory_order::relaxed);
   ESP_LOGI("init.cpp ","blockPeriod %d| new velocityflag: %d", b, c);//nti
   xTaskCreatePinnedToCore(executeGates, "gsn", 3000, NULL,  15, &executeGatesTask, 0);

   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   synchronizedTime = xTaskGetTickCount();
   int now1 = esp_timer_get_time();
   xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &synchronizedTime,  15, &getSectorNumberTask, 1);
   xTaskCreatePinnedToCore(debugMonitor, "debugLog", 5000, &synchronizedTime, 3, NULL, 0);
   // xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 2000, &synchronizedTime, 6, NULL,0);
   xTaskCreatePinnedToCore(initializeInterruptEnablePin, "startVtimer", 2000, &synchronizedTime, 6, NULL, 0);
   esp_intr_dump(stdout);
   esp_timer_dump(stdout);
   esp_err_t probeCheck = i2c_master_probe(busHandle, as5600Address, 1);
   int now2 = esp_timer_get_time()-now1;
   ESP_LOGI("init", "TaskCreation(us): %d, Probe Check %d", now2, probeCheck);
   vTaskDelete(NULL);
}
// xTaskCreatePinnedToCore(mathItOut, "mathItOut", 10000, &synchronizedTime, (int)(thisTaskPriority)+3, &mathItOutTask, 0);

void pinSetup(){
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
      gpio_set_level(gateArray[i], 0);
   }
}

void initializeInterruptEnablePin(void * startTick6){ 
   TickType_t startTick = *(TickType_t*)startTick6;
   
   xTaskDelayUntil(&startTick,initializationLatency);
   ESP_ERROR_CHECK(mcpwm_timer_start_stop(VTimer, MCPWM_TIMER_START_NO_STOP));
   #ifndef lastResort
   mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
   MCPWMx->int_clr.val=  clearReg.val;
   MCPWMx->int_ena.timer0_tez_int_ena = 1; 
   #endif
   vTaskDelete(NULL);
}

void IRAM_ATTR runOnESPTimerIntr(void * globe) {
   vTaskNotifyGiveFromISR(getSectorNumberTask, &xHigherPriorityTaskWoken);
   if(xHigherPriorityTaskWoken == pdTRUE){
      xHigherPriorityTaskWoken =pdFALSE;
      esp_timer_isr_dispatch_need_yield();
   }
}

#ifdef lastResort
bool IRAM_ATTR VTimerCallback(mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t *edata, void *user_ctx) {
   return runActualISR(user_ctx);
}
#else
void IRAM_ATTR runOnMCPWMIntr(void * user_ctx) {
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

volatile bool oneTimeFlag = false;
bool runActualISR(void * data){
   #define ACCEPTABLE_I2C_READ_WINDOW 230
   gVar_t *masterVar = (gVar_t*)data;
   // tag(cyan "V");
   int timeNow = esp_timer_get_time();
   if((timeNow - masterVar->tlog_readAS5600.load()) < ACCEPTABLE_I2C_READ_WINDOW ){
      // tag(cyan "V");
      //if global.readA S5600==false, the read is taking too long, so might as well let motor coast
      /*execute gates only if we have a valid i2c value and Vtimer tells us to switch phaee */;
      // if(oneTimeFlag){
      //    oneTimeFlag =false;
      //    xTaskNotifyFromISR(executeGatesTask, 0, eIncrement, &xHigherPriorityTaskWoken2);
      // }
      // if(xHigherPriorityTaskWoken2 == pdTRUE) {
      //    xHigherPriorityTaskWoken2 = pdFALSE;
      //    portYIELD_FROM_ISR();
      // }
      return true;
   }
   return false;
}

void as5600initialize(void * parameter) {
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));

   int startWatch  =esp_timer_get_time();
   //read current settings
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      -1)
   );
   
   int lapWatch =esp_timer_get_time()-startWatch;
   fthRegister[1]= (fthRegisterData[0] & fth_sf_clear_mask) | fth_sf_set_mask; //rese
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 2, fthRegisterData, 1, -1));
   int lapWatch2 =esp_timer_get_time()-startWatch;
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 1, fthRegisterData, 1, -1));
   int lapWatch3 =esp_timer_get_time()-startWatch;
   ESP_LOGI(magenta "init.cpp", "\nas5600 Fast Fillter Threshold Set to %d\n1st REG read time:%4d \nSF-FTH write time:%4d REG_Check time:%4d ", 
      (int)fthRegisterData[0],
      lapWatch,
      lapWatch2,
      lapWatch3
   );
   
   xTaskNotifyGive(setupTask);
   vTaskDelete(NULL);
}


void IRAM_ATTR getSectorNumber(void * startTick1){
   TickType_t startTick = *(TickType_t*)startTick1;
   int lap1 =0;
   int startTime =0;
   uint32_t printCounter=0;

    uint32_t counter =0;
    uint32_t failCounter =0;

    uint32_t startTimer =0;
    uint32_t file1 =0;
    uint32_t angle = 0;

   ESP_ERROR_CHECK(esp_timer_start_periodic(gsnTimerHandle, estimatedI2CReadTime_us));
   xTaskDelayUntil(&startTick,initializationLatency);
   while(1){
      file1 = file1 + ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1))-1;
      startTimer = esp_timer_get_time();
      counter++;

      // uint32_t file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(1));
      // // xTaskNotifyWaitIndexed(0, ULONG_MAX,ULONG_MAX, &file1, pdMS_TO_TICKS(1000));

      // #if (defined(debug_i2cTransmitTime) || defined(debug_useTagFlag))
      // if(isMinutelyCheckup(++printCounter)){ 
      //    startTime= esp_timer_get_time(); 
         
      //    #ifdef debug_useTagFlag
      //    tagFlag(true,0); //tags before and after transmit
      //    #endif
      // }
      // #endif
      esp_err_t valRequestStatus= i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, 1, as5600RawDataBuf, 2, i2cWaitout);
      // #if (defined(debug_i2cTransmitTime) || defined(debug_useTagFlag))
      // if(isMinutelyCheckup(printCounter)){ 
      //    lap1 = esp_timer_get_time() - startTime;
         
      //    #if (!defined(debug_i2cTransmitTime) && defined(debug_useTagFlag))
      //    tagFlag(false, lap1); //tags before and after transmit
      //    #endif
      // }
      // #endif

      // if(valRequestStatus == ESP_OK){
      //    // isr2i.fetch_add(1,std::memory_order::relaxed);
      //    // global.oldSectorTarget = global.sectorTarget;
         
      //    // uint32_t reading = (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]; 
      //    // global.rotorVal = reading;
      //    // global.sectorTarget = (uint32_t)(getRotorValAdjusted(reading)+global.dir) % 6; //0- bitsPerSector --> smaller sector
      //    // global.setMotorFreeTemporarily.store(false, std::memory_order::relaxed);
      //    uint32_t tlog = esp_timer_get_time();
      //    global.tlog_readAS5600.store(tlog);
      // } else{
      // //    global.oldSectorTarget=global.sectorTarget;
      // //    global.setMotorFreeTemporarily.store(true, std::memory_order::relaxed);
      // //    tag("#F ");
      // }

      // #if defined(debug_i2cTransmitTime)
      // if(isMinutelyCheckup(printCounter)){ 
      //    startTime = esp_timer_get_time() - startTime;      esp_rom_printf("@%d+%d,%d \n"  , lap1,startTime,file1);
      // }
      // #endif
      // // taskYIELD();
      // esp_timer_start_once(etimerHandle,250);
      uint32_t lap1 =esp_timer_get_time() -startTimer;
      if(valRequestStatus ==ESP_OK){
         angle = (( as5600RawDataBuf[0] << 8) | as5600RawDataBuf[1]);
         if((counter %1024)==0){
               esp_rom_printf("Pos:%4d C:%6d F:%d t-time:%d FailedHandoffs:%3d\n", angle, counter, failCounter, lap1, file1);
         }
      }else{
         failCounter++;
      }

   }
   
}
#include "Initialize.h"
#include "GateControl.h"
#include "esp_intr_alloc.h"
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
TickType_t pxPreviousWakeTime;
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
   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   pxPreviousWakeTime = xTaskGetTickCount();
   xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &pxPreviousWakeTime,  21, &getSectorNumberTask, 1);
   xTaskCreatePinnedToCore(debugLog, "debugLog", 5000, &pxPreviousWakeTime, 3, NULL, 0);
   xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 2000, &pxPreviousWakeTime, 6, NULL,0);

   initializeInterruptEnablePin(); //after isr init  and L sync 
   int b = global.blockPeriod.load(std::memory_order::relaxed);
   int c=global.newVelPotValue.load(std::memory_order::relaxed);
   ESP_LOGE("init.cpp","Priming Vpot blockPeriod %d| new velocityflag: %d", b, c);//nti
   esp_intr_dump(stdout);

   vTaskDelete(NULL);
}
// // // xTaskCreatePinnedToCore(mathItOut, "mathItOut", 10000, &pxPreviousWakeTime, (int)(thisTaskPriority)+3, &mathItOutTask, 0);


void pinSetup(){
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
   }
}

void initializeInterruptEnablePin(){
   ESP_ERROR_CHECK(mcpwm_timer_start_stop(VTimer, MCPWM_TIMER_START_NO_STOP));
   #ifndef lastResort
   mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
   MCPWMx->int_clr.val=  clearReg.val;
   MCPWMx->int_ena.timer0_tez_int_ena = 1; 
   #endif
}

void initializeISR(){
mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_ERROR_CHECK(esp_intr_alloc(
        ETS_PWM0_INTR_SOURCE,
        runOnMCPWMIntrPriority | ESP_INTR_FLAG_IRAM,
        runOnMCPWMIntr,
        NULL,
        &oneBlockISR
    ));
}

void IRAM_ATTR runOnESPTimerIntr(void * globe) {
   vTaskNotifyGiveIndexedFromISR(getSectorNumberTask, 0, &xHigherPriorityTaskWoken);
   esp_timer_isr_dispatch_need_yield();
}

#ifdef lastResort
bool IRAM_ATTR VTimerCallback(mcpwm_timer_handle_t timer, const mcpwm_timer_event_data_t *edata, void *user_ctx) {
   gVar_t *masterVar = (gVar_t*)user_ctx;
   tag(cyan "V");
   if(masterVar->readAS5600.exchange(false)){ //core 0
         //if global.readA S5600==false, the read is taking too long, so might as well let motor coast
         /*execute gates only if we have a valid i2c value and Vtimer tells us to switch phaee */;
         executeGates(false);
         portYIELD_FROM_ISR();
         return true;
      }
      return false;
}
#else
void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts

      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS VTIMER
         tag(cyan "V");
         if(global.readAS5600.exchange(false)){ //core 0
               //if global.readA S5600==false, the read is taking too long, so might as well let motor coast
               /*execute gates only if we have a valid i2c value and Vtimer tells us to switch phaee */;
               executeGates(false);
         }
         portYIELD_FROM_ISR();
         MCPWMx->int_clr.val = tempClearR1.val;
         return;
      } 
   }  
}
#endif


void as5600initialize(void * parameter) {
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));

   isr2CurrentTime =esp_timer_get_time();
   //read current settings
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      -1)
   );
   
   isr2CurrentTime2 =esp_timer_get_time()-isr2CurrentTime;
   //The watchdog timer allows saving power by switching into LMP3 if the angle stays within the watchdog threshold of 4 LSB for at least one minute, as
   fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask; //rese
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister,(size_t)2, fthRegisterData, (size_t)1, -1));
   isr2CurrentTime =esp_timer_get_time()-isr2CurrentTime;
   
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, (size_t)1, fthRegisterData, (size_t)1, -1));
   //150*4096*16/1000000 =9.8 lsb in 1 sample time ==> round up so it changes to slow filter faster
   ESP_LOGI(magenta "init.cpp", "as5600 Fast Fillter Threshold Set: %d \n", (int)fthRegisterData[0]);
   xTaskNotifyGive(setupTask);
   

   vTaskDelete(NULL);
}

void IRAM_ATTR getSectorNumber(void * startTick1){
   TickType_t startTick = *(TickType_t*)startTick1;
   ESP_ERROR_CHECK(esp_timer_start_periodic(gsnTimerHandle,estimatedI2CReadTimeInMicros));
   xTaskDelayUntil(&startTick,initializationLatency);
   while(1){
      //uint32_t file1 =0; //where to save notif value for counting sephamore- ensure it is 1()
      uint32_t file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(1));
      // xTaskNotifyWaitIndexed(0, ULONG_MAX,ULONG_MAX, &file1, pdMS_TO_TICKS(1000));

      #if (defined(debug_spamPrintTimeISR1) || defined(debug_useTagFlag))
      if((isr2CurrentCounter.fetch_add(1,std::memory_order::relaxed)%128)==0){ 
         /*TIMETHETIMER ttt*/isr2CurrentTime= esp_timer_get_time(); 
         isr2CurrentCounterCounted =true;
         #ifdef debug_useTagFlag
         tagFlag(true,0); //tags before and after transmit
         #endif
      }
      #endif
      esp_err_t valRequestStatus= i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, 1, as5600RawDataBuf, 2, i2cWaitout);
      #if (defined(debug_spamPrintTimeISR1) || defined(debug_useTagFlag))
      if(isr2CurrentCounterCounted){
         isr2CurrentTime2 = esp_timer_get_time() - isr2CurrentTime;
         #if (!defined(debug_spamPrintTimeISR1) && defined(debug_useTagFlag))
         tagFlag(false, isr2CurrentTime2); //tags before and after transmit
            isr2CurrentCounterCounted =false;
         #endif
      }
      #endif

      if(valRequestStatus == ESP_OK){
         isr2i.fetch_add(1,std::memory_order::relaxed);
         global.oldSectorTarget = global.sectorTarget;
         
         uint32_t reading = (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]; 
         global.rotorVal = reading;
         global.sectorTarget = (uint32_t)(getRotorValAdjusted(reading)+global.dir) % 6; //0- bitsPerSector --> smaller sector
         global.setMotorFreeTemporarily.store(false, std::memory_order::relaxed);
      } else{
         global.oldSectorTarget=global.sectorTarget;
         global.setMotorFreeTemporarily.store(true, std::memory_order::relaxed);
         tag("#F ");
      }
      global.readAS5600.store(true); //core 1

      #if defined(debug_spamPrintTimeISR1)
      if(isr2CurrentCounterCounted){
         isr2CurrentTime = esp_timer_get_time() - isr2CurrentTime;      esp_rom_printf("@%d+%d,%d \n"  , isr2CurrentTime2,isr2CurrentTime,file1);
         isr2CurrentCounterCounted =false;
      }
      #endif
      taskYIELD();
   }
   
}
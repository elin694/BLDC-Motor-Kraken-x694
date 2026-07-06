#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

#define SECTOR_PER_BITS static_cast<float>(1 / (4096.0f / (electricalCycles* 6.0f)))
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
UBaseType_t thisTaskPriority = uxTaskPriorityGet(setupTask);
void initialize(void * parameter){   
   pinSetup();
   vTaskDelay(pdMS_TO_TICKS(20)); //To let gate driver setup
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   int k = global.dir;
   #ifdef debug_dontReadVelocityPot
   global.targetVelocity=VTimerResolution/(18.0f* debug_dontReadVelocityPot);
   (global.targetVelocity < 0) ? (global.dir = 4) : (global.dir = 2);
   global.blockPeriod = debug_dontReadVelocityPot;//does not affect
   global.newVelPotValue =true; //nti
   #endif
   readPotOnce(NULL);
   int k2 = global.dir;
   ESP_LOGE("init.cpp","Priming Vpot blockPeriod %d| new velocityflag: %d, dir before read%d after%d", global.blockPeriod, global.newVelPotValue,k,k2);//nti
   as5600initialize(); 
   #ifdef useESPTimerLoopOverFreeRTOSLoop
   ESP_ERROR_CHECK(esp_timer_create(&gsnTimerSetup,&gsnTimerHandle));
   ESP_ERROR_CHECK(esp_timer_start_periodic(gsnTimerHandle, estimatedI2CReadTimeInMicros+30));
   //170+20 for poor quality, 170+30 for ok quality
   #endif
   xTaskCreatePinnedToCore(getSectorNumber, "SETUP", 8000, NULL,  21, &getSectorNumberTask, 1);
   xTaskNotifyStateClearIndexed(getSectorNumberTask,0);
   ulTaskNotifyValueClear(getSectorNumberTask ,0);

   mcpwmSetup(global.sectorTarget); //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt
   ESP_LOGW("init.cpp"," maximum target RPs; %6.3f, minimum target RPS: %6.3f",fMin, fMax);
   #ifdef enableReadPotRepeat
   xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 10000, NULL, 6, NULL,0);
   ESP_LOGW("init.cpp", "nableReadPotRepeat");
   #endif 
   xTaskCreatePinnedToCore(debugLog, "debugLog", 5000, NULL, 3, NULL, 0);
   // xTaskCreatePinnedToCore(mathItOut, "mathItOut", 10000, NULL, (int)(thisTaskPriority)+3, NULL, 0);
   vTaskDelete(NULL);
}

int mod6 (int value){ //for single add
    if(value > 5){
        value -= 6;
    } else if(value < 0){
        value += 6;
    }
    return value;
}

void pinSetup(){
   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
   }
}

void initializeInterruptEnablePin(){
    mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_clr.val=  clearReg.val;
    #ifndef useESPTimerLoopOverFreeRTOSLoop
    MCPWMx->int_ena.timer0_tez_int_ena = 1; // //timer 0= BTimer
    #endif
    MCPWMx->int_ena.timer1_tez_int_ena = 1; //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
    MCPWMx->int_ena.timer2_tez_int_ena = 1; // //timer 0= BTimer
    ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
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

void IRAM_ATTR runOnESPTimerIntr(void * globe) {
   vTaskNotifyGiveIndexedFromISR(getSectorNumberTask, 0, &xHigherPriorityTaskWoken);
   MCPWMx-> int_clr.val = tempClearR1.val;
   if(xHigherPriorityTaskWoken == pdTRUE){
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      xHigherPriorityTaskWoken =pdFALSE;
   }
}
void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts

      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
         if(global.newPhaseSwitchFlag.load()){
            #ifdef debug_fastPrints
            esp_rom_printf(blue "B");
            #elif defined(debug_hyperFastPrints)
            darray[dindex[0].fetch_add(1)]= blue "B ";
            // rA[rindx.fetch_add(1)] = (int)esp_timer_get_time()-rA[0];
            #endif
            xHigherPriorityTaskWoken =pdFALSE;
            vTaskNotifyGiveIndexedFromISR(getSectorNumberTask, 0, &xHigherPriorityTaskWoken);
            MCPWMx-> int_clr.val = tempClearR1.val;
            if(xHigherPriorityTaskWoken == pdTRUE){
               portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
         }else{
            MCPWMx-> int_clr.val = tempClearR1.val;
         }
         return;
         /*CASE 1 ABOVE*/

      } else if(tempStatusReg.timer1_tez_int_st){ /* BLV*/
         if(global.readAS5600.exchange(false)){
            if(global.newPhaseSwitchFlag.exchange(false)){
               //if global.readAS5600==false, the read is taking too long, so might as well let motor freespin
               // global.newPhaseSwitchFlag.exchange(false);
               executeGates(false);
            }
         // } else{
         //    if(global.newPhaseSwitchFlag.exchange(false)){
         //       executeGates(true);
         //    }
         }
         MCPWMx->int_clr.val = (tempClearR2.val | tempClearR3.val);
         return;
         /*CASE 2 ABOVE*/

      } else if(tempStatusReg.timer2_tez_int_st){
         #ifdef debug_fastPrints
         esp_rom_printf(magenta "V");
         #elif defined(debug_hyperFastPrints)
         darray[dindex[0].fetch_add(1)] = magenta "V";
         #endif
         global.newPhaseSwitchFlag.store(true);
         MCPWMx-> int_clr.val = tempClearR3.val;
         /*CASE 3 ABOVE*/
      }
   }  
}

void as5600initialize() {
   isr2CurrentTime =esp_timer_get_time();
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/20*i2cWaitout)
   );
   
   isr2CurrentTime2 =esp_timer_get_time()-isr2CurrentTime;
   
   fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask; //rese
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister,(size_t)2, fthRegisterData, (size_t)1, /*alpha*/20*i2cWaitout));
   isr2CurrentTime =esp_timer_get_time()-isr2CurrentTime;
   
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, (size_t)1, fthRegisterData, (size_t)1, /*alpha*/20*i2cWaitout));
   //150*4096*16/1000000 =9.8 lsb in 1 sample time ==> round up so it changes to slow filter faster
   ESP_LOGI(magenta "init.cpp", "as5600 Fast Fillter Threshold Set: %d \n", (int)fthRegisterData[0]);
   //===================================GET A STARTING SECTOR VALUE ===================================
   /*TIMETHETIMER ttt*/int t1= esp_timer_get_time();
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, as5600WriteSize,as5600RawDataBuf, as5600ReadSize, /*alpha*/20*i2cWaitout));
   t1= esp_timer_get_time() - t1;esp_rom_printf("==first i2c readTime: %d,%d,%d\n", isr2CurrentTime2, isr2CurrentTime,t1);

   global.rotorVal = (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1];
   int newBNumber = (int)((getRotorValAdjusted(global.rotorVal) * SECTOR_PER_BITS)+global.dir)%6; //0- bitsPerSector --> smaller sector
   global.oldSectorTarget = newBNumber; 
   global.sectorTarget = newBNumber;
}

//    #include "esp_private/pm_impl.h"
// #include "esp_pm.h"
// #include "esp_private/esp_clk.h"
void IRAM_ATTR getSectorNumber(void *returnValue){
   // uint32_t file1 =0; //where to save notif value for counting sephamore- ensure it is 1()
   vTaskDelay(pdMS_TO_TICKS(15));
   TickType_t pxPreviousWakeTime;
   pxPreviousWakeTime = xTaskGetTickCount();
   
   while(1){
      #ifdef useESPTimerLoopOverFreeRTOSLoop
      file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(100));
      #else
      vTaskDelayUntil(&pxPreviousWakeTime,pdMS_TO_TICKS(1));
      #endif
      #ifdef debug_fastPrints
      // esp_rom_printf("GSN");
      #elif defined(debug_hyperFastPrints)
      // darray[dindex[0].fetch_add(1)]= "GSN ";
      #endif
      // xTaskNotifyWaitIndexed(0, ULONG_MAX,ULONG_MAX, &file1, pdMS_TO_TICKS(1000));
      #if (defined(debug_spamPrintTimeISR1))
      if(!(isr2CurrentCounter++%16)){ 
         /*TIMETHETIMER ttt*/isr2CurrentTime= esp_timer_get_time(); 
         isr2CurrentCounterCounted =true;
      }
      #endif

      assert(as5600Handle != nullptr);
      esp_err_t valRequestStatus= i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, 1, (uint8_t*)as5600RawDataBuf, 2, i2cWaitout);
      #if defined(debug_spamPrintTimeISR1)
      if(isr2CurrentCounterCounted){
         isr2CurrentTime2 = esp_timer_get_time() - isr2CurrentTime;
      }
      #endif
      if(valRequestStatus != ESP_ERR_INVALID_STATE){
         global.rotorVal = (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]; 
         global.oldSectorTarget = global.sectorTarget;
         global.sectorTarget = static_cast<uint32_t>((getRotorValAdjusted(global.rotorVal)* SECTOR_PER_BITS)+global.dir) % 6; //0- bitsPerSector --> smaller sector
         int as5600location = (int)(getRotorValAdjusted(global.rotorVal)* SECTOR_PER_BITS)%6;
         as5600BfieldVectorSector.store(as5600location);
         global.setMotorFreeTemporarily.store(false);
      } else{
         global.setMotorFreeTemporarily.store(true);
         global.oldSectorTarget=global.sectorTarget;
         #if defined(debug_hyperFastPrints) 
            darray[dindex[0].fetch_add(1)]= "I2cF& ";
         #endif
      }
      preloadGates();
      global.readAS5600.store(true);

      #if defined(debug_spamPrintTimeISR1)
      if(isr2CurrentCounterCounted){
         isr2CurrentTime = esp_timer_get_time() - isr2CurrentTime;      esp_rom_printf("@%d+%d,%d" SET_CURSOR_FRONT  , isr2CurrentTime2,isr2CurrentTime,file1);
         isr2CurrentCounterCounted =false;
      }
      #endif
   }
   
}

void mathItOut(void *parameter){
   for(;;){
      if(global.rotorVal != -1){ //if a new position is recorded
         /* +error = ahead of target ccw*/
         uint32_t newThetas[3] = {global.rotorVal, global.measuredPositions[0], global.measuredPositions[1]};
         float newOmegas[2] = {
            (newThetas[0] - newThetas[1])/(float)(SetAs5600PollPeriod*timerResolution),
            (newThetas[1] - newThetas[2])/((float)SetAs5600PollPeriod*timerResolution),
         };
         float newAlphas[1] = {(newOmegas[0] - newOmegas[1])/((float)SetAs5600PollPeriod*timerResolution)};
         
         if(global.controlMethod == VELOCITY_CONTROL){
            float errorVel =global.targetVelocity- global.measureVelocities[0];
            float prevArea =1;
            float prevError =1;
            float dt = SetAs5600PollPeriod/timerResolution;
            float errorP = kPID[VELOCITY_CONTROL][0]*errorVel;
            float errorI = kPID[VELOCITY_CONTROL][1]*(dt * errorVel +prevArea); /*-area*k, */
            float errorD = kPID[VELOCITY_CONTROL][2]*(errorVel-prevError)/dt; //subtract the slope (for a + slope, error should be negative)
            /*finally changes set cmpVal*/
            float errorTotal  = errorP +errorI+errorD; //⍺
            //error can theoretically go from 0 to infinity for 1 c. ->
         }
         // global.measuredPositions = newThetas;
         // global.measureVelocities = newOmegas;
         // global.measureAccelerations = newAlphas;
         global.rotorVal = -1;
         
      }
   }
}
   
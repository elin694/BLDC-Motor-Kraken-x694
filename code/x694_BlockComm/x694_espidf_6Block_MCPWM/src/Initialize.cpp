#include "Initialize.h"
#include "GateControl.h"
// #include "GC.h"
#include "esp_intr_alloc.h"
bool changeFlag = true;
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
TickType_t pxPreviousWakeTime;
// UBaseType_t thisTaskPriority;
TaskHandle_t initializeI2CTask= NULL;
void initialize(void * parameter){   
   pinSetup();
   //aplePIE
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
   xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
   mcpwmSetup(global.sectorTarget); //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt
   ESP_ERROR_CHECK(esp_timer_create(&gsnTimerSetup,&gsnTimerHandle));

   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   pxPreviousWakeTime = xTaskGetTickCount();
   xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &pxPreviousWakeTime,  21, &getSectorNumberTask, 1);
   xTaskCreatePinnedToCore(debugLog, "debugLog", 5000, &pxPreviousWakeTime, 3, NULL, 0);

   #ifdef enableReadPotRepeat
   xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 2000, &pxPreviousWakeTime, 6, NULL,0);
   #endif 

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
   ESP_ERROR_CHECK(mcpwm_timer_start_stop(velocityTrackerTimer, MCPWM_TIMER_START_NO_STOP));
   mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_clr.val=  clearReg.val;
   //  MCPWMx->int_ena.timer1_Ptez_int_ena = 1; //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
    MCPWMx->int_ena.timer2_tez_int_ena = 1;
   //  ESP_ERROR_CHECK(esp_intr_enable(oneBlockISR)); //Starting AS5600 read ISR
}

void initializeISR(){
mcpwm_int_clr_reg_t clearReg = {.val = ~(static_cast<uint32_t>(0x00000000))};
    MCPWMx->int_ena.val = 0x00000000;
    MCPWMx->int_clr.val=  clearReg.val;
    ESP_ERROR_CHECK(esp_intr_alloc(
        ETS_PWM0_INTR_SOURCE,
        runOnMCPWMIntrPriority | ESP_INTR_FLAG_IRAM,
        runOnMCPWMIntr,
        (void *)&global,
        &oneBlockISR
    ));
}

void IRAM_ATTR runOnESPTimerIntr(void * globe) {
   vTaskNotifyGiveIndexedFromISR(getSectorNumberTask, 0, &xHigherPriorityTaskWoken);
   esp_timer_isr_dispatch_need_yield();
}
void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts

      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
         if(global.newPhaseSwitchFlag.load()){
            tag(blue "B ");
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
         if(global.readAS5600.exchange(false)){ //core 0
            if(global.newPhaseSwitchFlag.exchange(false)){
               //if global.readA S5600==false, the read is taking too long, so might as well let motor coast
               /*execute gates only if we have a valid i2c value and Vtimer tells us to switch phaee */;
               executeGates(false);
            }
         }
         MCPWMx->int_clr.val = (tempClearR2.val | tempClearR3.val);
         return;
         /*CASE 2 ABOVE*/

      } else if(tempStatusReg.timer2_tez_int_st){
        tag(cyan "V");
         global.newPhaseSwitchFlag.store(true);
         MCPWMx-> int_clr.val = tempClearR3.val;
         portYIELD_FROM_ISR();
         /*CASE 3 ABOVE*/
      }
   }  
}

void mathItOut(void * startTick4){ //updates arrrays with new ifo
   TickType_t startTick = *(TickType_t*)startTick4;
   xTaskDelayUntil(&startTick,initializationLatency);
   float dt  = estimatedI2CReadTimeInMicros;
   for(;;){
      // index shuld stay in here
      //cahgne init as5600 read &&&||||| move to next index then save
      uint32_t file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(100));
      int previousPos = global.measuredPos[(global.pindex.fetch_add(1))%cBufSize];
      float previousVel = global.measuredVel[(global.vindex.fetch_add(1))%cBufSize];
      float previousAccel = global.measuredAccel[(global.aindex.fetch_add(1))%cBufSize];
      
      uint32_t vidx = global.vindex;
      uint32_t pidx = global.pindex;
      //*assuming dir is always at ground
      // global.measuredPos[pidx%cBufSize] = global.rotorVal;
      // float newVel = global.measuredVel[vidx%cBufSize] = ((global.rotorVal-previousPos)%4096)/dt;
      // global.measuredAccel[global.aindex%cBufSize] = (newVel-previousVel)/dt;
      taskYIELD();
   }
}

void setTorque(float targetTorque){
      if(global.controlMethod <= TORQUE_CONTROL){
         float magnitude = fabsf(targetTorque);
         if(magnitude < minDuty || magnitude > maxDuty){
            global.setMotorFreeSpin = true; //rmeove delay between this and freespining in the future
         } else{
            for(int i=2; i>-1; i--){
               // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0,(1-magnitude)*(activePwmPeriod/2.0)));
            }
         }
         if(targetTorque < 0){
            global.dir = 5;
         }else{
            global.dir = 2;
         }
      }
}

void setVelocity(float targetVelocity){
   for(;;){
         /* +error = ahead of target ccw*/
         if(global.controlMethod <= VELOCITY_CONTROL){
            float dt  = estimatedI2CReadTimeInMicros;
            uint32_t vidx = global.vindex;
            float errorVel =targetVelocity- global.measuredVel[vidx%cBufSize];/////////////////
            float prevError = global.lastVelError;
            global.totalVelChange = global.totalVelChange + errorVel*dt;

            float errorP = kPID[VELOCITY_CONTROL][0]*errorVel;
            float errorI = kPID[VELOCITY_CONTROL][1]*global.totalVelChange; /*-area*k, */
            float errorD = kPID[VELOCITY_CONTROL][2]*(errorVel-prevError)/dt; //subtract the slope (for a + slope, error should be negative)
            /*finally changes set cmpVal*/
            float errorTotal  = errorP +errorI+errorD; //⍺
            if(errorTotal > maxDuty){
               errorTotal = maxDuty;
            } else if (errorTotal < -maxDuty){
               errorTotal = -maxDuty;
            }
            setTorque(errorTotal);
            ///remember to set prev Erorr and other past var
         }
      }
}

void setPosition(float targetPosition){
   for(;;){
      if(global.controlMethod <= POSITION_CONTROL){
         float dt  = estimatedI2CReadTimeInMicros;
         uint32_t pidx = global.pindex;
         float errorPos =global.targetPosition- global.measuredPos[pidx%cBufSize];/////////////////
         float prevError = global.lastPosError;
         global.totalPosChange = global.totalPosChange + errorPos*dt;

         float errorP = kPID[POSITION_CONTROL][0]*errorPos;
         float errorI = kPID[POSITION_CONTROL][1]*global.totalPosChange; /*-area*k, */
         float errorD = kPID[POSITION_CONTROL][2]*(errorPos-prevError)/dt; //subtract the slope (for a + slope, error should be negative)
         /*finally changes set cmpVal*/
         float errorTotal  = errorP +errorI+errorD; //⍺
         if(errorTotal > maxRPS){
            errorTotal = maxRPS;
         } else if (errorTotal < -maxRPS){
            errorTotal = -maxRPS;
         }
         setVelocity(errorTotal);
         //error can theoretically go from 0 to infinity for 1 c. ->
      }
   }
}

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
   // uint32_t i2cTransmitStatusCounter = 0;
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

      esp_err_t valRequestStatus= i2c_master_transmit_receive(as5600Handle, &as5600TargetRegister, 1, (uint8_t*)as5600RawDataBuf, 2, i2cWaitout);
      #if (defined(debug_spamPrintTimeISR1) || defined(debug_useTagFlag))
      if(isr2CurrentCounterCounted){
         isr2CurrentTime2 = esp_timer_get_time() - isr2CurrentTime;
         tagFlag(false, isr2CurrentTime2); //tags before and after transmit
         #if (!defined(debug_spamPrintTimeISR1) && defined(debug_useTagFlag))
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
      // preloadGates();
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
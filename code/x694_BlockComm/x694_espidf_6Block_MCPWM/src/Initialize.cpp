#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

#define SECTOR_PER_BITS static_cast<float>(1 / (4096.0f / (electricalCycles* 6.0f)))
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
UBaseType_t thisTaskPriority = uxTaskPriorityGet(setupTask);
void initialize(void * parameter){   
   pinSetup();
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   #ifndef debug_testOnLED
      readPotOnce(NULL);
      ESP_LOGE("init.cpp","Priming Vpot blockPeriod %d| newVelPotValue %d", global.blockPeriod, global.newVelPotValue);
   #endif

   #ifdef debug_testOnLED 
      global.sectorTarget = preCompStartingTargetSector;
      global.oldSectorTarget = global.sectorTarget;
   #else
   as5600initialize(); 
   #endif
   xTaskCreatePinnedToCore(getSectorNumber, "SETUP", 8000, NULL,  
      // uxTaskPriorityGet(setupTask)+1  /*priority*/, 
      22,
      &getSectorNumberTask, 1);
      
   mcpwmSetup(global.sectorTarget); //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt
   ESP_LOGW("init.cpp"," maximum RPs; %6.3f, minimum RPS: %6.3f",fMin, fMax);
   /*no bidirection compatability yet
    pull Low high to prime Bootstrap cap?  */
   #ifdef enableReadPotRepeat
   xTaskCreate(readPotRepeat, "readPotRepeat", 10000, NULL, (int)(thisTaskPriority*.5)-2, NULL);
   ESP_LOGW("init.cpp", "VelPot in Loop");
   #endif 
   #if ((defined(debug_spamPrintCounterStatus)) && debug_spamDelay)
   xTaskCreatePinnedToCore(spamSearchCV, "spamSearchCV", 5047,NULL, (int)(thisTaskPriority*.5)-1, NULL, 0);
   #endif
   xTaskCreatePinnedToCore(debugLog, "debugLog", 10000, NULL, (int)(thisTaskPriority*.5)-3, NULL, 0);
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

void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts
      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
         // getTimerCountNow("<");
         if(global.newPhaseSwitchFlag){
            #ifdef debug_fastPrints
            esp_rom_printf(blue "B");
            #endif
            xHigherPriorityTaskWoken = xTaskResumeFromISR(getSectorNumberTask);
            MCPWMx-> int_clr.val = tempClearR1.val;
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
         }else{
            MCPWMx-> int_clr.val = tempClearR1.val;
         }
         return;


      } else if(tempStatusReg.timer1_tez_int_st){ /* BLV*/
         // getTimerCountNow(">");
         if(global.newPhaseSwitchFlag && global.readAS5600){
            #ifdef debug_fastPrints
            // esp_rom_printf(white "| b# ^ %d, n# %d", global.oldSectorTarget, global.sectorTarget);
            #endif
            executeGates(MCPWMx);
            global.readAS5600 = false;
         } 
         MCPWMx->int_clr.val = (tempClearR2.val | tempClearR3.val);
         return;


      } else if(tempStatusReg.timer2_tez_int_st){
         // getTimerCountNow("?");
         global.newPhaseSwitchFlag= true;
         MCPWMx-> int_clr.val = tempClearR3.val;
      }
   }
}

void as5600initialize() {
   #define fth_sf_set_mask (0b00011100 | 0b00000011) //.5 bit error at 11 =sf
   //sets fth and sf , also reduces
   uint8_t fthRegisterData[1] = {0x00};
   uint8_t fthRegister[2] = {0x07, 0x00};
   
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/i2cWaitout)
   );
   fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask; //reset

   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)2, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/i2cWaitout)
   );
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/i2cWaitout)
   );
   //150*4096*16/1000000 =9.8 lsb in 1 sample time ==> round up so it changes to slow filter faster
   ESP_LOGI(magenta "I2cRead", "whole thing %d \n", (int)fthRegisterData[0]);
   //===================================GET A STARTING SECTOR VALUE ===================================
   /*TIMETHETIMER ttt*/int t1= esp_timer_get_time();
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      &as5600TargetRegister, 
      as5600WriteSize,
      as5600RawDataBuf, 
      as5600ReadSize, //ensure 2 bytes is read
      -1)
   );
   t1= esp_timer_get_time() - t1;esp_rom_printf("==first i2c readTime: %d\n", t1);

   global.rotorVal = getRotorValAdjusted((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]);
   
   int newBNumber = (int)((global.rotorVal * SECTOR_PER_BITS)+2*dir)%6; //0- bitsPerSector --> smaller sector
   global.oldSectorTarget = newBNumber; 
   global.sectorTarget = newBNumber;
   // ESP_LOGI(cyan "\nFirstPotRead", "ost, nst: (%d, %d), raw: %d, rotorAng: %d, bl# %d\n", global.oldSectorTarget, global.sectorTarget, rd, global.rotorVal, newBNumber);
}
/*========================================================================================*/
/*========================================================================================*/
void getSectorNumber(void *returnValue){
   while(1){
      vTaskSuspend(NULL);
      #if (defined(debug_spamPrintTimeISR1))
      if(!(isr2CurrentCounter++%16)){ 
         /*TIMETHETIMER ttt*/isr2CurrentTime= esp_timer_get_time(); 
         isr2CurrentCounterCounted =true;
      }
      #endif

      #ifdef debug_testOnLED
       esp_rom_delay_us(estimatedI2CReadTimeInMicros);
      if (motorStall){
         global.oldSectorTarget= global.sectorTarget;
      } else{
         global.oldSectorTarget = global.sectorTarget;
         global.sectorTarget = mod6(global.sectorTarget+1);
         // esp_rom_printf(white "GSNon(%d, %d)", global.oldSectorTarget, global.sectorTarget);
      }
      #else
      ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         &as5600TargetRegister, 
         as5600WriteSize,
         as5600RawDataBuf, 
         as5600ReadSize, //ensure 2 bytes is read
         /*alpha*/ i2cWaitout
      ));
      int debug_as5600V = (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1];
      // #if defined(debug_fastPrints) && defined(debug_spamPrintTimeISR1)
      // esp_rom_printf("val: %d\n", debug_as5600V);
      // #endif

      global.rotorVal = getRotorValAdjusted(debug_as5600V);
      global.oldSectorTarget = global.sectorTarget;
      global.sectorTarget = static_cast<uint32_t>((global.rotorVal * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
      #endif
      preloadGates();
      global.readAS5600 = true;

      #if defined(debug_spamPrintTimeISR1)
      if(isr2CurrentCounterCounted){
         isr2CurrentTime = esp_timer_get_time() - isr2CurrentTime;      esp_rom_printf("@%d\n", isr2CurrentTime);
         isr2CurrentCounterCounted =false;
      }
      #endif
   }
}
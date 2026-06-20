#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

#define SECTOR_PER_BITS static_cast<float>(1 / (4096.0f / (electricalCycles* 6.0f)))
BaseType_t xHigherPriorityTaskWoken = pdFALSE; 

void initialize(void * parameter){   
   pinSetup();
   initAnalogReadOnce();
   #ifndef debug_testOnLED
      readPotOnce(NULL);
   #endif

   for(int i = 0; i<4; i++){
      global.CMR_value_3[i] = global.blockPeriod*i;
      ESP_LOGI(blue "C_3 thirds", "%f", global.CMR_value_3[i]);
   };

   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   // vTaskDelay(pdMS_TO_TICKS(10000000)); //wait for i2c to be ready, otherwise first few reads might be wrong, which can cause wrong block commutation and motor stall
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
   mcpwmSetup(global.sectorTarget, &global.blockPeriod); //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt

   /*no bidirection compatability yet
    pull Low high to prime Bootstrap cap?  */
   UBaseType_t thisTaskPriority = uxTaskPriorityGet(setupTask);
   #ifdef debug_readPotRepeat
   xTaskCreate(readPotRepeat, "readPotRepeat", 10000, NULL, (int)(thisTaskPriority*.5)-2, NULL);
   #endif 
   
   #if (defined(debug_spamPrintBlockStatus) || defined(debug_spamPrintCounterStatus) || debug_spamDelay)
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
   #ifdef digitalReadPin
   gpio_reset_pin(digitalReadPin);
   gpio_set_direction(digitalReadPin, GPIO_MODE_INPUT);
   gpio_set_pull_mode(digitalReadPin, GPIO_FLOATING);
   #endif
   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      ESP_LOGW("pintSetup", "i: %d, lvl: %d",i,gpio_get_level(gateArray[i]));
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
   }
}

void initAnalogReadOnce(){
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
}
//run pwm at f ~40-50kHz for adjustable torque control

void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;   
   if(tempStatusReg.val){ //in case of ghost interrupts
      // getTimerCountNow("@");
      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
      //as5600 is default increasing on clockwise. set DIR high to invert 
      #ifdef debug_fastPrints
         esp_rom_printf(blue "B");
      #endif

      xHigherPriorityTaskWoken = xTaskResumeFromISR(getSectorNumberTask);
      MCPWMx-> int_clr.val = tempClearR1.val;
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      return;

   } else if(
      tempStatusReg.timer1_tez_int_st || //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
      tempStatusReg.timer1_tep_int_st || //timer 1= LTimer, (ie change from block 0-1), 2^7 = 128
      /*since phaseA_gen_one_third =0 */
      tempStatusReg.op0_tea_int_st || // op0 = phase A lowside, (ie change from block 5-0 or 1-2), 2^15 = 32765
      tempStatusReg.op0_teb_int_st) // timer, (ie change from block 2-3 or 4-5), 2^18 = 262144
      { //L TIMER = id1, SO WE USE TIMER 1
         
         #ifdef debug_fastPrints
         if(tempStatusReg.timer1_tez_int_st) esp_rom_printf(magenta "|TEZ");
         if(tempStatusReg.timer1_tep_int_st) esp_rom_printf(magenta "|TEP");
         if(tempStatusReg.op0_tea_int_st)    esp_rom_printf(magenta "|TEA");
         if(tempStatusReg.op0_teb_int_st)    esp_rom_printf(magenta "|TEB");
         // esp_rom_printf(white "| b# ^ %d, n# %d", global.oldSectorTarget, global.sectorTarget);
         #endif
         
         // esp_rom_printf(white "i2 BL 14, lvl: %d\n",gpio_get_level(digitalReadPin));
         // esp_rom_printf(green "s %d, %d",global.oldSectorTarget , global.sectorTarget );
         
         // 32768, 128, 32768, 262144, 16, 262144
         executeGates(&tempClearR2, MCPWMx);
         return;
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
      /*alpha*/100)
   );
   fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask; //reset

   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)2, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/100)
   );
   esp_rom_printf(blue "second tr done, ");
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
      /*alpha*/100)
   );
   esp_rom_printf(blue "first done, ");
   //max sample time at 150us
   //150*4096*16/1000000 =9.8 lsb in 1 sample time ==> round up so it changes to slow filter faster
   ESP_LOGI(magenta "Read:", "whole thing %d \n", (int)fthRegisterData[0]);
   //===================================GET A STARTING SECTOR VALUE ===================================
   /*TIMETHETIMER ttt*/int t1= esp_timer_get_time();
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      &as5600TargetRegister, 
      as5600WriteSize,
      as5600RawDataBuf, 
      as5600ReadSize, //ensure 2 bytes is read
      -1)
   );
   t1= esp_timer_get_time() - t1;esp_rom_printf("n ===========t1: %d\n", t1);

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
      #if defined(debug_fastPrints) || defined(debug_spamPrintTimeISR1) 
      if(!(isr2CurrentCounter++%16)){ //just in case, should never happen
         /*TIMETHETIMER ttt*/isr2CurrentTime= esp_timer_get_time(); 
         isr2CurrentCounterCounted =true;
      }
      #endif

      #ifdef debug_testOnLED
       esp_rom_delay_us(210);
      if (motorStall){
         global.oldSectorTarget= global.sectorTarget;
      } else{
         global.oldSectorTarget = global.sectorTarget;
         global.sectorTarget = mod6(global.sectorTarget+1);
         esp_rom_printf(white "on(%d, %d)", global.oldSectorTarget, global.sectorTarget);
      }
      #else
      ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         &as5600TargetRegister, 
         as5600WriteSize,
         as5600RawDataBuf, 
         as5600ReadSize, //ensure 2 bytes is read
         /*alpha*/100
      ));

      global.rotorVal = getRotorValAdjusted((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]);
      
      global.sectorTarget = static_cast<uint32_t>((global.rotorVal * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
      // int newBNumber = static_cast<uint32_t>((global.rotorVal * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
      // if((global.sectorTarget >= 0) && (std::abs(newBNumber - global.sectorTarget)>1) && (std::abs(newBNumber - global.sectorTarget)) != 5){
      //    ESP_LOGE("POTENTIOMETER READ",": Sector 1+ jump . Previous Sector: %2d. Incoming Sector: %2d, raw%d, blN: %d", global.sectorTarget, newBNumber, rd, newBNumber);
      //    abort();
      // }
      // global.oldSectorTarget = global.sectorTarget;
      // global.sectorTarget = newBNumber;
      #endif
      preloadGates();
      #if defined(debug_fastPrints) || defined(debug_spamPrintTimeISR1) 
      if(isr2CurrentCounterCounted){
         isr2CurrentTime = esp_timer_get_time() - isr2CurrentTime;      esp_rom_printf("@%d\n", isr2CurrentTime);
         isr2CurrentCounterCounted =false;
      }
      #endif
   }
}
#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

void initialize(void * parameter){
   
   pinSetup();
   initAnalogReadOnce();
   #ifndef debug_testOnLED
      // readPotOnce(NULL);
      
   #endif

   for(int i = 0; i<4; i++){
      global.CMR_value_3[i] = global.blockPeriod*i;
      ESP_LOGI(blue "C_3 thirds", "%f", global.CMR_value_3[i]);
   };


   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   #ifdef debug_testOnLED 
      global.sectorTarget = preCompStartingTargetSector;
      global.oldSectorTarget = global.sectorTarget;
      xTaskCreatePinnedToCore(getSectorNumber, "SETUP", 8000, NULL,  uxTaskPriorityGet(setupTask)+1  /*priority*/, &getSectorNumberTask, 1);
   #else
      as5600initialize();
   #endif
   
   mcpwmSetup(global.sectorTarget, &global.blockPeriod);
    //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt

   /*no bidirection compatability yet
    pull Low high to prime Bootstrap cap?  */
   UBaseType_t thisTaskPriority = uxTaskPriorityGet(setupTask);
   // xTaskCreate(readPotRepeat, "readPotRepeat", 10000, NULL, (int)(thisTaskPriority*.5)-2, NULL);
   #if (defined(debug_spamPrintBlockStatus) || defined(debug_spamPrintCounterStatus))
      xTaskCreatePinnedToCore(spamSearchCV, "spamSearchCV", 5047,NULL, (int)(thisTaskPriority*.5)-1, NULL, 0);
   #endif

   xTaskCreatePinnedToCore(debugLog, "debugLog", 10000, NULL, (int)(thisTaskPriority*.5)-3, NULL, 0);
   vTaskDelete(NULL);
}






int mod6 (int value){ //for single add
    if(value > 5){
        value -= 6;
      //   value = 0 ;
    } else if(value < 0){
        value += 6;
      //   value = 5;
    }
    return value;
}

i2c_master_bus_config_t busSetup = { 
   .i2c_port = -1,
   .sda_io_num= dataPin,
   .scl_io_num= clockPin,
   .clk_source = I2C_CLK_SRC_APB,
   // .glitch_ignore_cnt = 7,
   // .intr_priority = 1,
   .flags={.enable_internal_pullup = true}
};
i2c_master_bus_handle_t busHandle;

i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= 400000, //need fast enough  to avoid invalid state
   .scl_wait_us = 30,
   .flags = {.disable_ack_check = false}
};
i2c_master_dev_handle_t as5600Handle;

void pinSetup(){
   #ifdef digitalReadPin
   gpio_reset_pin(digitalReadPin);
   gpio_set_direction(digitalReadPin, GPIO_MODE_INPUT);
   gpio_set_pull_mode(digitalReadPin, GPIO_FLOATING);
   #endif

   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   // use ledc to set potentionmeter to input analog read
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      ESP_LOGW("pintSetup", "i: %d, lvl: %d",i,gpio_get_level(gateArray[i]));
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
   }
}

#define SECTOR_PER_BITS static_cast<float>(1 / (4096.0f / (electricalCycles* 6.0f)))
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
      3));
      fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask; //reset

   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)2, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
   3));


   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)1, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
   3));
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
   int rd= (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1];
   #ifdef as5600DirPinHigh
   global.rotorVal = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
      + as5600CalibratedOffset;
   #else
   global.rotorVal = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
      + as5600CalibratedOffset;
   #endif
   
   int newBNumber = (int)((global.rotorVal * SECTOR_PER_BITS)+2*dir)%6; //0- bitsPerSector --> smaller sector
   global.oldSectorTarget = newBNumber; 
   global.sectorTarget = newBNumber;
   ESP_LOGI(cyan "\nFirstPotRead", "ost, nst: (%d, %d), raw: %d, rotorAng: %d, bl# %d\n", global.oldSectorTarget, global.sectorTarget, rd, global.rotorVal, newBNumber);
   
   xTaskCreatePinnedToCore(getSectorNumber, "SETUP", 8000, NULL,  uxTaskPriorityGet(setupTask)+1  /*priority*/, &getSectorNumberTask, 1);  /*PIN TO SAME CORE AS CREATOR, and SMALER PRIORITY*/
   // xTaskCreatePinnedToCore(getSectorNumber, "SETUP", 8000, NULL, uxTaskPriorityGet(setupTask)-1 /*priority*/, &getSectorNumberTask, 1);
   // vTaskSuspend(getSectorNumberTask);
   // vTaskPrioritySet(getSectorNumberTask, uxTaskPriorityGet(setupTask)+1 );
}

void initAnalogReadOnce(){
   adc_oneshot_unit_init_cfg_t adcSetup= {
      .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  adc_oneshot_chan_cfg_t adcChannelSetup = {
    .atten =  ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
}
//run pwm at f ~40-50kHz for adjustable torque control




mcpwm_int_clr_reg_t tempClearR1 = { 
   .timer0_tez_int_clr =1,
};
mcpwm_int_clr_reg_t tempClearR2 = { 
   .timer1_tez_int_clr =1,
   .timer1_tep_int_clr =1,
   .op0_tea_int_clr = 1,
   .op0_teb_int_clr = 1
};
mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };
#if (lowSideGroup == 1)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM1)
#elif (lowSideGroup == 0)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM0)
#endif

void IRAM_ATTR runOnMCPWMIntr(void * returnValue) {
   tempStatusReg.val =  (MCPWMx)->int_st.val;
   if(tempStatusReg.val){ //in case of ghost interrupts

      // getTimerCountNow("@");
      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
      //120-170 gpt µs at 400kHz
      //as5600 is default increasing on clockwise. set DIR high to invert 

      #ifdef debug_fastPrints
         esp_rom_printf(blue "B");
      #endif

      xTaskResumeFromISR(getSectorNumberTask);
      MCPWMx-> int_clr.val = tempClearR1.val;
      // #ifdef debug_testOnLED
         
      // esp_rom_delay_us(150);
      // if (motorStall){
      //    global.oldSectorTarget= global.sectorTarget;
      // } else{
      //    global.oldSectorTarget = global.sectorTarget;
      //    global.sectorTarget = mod6(global.sectorTarget+1);
      // }
      
      // preloadGates(global.oldSectorTarget,global.sectorTarget, global.blockPeriod);
      // #else
      // xTaskResumeFromISR(getSectorNumberTask);
      // MCPWMx-> int_clr.val = tempClearR1.val;
      // #endif
      return;
   } else if(
      /*// ltimer has id 1*/
      tempStatusReg.timer1_tez_int_st || //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
      tempStatusReg.timer1_tep_int_st || //timer 1= LTimer, (ie change from block 0-1), 2^7 = 128
      /*since phaseA_gen_one_third =0 */
      tempStatusReg.op0_tea_int_st || // op0 = phase A lowside, (ie change from block 5-0 or 1-2), 2^15 = 32765
      tempStatusReg.op0_teb_int_st) // timer, (ie change from block 2-3 or 4-5), 2^18 = 262144
      { //L TIMER = id1, SO WE USE TIMER 1
         
         /*instant DBUG*/
         #ifdef debug_fastPrints
         if(tempStatusReg.timer1_tez_int_st) esp_rom_printf(magenta "|TEZ");
         if(tempStatusReg.timer1_tep_int_st) esp_rom_printf(magenta "|TEP");
         if(tempStatusReg.op0_tea_int_st)    esp_rom_printf(magenta "|TEA");
         if(tempStatusReg.op0_teb_int_st)    esp_rom_printf(magenta "|TEB");
         esp_rom_printf(white "| b# ^ %d, n# %d", global.oldSectorTarget, global.sectorTarget);
         esp_rom_printf( red "|L_ \n");
         #endif

         // esp_rom_printf(white "i2 BL 14, lvl: %d\n",gpio_get_level(digitalReadPin));
         // esp_rom_printf(green "s %d, %d",global.oldSectorTarget , global.sectorTarget );
         
         // 32768, 128, 32768, 262144, 16, 262144
         executeGates(&tempClearR2, MCPWMx);
         return;
      }
   }
}



void getSectorNumber(void *returnValue){
   while(1){
      vTaskSuspend(NULL);
      // esp_rom_printf(yellow", gsn");
      // /*TIMETHETIMER ttt*/int t1= esp_timer_get_time(); 

      #ifdef debug_testOnLED
       esp_rom_delay_us(110);
      if (motorStall){
         global.oldSectorTarget= global.sectorTarget;
      } else{
         global.oldSectorTarget = global.sectorTarget;
         global.sectorTarget = mod6(global.sectorTarget+1);
      }
      #else
      ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         &as5600TargetRegister, 
         as5600WriteSize,
         as5600RawDataBuf, 
         as5600ReadSize, //ensure 2 bytes is read
         4
      )
   );
   // ESP_ERROR_CHECK(i2c_master_receive(as5600Handle, as5600RawDataBuf, as5600ReadSize,1));

   int rd= (as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1];
   #ifdef as5600DirPinHigh
      global.rotorVal = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
      + as5600CalibratedOffset;
   #else
      global.rotorVal = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
      + as5600CalibratedOffset;
   #endif 
   int newBNumber = static_cast<uint32_t>((global.rotorVal * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
   if((global.sectorTarget >= 0) && (std::abs(newBNumber - global.sectorTarget)>1) && (std::abs(newBNumber - global.sectorTarget)) != 5){
      ESP_LOGE("POTENTIOMETER READ",": Sector 1+ jump . Previous Sector: %2d. Incoming Sector: %2d, raw%d, blN: %d", global.sectorTarget, newBNumber, rd, newBNumber);
      // ESP_LOGI(cyan "\n INVALID POT ReAD", "ost, nst: (%d, %d), raw: %d, rotorAng: %d, bl# %d\n", global.oldSectorTarget, global.sectorTarget, rd, global.rotorVal, newBNumber);
      vTaskDelay(pdMS_TO_TICKS(1));
      abort();
   }
   global.oldSectorTarget = global.sectorTarget;
   global.sectorTarget = newBNumber;
#endif

   preloadGates(global.oldSectorTarget,global.sectorTarget, global.blockPeriod);
   // t1= esp_timer_get_time() - t1;esp_rom_printf("t1: %d\n", t1);
   }
}


// mcpwm_comparator_set_comare_value()
// mcpwm_timer_set_period()
// threadsafe
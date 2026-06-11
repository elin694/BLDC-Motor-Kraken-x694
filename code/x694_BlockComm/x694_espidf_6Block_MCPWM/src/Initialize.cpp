#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

void initialize(void * parameter){
   for(int i = 0; i<4; i++){
      global.CMR_value_3[i] = global.blockPeriod*i;
      ESP_LOGI(blue "C_3 thirds", "%f", global.CMR_value_3[i]);
   }
   global.BTimerPhaseShift= global.blockPeriod-(estimatedI2CReadTimeInMicros*µsToTicks);
   gpio_reset_pin(digitalReadPin);
   gpio_set_direction(digitalReadPin, GPIO_MODE_INPUT);
   gpio_set_pull_mode(digitalReadPin, GPIO_FLOATING);
   // pinSetup();
   initAnalogReadOnce();
   // readPotOnce(NULL);
   // ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   // ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   // as5600initialize();
   global.sectorTarget = preCompStartingTargetSector;
   global.oldSectorTarget = global.sectorTarget;
   
   ESP_LOGI(cyan "STARTING", " MCPWM SETUP");
   // vTaskDelay(pdMS_TO_TICKS(1000));
   mcpwmSetup(global.sectorTarget, &global.blockPeriod);
    //blockPeriod has to be bigger than estimatedI2CReadTimeInMicros*µsToTicksInt
   /*no bidirection compatability yet
    pull Low high to prime Bootstrap cap?  */
   // xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 10000, NULL, 2, NULL, 0);
   xTaskCreatePinnedToCore(spamSearchCV, "spamSearchCV", 5047, NULL, 2, NULL, 1);
   // xTaskCreatePinnedToCore(debugLog, "debugLog", 10000, NULL, 2, NULL, 0);
   // vTaskDelete(NULL);
   for(;;){
      vTaskDelay(pdMS_TO_TICKS(100000));
   }
}
int mod6 (int value){ //for single add
    if(value > 5){
        value = 0;
    } else if(value < 0){
        value = 5;
    }
    return value;
}
i2c_master_bus_config_t busSetup = { 
   .i2c_port = -1,
   .sda_io_num= dataPin,
   .scl_io_num= clockPin,
   .clk_source = I2C_CLK_SRC_APB,
   .glitch_ignore_cnt = 7,
   // .intr_priority = 1,
   .flags={.enable_internal_pullup = true}
};
i2c_master_bus_handle_t busHandle;

i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= 390000, //need fast enough  to avoid invalid state
   .scl_wait_us = 30,
   .flags = {.disable_ack_check = false}
};
i2c_master_dev_handle_t as5600Handle;

void pinSetup(){
   ESP_LOGI(yellow "Pin Setip", "Set Begun ");
   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   ESP_LOGI(yellow "Pin Setupp", "Clock and Data pin reset ");
   // use ledc to set potentionmeter to input analog read
   for(int i = 0; i<6; i++){
      gpio_reset_pin(gateArray[i]);
      // gpio_set_direction(gateArray[i],GPIO_MODE_OUTPUT);
      gpio_set_direction(gateArray[i], GPIO_MODE_INPUT_OUTPUT);
      ESP_LOGW("pintSetup", "i: %d, lvl: %d",i,gpio_get_level(gateArray[i]));
      gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
   }
   // esp_rom_delay_us(6000000); IT ORKS
   ESP_LOGI(yellow "Pin Setup", " each pin is reset, set to output and floating");
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
   fthRegister[1]= (fthRegisterData[0] & 0b11000000) | fth_sf_set_mask;
   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
      fthRegister, //address to start on
      (size_t)2, //write 1 byte's woth from fthRegister
      fthRegisterData, //where to save the read data
      (size_t)1, //read 1 byte
   3));
   ESP_LOGI("As5600 Initialized", "FTH register: %s, SF registers set: %s ",
      std::bitset<3>((fthRegisterData[0] & 0b00011100) >> 2).to_string().c_str(),
      std::bitset<2>(fthRegisterData[0] & 0b00000011).to_string().c_str()
   );
   //===================================GET A STARTING SECTOR VALUE ===================================

   ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         &as5600TargetRegister, 
         as5600WriteSize,
         as5600RawDataBuf, 
         as5600ReadSize, //ensure 2 bytes is read
         -1));
      #ifdef as5600DirPinHigh
         uint32_t rotorAngle = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
         + as5600CalibratedOffset;
      #else
         uint32_t rotorAngle = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
         + as5600CalibratedOffset;
      #endif

      int newBNumber = 
         mod6(mod6(rotorAngle * SECTOR_PER_BITS+dir)+dir); //0- bitsPerSector --> smaller sector
      if((std::abs(newBNumber - global.sectorTarget)>1) && (std::abs(newBNumber - global.sectorTarget)) != 5){
         ESP_LOGE("POTENTIOMETER READ",": Sector jumped by more  than 1. Previous Sector: %2d. Incoming Sector: %2d", global.sectorTarget, newBNumber);
         vTaskDelay(pdMS_TO_TICKS(2000)); 
         abort();
      }
      //transfer old target
      global.oldSectorTarget = newBNumber; 
      //put in new vlaue
      global.sectorTarget = newBNumber;
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

void IRAM_ATTR getSectorNumber(void * returnValue) { 
   tempStatusReg.val =  (MCPWMx)->int_st.val;
   if(tempStatusReg.val){ //in case of ghost interrupts
      // getTimerCountNow("isr: ");
      // uint32_t az= isrGroupCounter;
      // az= az +1;
      // isrGroupCounter = az;

      // esp_rom_printf(cyan "\n R %6d, ", tempStatusReg.val);

      if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
      //120-170 gpt µs at 400kHz
      //as5600 is default increasing on clockwise. set DIR high to invert 
         uint32_t a= counter;
         a= a +1;
         counter = a;
         // esp_rom_printf( yellow "B, %d", (int) esp_timer_get_time());
         esp_rom_printf(blue "B");
         // getTimerCountNow("      ");
         // ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         //    &as5600TargetRegister, 
         //    as5600WriteSize,
         //    as5600RawDataBuf, 
         //    as5600ReadSize, //ensure 2 bytes is read
         //    -1));
         // #ifdef as5600DirPinHigh
         //    uint32_t rotorAngle = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
         //    + as5600CalibratedOffset;
         // #else
         //    uint32_t rotorAngle = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
         //    + as5600CalibratedOffset;
         // #endif

         // int newBNumber = static_cast<uint32_t>((rotorAngle * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
         // if((global.sectorTarget >= 0) && (std::abs(newBNumber - global.sectorTarget)>1) && (std::abs(newBNumber - global.sectorTarget)) != 5){
         //    ESP_LOGE("POTENTIOMETER READ",": Sector jumped by more  than 1. Previous Sector: %2d. Incoming Sector: %2d", global.sectorTarget, newBNumber);
         //    abort();
         // }
         
         //transfer old position and put in new vlaue
         #ifdef motorStall
            int newBNumber = preCompStartingTargetSector; //comment out to mimic motor stalling
            global.sectorTarget = newBNumber; //comment out to mimic motor stalling
            global.oldSectorTarget = newBNumber; //comment out to mimic motor stalling
            
         #else
         int newBNumber = (global.sectorTarget+1)%6; //comment out to mimic motor stalling
         global.oldSectorTarget = global.sectorTarget;
         global.sectorTarget = newBNumber; //
         #endif
         // esp_rom_printf(green "OST %d, NST %d",global.oldSectorTarget , global.sectorTarget );
         preloadGates(global.oldSectorTarget,global.sectorTarget, global.blockPeriod, MCPWMx, tempClearR1.val);
      } 
      else if(
         /*// ltimer has id 1*/
         tempStatusReg.timer1_tez_int_st || //timer 1= LTimer, (ie change from block 3-4), 2^4 = 16
         tempStatusReg.timer1_tep_int_st || //timer 1= LTimer, (ie change from block 0-1), 2^7 = 128
         /*since phaseA_gen_one_third =0 */
         tempStatusReg.op0_tea_int_st || // op0 = phase A lowside, (ie change from block 5-0 or 1-2), 2^15 = 32765
         tempStatusReg.op0_teb_int_st) // timer, (ie change from block 2-3 or 4-5), 2^18 = 262144
      { //L TIMER = id1, SO WE USE TIMER 1
         // uint32_t azz= isrCounter2;
         // azz= azz +1;
         // isrCounter2 = azz;   
         
         if(tempStatusReg.timer1_tez_int_st) esp_rom_printf(magenta "TEZ\n");
         if(tempStatusReg.timer1_tep_int_st) esp_rom_printf(magenta "TEP\n");
         if(tempStatusReg.op0_tea_int_st)    esp_rom_printf(magenta "TEA\n");
         if(tempStatusReg.op0_teb_int_st)    esp_rom_printf(magenta "TEB\n");

         // esp_rom_printf(white "i2 BL 14, lvl: %d\n",gpio_get_level(digitalReadPin));
         esp_rom_printf( red "L");
         // esp_rom_printf(green "s %d, %d",global.oldSectorTarget , global.sectorTarget );
         
         // 32768, 128, 32768, 262144, 16, 262144
         executeGates(&tempClearR2, MCPWMx);
      }
   }
}

// mcpwm_comparator_set_comare_value()
// mcpwm_timer_set_period()
// threadsafe
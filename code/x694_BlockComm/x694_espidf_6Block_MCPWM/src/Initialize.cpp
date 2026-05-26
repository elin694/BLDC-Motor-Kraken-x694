#include "Initialize.h"
#include "GateControl.h"
#include <bitset> 

void initialize(){
   for(int i = 0; i<4; i++){
      global.CMR_value_3[i] = global.blockPeriod*(float)i/6.0f;
   }
   global.BTimerPhaseShift= global.blockPeriod-(estimatedI2CReadTimeInMicros*µsToTicks);
   pinSetup();
   initAnalogReadOnce();
   // readPotOnce(NULL);
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   void as5600initialize();
   getSectorNumber((void *)(&global));
   global.oldSectorTarget = global.sectorTarget;
   mcpwmSetup((global.sectorTarget + 2*dir) % 6, &global.blockPeriod);
   //no bidirection compatability yet
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
   printf("Setup Begun \n ");
   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   // use ledc to set potentionmeter to input analog read
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(gate, GPIO_FLOATING);
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

#define SECTOR_PER_BITS static_cast<float>(1 / (4096.0f / (electricalCycles* 6.0f)))
void IRAM_ATTR getSectorNumber(void * returnValue) { //120µs at 400kHz
   //as5600 is default increasing on clockwise.
   //set DIR high to invert 
   #if (lowSideGroup == 1)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM1)
   #elif (lowSideGroup == 0)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM0)
   #endif
   mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };

   if(tempStatusReg.timer0_tez_int_st){ //TIMER ID 0 IS BTIMER, TIMER ID 1 IS  LTIMER
      ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
         &as5600TargetRegister, 
         as5600WriteSize,
         as5600RawDataBuf, 
         as5600ReadSize, //ensure 2 bytes is read
         3));
      #ifdef as5600DirPinHigh
         uint32_t rotorAngle = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
         + as5600CalibratedOffset;
      #else
         uint32_t rotorAngle = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
         + as5600CalibratedOffset;
      #endif

      int newBNumber = static_cast<uint32_t>((rotorAngle * SECTOR_PER_BITS)+2*dir) % 6; //0- bitsPerSector --> smaller sector
      if((global.sectorTarget >= 0) && (std::abs(newBNumber - global.sectorTarget)>1) && (std::abs(newBNumber - global.sectorTarget)) != 5){
         ESP_LOGE("POTENTIOMETER READ",": Sector jumped by more  than 1. Previous Sector: %2d. Incoming Sector: %2d", global.sectorTarget, newBNumber);
         vTaskDelay(pdMS_TO_TICKS(2000)); 
         abort();
      }
      //transfer old position
      global.oldSectorTarget = global.sectorTarget; 
      //put in new vlaue
      global.sectorTarget = newBNumber;
      // ((gVar_t *) returnValue)-> sectorTarget = (volatile uint32_t) newBNumber;
      
      //PRELAOD
      
      mcpwm_int_clr_reg_t tempClearReg = { .val = 0b0};
      tempClearReg.timer0_tez_int_clr = 1;
      (MCPWMx)->int_clr.val == tempClearReg.val;
   }
}

// mcpwm_comparator_set_compare_value()
// mcpwm_timer_set_period()
// threadsafe
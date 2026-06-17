#pragma once
#define as5600Address 0x36
#include "Globals.h"
#include "driver/i2c_master.h"

void pinSetup();
void initialize(void *parameter);      
void initializeGPIO();

void as5600initialize();
void initAnalogReadOnce();

void runOnMCPWMIntr(void *returnValue);
void getSectorNumber(void *returnValue);
void debugLog(void * parameter);
int mod6(int value);

#if (lowSideGroup == 1)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM1)
#elif (lowSideGroup == 0)
   #define MCPWMx ((mcpwm_dev_t * )&MCPWM0)
#endif
inline mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };
//+++++++++++++++++++++++++++++++++++I2C+++++++++++++++++++++++++++++++++++
inline i2c_master_bus_config_t busSetup = { 
    .i2c_port = -1,
    .sda_io_num= dataPin,
    .scl_io_num= clockPin,
    .clk_source = I2C_CLK_SRC_APB,
    // .glitch_ignore_cnt = 7,
    // .intr_priority = 1,
    .flags={.enable_internal_pullup = true}
};
inline i2c_master_bus_handle_t busHandle;

constexpr i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= 400000, //need fast enough  to avoid invalid state
   .scl_wait_us = 30,
   .flags = {.disable_ack_check = false}
};
inline i2c_master_dev_handle_t as5600Handle;

constexpr DRAM_ATTR uint8_t as5600Set = 0x36;
constexpr DRAM_ATTR uint8_t as5600TargetRegister = 0x0e;
constexpr DRAM_ATTR size_t as5600WriteSize = 1;
inline uint8_t as5600RawDataBuf[2];
constexpr size_t as5600ReadSize = 2;
// #define as5600DirPinHigh

constexpr adc_oneshot_unit_init_cfg_t adcSetup= {
   .unit_id = ADC_UNIT_1,
   .ulp_mode = ADC_ULP_MODE_DISABLE,
};
constexpr adc_oneshot_chan_cfg_t adcChannelSetup = {
   .atten =  ADC_ATTEN_DB_12,
   .bitwidth = ADC_BITWIDTH_12,
};
DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR1 = { 
   .timer0_tez_int_clr =1,
};
DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR2 = { 
   .timer1_tez_int_clr =1,
   .timer1_tep_int_clr =1,
   .op0_tea_int_clr = 1,
   .op0_teb_int_clr = 1
};

//======================================================
// static void initAnalogReadContinuous(void *parameter){
//   // Initialize the ADC Continuous Mode Driver
//   adc_continuous_handle_cfg_t dmaHandleSetup = {
//     .max_store_buf_size = 1024,
//     .conv_frame_size= 128,
//   };
//   ESP_ERROR_CHECK( adc_continuous_new_handle(&dmaHandleSetup,&dmaHandle) );
//   //adc io pin config 
//   // *dereferencing - get thing at this addr
//   // & referncing give addr of thing
//   adc_digi_pattern_config_t dmaChannelSetup = {
//     .atten = ADC_ATTEN_DB_12,
//     .channel = adcChannel,
//     .unit = ADC_UNIT_1,
//     .bit_width = ADC_BITWIDTH_12,
//     // .bit_width = ADC_BITWIDTH_12,
//   };
//   adc_continuous_config_t dmaSetup = {
//     .pattern_num = 1,
//     .adc_pattern = &dmaChannelSetup,
//     .sample_freq_hz = 20000,
//     .conv_mode = ADC_CONV_SINGLE_UNIT_1,
//     .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1, //12 bit
//   };
//   ESP_ERROR_CHECK(adc_continuous_config(dmaHandle, &dmaSetup)); //hndle, config

//   adc_continuous_evt_cbs_t dmaCallbackSetup = {
//     .on_conv_done = finishedConversionResult, // pass function
//     .on_pool_ovf = NULL,
//   };
//   ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(dmaHandle,&dmaCallbackSetup,NULL));
// }
//
//   static bool IRAM_ATTR finishedConversionResult(
//     adc_continuous_handle_t handle,
//     const adc_continuous_evt_data_t *edata,
//     void *user_data){
//     return true;
//   }
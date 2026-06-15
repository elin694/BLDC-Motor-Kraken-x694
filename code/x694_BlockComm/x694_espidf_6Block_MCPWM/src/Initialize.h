#pragma once
#define as5600Address 0x36
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

//+++++++++++++++++++++++++++++++++++I2C+++++++++++++++++++++++++++++++++++
extern i2c_master_bus_config_t busSetup;
extern i2c_master_bus_handle_t busHandle;
extern i2c_device_config_t as5600Setup;
extern i2c_master_dev_handle_t as5600Handle;

constexpr uint8_t as5600Set = 0x36;
constexpr uint8_t as5600TargetRegister = 0x0e;
constexpr size_t as5600WriteSize = 1;
inline uint8_t as5600RawDataBuf[2];
constexpr size_t as5600ReadSize = 2;
// #define as5600DirPinHigh


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
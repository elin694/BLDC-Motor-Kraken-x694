#ifndef GLOBALS_H
#include "globals.h"
#endif

void initialize();      
void initializeGPIO();
void switchBlock(int phase);
void initAnalogReadOnce(void * parameter);
uint8_t getSectorNumber();

  //Direction A
  // Phase:       A---B---C
  // block 1 =    L---H---N
  // block 2 =   N---H---L
  // block 3 =   H---N---L
  // block 4 =   H---L---N
  // block 5 =   N---L---H
  // block 6 =   L---N---H
// int steps[6][3] = {
//   {0,1,-1},
//   {-1,1,0},
//   {1,-1,0},
//   {1,0,-1},
//   {-1,0,1},
//   {0,-1,1},
// };// abc clockwise

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
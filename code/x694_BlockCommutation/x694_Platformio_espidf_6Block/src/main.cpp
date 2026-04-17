//============================ 6 step commutation! with ESPIDF ============================
#ifndef GLOBALS_H
#include "globals.h"
#endif
#include "initialize.h"


//https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
/* Using a potentiometer hooked up to 5V to control a 3 phase DC motor with Arduino Uno and sinusoidal control.*/
//espidf onshot vs continuous adc translation ; https://randomnerdtutorials.com/esp-idf-esp32-gpio-analog-adc/
//============================Initializing values ============================zxq`
// #define byTime //define the ontime by period, rather than frequency
// #define onTimeRatio 90 
#ifdef byTime
  //millisecond, per rotation
  const int periodMin =80;
  const int periodMax = 380;
#else
  //rot per second that is determined by potentiometer
  const double fMin = 1.3;
  const double fMax = 14;
#endif
const int electricalCycles =3;
const long printPeriod = 2e5;
uint64_t lastTime = 0;
uint32_t val =0; //how long to delay every phase
uint32_t onTime =0; 
uint32_t deadTime =0; 
int blockNumber = 0;
// adc_continuous_handle_t dmaHandle;
adc_oneshot_unit_handle_t adcHandle;
uint8_t potBuffer[128];
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
int steps[6][3] = {
    {1,-1,0},
    {-1,1,0},
    {0,1,-1},
    {0,-1,1},
    {-1,0,1},
    {1,0,-1},
};
//========================FUNCTION DECLARATIONS========================
void switchBlock(int phase, bool turnOn);

//==================================LOOP=====================================
  static bool IRAM_ATTR finishedConversionResult(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata,
    void *user_data){
    return true;
  }
  
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

static void initAnalogReadOnce(void *parameter){
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

void loop(void * parameter) {
  //  Serial.print("Phase A on (Green), ");
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for(;;){
    for (int phase = 0; phase <3; phase++){
      switchBlock(phase, false); //turn off first
      //aplies state to each block
    }
    #ifdef onTimeRatio
    if(deadTime>=1000){
      xTaskDelayUntil(&xLastWakeTime,(pdMS_TO_TICKS(deadTime/1000))); 
    }else{
      ets_delay_us(deadTime);
    }
    for (int phase = 0; phase <3; phase++){
      switchBlock(phase,true); //turn off first
    }
    #endif
    if(onTime>=1000){
      xTaskDelayUntil(&xLastWakeTime,(pdMS_TO_TICKS(onTime/1000))); 
    }else{
      ets_delay_us(onTime);
    }
    uint32_t ret= 0;
    blockNumber = (blockNumber +1)%6;
    if(esp_timer_get_time()-lastTime > printPeriod){
      // adc_adc ESP_ERROR_CHECK(adc_continuous_read(dmaHandle,potBuffer, sizeof(potBuffer), &ret,100 ));
      //adc_adc int raw_data  = ((adc_digi_output_data_t*)potBuffer )-> type1.data;
      int raw_data = 0;
      ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &raw_data));
      double rawValueNormalized = raw_data/4096.0;
      // rawValueNormalized= 0;
      printf("rawValueNormalized: %6.4f, ",rawValueNormalized);
      
      #ifndef byTime
        val = 1000000.0/((fMin+     (fMax-fMin)*std::sqrt(rawValueNormalized)) 
          // linear scaling frequency with respect to potentiometer reading
          *electricalCycles*6); //3 is for the pole pair count per rotation
          // printf("diff*nomral : %f, ",(fMax-fMin)*rawValueNormalized);
          //turning towards negative--> longer delay for value --> slower spins
      #else
        //turning towards negative--> smaller delay for value --> faster spins
        val=(rawValueNormalized*1000.0*(periodMax-periodMin)+1000.0*periodMin)/
        // linear scaling period with respect to potentiometer reading
        (electricalCycles*6); //3 is for the pole pair count per rotation
      #endif  
        
      #ifdef onTimeRatio
        onTime= static_cast<float>(onTimeRatio/100)*val; 
      #else
        onTime = val;
      #endif
        // printf("time delay/phase: %7.3f ms, RPM: %i, ",
      printf("RPM: %i, ", static_cast<int>(60.0*1e6/val/6.0/electricalCycles));
      //   onTime/1000.0,  
      //   static_cast<int>(60.0*1e6/onTime/3.0/eletricalCycles));
      //  printf("mu s pause time/phase: %7.3f, ", val);
      lastTime = esp_timer_get_time();
      double f_pwm;
      #ifdef byTime  
        f_pwm = 1000000.0/val; 
      #else
        f_pwm = 1.0/(3*val)*1000000; 
      #endif
      printf("f_pwm: %5.2f Hz \n", f_pwm);
      //  printf("µs : %lld, val: %lu \n", lastTime,val);
      printf("µs : %lld, blockNum: %i \n \n", lastTime,blockNumber);
      //val = map(analogRead(pot), 0, 1023, periodMin, periodMax);
    }
  }
}

extern "C"{
  void app_main(){
    //========================SETUP========================

    initialize();
    // initAnalogReadContinuous(NULL);
    // ESP_ERROR_CHECK(adc_continuous_start(dmaHandle));
    initAnalogReadOnce(NULL);
    xTaskCreate(loop,"loop", 32768, NULL, 1 , NULL);
  }
}

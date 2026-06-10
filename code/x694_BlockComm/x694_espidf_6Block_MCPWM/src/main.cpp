//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
float RPS= 0 ;
int rawData = 0;
portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

void spamSearchCV(void *parameter){
  for(;;){
    getTimerCountNow("\n -");
    vTaskDelay(pdMS_TO_TICKS(preComp_cvPeriod));
  }
}

void readPotRepeat(void * parameter){
  for(;;){
    readPotOnce(parameter);
    vTaskDelay(pdMS_TO_TICKS(70)); 
  }
}
void readPotOnce(void * parameter){
   rawData = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    RPS = (fMin+(fMax-fMin)*sqrtf((float)rawData/4096.0f));
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spin

    //This bottom part needs to be instantaneous assignment
    uint32_t bPeriod_temp= (uint32_t)((float)timerResolution/(float)(RPS *electricalCycles*6));
    // * µsToTicks; //mcpwm timer icks per block when spinnig
    float cmr_dividers_3_1 = (float)global.blockPeriod/3.0f;
    float cmr_dividers_3_2 = 2 * cmr_dividers_3_1;
    
    // ESP_LOGW("redPotonce", "bPeriod_temp :%" PRIu32 ", RPS: %f, rawData: %d", bPeriod_temp, RPS, rawData);
    taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
    if(global.blockPeriod != bPeriod_temp){
      newFrequency =true;
      //ISR checks this constatnly. If true, it runs code to update the CMRA trheshold.
      // might be useless
      ESP_ERROR_CHECK(mcpwm_timer_set_period(blockTimer, bPeriod_temp));
      ESP_ERROR_CHECK(mcpwm_timer_set_period(globalLowTimer, 6*bPeriod_temp));
    }
    global.blockPeriod = bPeriod_temp;
    global.CMR_value_3[1] = cmr_dividers_3_1;
    global.CMR_value_3[2] = cmr_dividers_3_2;
    taskEXIT_CRITICAL(&stepPeriodMux);
    //isr loop needs to keep checking
    // ESP_LOGI("readPotOnce", magenta "read pot once");
}
    int c1 =0;
    int c2 =0;
    int c3 =0;
void debugLog(void * parameter){
  for(;;){
    ESP_LOGI("\n REPORT STATUS",":pot%: %6.4f, RPS: %5.2f",(float)rawData/4096.0f, RPS);
    // taskENTER_CRITICAL(&counterMux); //300ns for enter and exit
    c1 =counter;
    c2 = isrCounter2;
    c3 =isrGroupCounter;
    // taskEXIT_CRITICAL(&counterMux); //300ns for enter and exit
    ESP_LOGI("Number of","BTimer intr:%7d, LTimer intr: %7d, #intr trigger: %7d ",c1, c2, c3);
    getTimerCountNow("");
    
    vTaskDelay(pdMS_TO_TICKS(10*143)); 
  }
}
      
extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 40000, NULL, 2, NULL, 1);
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
    // xTaskCreatePinnedToCore(run6Block, "run6Block", 16384, NULL, 4, NULL, 1);
    
  }
}
//attach block timer frequency to potentionmeter
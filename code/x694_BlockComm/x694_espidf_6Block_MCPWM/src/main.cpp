//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
const double fMin = 15; //in hertz
const double fMax = 17;

adc_oneshot_unit_handle_t adcHandle;
uint32_t potBuffer[128];
float RPS= 0 ;
int rawData = 0;
portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

// void run6Block(void * parameter) { 
//   TickType_t xLastWakeTime = xTaskGetTickCount();
//   for(;;){
//     getSectorNumber((void *)&global); //optimize ot remove modulo***********************
//     // phaseSwitching(blockNumber, (blockNumber + 2*dir) % 6);
//     taskYIELD();
//     }
// }

void readPotRepeat(void * parameter){
  for(;;){
    readPotOnce(parameter);
    vTaskDelay(pdMS_TO_TICKS(70)); 
  }
}
void readPotOnce(void * parameter){
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
      ESP_ERROR_CHECK(mcpwm_timer_set_period(globalLowTimer, bPeriod_temp));
    }
    global.blockPeriod = bPeriod_temp;
    global.CMR_value_3[1] = cmr_dividers_3_1;
    global.CMR_value_3[2] = cmr_dividers_3_2;
    taskEXIT_CRITICAL(&stepPeriodMux);
    //isr loop needs to keep checking
}

    uint32_t c1 =0;
    uint32_t c2 =0;
    uint32_t c3 =0;
void debugLog(void * parameter){
  for(;;){
    ESP_LOGI("REPORT STATUS",":pot%: %6.4f, RPS: %5.2f",(float)rawData/4096.0f, RPS);
    taskENTER_CRITICAL(&counterMux); //300ns for enter and exit
    c1 =counter;
    c2 = isrCounter2;
    c1 =isrGroupCounter;
    taskEXIT_CRITICAL(&counterMux); //300ns for enter and exit
    ESP_LOGI("Checkpoint","Is the counter Moving? counter:%" PRIu32 ", isrCouner2: %" PRIu32 ", isrGCouner: %" PRIu32 "\n", c1,c2,c3);
    vTaskDelay(pdMS_TO_TICKS(3*143)); 
  }
}
      
extern "C"{
  void app_main(){
    initialize(); //setup
    // xTaskCreatePinnedToCore(run6Block, "run6Block", 16384, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(readPotRepeat, "readPotRepeat", 10000, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(debugLog, "debugLog", 10000, NULL, 1, NULL, 1);
    //pull Low high to prime Bootstrap cap?
  }
}
//attach block timer frequency to potentionmeter
//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
float RPS= 0 ;
int rawData = 0;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

void spamSearchCV(void *parameter){
  for(;;){
    #ifdef digitalReadPin
      esp_rom_printf(yellow "%d",gpio_get_level(digitalReadPin));
    #endif
    #ifdef debugWithCounterStatus
      getTimerCountNow("");
      #endif
      vTaskDelay(pdMS_TO_TICKS(preComp_cvPeriod));
    }
  }
  
void debugLog(void * parameter){
  for(;;){
    #ifdef debug
    // taskENTER_CRITICAL(&counterMux); //300ns for enter and exit
    // taskEXIT_CRITICAL(&counterMux); //300ns for enter and exit
    ESP_LOGI("\n REPORT STATUS",":pot%: %6.4f, RPS: %5.2f",(float)rawData/4096.0f, RPS);
    ESP_LOGI("Number of","BTimer intr:%7d, LTimer intr: %7d, #intr trigger: %7d ",c1, c2, c3);
    #else
    motorStall = !motorStall;
    ESP_LOGW("mtrStl=","%d",motorStall);
    #endif
    vTaskDelay(pdMS_TO_TICKS(debugPeriodicity)); 
    // vTaskDelay(pdMS_TO_TICKS(10*143)); 
  }
}
  void readPotRepeat(void * parameter){
    for(;;){
      readPotOnce(parameter);
      vTaskDelay(pdMS_TO_TICKS(potReadPeriod)); 
    }
}

// const int lookUpTable[] = {
//   10000,
//   200,300,400,500,600,700,800,900,
//   1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
//   9000, 8000, 7000,6000,5000,4000,3000,2000, 1000,
//   4952,10340, 5000, 8422, 9123,5832, 2127,9321,8351,
//   7636,4241,4236,9463,8151,8230,3631,8589,6752,9638,1441,6509,4443,8043,2422,
//   3579,6587,6323,9214,9634,1553,7038,7477,10169,2918,5137,8707,9776,4325,4704,
//   6780,9152,5096,6334,10240,1409,8543,6087,3513,6028
// };
int lookUpTableIndex = 0;

void readPotOnce(void * parameter){
   rawData = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    RPS = (fMin+(fMax-fMin)*sqrtf((float)rawData/4096.0f));
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spin

    //This bottom part needs to be instantaneous assignment
    uint32_t bPeriod_temp= (uint32_t)((float)timerResolution/(float)(RPS *electricalCycles*6));
    bPeriod_temp = 10000;
    // bPeriod_temp = lookUpTable[lookUpTableIndex];
    // lookUpTableIndex++;

    ESP_LOGE("potRedaNewBP", "%d, gbp %d\n", (int )bPeriod_temp, (int)global.blockPeriod);
    // * µsToTicks; //mcpwm timer icks per block when spinnig
    float cmr_dividers_3_1 = (float)global.blockPeriod/3.0f;
    float cmr_dividers_3_2 = 2 * cmr_dividers_3_1;
    
    // ESP_LOGW("redPotonce", "bPeriod_temp :%" PRIu32 ", RPS: %f, rawData: %d", bPeriod_temp, RPS, rawData);
    if(global.blockPeriod != bPeriod_temp){
    taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
    global.blockPeriod = bPeriod_temp;
      global.newPotValue =true;
      /*ISR checks this constatnly. If true, it runs code to update the CMRA trheshold. might be useless */

      // ESP_ERROR_CHECK(mcpwm_timer_set_period(blockTimer, bPeriod_temp));
      // ESP_ERROR_CHECK(mcpwm_timer_set_period(globalLowTimer, 6*bPeriod_temp));
      taskEXIT_CRITICAL(&stepPeriodMux);
    }
    // global.CMR_value_3[1] = cmr_dividers_3_1;
    // global.CMR_value_3[2] = cmr_dividers_3_2;
    //isr loop needs to keep checking
    // ESP_LOGI("readPotOnce", magenta "read pot once");
}
    int c1 =0;
    int c2 =0;
    int c3 =0;
      
extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 40000, NULL, 2, NULL, 1);
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
    // xTaskCreatePinnedToCore(run6Block, "run6Block", 16384, NULL, 4, NULL, 1);
    
  }
}
//attach block timer frequency to potentionmeter
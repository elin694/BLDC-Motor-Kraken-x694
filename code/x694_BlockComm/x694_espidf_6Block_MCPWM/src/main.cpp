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
  #ifdef debug_spamDelay
  for(;;){
    #ifdef debug_spamPrintBlockStatus
      esp_rom_printf(blue "\nb#(%d, %d, %d)|", global.oldSectorTarget, global.sectorTarget, global.rotorVal);
    #endif
    #ifdef debug_spamPrintCounterStatus
      getTimerCountNow("");
    #endif
    
      vTaskDelay(pdMS_TO_TICKS(debug_spamDelay));
  }
  #endif
}
  
void debugLog(void * parameter){
  int tracker = 0;
  for(;;){
    #ifdef debug_printRPS
    // taskENTER_CRITICAL(&counterMux); //300ns for enter and exit
    ESP_LOGI("STATUS","^RPS: %4.1f, bperiod %d \n", (float)60*timerResolution/(18.0f*global.blockPeriod), global.blockPeriod);
    // ESP_LOGI("STATUS","^pot%: %6.4f, RPS: %5.2f, bperiod %d \n",(float)rawData/4096.0f, RPS, global.blockPeriod);
    #else

    #ifdef debug_testOnLED
      if(trakcer %3){
      motorStall = !motorStall;
      ESP_LOGW("mtrStl=","%d",motorStall);
    }
    #endif
    #endif
    vTaskDelay(pdMS_TO_TICKS(debug_RPSprint_period)); 
  }
}
  void readPotRepeat(void * parameter){
    for(;;){
      readPotOnce(parameter);
      vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod)); 
    }
}
/*
https://numbergenerator.org/numberlistrandomizer#!numbers=50&lines=1&range=1-4095&unique=true&unique_combinations=true&order_matters=false&csv=csv&del=&oddeven=&oddqty=0&sorted=true&addfilters=
*/
const int debug_bPeriodLookUpTable[] = {
  10000, 300,400,500,600,700,800,900, //dont include stuff that is smaller than the wait time
  1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
  9000, 8000, 7000,6000,5000,4000,3000,2000, 1000,
};
int lookUpTableIndex = 0;

const int debug_adcReadLookUpTable[50] = {
2,77,102,134,177,237,297,445,451,466,646,654,737,751,879,896,912,1022,1025,
1178,1220,1271,1315,1400,1403,1489,1881,1905,1914,1979,1985,2145,2201,2241,
2444,2592,2609,2636,2660,2741,2749,2959,3004,3133,3444,3514,3547,3662,
3703,4092};

void readPotOnce(void * parameter){
   rawData = 0; //higher voltage = higher rps
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    #ifdef debug_useLookUpTableADC
    rawData = debug_adcReadLookUpTable[lookUpTableIndex];
    if(lookUpTableIndex++ >=50){
      lookUpTableIndex =0;
    };
    #endif
    RPS = (fMin+(fMax-fMin)*sqrtf((float)rawData/4096.0f));
    esp_rom_printf(yellow "p@t: %d", rawData);
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spin

    //This bottom part needs to be instantaneous assignment 
    uint32_t bPeriod_temp= (uint32_t)((float)timerResolution/(float)(RPS *electricalCycles*6));

    #ifdef debug_constBlockPeriod
      #if (defined(debug_useLookUpTableOnBPeriod) && ! defined(debug_useLookUpTableADC))
      bPeriod_temp = lookUpTable[lookUpTableIndex];
      lookUpTableIndex++;
      #else
      bPeriod_temp = debug_constBlockPeriod;
      #endif
    #endif

    ESP_LOGE("potRead_BP", "(%d,%d)\n", (int)global.blockPeriod, (int )bPeriod_temp);
    // * µsToTicks; //mcpwm timer icks per block when spinnig
    // ESP_LOGW("redPotonce", "bPeriod_temp :%" PRIu32 ", RPS: %f, rawData: %d", bPeriod_temp, RPS, rawData);
    if(global.blockPeriod != bPeriod_temp){
    taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
    global.blockPeriod = bPeriod_temp;
      global.newVelPotValue =true;
      /*ISR checks this constatnly. If true, it runs code to update the CMRA trheshold. might be useless */

      // ESP_ERROR_CHECK(mcpwm_timer_set_period(blockTimer, bPeriod_temp));
      // ESP_ERROR_CHECK(mcpwm_timer_set_period(globalLowTimer, 6*bPeriod_temp));
      taskEXIT_CRITICAL(&stepPeriodMux);
    }
    //isr loop needs to keep checking
    // ESP_LOGI("readPotOnce", magenta "read pot once");
}
extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 40000, NULL, 12, &setupTask, 1); 
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
    // xTaskCreatePinnedToCore(run6Block, "run6Block", 16384, NULL, 4, NULL, 1);
    
  }
}
//attach block timer frequency to potentionmeter
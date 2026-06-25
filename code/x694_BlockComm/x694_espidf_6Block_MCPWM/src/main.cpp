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
    // ESP_LOGI("STATUS","^RPS: %4.1f, vel-period %d -\n", (float)VTimerResolution/(18.0f*global.blockPeriod), (int)global.blockPeriod);
    ESP_LOGI("a∂c","%6.4f|RPS: %5.2f|BPeriod %d|AS5600: %4d",(float)rawData/4096.0f, RPS, global.blockPeriod, global.rotorVal);
    #else
    // #ifdef debug_testOnLED
    //   if(tracker++ %3){
    //   motorStall = !motorStall;
    //   ESP_LOGW("mtrStl=","%d",motorStall);
    // }
    // #endif
    #endif
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod)); 
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
int lookUpTableIndex = 0;

const int debug_adcReadLookUpTable[31] = {4094,0, 4094,0, 100, 200, 300,400,500,600,700,800,900,1000, 2000,3000,4000,4095,4000,3000,2000,1000,900,800,700,600,500,400,300,200, 100};

void readPotOnce(void * parameter){
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    #ifdef debug_dontReadVelocityPot
      #ifdef debug_useLookUpTableADC
        rawData = debug_adcReadLookUpTable[lookUpTableIndex];
        if(++lookUpTableIndex >= 31){
          lookUpTableIndex =0;
        };
        // lookUpTableIndex++;
        esp_rom_printf(red "p@t raw: %d, %d, index%d\n", rawData,debug_adcReadLookUpTable[lookUpTableIndex],lookUpTableIndex);
      #else
        uint32_t bPeriod_temp = debug_dontReadVelocityPot;
      #endif
    #endif
    #if (!defined(debug_dontReadVelocityPot) || defined(debug_useLookUpTableADC))
    RPS = (fMin+(fMax-fMin)*(float)rawData/4096);
    uint32_t bPeriod_temp= (uint32_t)(VTimerResolution/(RPS*(electricalCycles*6)));
    #endif
    //This bottom part needs to be instantaneous assignment 
    if(global.blockPeriod != bPeriod_temp){
      taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
      global.blockPeriod = bPeriod_temp;
      global.newVelPotValue =true;
      taskEXIT_CRITICAL(&stepPeriodMux);
    }
}

extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 40000, NULL, 12, &setupTask, 1); 
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
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
    ESP_LOGI("STATUS","^RPS: %4.1f, vel-period %d -\n", (float)VTimerResolution/(18.0f*global.blockPeriod), (int)global.blockPeriod);
    // ESP_LOGI("STATUS","^pot%: %6.4f, RPS: %5.2f, bperiod %d \n",(float)rawData/4096.0f, RPS, global.blockPeriod);
    #else

    // #ifdef debug_testOnLED
    //   if(tracker++ %3){
    //   motorStall = !motorStall;
    //   ESP_LOGW("mtrStl=","%d",motorStall);
    // }
    // #endif
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

const int debug_adcReadLookUpTable[] = {0, 4096, 300,400,500,600,700,800,900,
2,77,102,134,177,237,297,445,451,466,646,654,737,751,879,896,912,1022,1025,
1178,1220,1271,1315,1400,1403,1489,1881,1905,1914,1979,1985,2145,2201,2241};

void readPotOnce(void * parameter){
   rawData = 0; //higher voltage = higher rps
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    #ifdef debug_dontReadVelocityPot
      #ifdef debug_useLookUpTableADC
        rawData = debug_adcReadLookUpTable[lookUpTableIndex];
        if(lookUpTableIndex++ >=50){
          lookUpTableIndex =0;
        };
        // if(lookUpTableIndex-- <0){
        //   lookUpTableIndex =49;
        // };
        esp_rom_printf(yellow "p@t raw: %d", rawData);
      #elif (defined(debug_useLookUpTableOnBPeriod))
        uint32_t bPeriod_temp = debug_bPeriodLookUpTable[lookUpTableIndex];
        lookUpTableIndex++;
      #else
        uint32_t bPeriod_temp = debug_dontReadVelocityPot;
      #endif
    #endif
    
    #if (!defined(debug_dontReadVelocityPot) || defined(debug_useLookUpTableADC))
    RPS = (fMin+(fMax-fMin)*(float)rawData/4096);
    uint32_t bPeriod_temp= (uint32_t)(VTimerResolution/(RPS*(electricalCycles*6)));
    // ESP_LOGI(magenta "BPESr", "%d, rps %5.2f\n",global.blockPeriod, RPS);
    #endif
    //This bottom part needs to be instantaneous assignment 

    ESP_LOGE("potRead-BlockPd(o,n)", "(%d,%d), RPS: %5.2f\n", (int)global.blockPeriod, (int )bPeriod_temp, RPS);
    // * µsToTicks; //mcpwm timer icks per block when spinnig
    // ESP_LOGW("redPotonce", "bPeriod_temp :%" PRIu32 ", RPS: %f, rawData: %d", bPeriod_temp, RPS, rawData);
    if(global.blockPeriod != bPeriod_temp){
      taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
      global.blockPeriod = bPeriod_temp;
      global.newVelPotValue =true;
        /*ISR checks this constatnly. If true, it runs code to update the CMRA trheshold. might be useless */
      taskEXIT_CRITICAL(&stepPeriodMux);
    }
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
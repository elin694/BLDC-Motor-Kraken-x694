//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
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
    // #if (!defined(debug_hyperFastPrints) && !defined(debug_hyperFastPrintsWithPot))
    #ifdef debug_printRPS
    // ESP_LOGI("STATUS","^targetVelocity: %4.1f, vel-period %d -\n", (float)VTimerResolution/(18.0f*global.blockPeriod), (int)global.blockPeriod);
    // esp_rom_printf("a∂c: %4d|" cyan "TRPM: %5d" green "|BPeriod %d|AS5600:%4d| Gates: %s \x1b[0K \x1b[1G",rawData, (int)(global.targetVelocity*60), global.blockPeriod, global.rotorVal, ghgl[global.sectorTarget]);
    esp_rom_printf("a∂c: %4d|" cyan "TRPM: %5d" green "|BPeriod %d|AS5600:%4d| Gates: %s\n",rawData, (int)(global.targetVelocity*60), global.blockPeriod, global.rotorVal, ghgl[global.sectorTarget]);
    #endif
    // #endif
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod)); 
  }
}

void readPotRepeat(void * parameter){
  for(;;){
    readPotOnce(parameter);
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod)); 
  }
}
/*https://numbergenerator.org/numberlistrandomizer#!numbers=50&lines=1&range=1-4095&unique=true&unique_combinations=true&order_matters=false&csv=csv&del=&oddeven=&oddqty=0&sorted=true&addfilters=*/
int lookUpTableIndex = 0;
DRAM_ATTR uint32_t vbPeriod_temp;
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
      #else //hold it constant
        vbPeriod_temp = debug_dontReadVelocityPot;
      #endif
    #endif

    #if (!defined(debug_dontReadVelocityPot) || defined(debug_useLookUpTableADC))
    if(global.controlMethod == VELOCITY_CONTROL){
        global.targetVelocity = (fMin+(fMax-fMin)*(float)rawData/4096);
        (global.targetVelocity < 0) ? (global.dir = 4) : (global.dir = 2);
        vbPeriod_temp= (uint32_t)(VTimerResolution/fabsf(global.targetVelocity*(electricalCycles*6)));
        if(vbPeriod_temp >>16 != 0){
          global.dir =0;
          vbPeriod_temp =minf_HTimerPeriod;
        }

      }else if(global.controlMethod == TORQUE_CONTROL){
        global.targetAcceleration = (aMin+(aMax-aMin)*(float)rawData/4096);
      /*conside case from motor stall - to fMIn*/
      
      }else if(global.controlMethod == POSITION_CONTROL){
       global.targetPosition = (pMin+(pMax-pMin)*(float)rawData/4096);
      }
    #endif

    if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment 

      taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
      global.blockPeriod = vbPeriod_temp;
      global.newVelPotValue =true;
      taskEXIT_CRITICAL(&stepPeriodMux);
    }
}

extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 40000, NULL, 12, &setupTask, 0); 
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
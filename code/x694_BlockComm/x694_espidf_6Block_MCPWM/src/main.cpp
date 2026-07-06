//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
int rawData = 0;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

void debugLog(void * parameter){
  int tracker = 0;
  for(;;){
    // #if (!defined(debug_hyperFastPrints) && !defined(debug_hyperFastPrintsWithPot))
    #ifdef debug_printRPS
    // ESP_LOGI("STATUS","^targetVelocity: %4.1f, vel-period %d -\n", (float)VTimerResolution/(18.0f*global.blockPeriod), (int)global.blockPeriod);
    // esp_rom_printf("a∂c: %4d|" cyan "TRPM: %5d" green "|BPeriod %d|AS5600:%4d| Gates: %s \x1b[0K \x1b[1G",rawData, (int)(global.targetVelocity*60), global.blockPeriod, global.rotorVal, ghgl[global.sectorTarget]);
    // bool bearing = global.dir;
    taskENTER_CRITICAL(&stepPeriodMux);
    int k = global.dir;
    taskEXIT_CRITICAL(&stepPeriodMux);
    esp_rom_printf("a∂c:%4d|" cyan "TRPM:%5d" green "|BPeriod %d|AS5600:%4d| G:%s, \n",rawData, (int)(global.targetVelocity*60), global.blockPeriod, global.rotorVal, ghgl[global.sectorTarget]);
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
DRAM_ATTR uint32_t vbPeriod_temp;
void readPotOnce(void * parameter){
  ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
  rawData = (rawData/2)*2;
  #ifdef debug_dontReadVelocityPot
  vbPeriod_temp = debug_dontReadVelocityPot;
  if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment
    esp_rom_printf("ENTIRNG CRITICAL"); 
    global.targetVelocity=VTimerResolution/(18.0f* vbPeriod_temp);
    (global.targetVelocity < 0) ? (global.dir = 5) : (global.dir = 2);
    taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
    global.blockPeriod = vbPeriod_temp;
    global.newVelPotValue =true;
    taskEXIT_CRITICAL(&stepPeriodMux);
  }
  #endif

  #if (!defined(debug_dontReadVelocityPot))
  if(global.controlMethod == VELOCITY_CONTROL){
    global.targetVelocity = (fMin+(fMax-fMin)*(float)rawData/4096);
    vbPeriod_temp= (uint32_t)(VTimerResolution/fabsf(global.targetVelocity*(electricalCycles*6)));
    
    bool notlegal = vbPeriod_temp >minf_HTimerPeriod;
    if(notlegal){
      // ESP_LOGI("F","SPIN");
    }
    if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment 
      taskENTER_CRITICAL(&stepPeriodMux); //300ns for enter and exit
      (global.targetVelocity < 0) ? (global.dir = 5) : (global.dir = 2);
      if(notlegal){
        global.setMotorFreeSpin.store(true); //spinlock
        global.blockPeriod  =minf_HTimerPeriod; //spinlock
      }else{
        global.setMotorFreeSpin.store(false);
        global.blockPeriod = vbPeriod_temp;
      }
        global.newVelPotValue =true; //spinlock
      taskEXIT_CRITICAL(&stepPeriodMux); //spinlock
    }

    }else if(global.controlMethod == TORQUE_CONTROL){
      global.targetAcceleration = (aMin+(aMax-aMin)*(float)rawData/4096);
      /*conside case from motor stall - to fMIn*/

    }else if(global.controlMethod == POSITION_CONTROL){
      global.targetPosition = (pMin+(pMax-pMin)*(float)rawData/4096);
    }
    #endif
}

extern "C"{
  void app_main(){
    xTaskCreatePinnedToCore(initialize, "SETUP", 30000, NULL, 12, &setupTask, 0); 
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
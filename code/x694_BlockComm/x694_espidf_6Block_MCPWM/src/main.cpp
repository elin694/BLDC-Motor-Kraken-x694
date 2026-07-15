//============================ 6 step commutation! with ESPIDF ============================
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
int rawData = 0;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

void debugLog(void * startTick2){
  char buf[400];
  TickType_t startTick = *(TickType_t*)startTick2;
  xTaskDelayUntil(&startTick,initializationLatency);
  uint32_t esp_timer_log_counter = 0;
  uint32_t task_list_log_counter = 0;
  for(;;){
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod*50)); 
    #ifdef debug_printRPS
    taskENTER_CRITICAL(&stepPeriodMux);
    int gp = global.blockPeriod;
    int tempCoast = global.setMotorFreeTemporarily.load(std::memory_order::relaxed);
    int stateIsCoast = global.setMotorFreeSpin.load(std::memory_order::relaxed);
    taskEXIT_CRITICAL(&stepPeriodMux);
    esp_rom_printf("a∂c%4d " cyan "TRPM%5d" green " BPeriod%5d I2C%4d TCoast%d,%d\n",rawData, (int)(global.targetVelocity*60), gp, global.rotorVal,tempCoast,stateIsCoast);
    #endif
    if((esp_timer_log_counter++%8 )==0){
      esp_rom_printf(blue); esp_timer_dump(stdout);
    }
    if((task_list_log_counter++%8 )==0){
      vTaskList(buf); ESP_LOGI(" ","\n%s",buf);
    }
  }
  
}

void readPotRepeat(void * startTick3){
  bool changeV =true;
  TickType_t startTick = *(TickType_t*)startTick3;
  xTaskDelayUntil(&startTick,initializationLatency);
  for(;;){
    readPotOnce((void *)&changeV);
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod)); 
  }
}


/*https://numbergenerator.org/numberlistrandomizer#!numbers=50&lines=1&range=1-4095&unique=true&unique_combinations=true&order_matters=false&csv=csv&del=&oddeven=&oddqty=0&sorted=true&addfilters=*/
uint32_t vbPeriod_temp;
void readPotOnce(void * parameter){
  bool flagOn = *(bool *)parameter;
  ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
  rawData = (rawData/2)*2;
  #ifdef debug_dontReadVelocityPot
  vbPeriod_temp = debug_dontReadVelocityPot;
  if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment
    esp_rom_printf("ENTIRNG CRITICAL"); 
    global.targetVelocity=VTimerResolution/(18.0f* vbPeriod_temp);
    (global.targetVelocity < 0) ? (global.dir = 5) : (global.dir = 2);
    taskENTER_CRITICAL(&stepPeriodMux); //don't read v pot, core 0
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
      taskENTER_CRITICAL(&stepPeriodMux); //read pot once, core0
      (global.targetVelocity < 0) ? (global.dir = 5) : (global.dir = 2);
      if(notlegal){
        global.setMotorFreeSpin.store(true); //spinlock
        global.blockPeriod  =minf_HTimerPeriod; //spinlock
      }else{
        global.setMotorFreeSpin.store(false);
        global.blockPeriod = vbPeriod_temp;
      }
      if(flagOn){
        global.newVelPotValue =true; 
      }
      taskEXIT_CRITICAL(&stepPeriodMux); //spinlock
      tag("Rp ");
      //prints after reading a new pot value
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
    vTaskDelay(pdMS_TO_TICKS(10)); //To let gate driver setup
    xTaskCreatePinnedToCore(initialize, "SETUP", 25000, NULL, 12, &setupTask, 0); 
    ulTaskNotifyValueClear(setupTask, 0xffffffff);
    xTaskNotifyStateClear(setupTask);
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
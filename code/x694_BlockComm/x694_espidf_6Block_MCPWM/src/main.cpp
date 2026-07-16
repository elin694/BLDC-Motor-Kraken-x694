//============================ 6 step commutation! with ESPIDF ============================
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
uint32_t potBuffer[128];
int rawData = 0;
portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

void debugLog(void * startTick2){
  // char buf[400];
  // uint32_t task_list_log_counter = 0;
  TickType_t startTick = *(TickType_t*)startTick2;
  uint32_t esp_timer_log_counter = 0;
  TickType_t loopStartTick ;
  
  #define esp_timer_cycle 8
  ESP_LOGI("Main.cpp","Printing time log every %d ms!",esp_timer_cycle*velPotReadPeriod*50);
  xTaskDelayUntil(&startTick,initializationLatency);
  for(;;){
    loopStartTick =xTaskGetTickCount();
    xTaskDelayUntil(&loopStartTick,pdMS_TO_TICKS(velPotReadPeriod*50)); 

    #ifdef debug_printRPS
    taskENTER_CRITICAL(&stepPeriodMux);
    int gp = global.blockPeriod;
    int tempCoast = global.setMotorFreeTemporarily.load(std::memory_order::relaxed);
    int stateIsCoast = global.setMotorFreeSpin.load(std::memory_order::relaxed);
    taskEXIT_CRITICAL(&stepPeriodMux);
     int numGsnCycled=isr2i.load(std::memory_order::relaxed);
    esp_rom_printf("a∂c%4d " cyan "TRPM%5d" green " BPeriod%5d I2C%4d TCoast%d,%d-%d\n",rawData, (int)(global.targetVelocity*60), gp, global.rotorVal,tempCoast,stateIsCoast,numGsnCycled);
    #endif

    if((esp_timer_log_counter++%esp_timer_cycle )==0){
      esp_rom_printf("\n" blue); esp_timer_dump(stdout);
    }
  //   if((task_list_log_counter++%8 )==0){
  //     vTaskList(buf); ESP_LOGI(" ","\n%s",buf);
  //   }

  }
}

void readPotRepeat(void * startTick3){
  TickType_t startTick = *(TickType_t*)startTick3;
  xTaskDelayUntil(&startTick,initializationLatency);
  int history[4] = {-1,-1,-1,-1};
  uint32_t counterIndex= 0;
  for(;;){
    if(history[3] ==-1){
      history[counterIndex++%4] = readPotOnce(false,0);
    }else{
      int sum=0;
      for(int i =-1; i>-4;i--){
        sum+=history[(counterIndex+i)%4];
      }
      history[counterIndex++%4] = readPotOnce(true,sum);
    }
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod));  
  }
}


/*https://numbergenerator.org/numberlistrandomizer#!numbers=50&lines=1&range=1-4095&unique=true&unique_combinations=true&order_matters=false&csv=csv&del=&oddeven=&oddqty=0&sorted=true&addfilters=*/
uint32_t vbPeriod_temp;
uint32_t readPotOnce(bool filter, int averager){
  ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
  rawData = (rawData/2)*2;
  if(global.controlMethod == VELOCITY_CONTROL){
    if(filter){
      int processedData = (averager+rawData)/(4);
      global.targetVelocity = (fMin+(fMax-fMin)*processedData/4096.0f);
    }else{
      float processedData= rawData/4096.0f;
      global.targetVelocity = (fMin+(fMax-fMin)*processedData);

    }
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
      global.newVelPotValue =true; 
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
    return rawData;
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
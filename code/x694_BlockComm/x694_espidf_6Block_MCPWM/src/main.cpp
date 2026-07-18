//============================ 6 step commutation! with ESPIDF ============================
#include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
int rawData = 0;

#define esp_timer_cycle 16
#define espTimer_isMinutelyCheckup(x) ((x % esp_timer_cycle ) == 15)

void debugMonitor (void * startTick2) {
  char buf[600];
  // uint32_t task_list_log_counter = 0;
  TickType_t startTick = *(TickType_t*) startTick2;
  uint32_t esp_timer_log_counter = 0;
  TickType_t loopStartTick ;
  ESP_LOGI("Main.cpp", "EspTimer log period:%d ms!", esp_timer_cycle * velPotReadPeriod * 40);

  xTaskDelayUntil(&startTick,initializationLatency);
  ESP_LOGI("main.cpp", "GOOO!\n\n");
  for(;;){
    loopStartTick = xTaskGetTickCount();
    xTaskDelayUntil(&loopStartTick, pdMS_TO_TICKS(velPotReadPeriod * 40)); 

    //read RPS
    taskENTER_CRITICAL(&stepPeriodMux);
    int gp = global.blockPeriod;
    int tempCoast = global.setMotorFreeTemporarily.load(std::memory_order::relaxed);
    int stateIsCoast = global.setMotorFreeSpin.load(std::memory_order::relaxed);
    taskEXIT_CRITICAL(&stepPeriodMux);
     int numGsnCycled = isr2i.load(std::memory_order::relaxed);
    esp_rom_printf("a∂c%4d " cyan "TRPM%5d" green " BPeriod%5d I2C%4d TCoast%d,%d-%d\n",rawData, (int)(global.targetVelocity*60), gp, global.rotorVal,tempCoast,stateIsCoast,numGsnCycled);

    if(espTimer_isMinutelyCheckup(esp_timer_log_counter++)){
      ESP_LOGI("\n", blue); esp_timer_dump(stdout);
      esp_rom_printf("\n\n");
      vTaskGetRunTimeStats(buf);
      esp_rom_printf(buf);
    }
  }
}

void readPotRepeat (void * startTick3) {
  TickType_t startTick = *(TickType_t*)startTick3;
  int history[adcReadBufferSize] = {-1,-1,-1,-1};
  uint32_t counterIndex= 0;

  xTaskDelayUntil(&startTick,initializationLatency);
  for(;;){
    if(history[adcReadBufferSize-1] ==-1){
      history[counterIndex++ % adcReadBufferSize] = readPotOnce(false, 0);
    }else{
      int sum=0;
      for(int i = -1; i > (-adcReadBufferSize); i--){
        sum+=history[(counterIndex + i) % adcReadBufferSize];
      }
      history[counterIndex++ % adcReadBufferSize] = readPotOnce(true, sum);
    }
    vTaskDelay(pdMS_TO_TICKS(velPotReadPeriod));  
  }
}


/*https://numbergenerator.org/numberlistrandomizer#!numbers=50&lines=1&range=1-4095&unique=true&unique_combinations=true&order_matters=false&csv=csv&del=&oddeven=&oddqty=0&sorted=true&addfilters=*/
uint32_t readPotOnce (bool filter, int averager) {
  uint32_t vbPeriod_temp;

  ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
  rawData = (rawData/2)*2;
  if(global.controlMethod == VELOCITY_CONTROL){
    int processedData = (filter) ? ((averager + rawData) / (4)) : rawData;
    float localTargetVelocity = (fMin + (((fMax - fMin)/4096.0f) * processedData));
    vbPeriod_temp= (uint32_t)((VTimerResolution / electricalCycles) / fabsf(localTargetVelocity));
    
    if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment 
      ESP_LOGW(yellow, "Tvel:%5.2f per.:%d ft:%d Avgr:%4d", localTargetVelocity, vbPeriod_temp, filter, averager);
      int dirWaitingLine = (localTargetVelocity < 0) ? (5) : (2);
      bool legal = vbPeriod_temp <= minf_HTimerPeriod;

      if(legal){
        taskENTER_CRITICAL(&stepPeriodMux); //read pot once, core0
        global.setMotorFreeSpin.store(false);
        global.blockPeriod = vbPeriod_temp;
        global.dir = dirWaitingLine;
        global.targetVelocity = localTargetVelocity;
        taskEXIT_CRITICAL(&stepPeriodMux); //spinlock
        ESP_ERROR_CHECK(mcpwm_timer_set_period(VTimer, vbPeriod_temp));  
        tag("newVel");               //set new block value on period ONLY WHEN POT HAS READ SMTH new, and updates period period on empty
      }else{
        taskENTER_CRITICAL(&stepPeriodMux); //read pot once, core0
        global.setMotorFreeSpin.store(true); //spinlock
        global.blockPeriod  =minf_HTimerPeriod; //spinlock
        global.dir = dirWaitingLine;
        global.targetVelocity = localTargetVelocity;
        taskEXIT_CRITICAL(&stepPeriodMux); //spinlock
      }

    }

    }else if(global.controlMethod == TORQUE_CONTROL){
      global.targetAcceleration = (aMin + ((aMax - aMin) / 4096.0f) * rawData);
      /*conside case from motor stall - to fMIn*/

    }else if(global.controlMethod == POSITION_CONTROL){
      global.targetPosition = (pMin + ((pMax - pMin) / 4096.0f) * rawData);
    }
    return rawData;
}

extern "C"{
  void app_main(){
    // uint32_t t1 = xPortGetRunTimeCounterValue();
    // uint32_t t2 =time();
    // ESP_LOGI("\n YEEEE","\n %d,  %d\n",(int)t1,t2);

    // vTaskDelay(694);
    // t1 = xPortGetRunTimeCounterValue();
    // t2 =time();
    // ESP_LOGI("\n YEEEE","\n %d,  %d\n",(int)t1,t2);
    // vTaskDelay(10000000);
    // vTaskDelay(pdMS_TO_TICKS(100)); //To let gate driver setup
    esp_rom_delay_us(100);
    xTaskCreatePinnedToCore(initialize, "SETUP", 25000, NULL, 20, &setupTask, 0); 
    CLEAR_ALL_NOTIFS(setupTask);
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
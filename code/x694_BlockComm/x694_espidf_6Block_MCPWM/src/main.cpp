//============================ 6 step commutation! with ESPIDF ============================
// #include "GateControl.h"
#include "Globals.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s

adc_oneshot_unit_handle_t adcHandle = NULL;
int rawData = 0;

#define esp32timer_dump_cycle 8
#ifdef DEBUG_ALLOW_DUMPING
#define espTimer_isMinutelyCheckup(x) (((x % esp32timer_dump_cycle ) == (esp32timer_dump_cycle - 1)) )
#else
#define espTimer_isMinutelyCheckup(x) (((x % esp32timer_dump_cycle ) == (esp32timer_dump_cycle + 1)) )
#endif

void debugMonitor (void * startTick2) {
  char buf[600];
  // uint32_t task_list_log_counter = 0;
  TickType_t startTick = *(TickType_t*) startTick2;
  uint32_t esp32timer_log_counter = 0;
  TickType_t loopStartTick ;
  ESP_LOGI("Main.cpp", "EspTimer log period:%d ms!", esp32timer_dump_cycle * velPotReadPeriod * 40);

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
     int encoder = (int)global.rotorVal;
    esp_rom_printf("a∂c%4d " cyan "TRPM%5d" white " BPeriod%5d I2C%4d TCoast%d,%d-%d\n",rawData, (int)(global.targetVelocity*60), gp, encoder, tempCoast, stateIsCoast, numGsnCycled);

    if(espTimer_isMinutelyCheckup(esp32timer_log_counter++)){
      #ifdef useGPTimerOverESP32Timer
      #else
      ESP_LOGI("\n", blue); esp_timer_dump(stdout);
      #endif
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

  xTaskDelayUntil(&startTick, initializationLatency);
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

  if(global.controlMethod == TORQUE_CONTROL){
      global.targetTorque = (TARGET_TORQUE_LB + ((TARGET_TORQUE_UB - TARGET_TORQUE_LB) / 4096.0f) * rawData);
      
    } else if (global.controlMethod == VELOCITY_CONTROL) {
    int processedData = (filter) ? ((averager + rawData) / (4)) : rawData;
    float localTargetVelocity = (TARGET_VELOCITY_LB + (TARGET_VELOCITY_UB - TARGET_VELOCITY_LB)*processedData/4096.0f); /*OLD*/
    vbPeriod_temp= (uint32_t)(VTIMER_CLOCK/fabsf(localTargetVelocity* BLOCKS_PER_ROTATION));  /*OLD*/
    // float localTargetVelocity = (fMin + (((fMax - fMin)/4096.0f) * processedData)); /*NEW*/
    // vbPeriod_temp= (uint32_t)((VTimerResolution / electricalCycles) / fabsf(localTargetVelocity));  /*NEW*/
    
    if(global.blockPeriod != vbPeriod_temp){//needs to be instantaneous assignment 
      ESP_LOGI("Tvel", "%7.3f per.:%d ft:%d Ar:%4d, %d", localTargetVelocity, vbPeriod_temp, filter, averager, processedData);
      int dirWaitingLine = (localTargetVelocity < 0) ? (5) : (2);
      bool legal = vbPeriod_temp <= SL_MIN_VELOCITY_PERIOD_TICKS;

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
        global.blockPeriod  =SL_MIN_VELOCITY_PERIOD_TICKS; //spinlock
        global.dir = dirWaitingLine;
        global.targetVelocity = localTargetVelocity;
        taskEXIT_CRITICAL(&stepPeriodMux); //spinlock
      }

    }

    } else if (global.controlMethod == POSITION_CONTROL) {
      global.targetPosition_BiPS = (TARGET_POSITION_LB + ((TARGET_POSITION_UB - TARGET_POSITION_LB) / 4096.0f) * rawData);
    }
    return rawData;
}

#include "hal/clk_tree_hal.h"
extern "C"{
  void app_main(){
    // uint32_t t1 = xPortGetRunTimeCounterValue();
    // uint32_t t2 =snap();
    // ESP_LOGI("\n YEEEE","\n %d,  %d\n",(int)t1,t2);

    // vTaskDelay(694);
    // t1 = xPortGetRunTimeCounterValue();
    // t2 = snap();
    // ESP_LOGI("\n YEEEE","\n %d,  %d\n",(int)t1,t2);
    // vTaskDelay(10000000);
    // vTaskDelay(pdMS_TO_TICKS(100)); //To let gate driver setup

    // ESP_LOGW("main","%d", clk_hal_apb_get_freq_hz());
    esp_rom_delay_us(100);
    xTaskCreatePinnedToCore(initialize, "SETUP", 25000, NULL, 20, &setupTask, 0); 
    CLEAR_ALL_NOTIFS(setupTask);
    ESP_LOGI("Checkpoint", "APP_MAIN INIT FINISHED");
  }
}
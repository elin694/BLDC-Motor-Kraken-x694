//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "GateControl.h"
#include "driver/i2c_master.h"
#define ticksToµs static_cast<float>((1e6)/timerResolution)
#define µsToTicks static_cast<float>(timerResolution/1e6) //ontime * this = tick
#define µsToTicksInt static_cast<int>(timerResolution/1e6) //ontime * this = tick
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
//============================Initializing values ============================zxq`
const double fMin = 1; //in hertz
const double fMax = 15;

uint64_t lastTime = 0;

adc_oneshot_unit_handle_t adcHandle;
uint8_t potBuffer[128];

// Offset exists so that block 3 is calibrate and maps to 30° on the electrical axes
//block, radians
//0 : 7π/6 
//1 :  9π/6
//2 : 11π/6
//3 : π/6
//4 : 3π/6
//5 : 5π/6
double RPS= 0 ;
int rawData = 0;
//format {A,B,C}, {-0-1,1} = {float,sink,source} = {float, low, high}
// int steps[6][3] = {  {1,-1,0},  {-1,1,0},  {0,1,-1},  {0,-1,1},  {-1,0,1},  {1,0,-1}  };  og 0=sink
//==================================LOOP=====================================
void run6Block(void * parameter) { 
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int rawData = 0;
  for(;;){
    for (int phase = 0; phase <3; phase++){ //aplies state to each block
      switchBlock(phase); 
    }
    xTaskDelayUntil(&xLastWakeTime,(pdMS_TO_TICKS(((blockPeriod/µsToTicksInt)/1000.0))+1)); 
    blockNumber = (getSectorNumber() + 2*dir) % 6; //optimize ot remove modulo***********************
    
    RPS = (fMin+(fMax-fMin)*sqrtf((float)rawData/4096.0f));
    blockPeriod= 1000000.0f/(float)(RPS *electricalCycles*6) * µsToTicks;
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spins

    lastTime = esp_timer_get_time();
    taskYIELD();
    
    }
}
void readPot1(void * parameter){
  for(;;){
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void debugLog(void * parameter){
  for(;;){
    ESP_LOGI("REPORT STATUS",":pot%: %6.4f, RPS: %5.2f \n ",(float)rawData/4096.0f, RPS);
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}
      
extern "C"{
  void app_main(){
    initialize(); //setup
    xTaskCreatePinnedToCore(run6Block, "run6Block", 16384, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(readPot1, "readPot1", 10000, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(debugLog, "debugLog", 10000, NULL, 1, NULL, 1);
    //pull Low high to prime Bootstrap cap?
  }
}
//attach block timer frequency to potentionmeter
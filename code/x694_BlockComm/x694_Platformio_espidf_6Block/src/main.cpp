//============================ 6 step commutation! with ESPIDF ============================
#include "initialize.h"
#include "globals.h"
#include "driver/i2c_master.h"
//https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
/* Using a potentiometer hooked up to 5V to control a 3 phase DC motor with Arduino Uno and sinusoidal control.*/
//espidf onshot vs continuous adc translation ; https://randomnerdtutorials.com/esp-idf-esp32-gpio-analog-adc/
//============================Initializing values ============================zxq`
const double fMin = 1; //in hertz
const double fMax = 15;
const int electricalCycles =3;
const long printPeriod = 2e5;
uint64_t lastTime = 0;
uint32_t onTime =0; //how long to delay every phase
uint32_t deadTime =5; 
int blockNumber = 0;
int dir = 1; //or 5 to go in reverse
adc_oneshot_unit_handle_t adcHandle;
uint8_t potBuffer[128];

//==================== as5600 ====================
uint8_t as5600Register = 0x36;
#define data_register_length 2
const uint8_t as5600TargetRegister = 0x0e;
size_t as5600WriteSize = 1;
uint8_t as5600RawDataBuf[2];
size_t as5600ReadSize = 2;
const uint8_t as5600CalibratedOffset =10; //in bits 

int steps[6][3] = {
  {1,-1,0},
  {-1,1,0},
  {0,1,-1},
  {0,-1,1},
  {-1,0,1},
  {1,0,-1},
};
//==================================LOOP=====================================
void loop(void * parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int rawData = 0;
  for(;;){
    for (int phase = 0; phase <3; phase++){ //aplies state to each block
      switchBlock(phase); 
    }
    xTaskDelayUntil(&xLastWakeTime,(pdMS_TO_TICKS((onTime/1000.0))+1)); 
    blockNumber = (getSectorNumber() + dir) % 6;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    
    double RPS = (fMin+(fMax-fMin)*std::sqrt(rawData/4096.0));
    #ifndef byTime
    onTime = 1000000.0/(RPS *electricalCycles*6);
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spins
    #else
    //turning towards negative--> smaller delay for value --> faster spins
    val=(rawData*1000.0/4096.0*(periodMax-periodMin)+1000.0*periodMin)/
    // linear scaling period with respect to potentiometer reading
    (electricalCycles*6); //3 is for the pole pair count per rotation
    #endif  
    #ifdef enableLogs
    printf("pot%: %6.4f, ",(rawData/4096.0));
    printf("RPS: %5.2f, ",RPS);
    printf("s: %9.4lld, PhaseOnDuration: %9lu, blockNum: %i \n ", lastTime, onTime, blockNumber);
    #endif 
    //(esp_timer_get_time()-lastTime > 400) ? lastTime = esp_timer_get_time() :;
    lastTime = esp_timer_get_time();
    taskYIELD();
    
    }
}
    
uint8_t getSectorNumber() {
  #define data_length 2
  ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
    &as5600TargetRegister, 
    as5600WriteSize,
    as5600RawDataBuf, 
    as5600ReadSize, //ensure 2 bytes is read
    3));
  uint16_t rotorAngle = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
  // + as5600CalibratedOffset
  ;
  #define bitsPerSector (4096.0 / (electricalCycles*6))
  return (static_cast<uint8_t>(rotorAngle/bitsPerSector) % 6); //0- bitsPerSector --> smaller sector
}
      
extern "C"{
  void app_main(){
    initialize(); //setup
    initAnalogReadOnce(NULL);
    xTaskCreatePinnedToCore(loop, "loop", 16384, NULL, 1, NULL, 1);
  }
}

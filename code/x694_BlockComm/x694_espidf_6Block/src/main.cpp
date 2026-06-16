//============================ 6 step commutation! with ESPIDF ============================
#include "Initialize.h"
#include "Globals.h"
#include "driver/i2c_master.h"
//Ti sinusoidal : https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
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
const uint8_t as5600TargetRegister = 0x0e;
size_t as5600WriteSize = 1;
uint8_t as5600RawDataBuf[2];
size_t as5600ReadSize = 2;
// Offset exists so that block 3 is calibrate and maps to 30° on the electrical axes
//block, radians
//0 : 7π/6 
//1 :  9π/6
//2 : 11π/6
//3 : π/6
//4 : 3π/6
//5 : 5π/6

// #define as5600DirPinHigh
#ifdef as5600DirPinHigh
const uint16_t as5600CalibratedOffset = static_cast<uint16_t>(
  -(2107-(4095.0/3)) + 30.0 *(4095/3)/360
); //2107 bit at c high a low (block #3 )with DIR  @5V
#else
const uint16_t as5600CalibratedOffset = static_cast<uint16_t>(
  -((4096-2107)-(4095.0/3)) + 30.0 *(4095/3)/360
); 
#endif
//format {A,B,C}, {-0-1,1} = {float,sink,source} = {float, low, high}
// int steps[6][3] = {  {1,-1,0},  {-1,1,0},  {0,1,-1},  {0,-1,1},  {-1,0,1},  {1,0,-1}  };  og 0=sink
int steps[6][3] = {  {1,0,-1},  {0,1,-1},  {-1,1,0},  {-1,0,1},  {0,-1,1},  {1,-1,0}  }; 
//==================================LOOP=====================================
void loop(void * parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int rawData = 0;
  for(;;){
    for (int phase = 0; phase <3; phase++){ //aplies state to each block
      switchBlock(phase); 
    }
    xTaskDelayUntil(&xLastWakeTime,(pdMS_TO_TICKS((onTime/1000.0))+1)); 
    blockNumber = (getSectorNumber() + 2*dir) % 6;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel, &rawData));
    
    double RPS = (fMin+(fMax-fMin)*std::sqrt(rawData/4096.0));
    onTime = 1000000.0/(RPS *electricalCycles*6);
    //3 is for the pole pair count per rotation
    //turning towards negative--> longer delay for value --> slower spins
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
  //as5600 is default increasing on clockwise.
  //set DIR high to invert 
  #define data_length 2
  esp_err_t result =(i2c_master_transmit_receive(as5600Handle, 
    &as5600TargetRegister, 
    as5600WriteSize,
    as5600RawDataBuf, 
    as5600ReadSize, //ensure 2 bytes is read
    3));
    #ifdef as5600DirPinHigh
    uint16_t rotorAngle = ((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1]) 
    + as5600CalibratedOffset;
    #else
    uint16_t rotorAngle = 4096-((as5600RawDataBuf[0]<<8)|as5600RawDataBuf[1])
    + as5600CalibratedOffset;
    #endif
    esp_rom_printf("I2C  check: %d, data: %d", (int)result, rotorAngle);
    
    #define bitsPerSector (4096.0 / (electricalCycles*6))
  return (static_cast<uint8_t>(rotorAngle/bitsPerSector) % 6); //0- bitsPerSector --> smaller sector
}
      
extern "C"{
  void app_main(){
    initialize(); //setup
    initAnalogReadOnce(NULL);
    ets_delay_us(100);
    blockNumber = getSectorNumber();
    xTaskCreatePinnedToCore(loop, "loop", 16384, NULL, 1, NULL, 1);
  }
}

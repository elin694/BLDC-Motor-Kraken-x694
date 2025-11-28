// USES ESP32's DAC to create a sine wave that mimics ac for inductor testing

// #include <Arduino.h>
// #include <driver/dac.h>
// //#include <LibPrintf.h>
// #define entries 45
// int dacPin = 25;
// // put function declarations here:
// uint8_t sinTable[entries];
// int counter = 0;
// float maxV = 3; // ptp
// float dcOffset=3.3/2; // ptp
// // bool flag =true;
// double freq =1;
// double period = 1000000/(freq*entries);
// bool maxF =true;
// bool mstime =false;
// //const int readPin =34;

// void setup() {
//   delay(1000);
//   period=1;
//   //pinMode(readPin,INPUT);
//   Serial.begin(115200);
//   //25 = dac1, 26 = dac2
//   maxV /= 2;
//   #if (period > 16383)
//   period /=1000;
//   mstime =true;
//   #endif
//   if (dacPin == 25){
//   //dac_output_voltage(DAC_CHANNEL_1,254);
//   dac_output_enable(DAC_CHANNEL_1);
//   } else{
//   //dac_output_voltage(DAC_CHANNEL_2,254);
//   dac_output_enable(DAC_CHANNEL_2);
//   }
//     //dac_output_voltage(DAC_CHANNEL_1,254);
//   dac_output_enable(DAC_CHANNEL_2);

//   dac_cw_generator_enable();
//   for (int i = 0; i<entries;i++){
//     //sinTable[i] = static_cast<uint8_t>((sin((i*1.0/entries)*2.0*PI) ));
//     float r1 = sin((i*1.0/entries)*2.0*PI) *maxV*255/3.3;
//     float r = r1+dcOffset*255/3.3;
//     sinTable[i]  = static_cast<uint8_t>(r+.5);
//     printf("ac map v: %f, acDC map V w/ decimal: %f, tableValue at i =%i: %i \n",r1,r,i,(sinTable[i]));
//     //delay(100);
//     //sinTable[i] = static_cast<uint8_t>((sin((i/entries)*2.0*PI) *maxV*255/3.3 + dcOffset*255/3.3)+.5);
    
//   }
//   Serial.println("setup bobbyialized");
//   dacWrite(25 + (25==dacPin), static_cast<uint8_t>(dcOffset*255/3.3));
// }

// void loop() {
//  for(int i =0; i<entries;i++){
//   //  dacWrite(dacPin,sinTable[i]);
//    dacWrite(dacPin,sinTable[i]);
//  }
//   //printf("valueRead: %i \n",analogRead(readPin));
//   #if (!maxF)
//     #if (mstime)
//     delay(period);
//     #else 
//     delayMicroseconds(period);
//     #endif
//   #endif
// }
#include <DacESP32.h>
#include <Arduino.h>
#include <driver/dac_types.h>
#include <driver/dac_cosine.h>
#include <driver/dac_continuous.h>
#include <driver/dac_oneshot.h>
 
float maxi;
float mini;
uint64_t bobby =0;
uint64_t counter= 0;
bool flag =false;
DacESP32 dac1(GPIO_NUM_25);
 float dcOffset=3.3/2; // ptp
 float ptp=2; // ptp


 void IRAM_ATTR buttonISR() {
flag = true;
}
void setup() {
  Serial.begin(115200);
  pinMode(22,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(22), buttonISR, CHANGE);
// dac1.outputCW(500,DAC_COSINE_ATTEN_DB_0 ,DAC_COSINE_PHASE_0,static_cast<uint8_t>(1/3.31 * 255)-128);
dacWrite(GPIO_NUM_26,127);
//dacWrite(GPIO_NUM_26,255);
//7=+
dac1.outputCW(497);
// int val =analogRead(32);
// maxi= val*3.3/4095.0;
// mini=maxi; 
dac1.setCwScale(DAC_COSINE_ATTEN_DB_6 );
bobby += static_cast<uint64_t>(millis());
}
void loop() {
  counter++;
  if(flag){
      bobby =  static_cast<uint64_t>(millis())-bobby;
  printf("the counter value: %llu, time since start: %llu", counter, bobby);
  delay(10000);
  flag =false;
  }
  
//  int  val = analogRead(32);
//  float p =val*3.3/4095;
//  if(maxi < p){
//   maxi= p;
//  } else if(mini>p){
//   mini =p;
//  }
// printf("Time: %ld, raw val: %4i, voltage: %6.3f, minmax:  %6.3f , %6.3f \n",millis(),val, p, mini, maxi);
// delay(400);
//   // Change signal amplitude every second
}
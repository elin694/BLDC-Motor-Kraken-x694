#ifndef ARDUINO_H
#define ARDUINO_H
#include "Arduino.h"
#endif 
#ifndef LIBPRINTF_H
#define LIBPRINTF_H
#include <LibPrintf.h>
#endif 
#include "globals.h"
#include "portAssignments.h"
#include "initialize.h"

//https://www.youtube.com/watch?v=-By_vt27Xhs&t=21s
/* Using a potentiometer hooked up to 5V to control a 3 phase DC motor with Arduino Uno and sinusoidal control.*/
//======Initializing values ======
// #define byTime //define the ontime by period, rather than frequency
// #define onTimeRatio 90
#define invertedHighside 
#ifdef byTime
  //millisecond, per rotation
  const int periodMin =80;
  const int periodMax = 380;
#else
  //rot per second
  const float fMin = 1.3;
  const float fMax = 14;
#endif
const int electricalCycles =3;
const long printPeriod = 2e5;
uint64_t lastTime = 0;
uint32_t val =0; //how long to delay every phase
uint32_t onTime =0; 
uint32_t deadTime =0; 
int blockNumber = 0;
  //Direction A
  // Phase:       A---B---C
  // block 1 =    L---H---N
  // block 2 =   N---H---L
  // block 3 =   H---N---L
  // block 4 =   H---L---N
  // block 5 =   N---L---H
  // block 6 =   L---N---H
// int steps[6][3] = {
//   {0,1,-1},
//   {-1,1,0},
//   {1,-1,0},
//   {1,0,-1},
//   {-1,0,1},
//   {0,-1,1},
// };// abc clockwise
int steps[6][3] = {
  {1,-1,0},
    {-1,1,0},
      {0,1,-1},

      {0,-1,1},
      {-1,0,1},
      {1,0,-1},
};
//========================FUNCTION DECLARATIONS========================
void switchBlock(int phase, bool turnOn);
//========================SETUP========================
void setup() { 
  initialize();
}

//==================================LOOP=====================================
void loop() {
//  Serial.print("Phase A on (Green), ");
  for (int phase = 0; phase <3; phase++){
    switchBlock(phase, false); //turn off first
  }
  #ifdef onTimeRatio
  if(deadTime>=16383){
      delay(deadTime/1000); 
    }else{
      delayMicroseconds(deadTime);
    }
  for (int phase = 0; phase <3; phase++){
    switchBlock(phase,true); //turn off first
  }
  #endif
  if(onTime>=16383){
      delay(onTime/1000); 
  }else{
    delayMicroseconds(onTime);
  }

  blockNumber = (blockNumber +1)%6;

  if(micros()-lastTime > printPeriod){
    double rawValueNormalized= analogRead(pot)/1023.0;
    // rawValueNormalized= 0;
    printf("rawValueNormalized: %6.4f, ",rawValueNormalized);

    #ifndef byTime
    val = 1000000.0/((fMin+
      (fMax-fMin)*sqrt(rawValueNormalized)) 
      // linear scaling frequency with respect to potentiometer reading
      *electricalCycles*6); //3 is for the pole pair count per rotation
    // printf("diff*nomral : %f, ",(fMax-fMin)*rawValueNormalized);
    //turning towards negative--> longer delay for value --> slower spins
    #else
    //turning towards negative--> smaller delay for value --> faster spins
    val=(rawValueNormalized*1000.0*(periodMax-periodMin)+1000.0*periodMin)/
    // linear scaling period with respect to potentiometer reading
    (electricalCycles*6); //3 is for the pole pair count per rotation
    #endif  

    #ifdef onTimeRatio
    onTime= static_cast<float>(onTimeRatio/100)*val;
    onTime= (1.0-static_cast<float>(onTimeRatio/100))*val;
    #else
    onTime = val;
    #endif
      // printf("time delay/phase: %7.3f ms, RPM: %i, ",
      printf("RPM: %i, ",
                static_cast<int>(60.0*1e6/val/6.0/electricalCycles));
      //   onTime/1000.0,  
      //   static_cast<int>(60.0*1e6/onTime/3.0/eletricalCycles));
      //  printf("mu s pause time/phase: %7.3f, ", val);
        lastTime = micros();
        double f_pwm;
        #ifdef byTime
        f_pwm = 1000000.0/val;
        #else
         f_pwm = 1.0/(3*val)*1000000;
        #endif
         printf("f_pwm: %5.2f Hz,"
          //  2*f_pwm= %5.2f Hz, "
           ,f_pwm
          //  , 2*f_pwm
          );
      //  printf("µs : %lld, val: %lu \n", lastTime,val);
       printf("µs : %lld, blockNum: %i \n", lastTime,blockNumber);
       Serial.println();
    //val = map(analogRead(pot), 0, 1023, periodMin, periodMax);
  }
}
//
#ifdef onTimeRatio
//the items being set on /off is the switch itself, not the pins!
  void switchBlock(int phase, boolean turnOn){
    // given a phase phase = (A), 1,(B), 2(C), sets the gates to the correct permutation,
    // assuming blockNumber is set correctly
    // 
    volatile uint8_t* highPort;
    int highPortShift;
    volatile uint8_t* lowPort;
    int lowPortShift;
    //set high/lowPort and high/lowPortShift to correct values
    switch (phase){
      case 0: //phaseA
        *highPort =phaseAHighPort;
        highPortShift = shiftAHigh;
        *lowPort =phaseALowPort;
        lowPortShift = shiftALow;
        break;
      case 1: //phaseB
        *highPort =phaseBHighPort;
        highPortShift = shiftBHigh;
        *lowPort =phaseBLowPort;
        lowPortShift = shiftBLow;
        break;
      case 2://Phase C
        *highPort =phaseCHighPort;
        highPortShift= shiftCHigh;
        *lowPort =phaseCLowPort;
        lowPortShift= shiftCLow;
        break;
      default:
        Serial.println("\n INVALID STATE");
        break;
    }
  // set signal to turn off current mosfet that is on
    if(!turnOn){
      switch(steps[blockNumber][phase]){

        #ifdef invertedHighside 
          case -1: //set both off
            *highPort |= (1<<highPortShift); //==============
            *lowPort &= ~(1<<lowPortShift);
            break;
          case 0: //set low on, high off
            *highPort |= (1<<highPortShift); //==============
            // lowPort |= (1<<lowPortShift);
            break;
          case 1: //set low off, high on
            // highPort |= (1<<highPortShift);
            *lowPort &= ~(1<<lowPortShift);
          break;
        #else //default
        
          case -1: //set both off
            *highPort &= ~(1<<highPortShift);
            *lowPort &= ~(1<<lowPortShift);
            break;
          case 0: //set low on, high off
            *highPort &= ~(1<<highPortShift);
            // lowPort |= (1<<lowPortShift);
            break;
          case 1: //set low off, high on
            // highPort |= (1<<highPortShift);
            *lowPort &= ~(1<<lowPortShift);
            break;
        #endif

      }
    }else {
      switch(steps[blockNumber][phase]){

        #ifdef invertedHighside 
          case -1: //set both off
            break;
          case 0: //set low on, high off
            // highPort &= ~(1<<highPortShift);
            *lowPort |= (1<<lowPortShift);
            break;
          case 1: //set low off, high on
            *highPort &= ~(1<<highPortShift); //==============
            // lowPort &= ~(1<<lowPortShift);
            break;
        #else
        
          case -1: //set both off
            break;
          case 0: //set low on, high off
            // highPort &= ~(1<<highPortShift);
            *lowPort |= (1<<lowPortShift);
            break;
          case 1: //set low off, high on
            *highPort |= (1<<highPortShift);
            // lowPort &= ~(1<<lowPortShift);
            break;
        #endif
      }
    }
  }
#else
  void switchBlock(int phase, boolean turnOn){
    // given a phase phase = (A), 1,(B), 2(C), sets the gates to the correct permutation,
    // assuming blockNumber is set correctly
    volatile uint8_t* highPort;
    int highPortShift;
    volatile uint8_t* lowPort;
    int lowPortShift;
    //set high/lowPort and high/lowPortShift to correct values
    switch (phase){
      case 0: //phaseA
        highPort =&phaseAHighPort;
        highPortShift = shiftAHigh;
        lowPort =&phaseALowPort;
        lowPortShift = shiftALow;
        break;
      case 1: //phaseB
        highPort =&phaseBHighPort;
        highPortShift = shiftBHigh;
        lowPort =&phaseBLowPort;
        lowPortShift = shiftBLow;
        break;
      case 2://Phase C
        highPort =&phaseCHighPort;
        highPortShift= shiftCHigh;
        lowPort =&phaseCLowPort;
        lowPortShift= shiftCLow;
        break;
      default:
        Serial.println("\n INVALID STATE");
        break;
    }
    switch(steps[blockNumber][phase]){
      #ifdef invertedHighside
        case -1: //set both off
          *highPort |= (1<<highPortShift);
          *lowPort &= ~(1<<lowPortShift);
          break;
        case 0: //set low on, high off
          *highPort |= (1<<highPortShift);
          *lowPort |= (1<<lowPortShift);
          break;
        case 1: //set low off, high on
          *highPort &= ~(1<<highPortShift);
          *lowPort &= ~(1<<lowPortShift);
          break;
      #else //default and normal
          case -1: //set both off
            *highPort &= ~(1<<highPortShift);
            *lowPort &= ~(1<<lowPortShift);
            break;
          case 0: //set low on, high off
            *highPort &= ~(1<<highPortShift);
            *lowPort |= (1<<lowPortShift);
            break;
          case 1: //set low off, high on
            *highPort |= (1<<highPortShift);
            *lowPort &= ~(1<<lowPortShift);
            break;
      #endif
    }
}

#endif
#include <LibPrintf.h>
#include <Arduino.h>

/* Using a potentiometer hooked up to 5V to control
 a 3 phase DC motor with Arduino Uno and 3 step commuatation
 - Meant to be done with a basic circuit- one phase active at a time by connecting phase wire to Vcc, 
  and star point to ground
*/

//======Initializing values ======
//millisecond, per rotation
// /#define byTime
// #define onTimeRatio 90
#ifdef byTime
const int periodMin =50;
const int periodMax = 1000;
#else
//rot per second

const float fMin = 2.5;
const float fMax = 25.5;
#endif
const int eletricalCycles =3;
const long printPeriod = 2e5;
uint64_t lastTime = 0;
uint32_t val =0; 
uint32_t onTime =0; 
uint32_t deadTime =0; 
//how long to delay every phase per block of 3 step commuation, in microseconds

//======Pin definition ======
#define phaseA 7 //inverted
#define phaseB 6
#define phaseC 5
#define pot A0

//Keep undefined!
//#define test
//#define sineDacTest 

#ifndef sineDacTest
#if (phaseA >= 8 && phaseA <= 13)
  #define phaseAPort PORTB
  #define shiftA (phaseA - 8)
  #define phaseADDR DDRB
#elif (phaseA >= 0 && phaseA <= 7)
  #define phaseAPort PORTD
  #define shiftA phaseA
  #define phaseADDR DDRD
#endif

#if (phaseB >= 8 && phaseB <= 13)
  #define phaseBPort PORTB
    #define shiftB (phaseB - 8)
    #define phaseBDDR DDRB
#elif (phaseB >= 0 && phaseB <= 7)
  #define phaseBPort PORTD
  #define shiftB phaseB
  #define phaseBDDR DDRD
#endif

#if (phaseC >= 8 && phaseC <= 13)
  #define phaseCPort PORTB
    #define shiftC (phaseC - 8)
    #define phaseCDDR DDRB
#elif (phaseC >= 0 && phaseC <= 7)
  #define phaseCPort PORTD
    #define shiftC phaseC
    #define phaseCDDR DDRD
#endif
//========================SETUP========================
void setup() {
  pinMode(pot, INPUT_PULLUP);
  // pinMode(phaseA, OUTPUT);
  // pinMode(phaseB, OUTPUT);
  // pinMode(phaseC, OUTPUT);
  DDRD = B00000000;
  DDRB = B00000000;
  phaseADDR |= (B00000000 | 1 << shiftA);
  phaseBDDR  |= (B00000000 | 1 << shiftB);
  phaseCDDR  |= (B00000000 | 1 << shiftC);
    phaseAPort |= 1 << shiftA;
  Serial.begin(115200);
  val = 111;
  Serial.println("reset success ig");
  // put your setup code here, to run once:
  lastTime = millis();
  pinMode(9,OUTPUT);
  analogWrite(9,85);
}
//==================================LOOP=====================================
#ifndef test
void loop() {
//  Serial.print("Phase A on (Green), ");
phaseAPort &= ~(1 << shiftA);  //set LOW
  if(onTime>=16383){
    delay(onTime/1000); 
  }else{
    delayMicroseconds(onTime);
  }
  phaseAPort |= 1 << shiftA;
  #ifdef onTimeRatio
    if(deadTime>=16383){
      delay(deadTime/1000); 
    }else{
      delayMicroseconds(deadTime);
    }
  #endif  

//  Serial.print("Phase B on, ");
// phaseBPort |= 1 << shiftB;
phaseBPort &= ~(1 << shiftB);
if(onTime>=16383){
  delay(onTime/1000); 
}else{
  delayMicroseconds(onTime);
}
// phaseBPort &= ~(1 << shiftB);
phaseBPort |= 1 << shiftB;
#ifdef onTimeRatio
      if(deadTime>=16383){
      delay(deadTime/1000); 
    }else{
      delayMicroseconds(deadTime);
    }
  #endif  

  // Serial.println("Phase C on, Electric Cycle Done!");
  //  phaseCPort |= 1 << shiftC;
   phaseCPort &= ~(1 << shiftC);
   if(onTime>=16383){
     delay(onTime/1000); 
    }else{
      delayMicroseconds(onTime);
    }
    // phaseCPort &= ~(1 << shiftC);
    phaseCPort |= 1 << shiftC;
  #ifdef onTimeRatio
    if(deadTime>=16383){
      delay(deadTime/1000); 
    }else{
      delayMicroseconds(deadTime);
    }
  #endif  

  if(micros()-lastTime > printPeriod){
    double rawValueNormalized= analogRead(pot)/1023.0;
    // rawValueNormalized= 0;
    printf("rawValueNormalized: %6.4f, ",rawValueNormalized);
    #ifndef byTime
    val = 1000000.0/((fMin+
      (fMax-fMin)*rawValueNormalized) 
      // linear scaling frequency with respect to potentiometer reading
      *eletricalCycles*3); //3 is for the pole pair count per rotation
    // printf("diff*nomral : %f, ",(fMax-fMin)*rawValueNormalized);
    #else
      val=(rawValueNormalized*1000.0*(periodMax-periodMin)+1000.0*periodMin)/
      // linear scaling frequency with respect to potentiometer reading
      (eletricalCycles*3); //3 is for the pole pair count per rotation
    #endif  
      #ifdef onTimeRatio
      onTime= static_cast<float>(onTimeRatio/100)*val;
      onTime= (1.0-static_cast<float>(onTimeRatio/100))*val;
      #else
      onTime = val;
      #endif
      // printf("time delay/phase: %7.3f ms, RPM: %i, ",
      printf("RPM: %i, ",
                static_cast<int>(60.0*1e6/onTime/3.0/eletricalCycles));
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
       // printf("µs : %lld, val: %lu \n", lastTime,val);
       Serial.println();
    //val = map(analogRead(pot), 0, 1023, periodMin, periodMax);
  }
}
#else
void loop(){
   phaseAPort |= 1 << shiftA;
  delayMicroseconds(val);
}
#endif
#else
DacESP32 dac1(GPIO_NUM_22);
void setup(){
  dac1.outputCW(20000);
}
void loop(){
}
#endif

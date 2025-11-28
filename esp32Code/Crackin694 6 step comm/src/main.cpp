#include <LibPrintf.h>
#include <Arduino.h>

/* Using a potentiometer hooked up to 5V to control
 a 3 phase DC motor with Arduino Uno and 6 step commuatation
*/

//======Initializing values ======
//millisecond, per phase
const int periodMin = 10;
const int periodMax = 300;
//rot per second
const float fMin = 1;
const float fMax = 10;
const int eletricalCycles =3;
const long printPeriod = 2e5;
uint64_t lastTime = 0;
uint32_t val =0; 
//how long to delay every phase per block of 6 step commuation, in microseconds

//======Pin definition ======
#define phaseA 4
#define phaseB 5
#define phaseC 6
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
  Serial.begin(115200);
  val = 111;
  Serial.println("reset success ig");
  // put your setup code here, to run once:
  lastTime = millis();
}

#ifndef test
void loop() {
//  Serial.print("Phase A on (Green), ");
  phaseAPort |= 1 << shiftA;
  if(val>=16383){
    delay(val/1000); 
  }else{
    delayMicroseconds(val);
  }
  phaseAPort &= ~(1 << shiftA);

//  Serial.print("Phase B on, ");
   phaseBPort |= 1 << shiftB;
 if(val>=16383){
    delay(val/1000); 
  }else{
    delayMicroseconds(val);
  }
  phaseBPort &= ~(1 << shiftB);
  // Serial.println("Phase C on, Electric Cycle Done!");
   phaseCPort |= 1 << shiftC;
  if(val>=16383){
      delay(val/1000); 
    }else{
      delayMicroseconds(val);
  }
  phaseCPort &= ~(1 << shiftC);

  // long timeNow = millis();
  // Serial.print("timeNow:");
  // Serial.println(timeNow);
  //   Serial.print("LastTime:");
  // Serial.println(lastTime);
  // Serial.println("\n");
  if(micros()-lastTime > printPeriod){
    double rawValueNormalized= analogRead(pot)/1023.0;
    printf("analogRead rawValueNormalized: %6.4f, ",rawValueNormalized);
    val = 1000000.0/(fMin+
      (fMax-fMin)*rawValueNormalized // linear scaling frequency with respect to potentiometer reading
      *eletricalCycles*3);
   printf("ms pause time/phase: %7.3f, RPM: %i, ",val/1000.0,  static_cast<int>(60.0*1e6/val/3.0/eletricalCycles));
  //  printf("mu s pause time/phase: %7.3f, ", val);
   lastTime = micros();
   printf("µs : %lld \n", lastTime);
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

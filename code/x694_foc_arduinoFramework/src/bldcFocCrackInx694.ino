#include "constants.h"
#include "Wire.h"
#include <Arduino.h>
#include "Arduino.h"
// #include "parkClarkTestCases.h"

// TwoWire Wire;
void setup() {
  Wire = TwoWire(1);
  Wire.begin(17, 18); // initalize I2C
  Serial.begin(115200);
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);
  pinMode(shuntA, INPUT);
  pinMode(shuntB, INPUT);
  ledcAttach(shuntA, 115200, 1);
  ledcAttach(shuntB, 115200, 1); 
  delay(100);
}

void loop() {
  readShunts(); //edits phaseCurrents
  #ifdef CP_TEST_CASES
    phaseCurrent[0] = phaseA_test;
    phaseCurrent[1] = phaseB_test;
    phaseCurrent[2] = phaseC_test;
  #endif
  /* takes phaseVector[] (format: Ia, Ib, Ic) and sums it into summationVector[] (format: arg, dir) 
  */
  toClark();
  /* reads encoders and add 90 degrees.
      Takes summationVector and represents it in terms of encoder.
      Value stored in iPark[] in (radial, tangential)/ dq format
  */
   #ifdef CP_TEST_CASES
      printf("After Clark: Current (I)  Vector with size (A) %f and angle (degrees) %f \n",summationVector[0] ,summationVector[1] );
  #endif
  calculateElectricalAngle();
  #ifdef CP_TEST_CASES
      printf("Post Calculation Rotor angle (degrees) %f \n",electricalDegree );
  #endif
  /* takes in direction and electrical degree, outputs current vector dq frame (based 
  around encoder value) in iPark[] (format: (float) direct, (float) quadrature)
  */
  toPark();
  #ifdef CP_TEST_CASES
      printf("After Park: 'DC' Current (I)  with direct (float) %f and quadrature (float) %f \n ",iPark[0] ,iPark[1] );
  #endif
  /*negates the error vector (format: dq axis) and adds it to the summationVector[] (format: arg, dir)
  */ 
 
  inversePark(runPI(0, iPark[0], periodPWM),
                      runPI(1, iPark[1], periodPWM)); // set first paramter to maximize current
  #ifdef CP_TEST_CASES
      printf("After inversePlark: Current (I)  Vector with size (A) %f and angle (degrees) %f \n",summationVector[0] ,summationVector[1] );
  #endif
  /* Gets summationVector (FOMRAT: ALPHA BETA) and changed into updatedCurrentVectors[] (format Ia,Ib, Ic)
   */
  inverseClark(cos(summationVector[1])*summationVector[0], sin(summationVector[1])*summationVector[0]);
  #ifdef CP_TEST_CASES
      printf("After inverseClark: Phase A current: %f,  Phase B current: %f,  Phase C current: %f \n"
        , updatedCurrentTargets[0] ,updatedCurrentTargets[1],updatedCurrentTargets[2] );
  #endif
  delay(30000);
}
void toClark() {
  // iClark[i] = {x axis (ia), y axis}
  // turns 3 vector phase representation into 1 in (arg, dir)
  //  call after all phase currents are measured/calculated
  //RR aangle for matrix calc
  int x, y = 0;
  for (int i = 2; i >= 0; i--)
  { // i=2, 1 ,0
    x += cos(120 * i) * phaseCurrent[i];
    y += sin(120 * i) * phaseCurrent[i];
  }
  summationVector[0] = sqrt(pow(x, 2) + pow(y, 2));
  summationVector[1] = atan2f(y, x);
}

void calculateElectricalAngle(){
  //  taking and comparing (90 + electrical degree of rotor (ideal)) and phase current (actual)
  // read encoder value for rotor flux (electrical angle)

  if (readEncoder())
  {
    // sensor bits --> mchanical rotations. --> electrical
    // current rotorVector
    electricalDegree = (encoderVal % (4096 / (poles * coilsPerPhase))) // 4096/(poles*colsPerPhase) = per Electric cycle revolution
                       / (4096 / (poles * coilsPerPhase)) * 360;       // flux degrees 
    // calc 90 degrees ahead
    electricalDegree += direction * 90;
  }
}

void toPark() {
  // iPark[i] stores resulting values {q axis (what we want maximized), d axis (what we want minized)}

  // Compare rotor to current vector
  // the actual current vector will be taken as the hypotenuse, and the expected as the major leg
  float Θ = (summationVector[1] - electricalDegree) * direction; // angle in between
  // + ==> rotor is ahead, please slow down
  // - ==> rotor is behind, please speed down=
  iPark[0] = summationVector[1] * cos(Θ);
  iPark[1] = summationVector[1] * sin(Θ);
}
// Pi controller
float runPI(float target, float given, float time) {
  float error = given - target; // when given is above target, error is positive
  return (kpid[0] * error + kpid[1] * time * error + kpid[2] * time * time * error);
}

void inversePark(float d, float q) {
  // Given d and q axes, convert to A and B axis vector (fixed reference plane)
  summationVector[0] += sqrt(pow(d, 2) + pow(q, 2)); //reuse summation vector to store sum of calculated feedback vectors
  summationVector[1] += electricalDegree + direction * atan2(d, q); // want to approach 0
  //summationVector[0] gives magnitude now,
  //summationVector[1] gives direction now,
  
}

void inverseClark(float alpha, float beta) {
  updatedCurrentTargets[0]= 2.0*alpha/3;//A
  updatedCurrentTargets[1]= (beta*betaConstant)-alpha/3;//B
  updatedCurrentTargets[2]= (-beta*betaConstant)-alpha/3;//C
  //all values of array are the magnitudes of each phase vector
}
//===========================================================================================
void readShunts() {
  phaseCurrent[0] = (analogReadMilliVolts(shuntA)) / shuntResistance; //50 mOhms
  //+ --> drawing power from +
  phaseCurrent[1] = (analogReadMilliVolts(shuntB)) / shuntResistance;
  phaseCurrent[2] = -phaseCurrent[0] - phaseCurrent[1];
}
bool readEncoder() {
  // return true if read successfully
  int rawEncoderValue;
  Wire.beginTransmission(as5600);
  Wire.write(0x0C);
  Wire.endTransmission(false);
  int bytesRecieved = Wire.requestFrom(as5600, 2, true);
  if (Wire.available() == 2)
  { // read 2 bytes of registers
    rawEncoderValue = (Wire.read() << 8) | Wire.read();
  }
  else
  {
    return false;
  }
  encoderVal = rawEncoderValue;
  return true;
} 

#define betaConstant 0.57735027 //(sqrt3)/3
volatile float phaseCurrent[3];// [A,B,C], + means goes into the phase
// all phases denoted with capital (A,B,C)
float kpid[] = {1, 1, 0};                  // pid tuning constants
const int pinA = 12, pinB = 14, pinC = 27; // pin # of phase
// const int shuntAHigh = 13, shuntALow = 14, shuntBHigh = 15, shuntBLow = 16;
const int shuntA = 25, shuntB = 26;
// highside shunt has higher voltage when phase is receiving conventional current
// (lowside connects to star point)
const int shuntResistance = 50; //in milliohms
const int as5600 = 0x36; // i2c address
const int poles = 3;     //(pairs of magnets)
const int coilsPerPhase = 3;
    // frequency in Hz 
    const int frequencyPWM = 16000;
    const int frequencySensorLoop = 400000;
    //period in nanoseconds 
    const int periodPWM = static_cast<int>(1e9 / frequencyPWM) ;
    const int periodSensorLoop = static_cast<int>(1e9 / frequencySensorLoop) ;

//================================ runtime variables========================================
// B
//    \ 
//        ----  A (0) å
//    /
// C
volatile float updatedCurrentTargets[3];  // sin and cos of all angle vectors
volatile float summationVector[2]; // single vector in polar from clark transform [mag,dir]
volatile float iPark[2];           // 2 axis coordinates --> dq (min, max)
volatile int encoderVal = 0;
volatile float electricalDegree = 0;
volatile int direction = 1; //+1 = ccw (A->B->C) , -1 = cw (C->B->A)
//  
// View from back of motor
// C          A
// B          B
// A          C
//     C B A

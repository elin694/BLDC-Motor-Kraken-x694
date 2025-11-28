#include "Wire.h"
#include <Arduino.h>
#include "Arduino.h"
float kpid[] = {1,1,1};
const int pa = 10, pb =11, pc =12; //pin # of phase
const int saHigh = 13, saLow = 14, sbHigh= 15, sbLow = 16;
const int as5600 = 0x36;
const int poles = 3; //(pairs of magnets)
//high is bigger when coil is receiving current

//runtime variables
volatile float current[3];
volatile float iCLark[3][2];
volatile float iPark[2];
volatile float encoderVal =0;
bool cw =true;

TwoWire busA;
void setup() {
  busA = TwoWire(1);
  busA.begin(17,18);
  Serial.begin(115200);
  pinMode(pa,OUTPUT);
  pinMode(pb,OUTPUT);
  pinMode(pc,OUTPUT);
  pinMode(saHigh,INPUT);
  pinMode(saLow,INPUT);
  pinMode(sbHigh,INPUT);
  pinMode(sbLow,INPUT);
ledcAttach(saHigh,115200,1);

}

// C          A 
// B          B
// A          C
//     C B A
//     ____

void loop() {
  // put your main code here, to run repeatedly:


}

//Clark
void toClark(){
  //iClark[i] = {x axis (ia), y axis}
  for (i=2; i>=0;i--){
    iClark[i][0] = cos(120*i)*current[i]; 
    iClark[i][1] = sin(120*i)*current[i];
  }
}
//Park
void toPark(){
    //iPark[i] = {q axis (max), f axis (min)}
    //read encoder value for rotor flux (electrical angle)
    //calc 90 degrees ahead
    //
    if(readEncoder()){
      float eDegree = (encoderVal %(4096/poles))
        /(4096/poles *360);//flux degrees
    }

}

//inverse Clark
void invClark(){}
//inverse Park
void invPark(){}

void readShunts(){
  current[0] = (analogReadMilliVolts(saHigh)-analogRead(saLow))/50; 
  //+ --> drawing power
 current[1] = (analogReadMilliVolts(sbHigh)-analogRead(sbLow))/50; 
  current[2] =  -current[0]-current[1];
}

bool readEncoder(){
  //return true if read successfully
  int rawEncoderValue;
  busA.beginTransmission(as5600);
  busA.write(0x0C);
  busA.endTransmission(false);
  int bytesRecieved = busA.requestFrom(as5600,2,true);
  if (busA.available() == 2){
    rawEncoderValue = (busA.read() << 8) | busA.read();
  }else {
    return false;
  }
  encoderVal = rawEncoderValue;
  return true;

}
//Pi
float runPI(float error, float time){
  return kpid[0]*error 
    + kpid[1]*time*error;
}

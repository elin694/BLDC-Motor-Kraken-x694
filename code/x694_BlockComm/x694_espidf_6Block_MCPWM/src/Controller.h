#pragma once
#include "Constants.h"
// #define cBufSize       /*For storing measured/calculated motor values*/

typedef struct{
    float kp = 0;
    float ki = 0;
    float kd = 0;
    int target = 0;
    std::atomic<uint32_t> mindex = 0;
    int measured[cBufSize] ;
    float netError = 0;                             /*for the area ∫v(t)dt (m position)*/
    std::atomic<uint32_t> eindex = 0;
    int lastError[cBufSize];                    /*for dx/dt (m position)*/
} int_kpid;

typedef struct{
    float kp = 0;
    float ki = 0;
    float kd = 0;
    float target = 0;
    std::atomic<uint32_t> mindex = 0;
    float measured[cBufSize];
    float netError = 0;                             /*for the area ∫a(t)dt (m velocity)*/
    std::atomic<uint32_t> eindex = 0; 
    float lastError[cBufSize];                  /*for dv/dt (m velocity)*/
} float_kpid;

int_kpid position ={
};

float_kpid velocity={
};

float_kpid torque ={
};
constexpr float kPID[3][3] = {
    { 1, 1, 1 }, /*Position*/
    { 1.1, 0,0 }, /*Velocity {kp, ki, kd}*/
    { 1, 1, 1 } /*Acceleration*/
};


void mathItOut(void * startTick4); //updates arrrays with new ifo
void setTorque(float targetTorque);
void setVelocity(float targetVelocity);
void setPosition(float targetPosition);

#pragma once
#include "Constants.h"
// #define cBufSize       /*For storing measured/calculated motor values*/
typedef struct{
   int target = 0;
    std::atomic<uint32_t> mindex = 0;
    int measured[cBufSize] ;
    float netError = 0;                             /*for the area ∫v(t)dt (m position)*/
    std::atomic<uint32_t> eindex = 0;
    int lastError[cBufSize];                    /*for dx/dt (m position)*/
    float kp = 0;
    float ki = 0;
    float kd = 0;
    /*------------------------------POWERED BY GPTIMER------------------------------*/
    gptimer_handle_t timer;
    gptimer_config_t timerConfig;
    gptimer_alarm_config_t alarmConfig;
    gptimer_event_callbacks_t callbackEvent;
} int_kpid;
typedef struct{
    std::atomic<uint32_t> mindex = 0;
    float measured[cBufSize];
    float netError = 0;                             /*for the area ∫a(t)dt (m velocity)*/
    std::atomic<uint32_t> eindex = 0; 
    float lastError[cBufSize];                  /*for dv/dt (m velocity)*/
    float kp = 0;
    float ki = 0;
    float kd = 0;
    float target = 0;
    /*------------------------------POWERED BY GPTIMER------------------------------*/
    gptimer_handle_t timer = NULL;
    gptimer_config_t timerConfig = NULL;
    gptimer_alarm_config_t alarmConfig = NULL;
    gptimer_event_callbacks_t callbackEvent = NULL;
} float_kpid;

int_kpid position ={
   .kp = 0,
   .ki = 0,
   .kd = 0,
   .target = 0,
};
float_kpid velocity={
   .kp = 0,
   .ki = 0,
   .kd = 0,
   .target = 0,
};
float_kpid torque ={
   .kp = 0,
   .ki = 0,
   .kd = 0,
   .target = 0
};

gptimer_config_t megaTimerSetup = {
   .clk_src = GPTIMER_CLK_SRC_DEFAULT,
   .direction = GPTIMER_COUNT_UP,
   //    .resolution_hz = MEGA_CLOCK_SPEED,
   // .intr_priority = MEGA_TIMER_INTR_PRIORITY,
   .flags = {
      .intr_shared = false,
      .allow_pd = false
   }
};
// #define ALARM_VAL (uint64_t)(MEGA_CLOCK_SPEED*(estimatedI2CReadTime_us/(1.0e6)))
// static_assert(ALARM_VAL <= 2.0);
gptimer_alarm_config_t megaTimerAlarmSetup = {
   // .alarm_count = ALARM_VAL,
   .reload_count =0,
   .flags = {
      .auto_reload_on_alarm = true
   }
};
gptimer_event_callbacks_t megaTimerCallback ={
   .on_alarm = NULL
};




void mathItOut(void * startTick4); //updates arrrays with new ifo
void setTorque(float* targetTorque);
void setVelocity(float* targetVelocity);
void setPosition(int* targetPosition);

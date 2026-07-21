#pragma once
#include "Constants.h"
// #define CL_CIRCULAR_SLOTS       /*For storing measured/calculated motor values*/


#define CL_TIMER_FREQ_HZ (uint32_t) (8e7/2)
constexpr gptimer_config_t timerConfigScaffold = {
   .clk_src = GPTIMER_CLK_SRC_DEFAULT,
   .direction = GPTIMER_COUNT_UP,
   .resolution_hz = CL_TIMER_FREQ_HZ,
   // .intr_priority = MEGA_TIMER_INTR_PRIORITY,
   .flags = {
      .intr_shared = false,
      .allow_pd = false
   }
};
// #define ALARM_VAL (uint64_t)(MEGA_CLOCK_SPEED*(estimatedI2CReadTime_us/(1.0e6)))
// static_assert(ALARM_VAL <= 2.0);
constexpr gptimer_alarm_config_t alarmScaffold = {
   // .alarm_count = ALARM_VAL,
   .reload_count =0,
   .flags = {
      .auto_reload_on_alarm = true
   }
};
constexpr gptimer_event_callbacks_t eventScaffold ={
   // .on_alarm = NULL
};
typedef struct{
   // float target = 0;
   std::atomic<uint32_t> mindex = 0;
   float measured[CL_CIRCULAR_SLOTS];
   float netError = 0;                             /*for the area ∫a(t)dt (m velocity)*/
   std::atomic<uint32_t> eindex = 0; 
   float lastError[CL_CIRCULAR_SLOTS];                  /*for dv/dt (m velocity)*/
   bool overIntegration = false;
   const float kp;
   const float ki;
   const float kd;
   /*------------------------------POWERED BY GPTIMER------------------------------*/
   int freq =0;
   gptimer_handle_t timer;
   gptimer_config_t timerConfig = timerConfigScaffold;
   gptimer_alarm_config_t alarmConfig = alarmScaffold;
   gptimer_event_callbacks_t callbackEvent = eventScaffold;
} float_kpid;
typedef struct{
   // int target = 0;
   std::atomic<uint32_t> mindex = 0;
   int measured[CL_CIRCULAR_SLOTS] ;
   float netError = 0;                             /*for the area ∫v(t)dt (m position)*/
   std::atomic<uint32_t> eindex = 0;
   int lastError[CL_CIRCULAR_SLOTS];                    /*for dx/dt (m position)*/
   bool overIntegration = false;
   const float kp;
   const float ki;
   const float kd;
   /*------------------------------POWERED BY GPTIMER------------------------------*/
   int freq =0;
   gptimer_handle_t timer;
   gptimer_config_t timerConfig = timerConfigScaffold;
   gptimer_alarm_config_t alarmConfig = alarmScaffold;
   gptimer_event_callbacks_t callbackEvent = eventScaffold;
} int_kpid;


inline DRAM_ATTR float_kpid torqueLoop ={
   .kp = 0,
   .ki = 0,
   .kd = 0
};
inline DRAM_ATTR float_kpid velocityLoop ={
   .kp = 0,
   .ki = 0,
   .kd = 0,
};
inline DRAM_ATTR int_kpid positionLoop ={
   .kp = 0,
   .ki = 0,
   .kd = 0
};



void stableLoopCheck(void * startTick4); //updates arrrays with new ifo
void torqueControlLoop(void* pointerToTargetTorque);
void velocityControlLoop(void* pointerToTargetTorque);
void positionControlLoop(void* pointerToTargetTorque);
bool torqueCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
bool velocityCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
bool positionCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
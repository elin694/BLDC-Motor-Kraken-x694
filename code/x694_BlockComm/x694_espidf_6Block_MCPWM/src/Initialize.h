#pragma once
#include "Globals.h"
#include "driver/i2c_master.h"
#include "esp_intr_alloc.h"


#define as5600Address 0x36
void pinSetup();
void as5600initialize(void* parameter);
void startAllTimersAndInterrupts(void * startTick6); 
extern void mcpwmSetup ();
/* #################### LOOPED FUCNTIONS #################### */
#ifdef useGPTimerOverESP32Timer
bool runOnMegaTimerIntr (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
#else
void runOnESPTimerIntr(void * globe);
#endif
extern void torqueControlLoop(void* pointerToTargetTorque);
extern void velocityControlLoop(void* pointerToTargetTorque);
extern void positionControlLoop(void* pointerToTargetTorque);
extern void executeGates (void * parameter);
bool runActualISR(void * data);
void debugMonitor(void * parameter);
void getSectorNumber(void *returnValue);


inline DRAM_ATTR mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };
/* #################### I2C #################### */
inline i2c_master_bus_config_t busSetup = { 
    .i2c_port = -1,
    .sda_io_num= dataPin,
    .scl_io_num= clockPin,
    .clk_source = I2C_CLK_SRC_APB,
    .glitch_ignore_cnt= 7,
    .intr_priority=i2c_intrPriority,
   //  .trans_queue_depth =2,
    .flags={
      .enable_internal_pullup = true,
      // .allow_pd =true
   }
};
inline DRAM_ATTR i2c_master_bus_handle_t busHandle;

constexpr i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= i2cClockSpeed, //need fast enough  to avoid invalid state
   .scl_wait_us = 50,
   .flags = {.disable_ack_check = false}
};
inline DRAM_ATTR i2c_master_dev_handle_t as5600Handle;
constexpr DRAM_ATTR uint8_t as5600TargetRegister = 0x0e;
inline DRAM_ATTR uint8_t as5600RawDataBuf[2] = {0x0,0x0};
#define as5600WriteSize 1
#define as5600ReadSize  2

// #define fth_sf_set_mask (0b00000000 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_set_mask (0b00011100 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_clear_mask (0b11000000) // Bit pos 5 (0 index) Watchdog off - don't save power
//The watchdog timer allows saving power by switching into LMP3 if the angle stays within the watchdog threshold of 4 LSB for at least one minute

#define power_set_mask (0x00000000)  //with |
#define power_clear_mask (~(0x00000011)) //with &
uint8_t fthRegisterData[2] = {0x00, 0x00};
uint8_t fthRegister[3] = {0x07, 0x00, 0x00}; //stores data/ registers to write to
/* #################### ADC #################### */

constexpr adc_oneshot_unit_init_cfg_t adcSetup= {
   .unit_id = ADC_UNIT_1,
   .clk_src =ADC_RTC_CLK_SRC_DEFAULT,
   .ulp_mode = ADC_ULP_MODE_DISABLE,
};
constexpr adc_oneshot_chan_cfg_t adcChannelSetup = {
   .atten =  ADC_ATTEN_DB_12,
   .bitwidth = ADC_BITWIDTH_12,
};
constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR1 = { 
   .timer0_tez_int_clr =1,
};
// constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR2 = { 
//    .timer1_tez_int_clr =1,
// };
// constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR3 = { 
//    .timer2_tez_int_clr =1
// };

/* #################### GPTIMER AND ESP_TIMER #################### */
#ifdef useGPTimerOverESP32Timer
#define MEGA_CLOCK_SPEED (20000000)
gptimer_handle_t megaTimer;
gptimer_config_t megaTimerSetup = {
   .clk_src = GPTIMER_CLK_SRC_DEFAULT,
   .direction = GPTIMER_COUNT_UP,
   .resolution_hz = MEGA_CLOCK_SPEED,
   .intr_priority = MEGA_TIMER_INTR_PRIORITY,
   .flags = {
      .intr_shared = false,
      .allow_pd = false
   }
};

#define ALARM_VAL (uint64_t)(MEGA_CLOCK_SPEED*(estimatedI2CReadTime_us/(1.0e6)))
// static_assert(ALARM_VAL <= 2.0);
gptimer_alarm_config_t megaTimerAlarmSetup = {
   // .alarm_count = (uint64_t)(MEGA_CLOCK_SPEED*(0.0002f)),
   .alarm_count = ALARM_VAL,
   .reload_count =0,
   .flags = {
      .auto_reload_on_alarm = true
   }
};
gptimer_event_callbacks_t megaTimerCallback ={
   .on_alarm = runOnMegaTimerIntr
};
#else
esp_timer_handle_t gsnTimerHandle;
esp_timer_create_args_t gsnTimerSetup= {
   .callback=runOnESPTimerIntr,
   .arg =(void*) &global,
   .dispatch_method=ESP_TIMER_ISR,
   .name= "i2ctimer",
   .skip_unhandled_events = true
   
}; 
#endif

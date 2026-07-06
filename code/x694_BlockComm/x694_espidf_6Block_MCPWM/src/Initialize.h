#pragma once
#define as5600Address 0x36
#include "Globals.h"
#include "driver/i2c_master.h"
#define MCPWMx ((mcpwm_dev_t * )&MCPWM0)

void pinSetup();
void initializeGPIO();

void as5600initialize();

void runOnESPTimerIntr(void * globe);
void runOnMCPWMIntr(void *returnValue);
void getSectorNumber(void *returnValue);
void debugLog(void * parameter);
int mod6(int value);
void mathItOut(void *parameter);

intr_handle_t oneBlockISR = NULL;

inline DRAM_ATTR mcpwm_int_st_reg_t tempStatusReg = { .val = (MCPWMx)->int_st.val };
//+++++++++++++++++++++++++++++++++++I2C+++++++++++++++++++++++++++++++++++
inline i2c_master_bus_config_t busSetup = { 
    .i2c_port = -1,
    .sda_io_num= dataPin,
    .scl_io_num= clockPin,
    .clk_source = I2C_CLK_SRC_APB,
    // .glitch_ignore_cnt = 7,
    // .intr_priority = 1,
    .flags={.enable_internal_pullup = true}
};
inline i2c_master_bus_handle_t busHandle;

constexpr i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= i2cClockSpeed, //need fast enough  to avoid invalid state
   .scl_wait_us = 50,
   .flags = {.disable_ack_check = false}
};
inline i2c_master_dev_handle_t as5600Handle;

constexpr DRAM_ATTR uint8_t as5600Set = 0x36;
constexpr DRAM_ATTR uint8_t as5600TargetRegister = 0x0e;
#define as5600WriteSize 1
inline uint8_t as5600RawDataBuf[2] = {0x0,0x0};
#define as5600ReadSize  2

#define fth_sf_set_mask (0b00011100 | 0b00000011) //.5 bit error at 11 =sf
uint8_t fthRegisterData[1] = {0x00};
uint8_t fthRegister[2] = {0x07, 0x00};
//==================+++++++ADC AND MCPWM CLEAR REG

constexpr adc_oneshot_unit_init_cfg_t adcSetup= {
   .unit_id = ADC_UNIT_1,
   .ulp_mode = ADC_ULP_MODE_DISABLE,
};
constexpr adc_oneshot_chan_cfg_t adcChannelSetup = {
   .atten =  ADC_ATTEN_DB_12,
   .bitwidth = ADC_BITWIDTH_12,
};
constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR1 = { 
   .timer0_tez_int_clr =1,
};
constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR2 = { 
   .timer1_tez_int_clr =1,
   // .timer1_tep_int_clr =1,
   // .op0_tea_int_clr = 1,
   // .op0_teb_int_clr = 1
};
constexpr DRAM_ATTR inline mcpwm_int_clr_reg_t tempClearR3 = { 
   .timer2_tez_int_clr =1
};
//
esp_timer_handle_t gsnTimerHandle;
esp_timer_create_args_t gsnTimerSetup= {
   .callback=runOnESPTimerIntr,
   .arg =(void*)&global,
   .dispatch_method=ESP_TIMER_ISR,
   .name= "i2ctimer",
   .skip_unhandled_events = false
   
}; 
   // esp_timer_cb_t callback;        //!< Callback function to execute when timer expires
   //  void* arg;                      //!< Argument to pass to callback
   //  esp_timer_dispatch_t dispatch_method;   //!< Dispatch callback from task or ISR; if not specified, esp_timer task
   //  //                                !< is used; for ISR to work, also set Kconfig option
   //  //                                !< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
   //  const char* name;               //!< Timer name, used in esp_timer_dump() function
   //  bool skip_unhandled_events;   
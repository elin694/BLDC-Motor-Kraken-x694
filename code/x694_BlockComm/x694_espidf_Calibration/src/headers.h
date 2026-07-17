#pragma once
#include "pins.h"

//================== #INSTALL MCPWM ==================
#define generatorGPIO phaseCHighPort //tx2 = bh= 17
#define phaseLowGate phaseALowPort //outwards
#define countingFrequency (4e6) //2432
#define timerPeriod (countingFrequency/20000)
#define dutyCycle (float)(1-(.85))
#define i2cReadPeriod 200

constexpr int id =  1;
mcpwm_timer_config_t timerSetup = {
    .group_id = id,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = static_cast<uint32_t>(countingFrequency),
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =static_cast<uint32_t>(timerPeriod),//
    .intr_priority =1,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0, //these 2 determine when set_period takes effect
        // .allow_pd =true
    }
}; 
static mcpwm_timer_handle_t timerHandle;

mcpwm_generator_config_t genSetup = {
    .gen_gpio_num = generatorGPIO,
    .flags = {
        .invert_pwm = false,
    }
};
mcpwm_gen_handle_t genHandle;

mcpwm_operator_config_t operatorSetup = {
    .group_id = id,
    // .intr_priority = 1,
    .flags = {
        .update_gen_action_on_tez = 1,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 0,
        .update_dead_time_on_tez = 1,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 0,
    },
};
mcpwm_oper_handle_t operatorHandle;

const mcpwm_comparator_config_t comparatorSetup = {
    .intr_priority = 1,
    .flags ={
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};
mcpwm_cmpr_handle_t comparatorHandle;

#define isrTickDeadTime 0
const mcpwm_dead_time_config_t highGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};
uint32_t compareValue = dutyCycle*.5*timerPeriod;
//=======================================I2C=====================================
#define as5600 0x36
constexpr DRAM_ATTR uint8_t as5600TargetRegister = 0x0e;
inline uint8_t as5600RawDataBuf[2] = {0x0,0x0};
// #define fth_sf_set_mask (0b00000000 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_set_mask (0b00011100 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_clear_mask (0b11000000) // Bit pos 5 (0 index) Watchdog off - don't save power

uint8_t fthRegisterData[1] = {0x00};
uint8_t fthRegister[2] = {0x07, 0x00};

i2c_master_bus_config_t master_config = {
    .i2c_port = -1,
    .sda_io_num = DATA,
    .scl_io_num = CLOCK,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority=3,
    // .trans_queue_depth =10,
    .flags = {
        .enable_internal_pullup = true,
        // .allow_pd =true
    }
};
i2c_master_bus_handle_t bus_handle;  

i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = as5600,   
    .scl_speed_hz = 1000000,
    .scl_wait_us =10,
    .flags = {
        .disable_ack_check = false
    }
};
i2c_master_dev_handle_t dev_handle;

constexpr uint8_t write_buffer = 0x0e;
inline uint8_t read_buffer[2];
#define data_length 2
//=====================================ESP_TIMER==================================

esp_timer_create_args_t etimerSetup ={
    .callback = cbk,
    .arg=NULL,
    .dispatch_method = ESP_TIMER_ISR,
    .name = "i2ctimer",
    .skip_unhandled_events = true
};

esp_timer_create_args_t padTimerSetup ={
    .callback = cbk,
    .arg=NULL,
    .dispatch_method = ESP_TIMER_ISR,
    .name = "i2cPadTimer",
    .skip_unhandled_events = true
};

#define LATENCY pdMS_TO_TICKS(30)
//=======================================HANDLES=====================================
esp_timer_handle_t etimerHandle;
esp_timer_handle_t padTimerHandle;

TaskHandle_t initializeI2CTask;
TaskHandle_t debugTask;
TaskHandle_t readTask;
TaskHandle_t setupTask;


#pragma once
#include "pins.h"

//================== #INSTALL MCPWM ==================
#define generatorGPIO phaseCHighPort //tx2 = bh= 17
#define phaseLowGate phaseALowPort //outwards
#define HighTimerResolution (4e6) //2432
#define activePwmPeriod (uint32_t)(HighTimerResolution/20000)  //change to 20khz when high
#define startingDuty (0.8) 
#define estimatedI2CReadTime_us 200

#define highSideGroup 1 
#define MCPWM_HighsideIntrPriority 1




constexpr int id =  1;
mcpwm_timer_config_t HTimerSetup = {
    .group_id = highSideGroup,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = (uint32_t)(HighTimerResolution),
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =(uint32_t)(activePwmPeriod),//
    .intr_priority =MCPWM_HighsideIntrPriority,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0, //these 2 determine when set_period takes effect
        // .allow_pd =true
    }
}; 

mcpwm_operator_config_t HOperatorSetup = {
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

inline mcpwm_comparator_config_t HComparatorSetup = {
    .intr_priority = MCPWM_HighsideIntrPriority,
    .flags ={
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};

inline mcpwm_generator_config_t HPWMSetup = {
    .flags = {
        .invert_pwm = false
    }
};

typedef struct {
    mcpwm_timer_config_t timerConfig;
    mcpwm_operator_config_t opConfig;
    mcpwm_comparator_config_t compConfig = HComparatorSetup;
    mcpwm_generator_config_t pwmConfig = HPWMSetup;

    mcpwm_timer_handle_t timer = NULL;
    mcpwm_oper_handle_t operatorModule= NULL;
    mcpwm_cmpr_handle_t comparator0 = NULL;
    mcpwm_cmpr_handle_t comparator1 = NULL; //null for high
    mcpwm_gen_handle_t pwmGate0 = NULL;
    mcpwm_gen_handle_t pwmGate1 = NULL;// stays null
    //shoutout gemini for suggest changing countval
} phaseMcpwm;
phaseMcpwm motorH[3];

#define isrTickDeadTime 0
const mcpwm_dead_time_config_t highGateDeadTimeSetup = {
    .posedge_delay_ticks = isrTickDeadTime,
    .negedge_delay_ticks = isrTickDeadTime,
    .flags = {
        // invert_output = 1;
    }
};

#define startingGateCmpValue (uint32_t)((1-startingDuty)*activePwmPeriod/2.0) //High gate comparator's comparatorValue when ON; can be modified later
//=======================================I2C=====================================
#define as5600Address 0x36
constexpr DRAM_ATTR uint8_t as5600TargetRegister = 0x0e;
inline uint8_t as5600RawDataBuf[2] = {0x0,0x0};
// #define fth_sf_set_mask (0b00000000 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_set_mask (0b00011100 | 0b00000011) //.5 bit error at 11 =sf
#define fth_sf_clear_mask (0b11000000) // Bit pos 5 (0 index) Watchdog off - don't save power

uint8_t fthRegisterData[1] = {0x00};
uint8_t fthRegister[2] = {0x07, 0x00};

i2c_master_bus_config_t master_config = {
    .i2c_port = -1,
    .sda_io_num = dataPin,
    .scl_io_num = clockPin,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority=3,
    // .trans_queue_depth =10,
    .flags = {
        .enable_internal_pullup = true,
        // .allow_pd =true
    }
};
i2c_master_bus_handle_t busHandle;  

i2c_device_config_t as5600Setup = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = as5600Address,   
    .scl_speed_hz = 1000000,
    .scl_wait_us =10,
    .flags = {
        .disable_ack_check = false
    }
};
i2c_master_dev_handle_t as5600Handle;

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
TaskHandle_t getSectorNumberTask;
TaskHandle_t setupTask;


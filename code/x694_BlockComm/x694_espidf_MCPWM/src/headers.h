#pragma once
#include "soc/mcpwm_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <iostream>
#include "rom/ets_sys.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <esp_rom_sys.h> 
#include "driver/i2c_master.h"
#include "driver/mcpwm_prelude.h"
#include "ANSI.h"


//PORTS
#define phaseAHighPort GPIO_NUM_33
#define phaseALowPort GPIO_NUM_14

#define phaseBHighPort GPIO_NUM_17
#define phaseBLowPort GPIO_NUM_16

#define phaseCHighPort GPIO_NUM_26
#define phaseCLowPort GPIO_NUM_32

#define freePort1 GPIO_NUM_18
#define freePort2 GPIO_NUM_19

#define CLOCK GPIO_NUM_22
#define DATA GPIO_NUM_21
#define as5600 0x36

const uint8_t write_buffer = 0x0e;
uint8_t read_buffer[2];
#define data_length 2
int16_t angle = 0;
void cbk(void * parameter);

//================== #INSTALL MASTER BUS AND DEVICE ==================
i2c_master_bus_config_t master_config = {
    .i2c_port = -1,
    .sda_io_num = DATA,
    .scl_io_num = CLOCK,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags = {.enable_internal_pullup = true},
};
i2c_master_bus_handle_t bus_handle;  

i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = as5600,   
    .scl_speed_hz = 1000000,
    .scl_wait_us =50,
    .flags = {
        .disable_ack_check = false
    }
};
i2c_master_dev_handle_t dev_handle;

//================== #INSTALL MCPWM ==================
constexpr int id =  0;
static mcpwm_timer_handle_t timerHandle;
//Register Timer Event Callbacks

mcpwm_operator_config_t operatorSetup = {
    .group_id = id,
    // .intr_priority = 0,
    .flags = {
        .update_gen_action_on_tez = 1,
        .update_gen_action_on_tep = 0,
        .update_gen_action_on_sync= 0,
        .update_dead_time_on_tez = 0,
        .update_dead_time_on_tep = 0,
        .update_dead_time_on_sync = 0,
    },
};
mcpwm_oper_handle_t operatorHandle;

const mcpwm_comparator_config_t comparatorSetup = {
    .intr_priority = 0,
    .flags ={
        .update_cmp_on_tez = 1,
        .update_cmp_on_tep = 0,
        .update_cmp_on_sync = 0
    }
};
mcpwm_cmpr_handle_t comparatorHandle;



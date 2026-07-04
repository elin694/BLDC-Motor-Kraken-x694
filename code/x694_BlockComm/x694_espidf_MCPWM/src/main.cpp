//CAILBRATE AS5600 MAGNET
#include "headers.h"

#define generatorGPIO phaseCHighPort //tx2 = bh= 17
#define phaseLowGate phaseALowPort //outwards

// #define countingFrequency (1048576*64)
#define countingFrequency (4e6) //2432

// #define timerPeriod countingFrequency/20000 //2e16
// #define timerPeriod 4000 //2e16
#define timerPeriod (countingFrequency/20000)
#define dutyCycle (float)(1-(.8))

uint32_t compareValue = dutyCycle*.5*timerPeriod;

void groundSetup(){
    gpio_num_t gateArray[8]= {
      phaseAHighPort,
      phaseALowPort,
      phaseBHighPort,
      phaseBLowPort,
      phaseCHighPort,
      phaseCLowPort,
      CLOCK,
      DATA
   };
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      ets_delay_us(10);
      gpio_set_pull_mode(gate, GPIO_PULLDOWN_ONLY);
      gpio_set_level(gate, 0);
   }
    gpio_reset_pin(phaseLowGate);
    gpio_set_direction(static_cast<gpio_num_t>(phaseLowGate),GPIO_MODE_OUTPUT);
    gpio_set_level(phaseLowGate, 1);
}

mcpwm_timer_config_t timerSetup = {
    .group_id = id,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = static_cast<uint32_t>(countingFrequency),
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    .period_ticks =static_cast<uint32_t>(timerPeriod),//
    // .intr_priority = 1,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0 //these 2 determine when set_period takes effect
    }
};

mcpwm_generator_config_t genSetup = {
    .gen_gpio_num = generatorGPIO,
    .flags = {
        .invert_pwm = false,
        .io_loop_back = 0,
        .io_od_mode = 0, //pull low or float only
        .pull_up = 0,
        .pull_down= 1
    }
};
mcpwm_gen_handle_t genHandle;

void setupMCPWM(){
    ets_delay_us(100);
    groundSetup();
    ESP_LOGE("DEBUG2easd", "period ticks %d", timerSetup.period_ticks);
    ESP_LOGE("DEBUG2easd", "resol: %d", timerSetup.resolution_hz);
    ESP_LOGE("DEBUG", "cmpvalue: %d", compareValue);
    ESP_ERROR_CHECK(mcpwm_new_timer(&timerSetup, &timerHandle));
    // int g_prescale =100; //gives current toal rpescaler
    // MCPWM0.clk_cfg.clk_prescale = g_prescale-1;
    // MCPWM0.timer[0].timer_cfg0.timer_prescale= (16e7/countingFrequency)*5-1;
    ESP_ERROR_CHECK(mcpwm_new_operator(&operatorSetup, &operatorHandle));
    ESP_LOGE("DEBUG2easd", "cmpvalue ");
    ESP_ERROR_CHECK(mcpwm_new_comparator(operatorHandle, &comparatorSetup, &comparatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_generator(operatorHandle, &genSetup, &genHandle));

/*I@C*/
 printf("loop start \n \n \n \n \n \n \n \n ");
        ESP_ERROR_CHECK(i2c_new_master_bus(&master_config, &bus_handle));
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}
uint16_t pastAngle =0;
    uint16_t counter =0;
extern "C" {
    void app_main() {
        setupMCPWM();
        /*RUNTIME FUNCTIONS*/
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(genHandle, 0, true)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operatorHandle, timerHandle)); //--
        ESP_LOGE("DEBUG", "cmpvalue: %d", compareValue);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparatorHandle,compareValue)); 

        
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(genHandle,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                comparatorHandle,
                MCPWM_GEN_ACTION_HIGH
            ),
              MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_DOWN,
                comparatorHandle,
                MCPWM_GEN_ACTION_LOW
            ),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        ));
        ESP_ERROR_CHECK(mcpwm_timer_enable(timerHandle));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timerHandle, MCPWM_TIMER_START_NO_STOP));
        int bt1= 0;
        for(;;){
            /*
             bt1 = MCPWM1.timer[0].timer_status.timer_value;
            esp_rom_printf("%d\n", bt1);
            */
            counter= (counter+1)%4096;
            ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &write_buffer, 1, read_buffer, data_length, 500));
            angle = (( read_buffer[0] << 8) | read_buffer[1]);
            // if(abs(pastAngle angle)<2){
                printf("recieved info: %4d, tick = %7d \n ", angle, counter);
                pastAngle= angle;
            // }
            vTaskDelay(pdMS_TO_TICKS(80));
        }       
    } 
} 

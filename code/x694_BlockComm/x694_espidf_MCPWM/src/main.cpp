//CAILBRATE AS5600 MAGNET
#include "headers.h"

#define generatorGPIO phaseAHighPort //tx2 = bh= 17
#define phaseLowGate phaseCLowPort //outwards

#define countingFrequency (4e6) //2432
#define timerPeriod (countingFrequency/20000)
#define dutyCycle (float)(1-(.9))

uint32_t compareValue = dutyCycle*.5*timerPeriod;
esp_timer_handle_t etimerHandle;
esp_timer_handle_t padTimerHandle;
esp_timer_create_args_t padTimerSetup ={
    .callback = cbk,
    .arg=NULL,
    .dispatch_method = ESP_TIMER_ISR,
    .name = "i2cPadTimer",
    .skip_unhandled_events = true
};

TaskHandle_t readTask;
#define gpt 
#ifdef gpt 
void read(void*parameter){
    // uint32_t pastAngle =0;
    uint32_t counter =0;
    uint32_t failCounter =0;
    uint32_t printCounter=0;

    uint32_t timer =0;
    // bool timerFlag =false;
std::atomic<bool> timerFlag =false;
    for(;;){
        // esp_timer_start_once(etimerHandle,250);
        // uint32_t file1 =ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
        timer = esp_cpu_get_cycle_count();
        if((counter++ %256)==0){
            timerFlag =true;//trouble
        }
        esp_err_t result = i2c_master_transmit_receive(dev_handle, &write_buffer, 1, read_buffer, data_length, 1);
        // uint32_t i2cReadDuration =esp_cpu_get_cycle_count() -timer;
        // if(timerFlag.exchange(false)){
        //     esp_rom_printf("us:%d\n",i2cReadDuration);
        //     // esp_rom_printf("Pos:%4d|T:%6d|F:%d|us:%d\n", angle, counter,failCounter,timer);
        // }
        if(result ==ESP_OK){
            angle = (( read_buffer[0] << 8) | read_buffer[1]);
            if((printCounter++ %256)==0){
                esp_rom_printf("Pos:%4d|T:%6d|F:%d\n", angle, counter, failCounter);
            }
        }else{
            failCounter++;
        }
#define pd (250*240)
        uint32_t periodBetweenIterations = esp_cpu_get_cycle_count() -timer; //time elapsed in 240Mhz ticks
        // int usRemaining = 250 - (int)(periodBetweenIterations/240.0f);
        if(periodBetweenIterations <pd){
            uint32_t papap=(pd-periodBetweenIterations);
            ets_delay_us(papap/240.0f);
        }

    }       
}
#else
void read(void* parameter){
    uint32_t pastAngle = 0;
    uint32_t counter = 0;
    uint32_t failCounter = 0;
    uint32_t printCounter = 0;
    
    // Target period in CPU cycles (e.g., 199us * 240 cycles per microsecond)
    const uint32_t TARGET_PERIOD_CYCLES = 250 * 240; 

    for(;;){
        uint32_t loop_start = esp_cpu_get_cycle_count();
        counter++;
        
        // 1. Run the I2C transaction
        esp_err_t result = i2c_master_transmit_receive(dev_handle, &write_buffer, 1, read_buffer, data_length, 1);
        
        if(result == ESP_OK){
            angle = ((read_buffer[0] << 8) | read_buffer[1]);
            
            // 2. Controlled printing (only once every 256 frames)
            if((printCounter++ % 256) == 0){
                esp_rom_printf("Pos:%4d|T:%6d|F:%d\n", angle, counter, failCounter);
            }
        } else {
            failCounter++;
            // Avoid heavy printing inside the failure path to keep timing stable
        }
        
        // 3. Dynamic Precision Padding
        uint32_t elapsed = esp_cpu_get_cycle_count() - loop_start;
        if (elapsed < TARGET_PERIOD_CYCLES) {
            uint32_t cycles_to_wait = TARGET_PERIOD_CYCLES - elapsed;
            // Convert remaining cycles back to microseconds for the ROM delay
            ets_delay_us(cycles_to_wait / 240.0f);
        }
    }       
}
#endif
TaskHandle_t debugTask;
void debug(void*parameter){
    for(;;){
        esp_rom_printf("\n" white);
        // esp_timer_dump(stdout);
        esp_rom_printf("\n" blue);
       vTaskDelay(pdMS_TO_TICKS(1000));
    }       
}

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
    // .intr_priority =1,
    .flags = {
        .update_period_on_empty = 1,
        .update_period_on_sync = 0, //these 2 determine when set_period takes effect
        // .allow_pd =true
    }
};

mcpwm_generator_config_t genSetup = {
    .gen_gpio_num = generatorGPIO,
    .flags = {
        .invert_pwm = false,
        // .io_loop_back = 0,
        // .io_od_mode = 0, //pull low or float only
        // .pull_up = 0,
        // .pull_down= 1
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
    ESP_ERROR_CHECK(mcpwm_new_comparator(operatorHandle, &comparatorSetup, &comparatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_generator(operatorHandle, &genSetup, &genHandle));

/*I@C*/
        ESP_ERROR_CHECK(i2c_new_master_bus(&master_config, &bus_handle));
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

esp_timer_create_args_t etimerSetup ={
    .callback = cbk,
    .arg=NULL,
    .dispatch_method = ESP_TIMER_ISR,
    .name = "i2ctimer",
    .skip_unhandled_events = true
};




BaseType_t pxHigherPriorityTaskWoken =pdFALSE;
IRAM_ATTR void cbk(void * parameter){
    vTaskNotifyGiveFromISR(readTask, &pxHigherPriorityTaskWoken);
    esp_timer_isr_dispatch_need_yield();
}

extern "C" {
    void app_main() {
        setupMCPWM();
        
        /*RUNTIME FUNCTIONS*/
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(genHandle, 0, true)); // Force low until ready
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operatorHandle, timerHandle)); //--
        // compareValue=49;
        ESP_LOGE("DEBUG", "cmpvalue: %d", compareValue);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparatorHandle,compareValue)); 

        
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(genHandle,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_UP,
                comparatorHandle,
                MCPWM_GEN_ACTION_HIGH
            )
        ));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(genHandle,
            MCPWM_GEN_COMPARE_EVENT_ACTION(
                MCPWM_TIMER_DIRECTION_DOWN,
                comparatorHandle,
                MCPWM_GEN_ACTION_LOW
            )
        ));
        // ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(genHandle,
        //     MCPWM_GEN_COMPARE_EVENT_ACTION(
        //         MCPWM_TIMER_DIRECTION_UP,
        //         comparatorHandle,
        //         MCPWM_GEN_ACTION_HIGH
        //     ),
        //       MCPWM_GEN_COMPARE_EVENT_ACTION(
        //         MCPWM_TIMER_DIRECTION_DOWN,
        //         comparatorHandle,
        //         MCPWM_GEN_ACTION_LOW
        //     ),
        //     MCPWM_GEN_COMPARE_EVENT_ACTION_END()
        // ));
        ESP_ERROR_CHECK(mcpwm_timer_enable(timerHandle));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timerHandle, MCPWM_TIMER_START_NO_STOP));
        xTaskCreatePinnedToCore(read,"i2c reader",4000, NULL, 21, &readTask, 1);
        xTaskCreatePinnedToCore(debug,"debug log",2000,NULL, 15,&debugTask,0);
        esp_timer_create(&etimerSetup, &etimerHandle);
        // esp_timer_start_once(etimerHandle,250);
    } 
} 

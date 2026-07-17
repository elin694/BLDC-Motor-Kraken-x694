#include "headers.h"
//CAILBRATE AS5600 MAGNET

BaseType_t pxHigherPriorityTaskWoken =pdFALSE;

void groundSetup(){
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(gate, GPIO_PULLDOWN_ONLY);
      gpio_set_level(gate, 0);
   }
    gpio_reset_pin(phaseLowGate);
    gpio_set_direction(static_cast<gpio_num_t>(phaseLowGate),GPIO_MODE_OUTPUT);
    gpio_set_level(phaseLowGate, 1);
}

void setupMCPWM(){
    ESP_LOGE("DEBUG", "cmpvalue: %d", compareValue);
    ESP_LOGE("DEBUG", "period ticks %d", timerSetup.period_ticks);
    ESP_LOGE("DEBUG", "timer resol: %d", timerSetup.resolution_hz);
    ESP_ERROR_CHECK(mcpwm_new_timer(&timerSetup, &timerHandle));
    // int g_prescale =100; //gives current toal rpescaler
    // MCPWM0.clk_cfg.clk_prescale = g_prescale-1;
    // MCPWM0.timer[0].timer_cfg0.timer_prescale= (16e7/countingFrequency)*5-1;
    ESP_ERROR_CHECK(mcpwm_new_operator(&operatorSetup, &operatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_comparator(operatorHandle, &comparatorSetup, &comparatorHandle));
    ESP_ERROR_CHECK(mcpwm_new_generator(operatorHandle, &genSetup, &genHandle));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(genHandle, genHandle, &highGateDeadTimeSetup));
    
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operatorHandle, timerHandle)); //--
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
}

TickType_t synchronizedTime;
void setup(void * parameter){
    ets_delay_us(100);
    groundSetup();
    setupMCPWM();
    ESP_ERROR_CHECK(mcpwm_timer_enable(timerHandle));
    xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
        
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    synchronizedTime = xTaskGetTickCount();
    int now1 = esp_timer_get_time();
    xTaskCreatePinnedToCore(read,"i2c reader",4000, &synchronizedTime, 15, &readTask, 1);
    xTaskCreatePinnedToCore(debug,"debug log",2000,&synchronizedTime, 6,&debugTask,0);

    esp_intr_dump(stdout);
   esp_timer_dump(stdout);
   esp_err_t probeCheck = i2c_master_probe(busHandle, as5600Address, 1);
   int now2 = esp_timer_get_time()-now1;
   ESP_LOGI("init", "TaskCreation(us): %d, Probe Check %d", now2, probeCheck);

    vTaskDelete(NULL);
}

void as5600initialize(void * parameter) {
    ESP_ERROR_CHECK(i2c_new_master_bus(&master_config, &busHandle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
    
    int startWatch  =esp_timer_get_time();
    //read current settings
    ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, 
        fthRegister, //address to start on
        (size_t)1, //write 1 byte's woth from fthRegister
        fthRegisterData, //where to save the read data
        (size_t)1, //read 1 byte
        -1)
    );
    
    int lapWatch =esp_timer_get_time()-startWatch;
    fthRegister[1]= (fthRegisterData[0] & fth_sf_clear_mask) | fth_sf_set_mask; //rese
    ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 2, fthRegisterData, 1, -1));
    int lapWatch2 =esp_timer_get_time()-startWatch;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(as5600Handle, fthRegister, 1, fthRegisterData, 1, -1));
    int lapWatch3 =esp_timer_get_time()-startWatch;
    ESP_LOGI(magenta "CALIB", "\nas5600 Fast Fillter Threshold Set to %d\n1st REG read time:%4d \nSF-FTH write time:%4d REG_Check time:%4d ", 
        (int)fthRegisterData[0],
        lapWatch,
        lapWatch2,
        lapWatch3
    );
    
    xTaskNotifyGive(setupTask);
    vTaskDelete(NULL);
}

void debug(void*parameter){
    TickType_t st = *(TickType_t *)parameter;
    xTaskDelayUntil(&st,LATENCY);
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timerHandle, MCPWM_TIMER_START_NO_STOP));
    for(;;){
        esp_rom_printf("\n" white);
        esp_timer_dump(stdout);
        esp_rom_printf("\n" blue);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }       
}

void read(void*parameter){
    TickType_t st = *(TickType_t *)parameter;
    uint32_t counter =0;
    uint32_t failCounter =0;

    uint32_t startTimer =0;
    uint32_t file1 =0;
    uint32_t angle = 0;

    esp_timer_create(&etimerSetup, &etimerHandle);
    esp_timer_start_periodic(etimerHandle, i2cReadPeriod);
    xTaskDelayUntil(&st,LATENCY);
    for(;;){
        // esp_timer_start_once(etimerHandle,250);
        file1 = file1 + ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1))-1;
        startTimer = esp_timer_get_time();
        counter++;
        esp_err_t result = i2c_master_transmit_receive(as5600Handle, &write_buffer, 1, read_buffer, data_length, 1);
        uint32_t lap1 =esp_timer_get_time() -startTimer;
        if(result ==ESP_OK){
            angle = (( read_buffer[0] << 8) | read_buffer[1]);
            if((counter %1024)==0){
                esp_rom_printf("Pos:%4d C:%6d F:%d t-time:%d FailedHandoffs:%3d\n", angle, counter, failCounter, lap1, file1);
            }
        }else{
            failCounter++;
        }

    }       
}

IRAM_ATTR void cbk(void * parameter){
    vTaskNotifyGiveFromISR(readTask, &pxHigherPriorityTaskWoken);
    esp_timer_isr_dispatch_need_yield();
}

extern "C" {
    void app_main() {
        xTaskCreatePinnedToCore(setup, "Setup", 9000, NULL, 22, &setupTask, 0); 
    } 
} 

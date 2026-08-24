#include "headers.h"
//CAILBRATE AS5600 MAGNET

BaseType_t pxHigherPriorityTaskWoken =pdFALSE;
void pinSetup(){
//    for(int i = 0; i<6; i++){
//       gpio_reset_pin(gateArray[i]);
//       gpio_set_direction(gateArray[i], GPIO_MODE_OUTPUT);
//       gpio_set_pull_mode(gateArray[i], GPIO_FLOATING);
//       gpio_set_level(gateArray[i], 0);
//     }
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

// void initializeLowGate(){
//     for (int i = 0; i <1; i++){
//         motorL[i] = { .opConfig = LOperatorSetup};
//         motorL[i].pwmConfig.gen_gpio_num = gateArray[2*i+1];
//     }
//     motorL[0].pwmConfig.gen_gpio_num = phaseLowGate;
//     for (int i = 0; i <1; i++){
//         ESP_ERROR_CHECK(mcpwm_new_operator(&motorL[i].opConfig, &motorL[i].operatorModule));
//         ESP_ERROR_CHECK(mcpwm_new_generator(motorL[i].operatorModule, &motorL[i].pwmConfig, &motorL[i].pwmGate0));
//         ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorL[i].pwmGate0, motorL[i].pwmGate0, &lowGateDeadTimeSetup));
//         // ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorL[i].operatorModule, globalLowTimer)); 
//         ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[i].pwmGate0, 0, true));
//     }
// }

void initializeHighGate(uint32_t comparatorOff_Duty){
    ESP_LOGI("High Gate CMP Value","%d ", startingGateCmpValue);
    for (int i = 0; i <1 ; i++){
        motorH[i] = {
            .timerConfig = HTimerSetup,
            .opConfig = HOperatorSetup
        };
        motorH[i].pwmConfig.gen_gpio_num = generatorGPIO;
        ESP_ERROR_CHECK(mcpwm_new_timer(&motorH[i].timerConfig, &motorH[i].timer));
        // int g_prescale =100; //gives current toal rpescaler
        // MCPWM0.clk_cfg.clk_prescale = g_prescale-1;
        // MCPWM0.timer[0].timer_cfg0.timer_prescale= (16e7/HighTimerResolution)*5-1;
        ESP_ERROR_CHECK(mcpwm_new_operator(&motorH[i].opConfig, &motorH[i].operatorModule));
        ESP_ERROR_CHECK(mcpwm_new_comparator(motorH[i].operatorModule, &motorH[i].compConfig, &motorH[i].comparator0)); //igh needs only 1 gen and cmra
        ESP_ERROR_CHECK(mcpwm_new_generator(motorH[i].operatorModule, &motorH[i].pwmConfig, &motorH[i].pwmGate0));
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(motorH[i].pwmGate0, motorH[i].pwmGate0, &highGateDeadTimeSetup));
        
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(motorH[i].operatorModule, motorH[i].timer)); 
        ESP_LOGI("DEBUG", cyan "cmpvalue: %d", startingGateCmpValue);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, startingGateCmpValue)); //set to max to be off
        // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorH[i].pwmGate0, 0, true));
    }
    //putting command of setting lowGate Low and high gate High (by comparator action event) into buffer
    for (int i = 0; i <1; i++){
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_HIGH
            )
        ));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(motorH[i].pwmGate0,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, 
                motorH[i].comparator0,
                MCPWM_GEN_ACTION_LOW
            )
        ));
    }
}

TickType_t synchronizedTime;
void setup(void * parameter){
    ets_delay_us(100);
    pinSetup();
    // initializeLowGate();
    initializeHighGate(startingGateCmpValue);
    ESP_ERROR_CHECK(mcpwm_timer_enable(motorH[0].timer));
    xTaskCreatePinnedToCore(as5600initialize, "Setup I2c", 3000, NULL, 22, &initializeI2CTask, 1); 
        
    // vTaskDelay(1);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    synchronizedTime = xTaskGetTickCount();
    int now1 = esp_timer_get_time();
    xTaskCreatePinnedToCore(getSectorNumber, "gsn", 8000, &synchronizedTime,  15, &getSectorNumberTask, 1);
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
    ESP_LOGI(magenta "CALIB", "\nas5600 Fast Fillter Threshold Set to %d\n1st REG read time:%4d \nSF-FTH write time:%4d\nREG_Check time:%4d ", 
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
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(motorH[0].timer, MCPWM_TIMER_START_NO_STOP));
    // ESP_ERROR_CHECK(mcpwm_generator_set_force_level(motorL[0].pwmGate0, 1, true));
    gpio_set_level(phaseLowGate, 1);
    for(;;){
        // esp_rom_printf("\n" white);
        // esp_timer_dump(stdout);
        // esp_rom_printf("\n" blue);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }       
}

void getSectorNumber(void*parameter){//gsng
    TickType_t st = *(TickType_t *)parameter;
    uint32_t counter =0;
    uint32_t failCounter =0;

    uint32_t startTimer =0;
    uint32_t file1 =0;
    uint32_t angle = 0;

    esp_timer_create(&gsnTimerSetup, &gsnTimerHandle);
    esp_timer_start_periodic(gsnTimerHandle, estimatedI2CReadTime_us);
    xTaskDelayUntil(&st,LATENCY);
    for(;;){
        // esp_timer_start_once(etimerHandle,250);
        file1 = file1 + ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1))-1;
        startTimer = esp_timer_get_time();
        counter++;
        esp_err_t result = i2c_master_transmit_receive(as5600Handle, &write_buffer, 1, read_buffer, data_length, 1);
        uint32_t lap1 =esp_timer_get_time() -startTimer;
        if((counter %1024)==0){
            esp_rom_printf("lap%d | Pos:%4d C:%6d F:%d FailedHandoffs:%3d\n", lap1, angle, counter, failCounter, file1);
        }
        if(result ==ESP_OK){
            angle = (( read_buffer[0] << 8) | read_buffer[1]);
        } else {
            failCounter++;
        }
        // ESP_ERROR_CHECK(result);
        
    }       
}

void IRAM_ATTR runOnESPTimerIntr(void * globe) {
   vTaskNotifyGiveFromISR(getSectorNumberTask, &xHigherPriorityTaskWoken);
   if(xHigherPriorityTaskWoken == pdTRUE){
      xHigherPriorityTaskWoken =pdFALSE;
      esp_timer_isr_dispatch_need_yield();
   }
}

extern "C" {
    void app_main() {
        xTaskCreatePinnedToCore(setup, "Setup", 9000, NULL, 22, &setupTask, 0); 
    } 
} 

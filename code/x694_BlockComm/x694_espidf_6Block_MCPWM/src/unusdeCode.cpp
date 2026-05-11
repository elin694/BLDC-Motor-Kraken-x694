// #include <string.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/semphr.h>
// #include <esp_adc/adc_continuous.h>


// #define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
// #define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
// #define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
// #define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
// #define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_0
// #define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
   
// #if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
// #define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
// #define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
// #define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)


// #define EXAMPLE_READ_LEN                    256 //bytes per read

// static adc_channel_t channel[2] = {ADC_CHANNEL_7, ADC_CHANNEL_4};

// static TaskHandle_t s_task_handle;
// static const char *TAG = "POTENTIOMETER";

// static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
// {
//     BaseType_t mustYield = pdFALSE;
//     // Notify that ADC continuous driver has done enough number of conversions
//     vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

//     return (mustYield == pdTRUE);
// }

// static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
// {
//     adc_continuous_handle_t handle = NULL;

//     adc_continuous_handle_cfg_t adc_config = {
//         .max_store_buf_size = 1024,
//         .conv_frame_size = EXAMPLE_READ_LEN,
//     };
//     ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

//     adc_continuous_config_t dig_cfg = {
//         .sample_freq_hz = 20 * 1000,
//         .conv_mode = EXAMPLE_ADC_CONV_MODE,
//         .format = EXAMPLE_ADC_OUTPUT_TYPE,
//     };

//     adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
//     dig_cfg.pattern_num = channel_num;
//     for (int i = 0; i < channel_num; i++) {
//         adc_pattern[i].atten = ADC_ATTEN_DB_12;
//         adc_pattern[i].channel = channel[i] & 0x7;
//         adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
//         adc_pattern[i].bit_width = ADC_BITWIDTH_12;

//         ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
//         ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
//         ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
//     }
//     dig_cfg.adc_pattern = adc_pattern;
//     ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

//     *out_handle = handle;
// }

// void app_main(void)
// {
//     esp_err_t ret;
//     uint32_t ret_num = 0;
//     uint8_t result[EXAMPLE_READ_LEN] = {0};
//     memset(result, 0xcc, EXAMPLE_READ_LEN);

//     s_task_handle = xTaskGetCurrentTaskHandle();

//     adc_continuous_handle_t handle = NULL;
//     continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

//     adc_continuous_evt_cbs_t cbs = {
//         .on_conv_done = s_conv_done_cb,
//     };
//     ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
//     ESP_ERROR_CHECK(adc_continuous_start(handle));

//     while (1) {

//          // This is to show you the way to use the ADC continuous mode driver event callback.
//          // This `ulTaskNotifyTake` will block when the data processing in the task is fast.
//          // However in this example, the data processing (print) is slow, so you barely block here.
//          // Without using this event callback (to notify this task), you can still just call
//          // adc_continuous_read() here in a loop, with/without a certain block timeout.
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

//         while (1) {
//             ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
//             if (ret == ESP_OK) {
//                 ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
//                 for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
//                     adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
//                     uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
//                     uint32_t data = EXAMPLE_ADC_GET_DATA(p);
//                     // Check the channel number validation, the data is invalid if the channel num exceed the maximum channel 
//                     if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
//                         //ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, data);
//                         ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %lu", unit, chan_num, data);
//                     } else {
//                         ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
//                     }
//                 }
//                 //  Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
//                 //  To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
//                 //  usually you don't need this delay (as this task will block for a while).
                
//                 vTaskDelay(1);
//             } else if (ret == ESP_ERR_TIMEOUT) {
//                 // We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
//                 break;
//             }
//         }
//     }
//     ESP_ERROR_CHECK(adc_continuous_stop(handle));
//     ESP_ERROR_CHECK(adc_continuous_deinit(handle));
// }

// // // making 2 leds breathe using freertos

// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "driver/gpio.h"
// // #include "esp_log.h"
// // #include <iostream>
// // #include <cmath> 
// // #define led GPIO_NUM_2
// // #define led_builtin GPIO_NUM_13

// // volatile int led_states[] = {0,0};
// // volatile int sineWaveLedOutput = 0;
// // const double sineWaveLedFrequency =1.0/3; // inHertz
// // const int sineWaveLedPWMPeriod= 25; // in #tick
// // const int maxDuty = 95;
// // const int minDuty = 5;
// // volatile int duty= (minDuty+maxDuty)/2.0;//in %
// // int t =  portTICK_PERIOD_MS;
// // TickType_t xLastWakeTime;
// // TickType_t xPauseDuration;
// // BaseType_t xWasDelayed;
// // void blinkDefault(void * parameter){
// //     //unitless
// //     int PWMPeriodsPerLEDPeriod= (configTICK_RATE_HZ/(sineWaveLedFrequency)
// //         /sineWaveLedPWMPeriod); //ms=500
// //         printf("\n configTICK_RATE_HZ: %d \n",configTICK_RATE_HZ);
// //         printf("sineWaveLedFrequency: %f \n",sineWaveLedFrequency);
// //         printf("sineWaveLedPWMPeriod: %d \n",sineWaveLedPWMPeriod);
// //         printf("PWMPeriodsPerLEDPeriod: %d \n \n \n",PWMPeriodsPerLEDPeriod);
// //         xLastWakeTime = xTaskGetTickCount();
// //         for(long int rad = 0; true ; rad  += 2){
// //             //rising edge 
// //             sineWaveLedOutput = 1;
// //             gpio_set_level(led_builtin, sineWaveLedOutput);
// //             // printf("Is Led supposed to be on? %s \n",sineWaveLedOutput==1 ? "Yes" : "No");
// //             xPauseDuration = static_cast<TickType_t>((sineWaveLedPWMPeriod)*duty/100.0); 
// //             printf("high # of ticks to Pause: %u \n",xPauseDuration);
// //             if(xPauseDuration==0){
// //                 ESP_LOGD("Pause Duration", "Pause duration is zero");
// //                 vTaskDelay(100000);
// //             }
// //             //number of FreeRTOS ticks to wait (10ms) 
// //             //by using duty*period
// //             xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xPauseDuration);

// //             //falling edge
// //             sineWaveLedOutput = 0;
// //             gpio_set_level(led_builtin, sineWaveLedOutput);
// //             // printf("Is Led suppsoed to be on? %s \n",sineWaveLedOutput==1 ? "Yes" : "No");
// //             xPauseDuration = sineWaveLedPWMPeriod- xPauseDuration;
// //             printf("Low # of ticks to Pause: %u \n",xPauseDuration);
// //             xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xPauseDuration);
// //             //fixed width pwm with period = <sineWaveLedPWMPeriod> amount of ticks
// //             printf("rad: %d \n", rad);
// //             duty = (maxDuty+minDuty)/2+(maxDuty-minDuty)/2*(std::sin(rad*std::numbers::pi/PWMPeriodsPerLEDPeriod
// //                 ));
// //             printf("Duty: %d \n \n", duty);
// //             }//2 tiems a second/ 100times a second
// //         }
// // void blinkExternal(void * parameter){
// //     TickType_t xLastWakeTime = xTaskGetTickCount();
// //     for(;;){
// //         gpio_set_level(led, led_states[1]);
// //         led_states[1] = !led_states[1];

// //             //number of FreeRTOS ticks to wait (10ms) 
// //             //by using duty*period
// //             //ticktype_t might need to be outside funciton
// //             TickType_t xPauseDuration = sineWaveLedPWMPeriod/2;
// //             // printf("Number of ticks to Pause: %u \n",xPauseDuration);
// //             BaseType_t xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xPauseDuration);
// //         //sdkconfig. h configTICK_RATE_HZ  set to 1000
// //     }
    
    
// // }
// // extern "C" {
// //     void app_main() {
// //         gpio_reset_pin(led);
// //         gpio_reset_pin(led_builtin);
// //         gpio_set_direction(led, GPIO_MODE_OUTPUT);
// //         gpio_set_direction(led_builtin, GPIO_MODE_OUTPUT);
// //         if (duty >100){ 
// //             ESP_LOGD("Invalid duty cycle", "Duty cycle must be between 0 and 100");
// //         }
// //         printf("loop start");
// //     xTaskCreate(blinkDefault, "blinkDefault", pow(2, 14), NULL, 1, NULL);
// //     xTaskCreate(blinkExternal, "blinkExternal",  pow(2, 14), NULL, 1, NULL);
// //     } 
// // }

// //================================================================== AS5600 SENSOR! ==================================================================
// //================================================================== AS5600 SENSOR! ==================================================================
// //================================================================== AS5600 SENSOR! ==================================================================
// //as5600 code

// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "driver/gpio.h"
// // #include "esp_log.h"
// // #include "esp_err.h"
// // #include <iostream>
// // #include <cmath> 

// // #define CLOCK GPIO_NUM_2
// // #define DATA GPIO_NUM_2
// // #define as5600 0x58
// // //================== #INSTALL MASTER BUS AND DEVICE ==================
// // #include "driver/i2c_master.h"
// // #include "driver/i2c_types.h"
// // #include "driver/i2c_slave.h"
// // i2c_master_bus_config_t i2c_mst_config = {
// //     .i2c_port = -1,
// //     .sda_io_num = DATA,
// //     .scl_io_num = CLOCK,
// //     .clk_source = I2C_CLK_SRC_DEFAULT,
// //     .glitch_ignore_cnt = 7,
// //     .flags = {.enable_internal_pullup = false},
// // };
// // /* OTHER POSSIBLE CONFIGS
// // i2c_master_bus_config_t::i2c_port sets the I2C port used by the controller.
// // i2c_master_bus_config_t::sda_io_num sets the GPIO number for the serial data bus (SDA).
// // i2c_master_bus_config_t::scl_io_num sets the GPIO number for the serial clock bus (SCL).
// // i2c_master_bus_config_t::clk_source selects the source clock for I2C bus (listed in 
// //       i2c_clock_source_t). Please refer to Power Management section forpower consumption.
// // i2c_master_bus_config_t::glitch_ignore_cnt - if the glitch period on the line is less than this value
// //        it can be filtered out, typically value is 7.
// // i2c_master_bus_config_t::intr_priority sets the priority of the interrupt. If set to 0 , then the driver will use a interrupt with low or medium priority 
// //       (priority level may be one of 1, 2 or 3), otherwise use the priority indicated by i2c_master_bus_config_t::intr_priority
// // i2c_master_bus_config_t::trans_queue_depth sets the depth of internal transfer queue. Only valid in asynchronous transaction.
// // i2c_master_bus_config_t::enable_internal_pullup enables internal pullups. Note: This is not strong enough to pullup buses under high-speed frequency.
// // i2c_master_bus_config_t::allow_pd configures if the driver allows the system to power down the peripheral in light sleep mode. 
// // */

// // i2c_master_bus_handle_t bus_handle;

// // i2c_device_config_t dev_cfg = {
// //     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
// //     .device_address = as5600,   
// //     .scl_speed_hz = 100000,
// // };
// // /*
// // i2c_device_config_t::dev_addr_length address bit length(enum I2C_ADDR_BIT_LEN_7 or I2C_ADDR_BIT_LEN_10)
// // i2c_device_config_t::device_address sets the I2C device raw address. Please parse the device address to this member
// // directly. For example, the device address is 0x28, then parse 0x28 to i2c_device_config_t::device_address, 
// // don't carry a write or read bit.
// // i2c_device_config_t::scl_speed_hz sets the SCL line frequency of this device.
// // i2c_device_config_t::scl_wait_us sets the SCL await time (in μs). U0 for default
// // */

// // i2c_master_dev_handle_t dev_handle;

// // //================== #Get I2C master handle via port ==================
// // // // Source File 1
// // // #include "driver/i2c_master.h"
// // // i2c_master_bus_handle_t bus_handle;
// // // i2c_master_bus_config_t i2c_mst_config = {
// //     //     ... // same as others
// //     // };
// //     // ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    
// //     // // Source File 2
// //     // #include "driver/i2c_master.h"
// //     // i2c_master_bus_handle_t handle;
// //     // ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &handle));
    
// //     //================== #Install I2C slave device ==================
// //     i2c_slave_config_t i2c_slv_config = {
// //         .i2c_port = -1,
// //         .sda_io_num = DATA,
// //         .scl_io_num = CLOCK,
// //         .clk_source = I2C_CLK_SRC_DEFAULT,
// //         .send_buf_depth = 100,
// //         .slave_addr = as5600,
// //         // .receive_buf_depth = 100,
// //     };
// //     //#Uninstall I2C slave device
// //     //#I2C Master Controller
// //     //#I2C Master Write
    
// //     i2c_slave_dev_handle_t slave_handle;
    
    
// //     //================== Main Loop ==================
// //     extern "C" {
// //         void app_main() {
// //             gpio_reset_pin(CLOCK);
// //             gpio_reset_pin(DATA);
// //             printf("loop start");
// //             ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
// //             ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
// //             ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &slave_handle));
// //     } 
// // }
// //====================================== 6 step commutaition! ======================================
// //  esp_err_t i2c_master_register_event_callbacks(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_callbacks_t *cbs, void *user_data)
// // include "hal/i2c_types.h" vs driver/i2c_types.h
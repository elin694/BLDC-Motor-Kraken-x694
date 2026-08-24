#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern "C" {
    #include "led_strip.h"
}

// Set explicitly to 48 (or 38 for some S3 board revisions)
#define LED_STRIP_GPIO 48
#define LED_STRIP_LED_NUMBERS 1

static const char *TAG = "RGB_BLINK";
static led_strip_handle_t led_strip;

static void configure_led(void) {
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = LED_STRIP_GPIO;
    strip_config.max_leds = LED_STRIP_LED_NUMBERS;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz
    rmt_config.flags.with_dma = false;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

extern "C" void app_main(void) {
    configure_led();
    ESP_LOGI(TAG, "Starting color cycle...");

    uint16_t hue = 0; // 0 to 360 degrees

    while (1) {
        led_strip_set_pixel_hsv(led_strip, 0, hue, 255, 255);
        led_strip_refresh(led_strip);

        hue = (hue + 1) % 360;
        ESP_LOGI(TAG, "Current hue: %d", hue);
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}
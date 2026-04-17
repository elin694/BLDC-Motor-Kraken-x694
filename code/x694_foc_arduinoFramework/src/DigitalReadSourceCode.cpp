// // ======================================From esp32-hal-gpio.c======================================
// // CONFIG_ARDUINO_ISR_IRAM udnefined 
//     #define ARDUINO_ISR_ATTR IRAM_AT
// extern void ARDUINO_ISR_ATTR __digitalWrite(uint8_t pin, uint8_t val) {
// #ifdef RGB_BUILTIN
//   if (pin == RGB_BUILTIN) {
//     //use RMT to set all channels on/off
//     RGB_BUILTIN_storage = val;
//     const uint8_t comm_val = val != 0 ? RGB_BRIGHTNESS : 0;
//     rgbLedWrite(RGB_BUILTIN, comm_val, comm_val, comm_val);
//     return;
//   }
// #endif  // RGB_BUILTIN
//   if (perimanGetPinBus(pin, ESP32_BUS_TYPE_GPIO) != NULL) {
//     gpio_set_level((gpio_num_t)pin, val);
//   } else {
//     log_e("IO %i is not set as GPIO.", pin);
//   }
// }


// // ======================================From esp32-hal-periman.c ======================================
// #define GPIO_NOT_VALID(p) ((p >= SOC_GPIO_PIN_COUNT) || ((SOC_GPIO_VALID_GPIO_MASK & (1ULL << p)) == 0))

// typedef enum {
//   ESP32_BUS_TYPE_INIT,      // IO has not been attached to a bus yet
//   ESP32_BUS_TYPE_GPIO,      // IO is used as GPIO
//   ESP32_BUS_TYPE_UART_RX,   // IO is used as UART RX pin
//   ESP32_BUS_TYPE_UART_TX,   // IO is used as UART TX pin
//   ESP32_BUS_TYPE_UART_CTS,  // IO is used as UART CTS pin
//   ESP32_BUS_TYPE_UART_RTS,  // IO is used as UART RTS pin
// #if SOC_SDM_SUPPORTED
//   ESP32_BUS_TYPE_SIGMADELTA,  // IO is used as SigmeDelta output
// #endif
// #if SOC_ADC_SUPPORTED
//   ESP32_BUS_TYPE_ADC_ONESHOT,  // IO is used as ADC OneShot input
//   ESP32_BUS_TYPE_ADC_CONT,     // IO is used as ADC continuous input
// #endif
// #if SOC_DAC_SUPPORTED
//   ESP32_BUS_TYPE_DAC_ONESHOT,  // IO is used as DAC OneShot output
//   ESP32_BUS_TYPE_DAC_CONT,     // IO is used as DAC continuous output
//   ESP32_BUS_TYPE_DAC_COSINE,   // IO is used as DAC cosine output
// #endif
// #if SOC_LEDC_SUPPORTED
//   ESP32_BUS_TYPE_LEDC,  // IO is used as LEDC output
// #endif
// #if SOC_RMT_SUPPORTED
//   ESP32_BUS_TYPE_RMT_TX,  // IO is used as RMT output
//   ESP32_BUS_TYPE_RMT_RX,  // IO is used as RMT input
// #endif
// #if SOC_I2S_SUPPORTED
//   ESP32_BUS_TYPE_I2S_STD_MCLK,  // IO is used as I2S STD MCLK pin
//   ESP32_BUS_TYPE_I2S_STD_BCLK,  // IO is used as I2S STD BCLK pin
//   ESP32_BUS_TYPE_I2S_STD_WS,    // IO is used as I2S STD WS pin
//   ESP32_BUS_TYPE_I2S_STD_DOUT,  // IO is used as I2S STD DOUT pin
//   ESP32_BUS_TYPE_I2S_STD_DIN,   // IO is used as I2S STD DIN pin

//   ESP32_BUS_TYPE_I2S_TDM_MCLK,  // IO is used as I2S TDM MCLK pin
//   ESP32_BUS_TYPE_I2S_TDM_BCLK,  // IO is used as I2S TDM BCLK pin
//   ESP32_BUS_TYPE_I2S_TDM_WS,    // IO is used as I2S TDM WS pin
//   ESP32_BUS_TYPE_I2S_TDM_DOUT,  // IO is used as I2S TDM DOUT pin
//   ESP32_BUS_TYPE_I2S_TDM_DIN,   // IO is used as I2S TDM DIN pin

//   ESP32_BUS_TYPE_I2S_PDM_TX_CLK,    // IO is used as I2S PDM CLK pin
//   ESP32_BUS_TYPE_I2S_PDM_TX_DOUT0,  // IO is used as I2S PDM DOUT0 pin
//   ESP32_BUS_TYPE_I2S_PDM_TX_DOUT1,  // IO is used as I2S PDM DOUT1 pin

//   ESP32_BUS_TYPE_I2S_PDM_RX_CLK,   // IO is used as I2S PDM CLK pin
//   ESP32_BUS_TYPE_I2S_PDM_RX_DIN0,  // IO is used as I2S PDM DIN0 pin
//   ESP32_BUS_TYPE_I2S_PDM_RX_DIN1,  // IO is used as I2S PDM DIN1 pin
//   ESP32_BUS_TYPE_I2S_PDM_RX_DIN2,  // IO is used as I2S PDM DIN2 pin
//   ESP32_BUS_TYPE_I2S_PDM_RX_DIN3,  // IO is used as I2S PDM DIN3 pin
// #endif
// #if SOC_I2C_SUPPORTED
//   ESP32_BUS_TYPE_I2C_MASTER_SDA,  // IO is used as I2C master SDA pin
//   ESP32_BUS_TYPE_I2C_MASTER_SCL,  // IO is used as I2C master SCL pin
//   ESP32_BUS_TYPE_I2C_SLAVE_SDA,   // IO is used as I2C slave SDA pin
//   ESP32_BUS_TYPE_I2C_SLAVE_SCL,   // IO is used as I2C slave SCL pin
// #endif
// #if SOC_GPSPI_SUPPORTED
//   ESP32_BUS_TYPE_SPI_MASTER_SCK,   // IO is used as SPI master SCK pin
//   ESP32_BUS_TYPE_SPI_MASTER_MISO,  // IO is used as SPI master MISO pin
//   ESP32_BUS_TYPE_SPI_MASTER_MOSI,  // IO is used as SPI master MOSI pin
//   ESP32_BUS_TYPE_SPI_MASTER_SS,    // IO is used as SPI master SS pin
// #endif
// #if SOC_SDMMC_HOST_SUPPORTED
//   ESP32_BUS_TYPE_SDMMC_CLK,  // IO is used as SDMMC CLK pin
//   ESP32_BUS_TYPE_SDMMC_CMD,  // IO is used as SDMMC CMD pin
//   ESP32_BUS_TYPE_SDMMC_D0,   // IO is used as SDMMC D0 pin
//   ESP32_BUS_TYPE_SDMMC_D1,   // IO is used as SDMMC D1 pin
//   ESP32_BUS_TYPE_SDMMC_D2,   // IO is used as SDMMC D2 pin
//   ESP32_BUS_TYPE_SDMMC_D3,   // IO is used as SDMMC D3 pin
// #endif
// #if SOC_TOUCH_SENSOR_SUPPORTED
//   ESP32_BUS_TYPE_TOUCH,  // IO is used as TOUCH pin
// #endif
// #if SOC_USB_SERIAL_JTAG_SUPPORTED || SOC_USB_OTG_SUPPORTED
//   ESP32_BUS_TYPE_USB_DM,  // IO is used as USB DM (+) pin
//   ESP32_BUS_TYPE_USB_DP,  // IO is used as USB DP (-) pin
// #endif
// #if SOC_GPSPI_SUPPORTED
//   ESP32_BUS_TYPE_ETHERNET_SPI,  // IO is used as ETHERNET SPI pin
// #endif
// #if CONFIG_ETH_USE_ESP32_EMAC
//   ESP32_BUS_TYPE_ETHERNET_RMII,  // IO is used as ETHERNET RMII pin
//   ESP32_BUS_TYPE_ETHERNET_CLK,   // IO is used as ETHERNET CLK pin
//   ESP32_BUS_TYPE_ETHERNET_MCD,   // IO is used as ETHERNET MCD pin
//   ESP32_BUS_TYPE_ETHERNET_MDIO,  // IO is used as ETHERNET MDIO pin
//   ESP32_BUS_TYPE_ETHERNET_PWR,   // IO is used as ETHERNET PWR pin
// #endif
// #if CONFIG_LWIP_PPP_SUPPORT
//   ESP32_BUS_TYPE_PPP_TX,   // IO is used as PPP Modem TX pin
//   ESP32_BUS_TYPE_PPP_RX,   // IO is used as PPP Modem RX pin
//   ESP32_BUS_TYPE_PPP_RTS,  // IO is used as PPP Modem RTS pin
//   ESP32_BUS_TYPE_PPP_CTS,  // IO is used as PPP Modem CTS pin
// #endif
//   ESP32_BUS_TYPE_MAX
// } peripheral_bus_type_t;

// void *perimanGetPinBus(uint8_t pin, peripheral_bus_type_t type) {
//   // *** check if GPIO pin is valid
//   // if (GPIO_NOT_VALID(pin)) { 
//   //   log_e("Invalid pin: %u", pin);
//   //   return NULL;
//   // }
//   if (type >= ESP32_BUS_TYPE_MAX || type == ESP32_BUS_TYPE_INIT) {
//     log_e("Invalid type %s (%u) for pin %u", perimanGetTypeName(type), (unsigned int)type, pin); 
//     // *** perimanGetTypeName(type) gets peripheral_bus_type_t and returns the pin type in String
//     return NULL;
//   }
//   // *** Sources pin tyep and compares it with funciton input
//   if (pins[pin].type == type) {
//     return pins[pin].bus;
//   }
//   return NULL;
// }



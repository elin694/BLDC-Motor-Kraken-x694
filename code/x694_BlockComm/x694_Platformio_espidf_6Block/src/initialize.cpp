#include "globals.h"
#define as5600Address 0x36

void initializeGPIO(){
   gpio_reset_pin(clockPin);
   gpio_reset_pin(dataPin);
   gpio_num_t gateArray[6]= {
      phaseAHighPort,
      phaseALowPort,
      
      phaseBHighPort,
      phaseBLowPort,

      phaseCHighPort,
      phaseCLowPort,
   };
   for(gpio_num_t gate : gateArray){
      gpio_reset_pin(gate);
      gpio_set_direction(gate,GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(gate, GPIO_PULLUP_ONLY);
   }
}

void initialize(){
   // use ledc to set potentionmeter to input analog read
   // Reset all GPIO directions
   deadTime = 10;
   onTime = 111;
   printf("Setup Begun \n ");
   initializeGPIO( );
   lastTime =  esp_timer_get_time();
   blockNumber = 0;
   //starting delay for user to react + setup
   ESP_ERROR_CHECK(i2c_new_master_bus(&busSetup, & busHandle));
   ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &as5600Setup, &as5600Handle));
   ets_delay_us(1000);
   printf("Loop Initialized \n ");
   //pull Low high to prime Bootstrap cap
}

i2c_master_bus_config_t busSetup = { 
   .i2c_port = -1,
   .sda_io_num= dataPin,
   .scl_io_num= clockPin,
   .clk_source = I2C_CLK_SRC_APB,
   .glitch_ignore_cnt = 7,
   // .intr_priority = 1,
   .flags={.enable_internal_pullup = true}
};
i2c_master_bus_handle_t busHandle;
i2c_device_config_t as5600Setup = {
   .dev_addr_length = I2C_ADDR_BIT_LEN_7,
   .device_address = as5600Address,
   .scl_speed_hz= 300000, //need fast enough  to avoid invaldi state
   .scl_wait_us = 30,
   .flags = {.disable_ack_check = false}
};
i2c_master_dev_handle_t as5600Handle;

//the items being set on /off is the switch itself, not the pins!
  void switchBlock(int phase){
    // given a phase phase = (A), 1,(B), 2(C), sets the gates to the correct permutation,
    // assuming blockNumber is set correctly
    volatile gpio_num_t pinConnectToPower;
    volatile gpio_num_t pinConnectToGND;

    //set high/pinConnectToGND and high/pinConnectToGNDShift to correct values
    switch (phase){
      case 0: //phaseA
        pinConnectToPower = phaseAHighPort;
        pinConnectToGND =phaseALowPort;
        break;
      case 1: //phaseB
        pinConnectToPower =phaseBHighPort;
        pinConnectToGND =phaseBLowPort;
        break;
      case 2://Phase C
        pinConnectToPower =phaseCHighPort;
        pinConnectToGND =phaseCLowPort;
        break;
      default:
            printf("\n INVALID STATE");
        break;
    }

    switch(steps[blockNumber][phase]){
          case -1: //set both off
            if (pinConnectToPower < 32){
               GPIO.out_w1tc |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToPower-32));
            }
            ets_delay_us(deadTime);
            if (pinConnectToGND < 32){
               GPIO.out_w1tc |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToGND-32));
            }
            break;
          case 0: //set low on, high off

            if (pinConnectToPower < 32){
               GPIO.out_w1tc |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToPower-32));
            }
            ets_delay_us(deadTime);
            if (pinConnectToGND < 32){
               GPIO.out_w1ts |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1ts.val |= (1<<(pinConnectToGND-32));
            }

            break;
          case 1: //set low off, high on
            // gpio_set_level( pinConnectToPower,1);
            // gpio_set_level( pinConnectToGND,0);
            if (pinConnectToGND < 32){
               GPIO.out_w1tc |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToGND-32));
            }
            ets_delay_us(deadTime);
            if (pinConnectToPower < 32){
               GPIO.out_w1ts |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1ts.val |= (1<<(pinConnectToPower-32));
            }
            break;
    }
}//run pwm at f ~40-50kHz for adjustable torque control

void initAnalogReadOnce(void *parameter){
  adc_oneshot_unit_init_cfg_t adcSetup= {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  adc_oneshot_chan_cfg_t adcChannelSetup = {
    .atten =  ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcSetup, &adcHandle));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel, &adcChannelSetup));
}

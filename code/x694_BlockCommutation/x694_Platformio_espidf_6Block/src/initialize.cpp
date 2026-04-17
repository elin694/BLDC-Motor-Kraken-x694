// #include "wiring_private.h"
#ifndef GLOBALS_H
#include "globals.h"
#define GLOBALS_H
#endif


void initialize(){
// use ledc to set potentionmeter to input analog read
// Reset all GPIO directions
//
  val = 111;
  printf("Setup Begun \n ");
  gpio_reset_pin(phaseAHighPort);
  gpio_reset_pin(phaseALowPort);
  gpio_reset_pin(phaseBHighPort);
  gpio_reset_pin(phaseBLowPort);
  gpio_reset_pin(phaseCHighPort);
  gpio_reset_pin(phaseCLowPort);
  gpio_set_direction(phaseAHighPort, GPIO_MODE_OUTPUT);
  gpio_set_direction(phaseALowPort, GPIO_MODE_OUTPUT);
  gpio_set_direction(phaseBHighPort, GPIO_MODE_OUTPUT);
  gpio_set_direction(phaseBLowPort, GPIO_MODE_OUTPUT);        
  gpio_set_direction(phaseCHighPort, GPIO_MODE_OUTPUT);
  gpio_set_direction(phaseCLowPort, GPIO_MODE_OUTPUT);
  // put your setup code here, to run once:
  lastTime =  esp_timer_get_time();
  blockNumber = 0;
  //starting delay for user to react + setup
  for (int i = 0; i<15; i++){
    ets_delay_us(100);
  }
  printf("Loop Initialized \n ");
}


#ifdef onTimeRatio
//the items being set on /off is the switch itself, not the pins!
  void switchBlock(int phase, boolean turnOn){
    // given a phase phase = (A), 1,(B), 2(C), sets the gates to the correct permutation,
    // assuming blockNumber is set correctly
    // 
    volatile uint8_t* pinConnectToPower;
    volatile uint8_t* pinConnectToGND;
    //set high/pinConnectToGND and high/pinConnectToGNDShift to correct values
    switch (phase){
      case 0: //phaseA
        pinConnectToPower =phaseAHighPort;
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
        Serial.println("\n INVALID STATE");
        break;
    }
  // set signal to turn off current mosfet that is on
    if(!turnOn){
      switch(steps[blockNumber][phase]){
          case -1: //set both off
            pinConnectToPower &= ~(1<<pinConnectToPowerShift);
            pinConnectToGND &= ~(1<<pinConnectToGNDShift);
            break;
          case 0: //set low on, high off
            pinConnectToPower &= ~(1<<pinConnectToPowerShift);
            // pinConnectToGND |= (1<<pinConnectToGNDShift);
            break;
          case 1: //set low off, high on
            // pinConnectToPower |= (1<<pinConnectToPowerShift);
            pinConnectToGND &= ~(1<<pinConnectToGNDShift);
            break;
      }
    }else {
      switch(steps[blockNumber][phase]){
        
          case -1: //set both off
            break;
          case 0: //set low on, high off
            // pinConnectToPower &= ~(1<<pinConnectToPowerShift);
            pinConnectToGND |= (1<<pinConnectToGNDShift);
            break;
          case 1: //set low off, high on
            pinConnectToPower |= (1<<pinConnectToPowerShift);
            // pinConnectToGND &= ~(1<<pinConnectToGNDShift);
            break;
      }
    }
  }
#else
  void switchBlock(int phase, bool turnOn){
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
            if (pinConnectToPower <= 32){
               GPIO.out_w1tc |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToPower-32));
            }
            if (pinConnectToGND <= 32){
               GPIO.out_w1tc |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToGND-32));
            }
            break;
          case 0: //set low on, high off

            if (pinConnectToPower <= 32){
               GPIO.out_w1tc |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToPower-32));
            }
            if (pinConnectToGND <= 32){
               GPIO.out_w1ts |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1ts.val |= (1<<(pinConnectToGND-32));
            }

            break;
          case 1: //set low off, high on
            // gpio_set_level( pinConnectToPower,1);
            // gpio_set_level( pinConnectToGND,0);
            if (pinConnectToPower <= 32){
               GPIO.out_w1ts |= (1<<pinConnectToPower);
            } else {
               GPIO.out1_w1ts.val |= (1<<(pinConnectToPower-32));
            }
            if (pinConnectToGND <= 32){
               GPIO.out_w1tc |= (1<<pinConnectToGND);
            } else {
               GPIO.out1_w1tc.val |= (1<<(pinConnectToGND-32));
            }
            break;
    }
}
#endif
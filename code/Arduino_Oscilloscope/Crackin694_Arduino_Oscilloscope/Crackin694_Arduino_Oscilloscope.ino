// #include "myAnalogRead.h"
String lowerRail = "0";
const int enableProbe[6] = {7,6,5,4,3,2};
double multiplier[6] = {
  (5/1024.0), //A0
  (5/1024.0), //A1
  (5/1024.0), //A2
  (5/1024.0), //A3
  (5/1024.0), //A4
  (25/1024.0) //A5
};
const int probePinA[6] = {(A0 - 14),(A1 - 14),(A2 - 14),(A3 - 14),(A4 - 14),(A5 - 14)};
const int provide0Vpin[6] = {8, 9, 10, 11, 12, 8};
String probeLabel[6] = {"", "", "", "", "", ""};
bool probeIsEnabled[6] = {false, false, false, false, false, false};
boolean phaseVTimeFlag = false;

void setup() {
 // Reference = AVcc
    ADMUX = (1 << REFS0);
    // ADC enable, prescaler = 16  → ADC clock ~1 MHz on a 16MHz (fastest stable)
    ADCSRA = (1 << ADEN) | (1 << ADPS2);
  //===========================================================
  //115200, 230400, 460800, and 921600 baud
  Serial.begin(921600);
  analogWrite(LED_BUILTIN, 64);
  //  Serial.println("OV,5V");
  //===========================================================
  initiateReadingPins();
}

void loop() {
  // 5 if(probeEnable) conditions takes up 11 samples/.1s
  if(phaseVTimeFlag){
    for (int i=0; i>=0;i-=2){
      Serial.print(probeLabel[i]);
      // Serial.print(myAnalogRead(probePinA[i]) * multiplier[i], 3);
      Serial.print((analogRead(probePinA[i])-analogRead(probePinA[i+1])) * multiplier[i], 3); //a0-a5 --> AH,AL,BH,BL,CH,CL 
      Serial.print(",");
    }
    // #define phaseAHighPort GPIO_NUM_33
    // #define phaseALowPort GPIO_NUM_14
    // #define phaseBHighPort GPIO_NUM_17 tx
    // #define phaseBLowPort GPIO_NUM_16 rx
    // #define phaseCHighPort GPIO_NUM_26
    // #define phaseCLowPort GPIO_NUM_32
  }else{
    for (int i=5; i>=0;i--){
      if (probeIsEnabled[i]) {
        Serial.print(probeLabel[i]);
        // Serial.print(myAnalogRead(probePinA[i]) * multiplier[i], 3);
        Serial.print(analogRead(probePinA[i]) * multiplier[i], 3);
        Serial.print(",");
      }
    }
  }
  // Serial.print("SharedGnd:0,"); //takes up 17samples/.1s
  // timeDiff = (millis() % 100) > 197 || (millis() % 100) < 3;  //in ms
  if ((millis() % 100) > 96) {
    Serial.println("10Hz_5.3V:"+ lowerRail);
  } else {
    Serial.println("10Hz_5.3V:5.33");
  }
}

void initiateReadingPins() {
  for (int i=5; i>=0;i--){
    pinMode(provide0Vpin[i], OUTPUT);  //
    pinMode(enableProbe[i], INPUT_PULLUP);  //
    digitalWrite(provide0Vpin[i], LOW);  //give more ground pins
    probeLabel[i]= "PinA" + static_cast<String>(probePinA[i]) + ":";
    
    if (digitalRead(enableProbe[i]) == LOW) {probeIsEnabled[i] = true;}
  }
  if (digitalRead(13) == LOW) {
    phaseVTimeFlag = true;
    multiplier[0] = multiplier[2] =multiplier[4] = 5.0/1024;
    probeLabel[0] = "A:";
    probeLabel[2] = "B:";
    probeLabel[4] = "C:";
    lowerRail = "-5";
  }
  pinMode(LED_BUILTIN, OUTPUT);
  // analogWrite(11, 128);
}

// Contents/Resources/app/lib/backend/resources/arduino-serial-plotter-webapp/static/js.
// main.xxxxxx.chunk.js.


// /Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-serial-plotter-webapp/static/js
// useState)(50)
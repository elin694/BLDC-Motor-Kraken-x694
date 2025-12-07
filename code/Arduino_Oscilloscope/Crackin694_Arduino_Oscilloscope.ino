#include "myAnalogRead.h"
#define majorPin 3
#define minorPin 2
// arduino uno only
#define probePin1 (A0 - 14)
#define probePin2 (A1 - 14)
#define probePin3 (A2 - 14)
#define probePin4 (A3 - 14)
#define probePin5 (A4 - 14)
#define probePin6 (A5 - 14)
// #define probePin1 (A0)
// #define probePin2 (A1)
// #define probePin3 (A2)
// #define probePin4 (A3)
// #define probePin5 (A4)
// #define probePin6 (A5)
double const_5V = (5.0 / 1024);

//perfected!
const int enableProbe1 = 7;  //5v --> 4.88mV resolution
const int enableProbe2 = 6;  //5v
const int enableProbe3 = 5;  // 10v --> -> 9.765mV resolution
const int enableProbe4 = 4;  //25V --> 24.41mV resolution
const int enableProbe5 = 3;  //25V --> 24.41mV resolution
const int enableProbe6 = 2;  //25V --> 24.41mV resolution

const int provide0VpinA = 8;
const int provide0VpinB = 9;
const int provide0VpinC = 10;
const int provide0VpinD = 11;
const int provide0VpinE = 12;
const int provide0VpinF = 13;
// bool timeDiff;

String probe1Label, probe2Label, probe3Label, probe4Label, probe5Label, probe6Label = "";
// int maxVolt= 0;
bool probe1Enabled, probe2Enabled, probe3Enabled, probe4Enabled, probe5Enabled, probe6Enabled = false;

void setup() {
 // Reference = AVcc
    ADMUX = (1 << REFS0);

    // ADC enable, prescaler = 16  → ADC clock ~1 MHz on a 16MHz Arduino (fastest stable)
    ADCSRA = (1 << ADEN) | (1 << ADPS2);
  Serial.begin(115200);
  initiate5V5V10V20Vpins();
  probe1Label = "5V_pin" + static_cast<String>(probePin1) + ":";
  probe2Label = "5V_pin" + static_cast<String>(probePin2) + ":";
  probe3Label = "5V_pin" + static_cast<String>(probePin3) + ":";
  probe4Label = "5V_pin" + static_cast<String>(probePin4) + ":";
  probe5Label = "5V_pin" + static_cast<String>(probePin5) + ":";
  probe6Label = "5V_pin" + static_cast<String>(probePin6) + ":";
  //================================================================================================
  enableSerial();
  //================================================================================================
  // LABELING GRAPHS
  if (digitalRead(enableProbe1) == LOW) {
    Serial.print("5V Probe at " + static_cast<String>(probePin1) + ",");
    probe1Enabled = true;
  }
  if (digitalRead(enableProbe2) == LOW) {
    Serial.print("5V Probe at " + static_cast<String>(probePin2) + ",");
    probe2Enabled = true;
  }
  if (digitalRead(enableProbe3) == LOW) {
    Serial.print("5V Probe at " + static_cast<String>(probePin3) + ",");
    probe3Enabled = true;
  }
  if (digitalRead(enableProbe4) == LOW) {
    Serial.print("10V Probe at " + static_cast<String>(probePin4) + ",");
    probe4Enabled = true;
  }
  if (digitalRead(enableProbe5) == LOW) {
    Serial.print("5V Probe at " + static_cast<String>(probePin5) + ",");
    probe5Enabled = true;
  }
  if (digitalRead(enableProbe6) == LOW) {
    Serial.print("5V Probe at " + static_cast<String>(probePin6) + ",");
    probe6Enabled = true;
  }

  //LABELING GRAPHS
}

void loop() {
  // //voltage
  // // delayMicroseconds(400);
  // 5 if(probeEnable) conditions takes up 11 samples/.1s
  if (probe1Enabled) {
    Serial.print(probe1Label);
    // Serial.print(myAnalogRead(probePin1) * const_5V, 3);
    Serial.print(analogRead(probePin1) * const_5V, 3);
    Serial.print(",");
  }
  if (probe2Enabled) {
    Serial.print(probe2Label);
    // Serial.print(myAnalogRead(probePin2) * const_5V, 3);
    Serial.print(",");
    Serial.print(analogRead(probePin2) * const_5V, 3);
  }
  if (probe3Enabled) {
    Serial.print(probe3Label);
    // Serial.print(myAnalogRead(probePin3) * const_5V, 3);
    Serial.print(analogRead(probePin3) * const_5V, 3);
    Serial.print(",");
  }
  if (probe4Enabled) {
    Serial.print(probe4Label);
    // Serial.print(myAnalogRead(probePin4) * const_5V, 3);
    Serial.print(analogRead(probePin4) * const_5V, 3);
    Serial.print(",");
  }
  if (probe5Enabled) {
    Serial.print(probe5Label);
    // Serial.print(myAnalogRead(probePin5) * const_5V, 3);
    Serial.print(analogRead(probePin5) * const_5V, 3);
    Serial.print(",");
  }
  if (probe6Enabled) {
    Serial.print(probe6Label);
    // Serial.print(myAnalogRead(probePin6) * 10 / 1024.0, 3);
    Serial.print(myAnalogRead(probePin6) * const_5V);
    Serial.print(",");
  }
  // Serial.print("SharedGnd:0,"); //takes up 17samples/.1s
  // timeDiff = (millis() % 100) > 197 || (millis() % 100) < 3;  //in ms
  if ((millis() % 100) > 95) {
    Serial.println("f10_5.3V:0");
  } else {
    Serial.println("f10_5.3V:5.33");
  }
  // Serial.println((a+2.5));
  // a= -a;
}

void enableSerial() {
  //115200, 230400, 460800, and 921600 baud
  // setting =0;
  /*neither grounded --> case 3
    majorPin grounded --> case 2
    minor grounded --> case 1
    both  grounded --> case 0
    */
  int setting = (~digitalRead(majorPin)) * 2 + (~digitalRead(minorPin));
  setting = 12;
  switch (setting) {
    // case 0:
    //   Serial.begin(115200);
    //   analogWrite(LED_BUILTIN, 64);
    //   break;
    // case 1:
    //   Serial.begin(250000);
    //   analogWrite(LED_BUILTIN, 128);
    //   break;
    // case 2:
    //   Serial.begin(460800);    q
    //   analogWrite(LED_BUILTIN, 192);
    //   break;
    // case 3:
    //   Serial.begin(921600);
    //   analogWrite(LED_BUILTIN, 255);
    //   break;
    default:
      // Serial.begin(921600);
      Serial.begin(2000000);
      analogWrite(LED_BUILTIN, 64);
      //  Serial.println("OV,5V");
  }
}

void initiate5V5V10V20Vpins() {
  // setting to 5V
  pinMode(provide0VpinA, OUTPUT);
  pinMode(provide0VpinB, OUTPUT);  //
  pinMode(provide0VpinC, OUTPUT);  //
  pinMode(provide0VpinD, OUTPUT);  //
  pinMode(provide0VpinE, OUTPUT);  //
  pinMode(provide0VpinF, OUTPUT);  //

  // pinMode(minorPin, INPUT_PULLUP);
  // pinMode(majorPin, INPUT_PULLUP);
  pinMode(enableProbe1, INPUT_PULLUP);  //
  pinMode(enableProbe2, INPUT_PULLUP);  //
  pinMode(enableProbe3, INPUT_PULLUP);  //
  pinMode(enableProbe4, INPUT_PULLUP);  //
  pinMode(enableProbe5, INPUT_PULLUP);  // q
  pinMode(enableProbe6, INPUT_PULLUP);  //
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(provide0VpinA, LOW);  //give more ground pins
  digitalWrite(provide0VpinB, LOW);
  digitalWrite(provide0VpinC, LOW);
  // analogWrite(11, 128);
  digitalWrite(provide0VpinD, LOW);
  digitalWrite(provide0VpinE, LOW);
  digitalWrite(provide0VpinF, LOW);
}
#include <GyverOLED.h>
#include "GyverButton.h"
#include "GyverTimer.h"
#include <GreeHeatpumpIR.h>
#include <AM2320_asukiaaa.h>
#include <EEManager.h>

#define IR_LED_PIN 3
#define ENABLE_BUTTON_PIN 4
#define ENABLED_LED_PIN 9
#define FEEDBACK_LED_PIN 13

#define UP_BUTTON_PIN 8
#define DOWN_BUTTON_PIN 7

#define EEPROM_WRITE_DELAY_MS 10000

IRSenderPWM irSender(IR_LED_PIN);
GreeYACHeatpumpIR *heatpumpIR;

GButton upButton(UP_BUTTON_PIN);
GButton downButton(DOWN_BUTTON_PIN);

GButton enableButton(ENABLE_BUTTON_PIN);
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
GTimer_ms screenUpdateTimer;
GTimer_ms ledResetTimer;

GTimer_ms tempSensorUpdateTimer;
AM2320_asukiaaa tempSensor;

GTimer_ms tempControlUpdateTimer;

struct Data {
  double setTemperature;
  double temperatureThreshold;
};
Data data;

EEManager memory(data, EEPROM_WRITE_DELAY_MS);

double temperature = 0;
double humidity = 0;

// double setTemperature = 21.2;
// double temperatureThreshold = 0.18;

boolean isTempError = false;
boolean isAcOn = false;
boolean previousAcState = false;

boolean isEnabled = false;
boolean isThresholdChange = false;

void setup() {
  Serial.begin(9600);
  oled.init();
  tempSensor.setWire(&Wire);

  //------------------------------- Buttons

  enableButton.setDebounce(50);
  enableButton.setTimeout(300);
  enableButton.setClickTimeout(300);
  enableButton.setType(HIGH_PULL);
  enableButton.setDirection(NORM_OPEN);

  upButton.setDebounce(50);
  upButton.setTimeout(300);
  upButton.setClickTimeout(300);
  upButton.setType(HIGH_PULL);
  upButton.setDirection(NORM_OPEN);

  downButton.setDebounce(50);
  downButton.setTimeout(300);
  downButton.setClickTimeout(300);
  downButton.setType(HIGH_PULL);
  downButton.setDirection(NORM_OPEN);

  //-------------------------------

  screenUpdateTimer.setInterval(200);
  ledResetTimer.setInterval(500);
  tempSensorUpdateTimer.setInterval(5000);
  tempControlUpdateTimer.setInterval(5000);

  pinMode(ENABLED_LED_PIN, OUTPUT);
  pinMode(FEEDBACK_LED_PIN, OUTPUT);
  pinMode(IR_LED_PIN, OUTPUT);

  heatpumpIR = new GreeYACHeatpumpIR();

  oled.clear();
  initializeMemory();
  updateTempSensor();

  // Reset EEPROM
  // memory.reset();  
  // for (;;);
}

void loop() {
  enableButton.tick();
  upButton.tick();
  downButton.tick();

  if (memory.tick()) {
    Serial.println("Updated EEPROM!");
  }

  if (enableButton.isSingle()) {
    Serial.println("Press");
    handleEnableButtonPress();
  }

  if (enableButton.isDouble()) {
    Serial.println("Double Press");
    isThresholdChange = !isThresholdChange;
  }

  if (upButton.isPress()) {
    Serial.println("Up");
    handleUpButton();
  }

  if (downButton.isPress()) {
    Serial.println("Down");
    handleDownButton();
  }

  if (tempSensorUpdateTimer.isReady()) {
    updateTempSensor();
  }
  
  if (screenUpdateTimer.isReady()) {
    updateScreen();
    handleIsEnabledLed();
  }

  if (ledResetTimer.isReady()) {
    digitalWrite(FEEDBACK_LED_PIN, LOW);
  }

  if (tempControlUpdateTimer.isReady()) {
    updateTempControl();
  }
}

void handleUpButton() {
  if (isThresholdChange) {
    data.temperatureThreshold += 0.01;
  } else {
    data.setTemperature += 0.1;
  }
  memory.update();
}

void handleDownButton() {
  if (isThresholdChange) {
    data.temperatureThreshold -= 0.01;
  } else {
    data.setTemperature -= 0.1;
  }
  memory.update();
}

void updateTempControl() {
  Serial.println("Update temp control.");

  if (temperature > data.setTemperature + data.temperatureThreshold) {
    isAcOn = false;
  }

  if (temperature < data.setTemperature - data.temperatureThreshold) {
    isAcOn = true;
  }

  if (isEnabled == false) {
    isAcOn = false;
  }

  updateAcIRState();
}

void handleEnableButtonPress() {
  isEnabled = !isEnabled;
  updateTempControl();
}

void handleIsEnabledLed() {
  if (isEnabled) {
    analogWrite(ENABLED_LED_PIN, 1);
  } else {
    analogWrite(ENABLED_LED_PIN, 0);
  }
}

void updateAcIRState() {
  if (isAcOn == previousAcState) {
    Serial.println("AC state has not changed.");
    return;
  }

  if (isAcOn) {
    turnOnAc();
  } else {
    turnOffAc();
  }

  previousAcState = isAcOn;
}

void updateScreen() {
  oled.home();
  oled.setScale(1);

  oled.setCursor(0, 0);
  oled.print("IsAc: ");
  oled.print(isAcOn);
  oled.print(" PAc: ");
  oled.print(previousAcState);

  if (isTempError) {
    oled.setCursor(0, 1);
    oled.print("TEMP ERROR");
  } else {
    oled.print("                   ");
  }

  oled.setCursor(0, 3);
  oled.print("Cur: ");
  oled.print(temperature);
  oled.print(" C");

  // oled.setCursor(0, 2);
  // oled.print(humidity);
  // oled.print(" %");

  oled.setCursor(0, 4);
  oled.print("Set: ");
  oled.print(data.setTemperature);
  oled.print(" C");
  if (isThresholdChange) {
    oled.print("   ");
  } else {
    oled.print(" *");
  }

  oled.setCursor(0, 5);
  oled.print("THR: ");
  oled.print(data.temperatureThreshold);
  if (isThresholdChange) {
    oled.print(" *");
  } else {
    oled.print("   ");
  }

  oled.setCursor(0, 7);
  if (isEnabled) {
    oled.print("Enabled   ");
  } else {
    oled.print("Disabled  ");
  }

  oled.update();
}

void updateTempSensor() {
  if (tempSensor.update() != 0) {
    Serial.println("Error: Cannot update sensor values.");
    isTempError = true;
  } else {
    temperature = tempSensor.temperatureC;
    humidity = tempSensor.humidity;
    isTempError = false;

    Serial.print("T: " + String(tempSensor.temperatureC) + " C");    
    Serial.println(" H: " + String(tempSensor.humidity) + " %");
  }
}

void turnOnAc() {
  Serial.println("Turn ON ac.");
  heatpumpIR->send(irSender, POWER_ON, MODE_HEAT, FAN_2, 29, VDIR_DOWN, HDIR_AUTO, false, true);
  digitalWrite(FEEDBACK_LED_PIN, HIGH);
  ledResetTimer.reset();
}

void turnOffAc() {
  Serial.println("Turn OFF ac.");
  heatpumpIR->send(irSender, POWER_OFF, MODE_HEAT, FAN_2, 29, VDIR_DOWN, HDIR_AUTO, false, true);
  digitalWrite(FEEDBACK_LED_PIN, HIGH);
  ledResetTimer.reset();
}

void initializeMemory() {
  byte stat = memory.begin(0, 'b');
  Serial.print("Memory State: ");
  Serial.println(stat);
  Serial.print("SetTemperature: ");
  Serial.println(data.setTemperature);
  Serial.print("TemperatureThreshold: ");
  Serial.println(data.temperatureThreshold);

  if (stat == 1) {
    Serial.println("Writing Initial Memory State.");
    data.setTemperature = 20.0;
    data.temperatureThreshold = 0.5;
    memory.updateNow();
  }
}
#include <Arduino.h>
#include "controllers/RelayController.h"
#include "controllers/BleGattController.h"

RelayController relayController;
BleGattController bleController;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("   ECHO WATT ADVANCED PROTOTYPE ");
  Serial.println("================================");

  relayController.begin();
  bleController.begin(relayController);

  Serial.println("[SYSTEM] Echo Watt siap.");
}

void loop() {
  bleController.update();
  delay(10);
}
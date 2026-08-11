#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>

class RelayController {
private:
  static constexpr uint8_t RELAY_COUNT = 4;
  static constexpr bool RELAY_ACTIVE_LOW = true;

  /*
   * PEMETAAN GPIO TERBARU
   *
   * Relay 1 / IN1 → GPIO 33
   * Relay 2 / IN2 → GPIO 26
   * Relay 3 / IN3 → GPIO 27
   * Relay 4 / IN4 → GPIO 25
   */
  const uint8_t relayPins[RELAY_COUNT] = {
    33,
    26,
    27,
    25
  };

  bool relayStates[RELAY_COUNT] = {
    false,
    false,
    false,
    false
  };

  Preferences preferences;
  uint8_t savedStateMask = 0;

  bool isValidRelay(uint8_t relayNumber) const {
    return relayNumber >= 1 &&
           relayNumber <= RELAY_COUNT;
  }

  void writeHardware(uint8_t index) {
    bool state = relayStates[index];

    if (RELAY_ACTIVE_LOW) {
      digitalWrite(
        relayPins[index],
        state ? LOW : HIGH
      );
    } else {
      digitalWrite(
        relayPins[index],
        state ? HIGH : LOW
      );
    }
  }

  uint8_t currentMask() const {
    uint8_t mask = 0;

    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      if (relayStates[i]) {
        mask |= (1U << i);
      }
    }

    return mask;
  }

  void persistCurrentMask() {
    uint8_t mask = currentMask();

    if (mask == savedStateMask) {
      return;
    }

    savedStateMask = mask;
    preferences.putUChar("last_mask", savedStateMask);
  }

public:
  void begin() {
    preferences.begin("echo-relay", false);

    savedStateMask =
      preferences.getUChar("last_mask", 0);

    // Kondisi awal fisik selalu OFF
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      pinMode(relayPins[i], OUTPUT);
      relayStates[i] = false;
      writeHardware(i);
    }

    Serial.println();
    Serial.println("[RELAY] Konfigurasi GPIO:");
    Serial.println("[RELAY] Relay 1 -> GPIO 33");
    Serial.println("[RELAY] Relay 2 -> GPIO 26");
    Serial.println("[RELAY] Relay 3 -> GPIO 27");
    Serial.println("[RELAY] Relay 4 -> GPIO 25");

    Serial.print("[RELAY] Saved mask: ");
    Serial.println(savedStateMask, BIN);

    Serial.println(
      "[RELAY] Kondisi awal semua relay OFF."
    );
  }

  bool setRelay(
    uint8_t relayNumber,
    bool state,
    bool remember = true
  ) {
    if (!isValidRelay(relayNumber)) {
      Serial.println(
        "[RELAY] Nomor relay tidak valid."
      );
      return false;
    }

    uint8_t index = relayNumber - 1;

    relayStates[index] = state;
    writeHardware(index);

    if (remember) {
      persistCurrentMask();
    }

    Serial.print("[RELAY] Relay ");
    Serial.print(relayNumber);
    Serial.println(state ? " ON" : " OFF");

    return true;
  }

  bool toggleRelay(
    uint8_t relayNumber,
    bool remember = true
  ) {
    if (!isValidRelay(relayNumber)) {
      Serial.println(
        "[RELAY] Nomor relay tidak valid."
      );
      return false;
    }

    return setRelay(
      relayNumber,
      !relayStates[relayNumber - 1],
      remember
    );
  }

  void applyMask(
    uint8_t mask,
    bool remember = true
  ) {
    mask &= 0x0F;

    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      relayStates[i] =
        (mask & (1U << i)) != 0;

      writeHardware(i);
    }

    if (remember) {
      persistCurrentMask();
    }
  }

  void allOn(bool remember = true) {
    applyMask(0x0F, remember);
    Serial.println("[RELAY] Semua relay ON.");
  }

  void allOff(bool remember = true) {
    applyMask(0x00, remember);
    Serial.println("[RELAY] Semua relay OFF.");
  }

  void safetyOff() {
    applyMask(0x00, false);

    Serial.println(
      "[RELAY] Safety OFF aktif."
    );

    Serial.println(
      "[RELAY] Status terakhir tetap tersimpan."
    );
  }

  bool getRelayState(
    uint8_t relayNumber
  ) const {
    if (!isValidRelay(relayNumber)) {
      return false;
    }

    return relayStates[relayNumber - 1];
  }

  uint8_t getRelayMask() const {
    return currentMask();
  }

  uint8_t getSavedStateMask() const {
    return savedStateMask;
  }
};

#endif
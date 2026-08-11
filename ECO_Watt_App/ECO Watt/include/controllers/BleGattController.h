#ifndef BLE_GATT_CONTROLLER_H
#define BLE_GATT_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include "RelayController.h"

#define ECOWATT_SERVICE_UUID "5f524c4e-0001-4a5b-9c1e-6f2b1a8d3c00"
#define CHAR_STATUS_UUID     "5f524c4e-0002-4a5b-9c1e-6f2b1a8d3c00"
#define CHAR_CONTROL_UUID    "5f524c4e-0003-4a5b-9c1e-6f2b1a8d3c00"

class BleGattController {
private:
  enum ConnectMode : uint8_t {
    ALL_ON = 0,
    RESTORE_LAST = 1,
    STAY_OFF = 2
  };

  RelayController* relayController = nullptr;
  Preferences preferences;

  NimBLEServer* pServer = nullptr;
  NimBLEAdvertising* pAdvertising = nullptr;
  NimBLECharacteristic* pStatusChar = nullptr;
  NimBLECharacteristic* pControlChar = nullptr;

  volatile bool connectEventPending = false;
  volatile bool disconnectEventPending = false;
  volatile bool statusChanged = true;

  bool bluetoothConnected = false;
  bool disconnectTimerActive = false;
  uint32_t disconnectStartedAt = 0;
  uint32_t lastStatusSentAt = 0;

  uint8_t disconnectDelaySec = 3;
  ConnectMode connectMode = ALL_ON;

  bool countdownActive[4] = {false, false, false, false};
  uint32_t countdownEndAt[4] = {0, 0, 0, 0};

  static constexpr uint32_t MAX_COUNTDOWN_SEC = 86400; // 24 jam

  class ServerCallbacks : public NimBLEServerCallbacks {
  private:
    BleGattController* owner;

  public:
    explicit ServerCallbacks(BleGattController* controller) : owner(controller) {}

    void onConnect(NimBLEServer* server) override {
      owner->connectEventPending = true;
      owner->disconnectEventPending = false;
    }

    void onDisconnect(NimBLEServer* server) override {
      owner->disconnectEventPending = true;
      owner->connectEventPending = false;
    }
  };

  class ControlCallbacks : public NimBLECharacteristicCallbacks {
  private:
    BleGattController* owner;

  public:
    explicit ControlCallbacks(BleGattController* controller) : owner(controller) {}

    void onWrite(NimBLECharacteristic* characteristic) override {
      owner->handleControlWrite(characteristic);
    }
  };

  ServerCallbacks* serverCallbacks = nullptr;
  ControlCallbacks* controlCallbacks = nullptr;

  bool isAllowedDelay(uint8_t seconds) const {
    return seconds == 3 || seconds == 5 || seconds == 10 || seconds == 30;
  }

  const char* connectModeName() const {
    switch (connectMode) {
      case RESTORE_LAST: return "RESTORE_LAST";
      case STAY_OFF: return "STAY_OFF";
      default: return "ALL_ON";
    }
  }

  bool setConnectModeFromString(const String& value) {
    if (value == "ALL_ON") {
      connectMode = ALL_ON;
    } else if (value == "RESTORE_LAST") {
      connectMode = RESTORE_LAST;
    } else if (value == "STAY_OFF") {
      connectMode = STAY_OFF;
    } else {
      return false;
    }

    preferences.putUChar("conn_mode", static_cast<uint8_t>(connectMode));
    return true;
  }

  void cancelCountdown(uint8_t relayNumber) {
    if (relayNumber < 1 || relayNumber > 4) return;
    const uint8_t index = relayNumber - 1;
    countdownActive[index] = false;
    countdownEndAt[index] = 0;
    statusChanged = true;
  }

  void cancelAllCountdowns() {
    for (uint8_t i = 0; i < 4; i++) {
      countdownActive[i] = false;
      countdownEndAt[i] = 0;
    }
    statusChanged = true;
  }

  uint32_t remainingSeconds(uint8_t index) const {
    if (index >= 4 || !countdownActive[index]) return 0;

    const uint32_t now = millis();
    if (static_cast<int32_t>(countdownEndAt[index] - now) <= 0) return 0;

    return (countdownEndAt[index] - now + 999U) / 1000U;
  }

  void startCountdown(uint8_t relayNumber, uint32_t seconds) {
    if (relayNumber < 1 || relayNumber > 4) return;

    if (seconds == 0) {
      cancelCountdown(relayNumber);
      Serial.print("[TIMER] Countdown Relay ");
      Serial.print(relayNumber);
      Serial.println(" dibatalkan.");
      return;
    }

    if (seconds > MAX_COUNTDOWN_SEC) seconds = MAX_COUNTDOWN_SEC;

    const uint8_t index = relayNumber - 1;

    // Saat timer dimulai, relay dipastikan ON, lalu akan OFF saat waktu habis.
    relayController->setRelay(relayNumber, true, true);
    countdownActive[index] = true;
    countdownEndAt[index] = millis() + (seconds * 1000UL);
    statusChanged = true;

    Serial.print("[TIMER] Relay ");
    Serial.print(relayNumber);
    Serial.print(" akan OFF dalam ");
    Serial.print(seconds);
    Serial.println(" detik.");
  }

  void handleSettings(JsonObject settings) {
    bool changed = false;

    if (settings.containsKey("disconnect_delay")) {
      const int requested = settings["disconnect_delay"].as<int>();
      if (requested >= 0 && requested <= 255 && isAllowedDelay(static_cast<uint8_t>(requested))) {
        disconnectDelaySec = static_cast<uint8_t>(requested);
        preferences.putUChar("auto_off", disconnectDelaySec);
        changed = true;
      } else {
        Serial.println("[BLE] Auto-OFF hanya boleh 3, 5, 10, atau 30 detik.");
      }
    }

    if (settings.containsKey("connect_mode")) {
      const String requestedMode = settings["connect_mode"].as<String>();
      if (setConnectModeFromString(requestedMode)) {
        changed = true;
      } else {
        Serial.println("[BLE] Mode koneksi tidak valid.");
      }
    }

    if (changed) {
      statusChanged = true;
      Serial.print("[BLE] Pengaturan disimpan. Auto-OFF: ");
      Serial.print(disconnectDelaySec);
      Serial.print(" detik, mode: ");
      Serial.println(connectModeName());
    }
  }

  void handleControlWrite(NimBLECharacteristic* characteristic) {
    const std::string rawValue = characteristic->getValue();
    if (rawValue.empty()) return;

    StaticJsonDocument<256> doc;
    const DeserializationError error = deserializeJson(doc, rawValue);
    if (error) {
      Serial.print("[BLE] JSON tidak valid: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc.containsKey("command")) {
      const String command = doc["command"].as<String>();

      if (command == "ALL_ON") {
        cancelAllCountdowns();
        relayController->allOn(true);
        statusChanged = true;
        return;
      }

      if (command == "ALL_OFF") {
        cancelAllCountdowns();
        relayController->allOff(true);
        statusChanged = true;
        return;
      }

      if (command == "GET_STATUS") {
        statusChanged = true;
        return;
      }
    }

    if (doc.containsKey("settings")) {
      JsonObject settings = doc["settings"].as<JsonObject>();
      if (!settings.isNull()) handleSettings(settings);
      return;
    }

    if (doc.containsKey("timer")) {
      JsonObject timer = doc["timer"].as<JsonObject>();
      if (timer.isNull()) return;

      const int relayNumber = timer["relay"] | 0;
      const long seconds = timer["seconds"] | 0;

      if (relayNumber < 1 || relayNumber > 4 || seconds < 0) {
        Serial.println("[BLE] Data timer tidak valid.");
        return;
      }

      startCountdown(static_cast<uint8_t>(relayNumber), static_cast<uint32_t>(seconds));
      return;
    }

    if (doc.containsKey("relay") && doc.containsKey("state")) {
      const int relayNumber = doc["relay"].as<int>();
      const bool relayState = doc["state"].as<bool>();

      if (relayNumber < 1 || relayNumber > 4) return;

      cancelCountdown(static_cast<uint8_t>(relayNumber));
      relayController->setRelay(static_cast<uint8_t>(relayNumber), relayState, true);
      statusChanged = true;
      return;
    }

    if (doc.containsKey("relay") && doc.containsKey("toggle")) {
      const int relayNumber = doc["relay"].as<int>();
      if (relayNumber < 1 || relayNumber > 4) return;

      cancelCountdown(static_cast<uint8_t>(relayNumber));
      relayController->toggleRelay(static_cast<uint8_t>(relayNumber), true);
      statusChanged = true;
      return;
    }
  }

  void applyConnectMode() {
    cancelAllCountdowns();

    switch (connectMode) {
      case RESTORE_LAST:
        relayController->applyMask(relayController->getSavedStateMask(), false);
        Serial.println("[BLE] Mode connect: kembalikan status terakhir.");
        break;

      case STAY_OFF:
        relayController->safetyOff();
        Serial.println("[BLE] Mode connect: tetap OFF sampai dikontrol manual.");
        break;

      case ALL_ON:
      default:
        relayController->allOn(true);
        Serial.println("[BLE] Mode connect: semua relay ON.");
        break;
    }
  }

  void updateCountdowns() {
    const uint32_t now = millis();

    for (uint8_t i = 0; i < 4; i++) {
      if (!countdownActive[i]) continue;

      if (static_cast<int32_t>(now - countdownEndAt[i]) >= 0) {
        countdownActive[i] = false;
        countdownEndAt[i] = 0;
        relayController->setRelay(i + 1, false, true);
        statusChanged = true;

        Serial.print("[TIMER] Countdown Relay ");
        Serial.print(i + 1);
        Serial.println(" selesai. Relay OFF.");
      }
    }
  }

  bool hasActiveCountdown() const {
    for (uint8_t i = 0; i < 4; i++) {
      if (countdownActive[i]) return true;
    }
    return false;
  }

  void publishStatus() {
    if (pStatusChar == nullptr || relayController == nullptr) return;

    // Payload dibuat ringkas agar aman untuk notifikasi BLE.
    StaticJsonDocument<192> doc;
    doc["c"] = bluetoothConnected ? 1 : 0;

    JsonArray relays = doc.createNestedArray("r");
    JsonArray timers = doc.createNestedArray("t");

    for (uint8_t i = 0; i < 4; i++) {
      relays.add(relayController->getRelayState(i + 1) ? 1 : 0);
      timers.add(remainingSeconds(i));
    }

    doc["ad"] = disconnectDelaySec;
    doc["cm"] = static_cast<uint8_t>(connectMode);
    doc["su"] = 1; // single user aktif

    String payload;
    serializeJson(doc, payload);

    pStatusChar->setValue(
      reinterpret_cast<const uint8_t*>(payload.c_str()),
      payload.length()
    );

    if (bluetoothConnected && pServer->getConnectedCount() > 0) {
      pStatusChar->notify();
    }

    lastStatusSentAt = millis();
    statusChanged = false;
  }

public:
  void begin(RelayController& relay) {
    relayController = &relay;

    preferences.begin("echo-cfg", false);
    disconnectDelaySec = preferences.getUChar("auto_off", 3);
    if (!isAllowedDelay(disconnectDelaySec)) disconnectDelaySec = 3;

    const uint8_t savedMode = preferences.getUChar("conn_mode", ALL_ON);
    connectMode = savedMode <= STAY_OFF ? static_cast<ConnectMode>(savedMode) : ALL_ON;

    NimBLEDevice::init("Echo_Watt");
    NimBLEDevice::setMTU(185);

    pServer = NimBLEDevice::createServer();
    serverCallbacks = new ServerCallbacks(this);
    pServer->setCallbacks(serverCallbacks);

    NimBLEService* service = pServer->createService(ECOWATT_SERVICE_UUID);

    pStatusChar = service->createCharacteristic(
      CHAR_STATUS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pControlChar = service->createCharacteristic(
      CHAR_CONTROL_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    controlCallbacks = new ControlCallbacks(this);
    pControlChar->setCallbacks(controlCallbacks);

    service->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(ECOWATT_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    publishStatus();

    Serial.println("[BLE] Echo_Watt aktif.");
    Serial.println("[BLE] Single-user aktif: hanya satu HP pada satu waktu.");
    Serial.print("[BLE] Auto-OFF disconnect: ");
    Serial.print(disconnectDelaySec);
    Serial.println(" detik.");
    Serial.print("[BLE] Mode saat connect: ");
    Serial.println(connectModeName());
  }

  void update() {
    if (connectEventPending) {
      connectEventPending = false;
      bluetoothConnected = true;
      disconnectTimerActive = false;

      // Stop advertising memastikan perangkat tidak menerima HP kedua.
      if (pAdvertising != nullptr) pAdvertising->stop();

      applyConnectMode();
      statusChanged = true;
      Serial.println("[BLE] Satu aplikasi tersambung. Advertising dihentikan.");
    }

    if (disconnectEventPending) {
      disconnectEventPending = false;
      bluetoothConnected = false;
      disconnectTimerActive = true;
      disconnectStartedAt = millis();

      // Perangkat terlihat kembali setelah pengguna pertama terputus.
      if (pAdvertising != nullptr) pAdvertising->start();

      statusChanged = true;
      Serial.print("[BLE] Aplikasi terputus. Auto-OFF dalam ");
      Serial.print(disconnectDelaySec);
      Serial.println(" detik.");
    }

    if (disconnectTimerActive &&
        millis() - disconnectStartedAt >= disconnectDelaySec * 1000UL) {
      disconnectTimerActive = false;
      cancelAllCountdowns();
      relayController->safetyOff();
      statusChanged = true;
      Serial.println("[BLE] Auto-OFF disconnect selesai.");
    }

    updateCountdowns();

    const uint32_t interval = hasActiveCountdown() ? 1000UL : 3000UL;
    if (statusChanged || millis() - lastStatusSentAt >= interval) {
      publishStatus();
    }
  }

  bool isConnected() const {
    return bluetoothConnected;
  }
};

#endif
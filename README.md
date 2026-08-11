<div align="center">

# ⚡ ECO Watt

### Smart Offline Power Controller berbasis **ESP32 + BLE + PZEM-004T**

**Kontrol listrik pintar tanpa internet, hemat energi, dan sepenuhnya berjalan secara offline.**

![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge)
![Framework](https://img.shields.io/badge/Framework-Arduino-green?style=for-the-badge)
![Communication](https://img.shields.io/badge/Communication-BLE-8A2BE2?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Offline-success?style=for-the-badge)

</div>

---

## 🌱 Tentang Proyek

**ECO Watt** adalah sistem kontrol daya listrik berbasis **ESP32** yang dirancang untuk mengontrol perangkat listrik secara **otomatis maupun manual tanpa koneksi internet**.

Sistem menggunakan **Bluetooth Low Energy (BLE)** sebagai media komunikasi utama, **PZEM-004T v3.0** sebagai sensor monitoring listrik, dan **relay** sebagai aktuator untuk menghidupkan atau mematikan beban AC.

Proyek ini mengimplementasikan konsep **presence-based automation**, yaitu perangkat listrik akan aktif atau nonaktif berdasarkan keberadaan smartphone tertentu yang dipindai melalui **BLE (MAC Address + RSSI)**.

---

# ✨ Fitur Utama

* 📶 **Full Offline** (tanpa WiFi dan tanpa internet)
* 📡 **BLE GATT Server** untuk kontrol dari aplikasi mobile / Web Bluetooth
* 📱 **BLE Presence Detection** berdasarkan MAC Address smartphone
* ⚡ **Monitoring listrik real-time** (Voltage, Current, Power, Energy)
* 🔌 **Kontrol relay otomatis** berdasarkan keberadaan perangkat
* 🎛️ **Mode manual** (FORCE ON / FORCE OFF)
* 🛡️ **Proteksi tegangan** (Undervoltage & Overvoltage)
* 💾 **Penyimpanan konfigurasi permanen** menggunakan ESP32 NVS (Flash)
* 🌐 **Panel kontrol web** melalui Web Bluetooth API (Chrome/Edge)

---

# 🧠 Cara Kerja Sistem

```text
        📱 Smartphone
             │
             │  BLE Scan
             ▼
        ESP32 Controller
             │
      ┌──────┴──────┐
      │             │
      ▼             ▼
PZEM-004T       Relay Module
      │             │
      └──────┬──────┘
             ▼
       🔌 Perangkat AC
```

Urutan logika kontrol:

```text
Safety Voltage
       ↓
Manual Override (FORCE ON / FORCE OFF)
       ↓
AUTO_BLE (Presence Detection)
```

---

# 🛠️ Hardware yang Digunakan

| Komponen               | Fungsi                         |
| ---------------------- | ------------------------------ |
| ESP32 DevKit           | Kontrol utama sistem           |
| PZEM-004T v3.0         | Monitoring konsumsi listrik    |
| Relay Module           | Menghidupkan / mematikan beban |
| Smartphone Android/iOS | Perangkat target BLE           |
| Power Supply           | Catu daya sistem               |

---

# 📚 Software & Library

* 🚀 PlatformIO
* ⚙️ Arduino Framework
* 📦 ArduinoJson
* ⚡ PZEM-004T-v30
* 📶 NimBLE-Arduino

---

# 📁 Struktur Proyek

```text
ECO-Watt/
├── src/
│   └── main.cpp
├── include/
│   ├── controllers/
│   ├── models/
│   └── README
├── web/
│   └── index.html
├── lib/
├── platformio.ini
└── BLE_PROTOCOL.md
```

---

# 🎛️ Mode Operasi

| Mode           | Deskripsi                                    |
| -------------- | -------------------------------------------- |
| 🟢 `AUTO_BLE`  | Relay mengikuti hasil deteksi BLE smartphone |
| 🔵 `FORCE_ON`  | Relay selalu ON                              |
| 🔴 `FORCE_OFF` | Relay selalu OFF                             |

---

# 📊 Data Telemetri

ESP32 mengirimkan status sistem dalam format **JSON** melalui karakteristik BLE.

Contoh:

```json
{
  "mode": "AUTO_BLE",
  "v": 220.3,
  "i": 1.25,
  "p": 275.4,
  "e": 3.42,
  "found": true,
  "rssi": -62,
  "relay": true,
  "reason": "NONE"
}
```

---

# 🚀 Menjalankan Proyek

## 1️⃣ Clone Repository

```bash
git clone https://github.com/USERNAME/ECO-Watt.git
cd ECO-Watt
```

## 2️⃣ Build Project

```bash
pio run
```

## 3️⃣ Upload ke ESP32

```bash
pio run --target upload
```

## 4️⃣ Monitor Serial

```bash
pio device monitor
```

Konfigurasi board:

```ini
board = esp32dev
framework = arduino
```

---

# 🌐 Panel Kontrol Web

Folder `web/` berisi panel kontrol berbasis **Web Bluetooth API**.

### Cara Menggunakan

1. 🌍 Buka `web/index.html`
2. 🟢 Klik **Hubungkan**
3. 📱 Pilih perangkat **ECO-Watt**
4. ⚡ Lakukan monitoring dan kontrol relay secara langsung

> ⚠️ **Web Bluetooth hanya didukung di Google Chrome dan Microsoft Edge.**

---

# 📡 BLE GATT

Nama perangkat BLE:

```text
ECO-Watt
```

UUID Service dan Characteristic dijelaskan secara lengkap pada file:

```text
BLE_PROTOCOL.md
```

---

# 💾 Penyimpanan Konfigurasi

Seluruh konfigurasi disimpan pada **ESP32 NVS (Non-Volatile Storage)** sehingga tetap tersimpan meskipun perangkat dimatikan.

Parameter yang dapat disimpan:

* 📱 Target MAC Address
* 📶 RSSI Threshold
* ⏱️ Buffer Delay
* ⚡ Minimum Voltage
* ⚡ Maximum Voltage
* 🎛️ Mode Operasi

---

# 🗺️ Roadmap

* 📱 Aplikasi Flutter Native
* 👥 Multi-device Presence Detection
* 📈 Logging konsumsi energi
* 📊 Dashboard statistik
* 🔄 OTA Update melalui BLE
* 🔐 Enkripsi dan autentikasi BLE yang lebih kuat

---

# 🤝 Kontribusi

Kontribusi sangat terbuka.

Langkah kontribusi:

1. 🍴 Fork repository
2. 🌿 Buat branch baru
3. 💻 Commit perubahan
4. 🚀 Push ke branch
5. 📬 Buat Pull Request

---

# 📜 Lisensi

Proyek ini dibuat untuk kebutuhan pengembangan sistem otomasi dan monitoring daya listrik berbasis **ESP32**.

---

<div align="center">

## ⚡ ECO Watt

### *Offline Smart Power Control with ESP32 & Bluetooth Low Energy*

**Made with ❤️ for Embedded Systems, IoT, and Energy Efficiency**

</div>

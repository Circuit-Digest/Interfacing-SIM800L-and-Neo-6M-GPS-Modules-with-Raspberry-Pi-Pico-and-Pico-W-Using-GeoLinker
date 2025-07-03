# 🌐 GeoLinker GPS + GSM Tracker using Raspberry Pi Pico

A compact and efficient GPS + GSM tracker project built using the **Raspberry Pi Pico / Pico W**, **Neo-6M GPS module**, and **SIM800L GSM module**, powered by the **GeoLinker** library. This system provides real-time location tracking, cloud-based data uploads, and robust offline fallback storage for uninterrupted tracking.

---

## 🔧 Features

- 📍 Real-time GPS tracking using Neo-6M (NMEA protocol)
- 🌐 Cellular network connectivity via SIM800L (GPRS)
- ☁️ Seamless cloud data uploads using GeoLinker API
- 📥 Offline data buffering when GSM is unavailable
- 🔋 Battery level telemetry support
- 🚦 LED indicators for GPS error, GSM error, and successful uploads
- 📅 Timezone and timestamp adjustment (e.g., UTC+5:30)
- 🔄 Automatic retry and reconnection support

---

## 🧰 Hardware Used

- Raspberry Pi Pico or Pico W
- SIM800L GSM Module (GPRS)
- Neo-6M GPS Module (UART, 9600 baud)
- 3x LEDs (Green, Yellow, Red) for status
- External 3.7V–4.2V Li-Ion battery (recommended for GSM)
- Capacitors (for GSM module stability)

---

## 🔌 Wiring Overview

| Component | Pico GPIO | Notes             |
|----------|------------|-------------------|
| GPS RX   | GP4        | Serial2 TX        |
| GPS TX   | GP5        | Serial2 RX        |
| GSM RX   | GP0        | Serial1 TX        |
| GSM TX   | GP1        | Serial1 RX        |
| LED Green| GP21       | Success Indicator |
| LED Yellow| GP20      | GPS Error         |
| LED Red  | GP19       | GSM Error         |

Ensure GSM module has stable power (via capacitor if needed).

---

## 📦 Software Libraries

- [GeoLinker](https://github.com/GeoLinker/GeoLinker) – Unified GNSS + GSM + Cloud library
- Arduino framework on Raspberry Pi Pico (RP2040 core)

---

## 🚀 Getting Started

1. Clone this repository  
2. Install GeoLinker library from GitHub or Library Manager  
3. Wire GPS and GSM modules as per the table above  
4. Upload the sketch using Arduino IDE  
5. Open Serial Monitor at 115200 baud

---

## ⚙️ Configuration

Edit the following in the code as per your setup:

```
const char* apn = "your_apn";
const char* apiKey = "your_api_key";
const char* deviceID = "your_device_id";
```

---

## 🧪 Sample Payload

```
{
  "temperature": 27.5,
  "humidity": 65.3,
  "battery": 90
}
```

---

## 🧵 Status LEDs

- ✅ Green: Data sent successfully
- ⚠️ Yellow: GPS parse/fix error
- ❌ Red: GSM/network/modem error

---

## 🔍 Keywords

`SIM800L`, `Neo-6M`, `GeoLinker`, `Raspberry Pi Pico`, `GPS Tracker`, `GSM IoT`, `RP2040`, `GPRS Tracker`, `Location Tracker`, `Offline GNSS`, `AT Commands`, `IoT Cloud Logger`, `GSM GPS`

---

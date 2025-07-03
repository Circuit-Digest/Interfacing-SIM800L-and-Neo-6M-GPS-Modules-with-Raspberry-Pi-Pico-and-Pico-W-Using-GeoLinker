# GeoLinker
![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Platforms](https://img.shields.io/badge/Platforms-ESP32%20%7C%20ESP8266%20%7C%20Arduino%20Uno%20R4%20WiFi%20%7C%20RPi%20Pico%20W%20%7C%20Arduino-green)
![Version](https://img.shields.io/badge/Version-1.0.2-brightgreen)

GeoLinker Cloud API Library

A comprehensive Arduino library for GPS tracking and cloud data transmission, designed for IoT applications. The GeoLinker library enables seamless integration between GPS modules and GeoLinker cloud services with support for offline data storage, automatic reconnection, and multiple microcontroller platforms.

## 🚀 Features

- **Multi-Platform Support**: ESP32, ESP8266, Arduino Uno R4 WiFi, Raspberry Pi Pico W, Raspberry Pi Pico 2 W, and others
-  **Dual Network Support**
  - WiFi (ESP32/ESP8266/RPi Pico W/Arduino Uno R4)
  - Cellular (SIM800L and compatible modules)
- **GPS Integration**: NMEA sentence parsing with coordinate conversion
- **Cloud Connectivity**: Seamless communication with GeoLinker Cloud API
- **Offline Storage**: Automatic data buffering when the network is unavailable (up to 100 entries)
- **Auto-Reconnection**: WiFi reconnection handling
- **Timezone Support**: Configurable time offset for local timezone
- **Custom Payloads**: Support for additional sensor data (up to 5 parameters)
- **Battery Monitoring**: Optional battery level reporting
- **Debug Levels**: Comprehensive debugging with multiple verbosity levels

## 📋 Requirements

### Hardware
- Compatible microcontroller (ESP32, ESP8266, Arduino Uno R4 WiFi, Raspberry Pi Pico W/2W, etc.)
- GPS module with NMEA output capability
- WiFi connectivity

### Software Dependencies
- **ArduinoJson** library (version 7.x or later)
- Platform-specific WiFi libraries (automatically included)

## 🔧 Installation

### Method 1: Arduino Library Manager (Recommended)
1. **Open Arduino IDE**
2. **Navigate to Library Manager**
   ```
   Tools → Manage Libraries... (or Ctrl+Shift+I)
   ```
3. **Search for GeoLinker**
   - Type "GeoLinker" in the search box
   - Install version 1.0.2
4. **Install Dependencies**
   - The ArduinoJson dependency should be installed, search for "ArduinoJson" and install version 7.x or later
5. **Include in Your Sketch**
   ```cpp
   #include <GeoLinker.h>
   ```

### Method 2: Manual Installation
1. **Download Library Files**
   - Download `GeoLinker.h` and `GeoLinker.cpp` from the repository
   - Create a folder named `GeoLinker` in your Arduino libraries directory

2. **Locate Arduino Libraries Folder**
   - **Windows**: `Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

3. **Install Library Files**
   - Copy `GeoLinker.h` and `GeoLinker.cpp` into `Arduino/libraries/GeoLinker/`
   - Your folder structure should look like:
     ```
     Arduino/libraries/GeoLinker/
     ├── GeoLinker.h
     └── GeoLinker.cpp
     ```

4. **Install Dependencies**
   ```
   Tools → Manage Libraries → Search for "ArduinoJson" → Install (version 7.x or later)
   ```

5. **Include in Your Sketch**
   ```cpp
   #include <GeoLinker.h>
   ```

## 🏗️ Hardware Setup

### Option 1: Hardware Serial with Custom Pins
```cpp
//GPS
HardwareSerial gpsSerial(1);  // Using Serial1
#define GPS_RX 16       // GPIO16 for RX
#define GPS_TX 17       // GPIO17 for TX
#define GPS_BAUD 9600   // Standard NMEA baud rate
//GSM
HardwareSerial gsmSerial(2);  // Using Serial2
#define GSM_RX 18       // GPIO18 for RX
#define GSM_TX 19       // GPIO19 for TX
#define GSM_BAUD 9600   // Standard modem baud rate

void setup() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);  // GPS
    gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX, GSM_TX);  // GSM
}
```

### Option 2: Standard Hardware Serial (Uno R4 WiFi, Pico W, Pico 2W)
```cpp
HardwareSerial& gpsSerial = Serial1;
#define GPS_BAUD 9600   // Standard NMEA baud rate
HardwareSerial& gsmSerial = Serial2;
#define GSM_BAUD 9600   // Standard modem baud rate

void setup() {
    gpsSerial.begin(GPS_BAUD);
    gsmSerial.begin(GSM_BAUD);
}
```

### Option 3: Software Serial (ESP8266)
```cpp
#include <SoftwareSerial.h>
#define GPS_RX 14
#define GPS_TX 12
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
#define GPS_BAUD 9600   // Standard NMEA baud rate

#define GSM_RX 5       // Custom RX pin
#define GSM_TX 6       // Custom TX pin
SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
#define GSM_BAUD 9600   // Standard modem baud rate

void setup() {
    gpsSerial.begin(GPS_BAUD);
    gsmSerial.begin(GSM_BAUD);
}
```

## 📝 Usage

### Minimal Example
```cpp
#include <GeoLinker.h>

// ==================================================================
//                   GPS SERIAL CONFIGURATION
// ==================================================================

/* ----- Option 1: Hardware Serial with Custom Pins ----- */
HardwareSerial gpsSerial(1);  // Using Serial1
#define GPS_RX 16       // GPIO16 for RX
#define GPS_TX 17       // GPIO17 for TX

/* ----- Option 2: Standard Hardware Serial (for Uno R4 WiFi, Pico W) ----- */
// HardwareSerial& gpsSerial = Serial1;  // Uses default pins:
                                  // Uno R4 WiFi: RX= D0, TX= D1
                                  // Pico W/2W: TX= 0, RX= 1 

/* ----- Option 3: Software Serial (for ESP8266 etc) ----- */
// #include <SoftwareSerial.h>
// #define GPS_RX 14      // Custom RX pin
// #define GPS_TX 12      // Custom TX pin
// SoftwareSerial gpsSerial(GPS_RX, GPS_TX);  // RX, TX pins (avoid conflict pins)

// Common GPS Settings
#define GPS_BAUD 9600   // Standard NMEA baud rate

// ==================================================================
//                   GSM SERIAL CONFIGURATION
// ==================================================================

/* ----- Option 1: Hardware Serial with Custom Pins ----- */
HardwareSerial gsmSerial(2);  // Using Serial2
#define GSM_RX 18       // GPIO18 for RX
#define GSM_TX 19       // GPIO19 for TX

/* ----- Option 2: Standard Hardware Serial (for Uno R4 WiFi, Pico W) ----- */
// HardwareSerial& gsmSerial = Serial2;  // Uses default pins where available

/* ----- Option 3: Software Serial (for basic boards) ----- */
// #include <SoftwareSerial.h>
// #define GSM_RX 5       // Custom RX pin
// #define GSM_TX 6       // Custom TX pin
// SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

// Common GSM Settings
#define GSM_BAUD 9600   // Standard modem baud rate
#define GSM_PWR_PIN -1  // Modem power pin (-1 if not used)
#define GSM_RST_PIN -1  // Modem reset pin (-1 if not used)


// ==================================================================
//                   NETWORK CONFIGURATION
// ==================================================================
/*-------------------------- For WiFi ------------------------------*/
//const char* ssid = "WiFi_SSID";       // Your network name
//const char* password = "WiFi_Password"; // Your network password

/*-------------------------- For GSM ------------------------------*/
const char* apn = "your.apn";         // Cellular APN
const char* gsmUser = nullptr;        // APN username if required
const char* gsmPass = nullptr;        // APN password if required

// ==================================================================
//                   GeoLinker CONFIGURATION
// ==================================================================
const char* apiKey = "FhBxxxxxxxxx";  // Your GeoLinker API key
const char* deviceID = "GeoLinker_tracker"; // Unique device identifier
const uint16_t updateInterval = 10;   // Data upload interval (seconds)
const bool enableOfflineStorage = true; // Enable offline data storage
const uint8_t offlineBufferLimit = 20;  // Max stored offline record, Keep it minimal for MCUs with less RAM
const bool enableAutoReconnect = true;  // Enable auto-reconnect Only for WiFi, Ignored with GSM
const int8_t timeOffsetHours = 5;      // Timezone hours offset
const int8_t timeOffsetMinutes = 30;   // Timezone minutes offset

// ==================================================================
//                   Create GeoLinker instance
// ==================================================================

GeoLinker geo;

// ==================================================================
//                   SETUP FUNCTION
// ==================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize GPS Serial (select one method)
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);  // Custom pins
  // gpsSerial.begin(GPS_BAUD);  // Default pins

  // Initialize GSM Serial (select one method)
  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX, GSM_TX);  // Custom pins
  // gsmSerial.begin(GSM_BAUD);  // Default pins

  // Core GeoLinker Configuration
  geo.begin(gpsSerial);
  geo.setApiKey(apiKey);
  geo.setDeviceID(deviceID);
  geo.setUpdateInterval_seconds(updateInterval);
  geo.setDebugLevel(DEBUG_BASIC); // Debug verbosity Options: DEBUG_NONE, DEBUG_BASIC, DEBUG_ VERBOSE
  geo.enableOfflineStorage(enableOfflineStorage);
  geo.enableAutoReconnect(enableAutoReconnect);
  geo.setOfflineBufferLimit(offlineBufferLimit); // for small MCUs
  geo.setTimeOffset(timeOffsetHours, timeOffsetMinutes);

  // ===================== Choose Network Backend =====================
  // --- WiFi Example (uncomment to use) ---
   //geo.setNetworkMode(GEOLINKER_WIFI);
   //geo.setWiFiCredentials(ssid, password);
   //if (!geo.connectToWiFi()) Serial.println("WiFi connection failed!");

  // --- SIM800L/Cellular Example (default) ---
  
  geo.setNetworkMode(GEOLINKER_CELLULAR);
  geo.setModemCredentials(apn, gsmUser, gsmPass);
  geo.beginModem(gsmSerial, GSM_PWR_PIN, GSM_RST_PIN, true);
  geo.setModemTimeouts(5000, 15000);
  

  Serial.println("GeoLinker setup complete.");
}

// ==================================================================
//                   MAIN PROGRAM LOOP
// ==================================================================

void loop() {
  // Example sensor payloads (optional - up to 5 payloads)
  geo.setPayloads({
    {"temperature", 27.2},
    {"humidity", 53.1}
  });

  // Example battery level (optional)
  geo.setBatteryLevel(89);
  
  // ==========================================
  //         GEO LINKER OPERATION
  // ==========================================
  uint8_t status = geo.loop();
  if (status > 0) {
    Serial.print("GeoLinker Status: ");
    switch(status) {
      case STATUS_SENT: Serial.println("Data sent successfully!"); break;
      case STATUS_GPS_ERROR: Serial.println("GPS connection error!"); break;
      case STATUS_NETWORK_ERROR: Serial.println("Network error (buffered)."); break;
      case STATUS_BAD_REQUEST_ERROR : Serial.println("Bad request error!"); break;
      case STATUS_PARSE_ERROR: Serial.println("GPS data format error!"); break;
      case STATUS_CELLULAR_NOT_REGISTERED: Serial.println("GSM: Not registered to network!"); break;
      case STATUS_CELLULAR_CTX_ERROR: Serial.println("GPRS Context Error!"); break;
      case STATUS_CELLULAR_DATA_ERROR: Serial.println("GSM HTTP POST Failed!"); break;
      case STATUS_CELLULAR_TIMEOUT: Serial.println("GSM Module Timeout!"); break;
      case STATUS_INTERNAL_SERVER_ERROR: Serial.println("Internal Server Error!"); break;
      default: Serial.println("Unknown status code.");
    }
  }
}
```

## 🔧 API Reference

### GeoLinker API Reference

### Common Methods

#### `void setApiKey(const String& key)`
Set your GeoLinker API key.  
- **Parameters:** `key` — API key string

#### `void setDeviceID(const String& id)`
Set the unique device identifier.  
- **Parameters:** `id` — Device name or ID

#### `void setUpdateInterval_seconds(uint32_t interval_s)`
Set data upload interval (in seconds).  
- **Parameters:** `interval_s` — Interval in seconds (default: 60)

#### `void setDebugLevel(uint8_t level)`
Set debug verbosity.  
- **Parameters:** `level` — `DEBUG_NONE` (0), `DEBUG_BASIC` (1), `DEBUG_VERBOSE` (2)

#### `void enableOfflineStorage(bool enable)`
Enable/disable offline data buffering.  
- **Parameters:** `enable` — `true` to enable

#### `void enableAutoReconnect(bool enable)`
Enable/disable automatic reconnection (WiFi or GSM).  
- **Parameters:** `enable` — `true` to enable

#### `void setPayloads(const std::map<String, float>& payloads)`
Set additional sensor data.  
- **Parameters:** `payloads` — Map of sensor names/values (max 5)

#### `void setBatteryLevel(uint8_t percent)`
Set battery level for reporting.  
- **Parameters:** `percent` — Battery % (0–100)

#### `void setTimeOffset(int hourOffset, int minuteOffset)`
Set timezone offset from UTC.  
- **Parameters:**  
  - `hourOffset` (int): Hours offset  
  - `minuteOffset` (int): Minutes offset

#### `void setOfflineBufferLimit(uint8_t limit)`
Set offline buffer size.  
- **Parameters:** `limit` — Max entries (default: 100)

#### `void setNetworkMode(NetworkBackendType mode)`
Set backend to WiFi or GSM/Cellular.  
- **Parameters:** `mode` — `GEOLINKER_WIFI` or `GEOLINKER_CELLULAR`

#### `uint8_t loop()`
Main library function; call regularly in your loop.  
- **Returns:** Status code (see Status Codes table)

---

### WiFi Related Methods

#### `void setWiFiCredentials(const char* ssid, const char* password)`
Set WiFi credentials.  
- **Parameters:**  
  - `ssid`: WiFi network name  
  - `password`: WiFi password

#### `bool connectToWiFi()`
Connect to WiFi using stored credentials.  
- **Returns:** `true` if connected successfully

---

### GSM/Cellular Related Methods

#### `void setModemCredentials(const char* apn, const char* user = nullptr, const char* pass = nullptr)`
Set APN/credentials for GSM modem.  
- **Parameters:**  
  - `apn`: Access Point Name  
  - `user`: Username (optional)  
  - `pass`: Password (optional)

#### `void beginModem(Stream& serial, uint8_t powerPin = 255, uint8_t resetPin = 255, bool autoPowerCycle = false)`
Initialise GSM modem interface.  
- **Parameters:**  
  - `serial`: Serial interface for modem  
  - `powerPin`: (Optional) Modem power pin  
  - `resetPin`: (Optional) Modem reset pin  
  - `autoPowerCycle`: (Optional) Power cycle at startup

#### `void setModemTimeouts(uint32_t commandTimeoutMs = 5000, uint32_t httpTimeoutMs = 10000)`
Configure GSM modem timeouts.  
- **Parameters:**  
  - `commandTimeoutMs`: AT command timeout  
  - `httpTimeoutMs`: HTTP timeout

#### `int getModemNetworkStatus()`
Get modem network registration status.  
- **Returns:** Registration status code

#### `int getModemGprsStatus()`
Get modem GPRS context status.  
- **Returns:** 1 if attached, 0 otherwise

---

### GPS Related Methods

#### `bool begin(Stream& serial)`
Initialise GeoLinker with GPS serial connection.  
- **Parameters:** `serial` — Serial stream (e.g. `Serial1`, `SoftwareSerial`)  
- **Returns:** `true` if successful

---

## 📊 Status Codes

The `loop()` method returns status codes indicating operation results:

| Code | Status                        | Description                 |
|------|-------------------------------|-----------------------------|
| 0    | STATUS_OK                     | Normal operation            |
| 1    | STATUS_SENT                   | Data sent successfully      |
| 2    | STATUS_GPS_ERROR              | No GPS fix                  |
| 3    | STATUS_NETWORK_ERROR          | Connection failed           |
| 4    | STATUS_BAD_REQUEST_ERROR      | API rejected data           |
| 5    | STATUS_PARSE_ERROR            | Invalid GPS data            |
| 6    | STATUS_CELLULAR_NOT_REGISTERED| No network                  |
| 7    | STATUS_CELLULAR_CTX_ERROR     | GPRS failure                |
| 8    | STATUS_CELLULAR_DATA_ERROR    | HTTP POST failed            |
| 9    | STATUS_CELLULAR_TIMEOUT       | Modem timeout               |
| 10   | STATUS_INTERNAL_SERVER_ERROR  | Server error                |

## 🐛 Debugging

### Debug Levels

```cpp
geo.setDebugLevel(DEBUG_NONE);     // No debug output
geo.setDebugLevel(DEBUG_BASIC);    // Basic status messages
geo.setDebugLevel(DEBUG_VERBOSE);  // Detailed GPS and network info
```

## 🌐 Cloud Integration

### API Endpoint
- **URL**: `https://www.circuitdigest.cloud/geolinker`
- **Method**: POST
- **Headers**: 
  - `Authorization: YOUR_API_KEY`
  - `Content-Type: application/json`

### Data Format
The library sends JSON data to the GeoLinker cloud service:

```json
{
  "device_id": "weather_tracker",
  "timestamp": ["2025-06-04 14:30:45"],
  "lat": [12.9716],
  "long": [77.5946],
  "battery": [87],
  "payload": [{
    "temperature": 25.5,
    "humidity": 60.0
  }]
}
```

### Common Issues

**GPS Not Working**
- Check wiring connections
- Ensure GPS module has clear sky view
- Wait for GPS cold start (may take several minutes)
- Enable verbose debugging to see raw NMEA data

**WiFi Connection Issues**
- Verify SSID and password
- Check signal strength
- Enable auto-reconnect for unstable connections

**Cloud Upload Failures**
- Verify API key is correct
- Check internet connectivity
- Enable offline storage for reliability

## 🔒 License

This library is licensed under the MIT License. See the license header in source files for full details.

**Copyright (C) 2025 Jobit Joseph, Semicon Media Pvt Ltd (Circuit Digest)**

## 👨‍💻 Author

**Jobit Joseph**  
Semicon Media Pvt Ltd (Circuit Digest)



## 🔄 Version History

- **v1.0.2** - Current Version (Latest)
- **Added full GSM/cellular modem(SIM800L at the moment)**
- Improvements, documentation updates, and stability enhancements.
- **v1.0.1** - Bug Fix  Release
- **v1.0.0** - Initial release with multi-platform support
- Features: GPS tracking, cloud integration, offline storage, auto-reconnection

---

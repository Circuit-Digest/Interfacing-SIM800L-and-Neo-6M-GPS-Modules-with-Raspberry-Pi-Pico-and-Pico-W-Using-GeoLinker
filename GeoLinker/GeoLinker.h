/* 
 * GeoLinker Library
 * Copyright (C) 2025 Jobit Joseph, Semicon Media Pvt Ltd (Circuit Digest)
 * Author: Jobit Joseph
 * Project: GeoLinker Cloud API Library
 *
 * Licensed under the MIT License
 * You may not use this file except in compliance with the License.
 * 
 * You may obtain a copy of the License at:
 * https://opensource.org/license/mit/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software (the "Software") and associated documentation files, to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, subject to the following additional conditions:

 * 1. All copies or substantial portions must retain:  
 *    - The original copyright notice  
 *    - A prominent statement crediting the original author/creator  

 * 2. Modified versions must:  
 *    - Clearly mark the changes as their own  
 *    - Preserve all original credit notices
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef GEOLINKER_H
#define GEOLINKER_H

#include <Arduino.h>
#include <vector>
#include <map>

// === WiFi/Network Library Includes ===
#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <HTTPClient.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecure.h>
#elif defined(ARDUINO_UNOWIFIR4)
  #include <WiFiS3.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_2W)
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #include <HTTPClient.h>
  
#else

#endif

#include <ArduinoJson.h>
#include <Stream.h>

// ==================== Debug Levels ====================
#define DEBUG_NONE     0
#define DEBUG_BASIC    1
#define DEBUG_VERBOSE  2

// ==================== Network Backend Type ====================
enum NetworkBackendType {
    GEOLINKER_WIFI = 0,
    GEOLINKER_CELLULAR = 1 // GSM/4G/5G modules
};

// ==================== Status Codes ====================
enum GeoLinkerStatus {
    STATUS_OK = 0,
    STATUS_SENT = 1,
    STATUS_GPS_ERROR = 2,
    STATUS_NETWORK_ERROR = 3,
    STATUS_BAD_REQUEST_ERROR = 4,
    STATUS_PARSE_ERROR = 5,
    STATUS_CELLULAR_NOT_REGISTERED = 6,
    STATUS_CELLULAR_CTX_ERROR = 7,
    STATUS_CELLULAR_DATA_ERROR = 8,
    STATUS_CELLULAR_TIMEOUT = 9,
    STATUS_INTERNAL_SERVER_ERROR = 10
};

// ==================== NMEA Parse Result ====================
enum ParseResult {
    PARSE_SUCCESS = 0,
    PARSE_NO_DATA = 1,
    PARSE_INVALID_STATUS = 2,
    PARSE_INVALID_FORMAT = 3
};

class GeoLinker {
public:
    GeoLinker();

    // ---- GPS Methods ----
    bool begin(Stream& gpsSerial);

    // ---- Universal API ----
    void setApiKey(const String& key);
    void setDeviceID(const String& id);
    void setUpdateInterval_seconds(uint32_t interval_s);
    void setDebugLevel(uint8_t level);
    void enableOfflineStorage(bool enable);
    void enableAutoReconnect(bool enable);
    void setPayloads(const std::map<String, float>& payloads);
    void setBatteryLevel(uint8_t percent);
    void setTimeOffset(int hourOffset, int minuteOffset);
    void setOfflineBufferLimit(uint8_t limit);

    // ---- Network Selection ----
    void setNetworkMode(NetworkBackendType mode);

    // ---- WiFi Methods ----
    void setWiFiCredentials(const char* ssid, const char* password);
    bool connectToWiFi();

    // ---- Modem/Cellular (GSM/4G/etc) ----
    void setModemCredentials(const char* apn, const char* user = nullptr, const char* pass = nullptr);
    void beginModem(Stream& serial, uint8_t powerPin = 255, uint8_t resetPin = 255, bool autoPowerCycle = false);
    void setModemTimeouts(uint32_t commandTimeoutMs = 5000, uint32_t httpTimeoutMs = 10000);

    // ---- Optional Diagnostics ----
    int getModemNetworkStatus();   // Registration status (Not Registered, Registered, etc)
    int getModemGprsStatus();      // PDP/Context status

    // ---- Main Operation ----
    uint8_t loop();

private:
    // --- GPS Serial ---
    Stream* gpsSerial = nullptr;
    String gpsBuffer;
    String lastValidGPRMC;
    bool newDataAvailable = false;
    unsigned long lastDataTime = 0;

    struct GpsData {
        float lat;
        float lon;
        String timestamp;
    };

    // --- Buffer/Storage ---
    std::vector<GpsData> offlineBuffer;
    uint8_t offlineBufferLimit = 100;

    // --- API/Data ---
    String apiKey;
    String deviceID;
    uint8_t debugLevel = DEBUG_NONE;
    uint32_t updateInterval = 60000;
    uint32_t lastUpdate = 0;
    bool offlineEnabled = false;
    bool reconnectEnabled = false;
    uint8_t batteryLevel = 255;
    int offsetHour = 0, offsetMin = 0;
    std::map<String, float> payloadMap;

    // --- Network Mode ---
    NetworkBackendType netMode = GEOLINKER_WIFI;

    // --- WiFi Credentials ---
    const char* wifi_ssid = nullptr;
    const char* wifi_password = nullptr;

    // --- Modem (GSM/4G) Backend ---
    Stream* modemSerial = nullptr;
    const char* modem_apn = nullptr;
    const char* modem_user = nullptr;
    const char* modem_pass = nullptr;
    int8_t modem_powerPin = -1, modem_resetPin = -1;
    bool modem_autoPowerCycle = false;
    uint32_t modem_cmdTimeout = 5000;
    uint32_t modem_httpTimeout = 10000;
    int lastNetworkRegStatus = 0;
    int lastGprsStatus = 0;

    // --- GPS/Parse Helpers ---
    void handleSerialData();
    ParseResult parseNMEA(String& line, float& lat, float& lon, String& utcTime);
    bool getCoordinates(float& lat, float& lon, String& timestamp, ParseResult& parseResult);
    void storeOfflineData(float lat, float lon, String timestamp);
    bool sendOfflineData();
    bool isValidGps(float lat, float lon);
    String serializePayload(float lat, float lon, String timestamp);

    // --- WiFi Networking ---
    uint16_t sendData(float lat, float lon, String timestamp);
    bool postToServer(const String& json);
    bool isWiFiConnected();
    void handleWiFiReconnect();

    // --- Modem Networking (Multi-module, SIM800L etc) ---
    bool modemPowerCycle();
    bool modemReset();
    bool modemInit();
    bool checkNetworkRegistration();
    bool checkGprsContext();
    bool modemHttpPost(const String& json, int& httpStatus, String& httpResponse);
    String modemSendAT(const String& cmd, uint32_t timeout = 2000, const char* expect = nullptr);
    bool modemWaitForResponse(const char* expect, uint32_t timeoutMs, String* respOut = nullptr);
    bool decodeModemResponse(const String& response);

    // --- Debug Helpers ---
    void debugPrint(const String& msg, uint8_t level);
    void debugPrintNoNewline(const String& msg, uint8_t level);
};

#endif

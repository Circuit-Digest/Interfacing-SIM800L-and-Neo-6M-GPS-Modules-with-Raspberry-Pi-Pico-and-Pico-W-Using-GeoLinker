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
 
#include <GeoLinker.h>

// ==================================================================
//                   GPS SERIAL CONFIGURATION
// ==================================================================

/* ----- Option 1: Hardware Serial with Custom Pins ----- */
#define gpsSerial Serial2  // UART1
#define GPS_RX 4       
#define GPS_TX 5       

/* ----- Option 2: Standard Hardware Serial (for Uno R4 WiFi, Pico W) ----- */
//HardwareSerial & gpsSerial = Serial2;  // Uses default pins:
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
#define gsmSerial Serial1  // UART0
#define GSM_RX 0
#define GSM_TX 1

/* ----- Option 2: Standard Hardware Serial (for Uno R4 WiFi, Pico W) ----- */
//HardwareSerial& gsmSerial = Serial1;  // Uses default pins where available

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
const char* apn = "gprs";         // Cellular APN
const char* gsmUser = nullptr;        // APN username if required
const char* gsmPass = nullptr;        // APN password if required

// ==================================================================
//                   GeoLinker CONFIGURATION
// ==================================================================
const char* apiKey = "xxxxxx";  // Your GeoLinker API key
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

  gpsSerial.setTX(GPS_RX);
  gpsSerial.setRX(GPS_TX);
  gpsSerial.begin(GPS_BAUD);

  gsmSerial.setTX(GSM_RX);
  gsmSerial.setRX(GSM_TX);
  gsmSerial.begin(GSM_BAUD);


  // Initialize GPS Serial (select one method)
  //gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_TX, GPS_RX);
 // gpsSerial.begin(GPS_BAUD);  // Default pins

  // Initialize GSM Serial (select one method)
  //gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_TX, GSM_RX);
  //gsmSerial.begin(GSM_BAUD);  // Default pins

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
  geo.beginModem(gsmSerial);
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
  geo.setBatteryLevel(90);
  
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

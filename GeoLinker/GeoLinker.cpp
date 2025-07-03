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

#include "GeoLinker.h"

// ======= PUBLIC API/CONFIG =======

GeoLinker::GeoLinker()
  : gpsSerial(nullptr), newDataAvailable(false), lastDataTime(0), netMode(GEOLINKER_WIFI)
{}

// --- Universal user functions ---
bool GeoLinker::begin(Stream& serial) {
    gpsSerial = &serial;
    return true;
}
void GeoLinker::setApiKey(const String& key)         { apiKey = key; }
void GeoLinker::setDeviceID(const String& id)        { deviceID = id; }
void GeoLinker::setUpdateInterval_seconds(uint32_t s){ updateInterval = s * 1000; }
void GeoLinker::setDebugLevel(uint8_t level)         { debugLevel = level; }
void GeoLinker::enableOfflineStorage(bool enable)    { offlineEnabled = enable; }
void GeoLinker::enableAutoReconnect(bool enable)     { reconnectEnabled = enable; }
void GeoLinker::setPayloads(const std::map<String, float>& pl) { payloadMap = pl; }
void GeoLinker::setBatteryLevel(uint8_t percent)     { batteryLevel = percent; }
void GeoLinker::setTimeOffset(int h, int m)          { offsetHour = h; offsetMin = m; }
void GeoLinker::setOfflineBufferLimit(uint8_t limit) { offlineBufferLimit = limit; }

// --- Network Backend Selection ---
void GeoLinker::setNetworkMode(NetworkBackendType mode) { netMode = mode; }

// --- WiFi backend API  ---
void GeoLinker::setWiFiCredentials(const char* ssid, const char* password) {
    wifi_ssid = ssid;
    wifi_password = password;
}
bool GeoLinker::connectToWiFi() {
    if (wifi_ssid == nullptr || wifi_password == nullptr) {
        debugPrint("WiFi credentials not set!", DEBUG_BASIC);
        return false;
    }
    debugPrint("Starting WiFi connection to: " + String(wifi_ssid), DEBUG_BASIC);
    WiFi.begin(wifi_ssid, wifi_password);

    unsigned long start = millis();
    unsigned long previousPrint = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        if (millis() - previousPrint >= 500) {
            previousPrint = millis();
            debugPrintNoNewline(".", DEBUG_BASIC);
        }
        yield();
    }
    if (WiFi.status() == WL_CONNECTED) {
        debugPrint("", DEBUG_BASIC);
        debugPrint("WiFi Connected Successfully!", DEBUG_BASIC);
        debugPrint("IP Address: " + WiFi.localIP().toString(), DEBUG_BASIC);
        debugPrint("Signal Strength: " + String(WiFi.RSSI()) + " dBm", DEBUG_BASIC);
        return true;
    }
    debugPrint("WiFi Connection Failed - Timeout", DEBUG_BASIC);
    return false;
}

// --- Modem (GSM/Cellular) API ---
void GeoLinker::setModemCredentials(const char* apn, const char* user, const char* pass) {
    modem_apn = apn; modem_user = user; modem_pass = pass;
}
void GeoLinker::beginModem(Stream& serial, uint8_t powerPin, uint8_t resetPin, bool autoPowerCycle) {
    modemSerial = &serial;
    modem_powerPin = powerPin;
    modem_resetPin = resetPin;
    modem_autoPowerCycle = autoPowerCycle;

    // Setup modem power/reset pins if needed
    if (modem_powerPin != -1) pinMode(modem_powerPin, OUTPUT);
    if (modem_resetPin != -1) pinMode(modem_resetPin, OUTPUT);

    // Power cycle if requested
    if (modem_autoPowerCycle && modem_powerPin != -1) {
        debugPrint("Power cycling modem...", DEBUG_BASIC);
        modemPowerCycle();
    }

    // Reset if reset pin is set
    if (modem_resetPin != -1) {
        debugPrint("Resetting modem...", DEBUG_BASIC);
        modemReset();
    }
}

void GeoLinker::setModemTimeouts(uint32_t commandTimeoutMs, uint32_t httpTimeoutMs) {
    modem_cmdTimeout = commandTimeoutMs;
    modem_httpTimeout = httpTimeoutMs;
}

// --- Diagnostics ---
int GeoLinker::getModemNetworkStatus() { return lastNetworkRegStatus; }
int GeoLinker::getModemGprsStatus()    { return lastGprsStatus; }


// ======= MAIN OPERATION =======

uint8_t GeoLinker::loop() {
    uint8_t status = 0;
    handleSerialData();

    // Send any buffered offline data (WiFi or Modem)
    if (offlineEnabled && !offlineBuffer.empty()) {
        bool sent = false;
        if ((netMode == GEOLINKER_WIFI && isWiFiConnected()) ||
            (netMode == GEOLINKER_CELLULAR && modemSerial && checkNetworkRegistration() && checkGprsContext())) {
            sent = sendOfflineData();
            if (sent && status != STATUS_BAD_REQUEST_ERROR) status = STATUS_SENT;
            else if (!sent && status == 0) status = STATUS_NETWORK_ERROR;
        }
    }

    // Main update cycle
    if (millis() - lastUpdate >= updateInterval) {
        float lat = 0.0, lon = 0.0;
        String time;
        ParseResult parseResult;
        if (getCoordinates(lat, lon, time, parseResult)) {
            int sendResult = -1;
            if (netMode == GEOLINKER_WIFI) {
                sendResult = sendData(lat, lon, time);
                if (sendResult == 200 || sendResult == 201) { status = STATUS_SENT; offlineBuffer.clear(); }
                else if (sendResult == 401 || sendResult == 400) {
                    status = STATUS_BAD_REQUEST_ERROR;
                    if (offlineEnabled) 
                        {
                            storeOfflineData(lat, lon, time);
                        } 
                    }
                else if (sendResult == 500) {
                    status = STATUS_INTERNAL_SERVER_ERROR; 
                    if (offlineEnabled) 
                        {
                            storeOfflineData(lat, lon, time);
                        }
                    }
                else if (netMode == GEOLINKER_WIFI && offlineEnabled) { storeOfflineData(lat, lon, time); status = STATUS_NETWORK_ERROR; }
            } else if (netMode == GEOLINKER_CELLULAR && modemSerial) {
                int httpCode = 0; String response;
                if (!checkNetworkRegistration()) { status = STATUS_CELLULAR_NOT_REGISTERED; }
                else if (!checkGprsContext())    { status = STATUS_CELLULAR_CTX_ERROR; }
                else {
                    if (modemHttpPost(serializePayload(lat, lon, time), httpCode, response)) {
                        if (httpCode == 200 || httpCode == 201) { status = STATUS_SENT; offlineBuffer.clear(); }
                        else if (httpCode == 401 || httpCode == 400) {
                             status = STATUS_BAD_REQUEST_ERROR; 
                             if (offlineEnabled) 
                                {
                                    storeOfflineData(lat, lon, time);
                                }
                            }
                            else if (httpCode == 500) {
                             status = STATUS_INTERNAL_SERVER_ERROR; 
                             if (offlineEnabled) 
                                {
                                    storeOfflineData(lat, lon, time);
                                }
                            }
                        else { status = STATUS_CELLULAR_DATA_ERROR; if (offlineEnabled) storeOfflineData(lat, lon, time); }
                        debugPrint("HTTP POST response: " + response, DEBUG_VERBOSE);
                    } else {
                        status = STATUS_CELLULAR_TIMEOUT; if (offlineEnabled) storeOfflineData(lat, lon, time); 
                    }
                }
            }

        } else {
            if (parseResult == PARSE_INVALID_STATUS || parseResult == PARSE_INVALID_FORMAT) { status = STATUS_PARSE_ERROR; }
            else { status = STATUS_GPS_ERROR; }
        }
        lastUpdate = millis();
    }

    // WiFi reconnect logic
    if (netMode == GEOLINKER_WIFI && reconnectEnabled && !isWiFiConnected()) {
        handleWiFiReconnect();
    }

    return status;
}

// ======= GPS/PARSE/BUFFER =======
void GeoLinker::handleSerialData() {
    while (gpsSerial && gpsSerial->available()) {
        char c = gpsSerial->read();
        if (debugLevel >= DEBUG_VERBOSE) Serial.write(c);
        if (c == '\n') {
            if (gpsBuffer.startsWith("$GPRMC")) {
                lastValidGPRMC = gpsBuffer;
                newDataAvailable = true;
                lastDataTime = millis();
                if (debugLevel >= DEBUG_VERBOSE) debugPrint("Stored GPRMC: " + lastValidGPRMC, DEBUG_VERBOSE);
            }
            gpsBuffer = "";
        } else if (c != '\r') {
            gpsBuffer += c;
        }
        if (gpsBuffer.length() > 128) gpsBuffer = "";
    }
}
bool GeoLinker::getCoordinates(float& lat, float& lon, String& timestamp, ParseResult& parseResult) {
    if (!newDataAvailable) {
        debugPrint("No new GPS data available", DEBUG_BASIC);
        parseResult = PARSE_NO_DATA;
        return false;
    }
    parseResult = parseNMEA(lastValidGPRMC, lat, lon, timestamp);
    newDataAvailable = false;
    return (parseResult == PARSE_SUCCESS);
}
ParseResult GeoLinker::parseNMEA(String& line, float& lat, float& lon, String& utcTime) {
    debugPrint("Parsing GPRMC: " + line, DEBUG_VERBOSE);
    int idx = 0; String parts[13]; String l = line;
    while (l.length() && idx < 13) {
        int comma = l.indexOf(',');
        parts[idx++] = l.substring(0, comma);
        l = (comma == -1) ? "" : l.substring(comma + 1);
    }
    if (idx < 10) { debugPrint("GPS NMEA format invalid - insufficient fields", DEBUG_BASIC); return PARSE_INVALID_FORMAT; }
    if (parts[2] != "A") { debugPrint("GPS data invalid (status not 'A')", DEBUG_VERBOSE); return PARSE_INVALID_STATUS; }
    String date = parts[9];
    if (date.length() != 6) { debugPrint("Invalid date format", DEBUG_VERBOSE); return PARSE_INVALID_FORMAT; }
    int day = date.substring(0,2).toInt(); int month = date.substring(2,4).toInt();
    if (day < 1 || day > 31 || month < 1 || month > 12) { debugPrint("Invalid day/month", DEBUG_VERBOSE); return PARSE_INVALID_FORMAT; }
    float rawLat = parts[3].toFloat(); float rawLon = parts[5].toFloat();
    int latDeg = int(rawLat/100); float latMin = rawLat - latDeg*100; lat = latDeg + (latMin/60.0); if (parts[4]=="S") lat = -lat;
    int lonDeg = int(rawLon/100); float lonMin = rawLon - lonDeg*100; lon = lonDeg + (lonMin/60.0); if (parts[6]=="W") lon = -lon;
    utcTime = "20" + date.substring(4,6) + "-" + date.substring(2,4) + "-" + date.substring(0,2) + " ";
    String t = parts[1];
    if (t.length() >= 6) {
        int hh = t.substring(0,2).toInt() + offsetHour, mm = t.substring(2,4).toInt() + offsetMin, ss = t.substring(4,6).toInt();
        if (mm >= 60) { mm -= 60; hh += 1; } if (hh >= 24) hh -= 24;
        if (mm < 0) { mm += 60; hh -= 1; } if (hh < 0) hh += 24;
        char buff[20]; sprintf(buff, "%02d:%02d:%02d", hh, mm, ss); utcTime += buff;
    } else { debugPrint("Invalid time format", DEBUG_BASIC); return PARSE_INVALID_FORMAT; }
    return PARSE_SUCCESS;
}
void GeoLinker::storeOfflineData(float lat, float lon, String timestamp) {
    if (offlineBuffer.size() >= offlineBufferLimit) offlineBuffer.erase(offlineBuffer.begin());
    offlineBuffer.push_back({lat, lon, timestamp});
    debugPrint("Stored offline data. Size: " + String(offlineBuffer.size()), DEBUG_BASIC);
}
bool GeoLinker::sendOfflineData() {
    bool atLeastOneSuccess = false;
    for (auto it = offlineBuffer.begin(); it != offlineBuffer.end(); ) {
        if (!isValidGps(it->lat, it->lon) || it->timestamp.length() == 0) {
            debugPrint("Skipping invalid offline data", DEBUG_BASIC);
            it = offlineBuffer.erase(it); continue;
        }
        if(netMode == GEOLINKER_WIFI)
        {
            int httpCode = sendData(it->lat, it->lon, it->timestamp);
        
            if ((httpCode == 200 || httpCode == 201 )) {
                debugPrint("Sent offline data", DEBUG_BASIC);
                it = offlineBuffer.erase(it);
                atLeastOneSuccess = true;
            } 
            else {
                debugPrint("Failed to send (Code: " + String(httpCode) + ")", DEBUG_BASIC);
                ++it;  // Keep for retry (network/timeout issues)
            }

            delay(100); // Brief pause between sends
        }
        else if (netMode == GEOLINKER_CELLULAR && modemSerial) {
            String response;int httpCode = 0;
            if (modemHttpPost(serializePayload(it->lat, it->lon, it->timestamp), httpCode, response) && (httpCode == 200 || httpCode == 201 )) {
                debugPrint("Sent offline data via cellular", DEBUG_BASIC); it = offlineBuffer.erase(it); atLeastOneSuccess = true; continue;
            }
            else {
                debugPrint("Failed to send Offline Data", DEBUG_BASIC);
                ++it;  // Keep for retry (network/timeout issues)
            }

            delay(100); // Brief pause between sends
            }
    }
    return atLeastOneSuccess;
}
bool GeoLinker::isValidGps(float lat, float lon) { return !(lat == 0.0 && lon == 0.0); }

// ======= JSON/PAYLOAD =======
String GeoLinker::serializePayload(float lat, float lon, String timestamp) {
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["timestamp"].to<JsonArray>().add(timestamp);
    doc["lat"].to<JsonArray>().add(lat);
    doc["long"].to<JsonArray>().add(lon);
    if (batteryLevel <= 100) doc["battery"].to<JsonArray>().add(batteryLevel);
    if (!payloadMap.empty()) {
        JsonArray p = doc["payload"].to<JsonArray>();
        JsonObject pl = p.add<JsonObject>();
        for (const auto& kv : payloadMap) pl[kv.first] = kv.second;
    }
    String out; serializeJson(doc, out); return out;
}

// ======= WIFI BACKEND =======
uint16_t GeoLinker::sendData(float lat, float lon, String timestamp) {
    if (!isWiFiConnected()) return 0;
    String out = serializePayload(lat, lon, timestamp);
#if defined(ESP32) || defined(ESP8266)
    HTTPClient http;
    #if defined(ESP8266)
        WiFiClientSecure client; client.setInsecure();
        http.begin(client, "https://www.circuitdigest.cloud/geolinker");
    #else
        http.begin("https://www.circuitdigest.cloud/geolinker");
    #endif
    http.addHeader("Authorization", apiKey);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(out);
    debugPrint("Payload: " + out, DEBUG_VERBOSE);
    debugPrint("HTTP code: " + String(httpCode), DEBUG_BASIC);
    http.end();
    return httpCode;
#elif defined(ARDUINO_UNOWIFIR4)
    WiFiSSLClient client;
    if (!client.connect("www.circuitdigest.cloud", 443)) {
        debugPrint("Connection failed", DEBUG_BASIC); return -1;
    }
    client.println("POST /geolinker HTTP/1.1");
    client.println("Host: www.circuitdigest.cloud");
    client.println("Authorization: " + apiKey);
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(out.length()));
    client.println("Connection: close");
    client.println();
    client.print(out);
    debugPrint("Payload: " + out, DEBUG_VERBOSE);
    unsigned long timeout = millis();
    while (client.available() == 0) { if (millis() - timeout > 5000) { debugPrint("Client timeout", DEBUG_BASIC); client.stop(); return -1; } }
    int httpCode = -1;
    if (client.available()) {
        String statusLine = client.readStringUntil('\n');
        int firstSpace = statusLine.indexOf(' '), secondSpace = statusLine.indexOf(' ', firstSpace + 1);
        if (firstSpace != -1 && secondSpace != -1) {
            String statusCode = statusLine.substring(firstSpace + 1, secondSpace); httpCode = statusCode.toInt();
        }
    }
    debugPrint("HTTP code: " + String(httpCode), DEBUG_BASIC);
    client.stop();
    return httpCode;
#else
    debugPrint("Unsupported board for HTTPS requests", DEBUG_BASIC);
    return 0;
#endif
}
bool GeoLinker::isWiFiConnected() {
#if defined(ESP32) || defined(ESP8266)
    return WiFi.status() == WL_CONNECTED;
#else
    return WiFi.status() == WL_CONNECTED;
#endif
}
void GeoLinker::handleWiFiReconnect() {
    static unsigned long lastReconnectAttempt = 0;
    static bool wasConnected = true;
    const unsigned long reconnectInterval = 5000;
    bool currentlyConnected = isWiFiConnected();
    if (wasConnected && !currentlyConnected) {
        debugPrint("WiFi Disconnected!", DEBUG_BASIC); wasConnected = false;
    }
    if (!wasConnected && currentlyConnected) {
        debugPrint("WiFi Reconnected Successfully!", DEBUG_BASIC);
        debugPrint("IP Address: " + WiFi.localIP().toString(), DEBUG_BASIC); wasConnected = true; return;
    }
    if (!currentlyConnected && millis() - lastReconnectAttempt >= reconnectInterval) {
        debugPrint("Attempting WiFi reconnection...", DEBUG_BASIC);
        lastReconnectAttempt = millis();
#if defined(ESP8266)
        WiFi.reconnect();
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_2W)
        if (wifi_ssid && wifi_password) WiFi.begin(wifi_ssid, wifi_password);
#else
        WiFi.disconnect();
        WiFi.begin(wifi_ssid, wifi_password);
#endif
    }
}

// ======= MODEM BACKEND (SIM800L Example) =======
bool GeoLinker::modemPowerCycle() {
    if (modem_powerPin == -1) return false;
    digitalWrite(modem_powerPin, LOW); delay(1200);
    digitalWrite(modem_powerPin, HIGH); delay(5000);
    debugPrint("Modem power cycled", DEBUG_BASIC); return true;
}
bool GeoLinker::modemReset() {
    if (modem_resetPin == -1) return false;
    digitalWrite(modem_resetPin, LOW); delay(200);
    digitalWrite(modem_resetPin, HIGH); delay(1000);
    debugPrint("Modem reset", DEBUG_BASIC); return true;
}
bool GeoLinker::modemInit() {
    // You may implement more advanced init if needed.
    modemSendAT("AT", modem_cmdTimeout, "OK");
    modemSendAT("ATE0", modem_cmdTimeout, "OK");
    return true;
}
bool GeoLinker::checkNetworkRegistration() {
    String resp = modemSendAT("AT+CREG?", modem_cmdTimeout, "+CREG:");
    int idx = resp.indexOf(',');
    lastNetworkRegStatus = (idx != -1 && idx + 1 < resp.length()) ? resp.substring(idx + 1, idx + 2).toInt() : -1;
    String reg;
    switch (lastNetworkRegStatus) {
        case 1: reg = "Registered (home)"; break;
        case 5: reg = "Registered (roaming)"; break;
        case 0: reg = "Not registered (searching)"; break;
        case 2: reg = "Searching"; break;
        case 3: reg = "Registration denied"; break;
        default: reg = "Unknown/No SIM"; break;
    }
    debugPrint("Modem network reg status: " + reg, DEBUG_BASIC);
    return (lastNetworkRegStatus == 1 || lastNetworkRegStatus == 5);
}
bool GeoLinker::checkGprsContext() {
    String resp = modemSendAT("AT+CGATT?", modem_cmdTimeout, "+CGATT:");
    lastGprsStatus = (resp.indexOf("1") != -1) ? 1 : 0;
    debugPrint("GPRS attach: " + String(lastGprsStatus ? "Attached" : "Not attached"), DEBUG_VERBOSE);
    if (!lastGprsStatus) {
        // Try to attach
        modemSendAT("AT+CGATT=1", modem_cmdTimeout, "OK");
        resp = modemSendAT("AT+CGATT?", modem_cmdTimeout, "+CGATT:");
        lastGprsStatus = (resp.indexOf("1") != -1) ? 1 : 0;
    }
    return lastGprsStatus == 1;
}


bool GeoLinker::modemHttpPost(const String& json, int& httpStatus, String& httpResponse) {
    debugPrint("Sending Data using GSM...", DEBUG_BASIC);

    // 0. Always close any existing session before new connection
    modemSendAT("AT+CIPCLOSE", modem_cmdTimeout, "CLOSE");
    modemSendAT("AT+CIPSHUT", modem_cmdTimeout, "SHUT OK");

    // 1. Configure GPRS
    modemSendAT("AT+CGATT=1", modem_cmdTimeout, "OK");
    modemSendAT("AT+CIPMUX=0", modem_cmdTimeout, "OK");
    modemSendAT("AT+CSTT=\"" + String(modem_apn) + "\"", modem_cmdTimeout, "OK");
    modemSendAT("AT+CIICR", modem_cmdTimeout, "OK");
    modemSendAT("AT+CIFSR", modem_cmdTimeout, "."); // Get IP

    // 2. Open TCP connection to server (port 80 for HTTP)
    String domain = "www.circuitdigest.cloud";
    String apiUrl = "/geolinker";
    int port = 80;
    modemSendAT("AT+CIPSTART=\"TCP\",\"" + domain + "\"," + String(port), modem_cmdTimeout, "OK");

    // 3. Wait for "CONNECT"
    unsigned long connectStart = millis();
    bool isConnected = false;
    String connectResponse = "";
    while (millis() - connectStart < 10000) {
        while (modemSerial && modemSerial->available()) {
            char c = modemSerial->read();
            connectResponse += c;
            if (connectResponse.indexOf("CONNECT") != -1) {
                isConnected = true;
                break;
            }
        }
        if (isConnected) break;
        delay(10);
    }
    if (!isConnected) {
        debugPrint("TCP connection failed!", DEBUG_BASIC);
        modemSendAT("AT+CIPCLOSE", modem_cmdTimeout, "CLOSE");
        modemSendAT("AT+CIPSHUT", modem_cmdTimeout, "SHUT OK");
        httpStatus = 0;
        httpResponse = "TCP connection failed";
        debugPrint("HTTP status: 0 (connection failed)", DEBUG_VERBOSE);
        return false;
    }

    // 4. Build HTTP POST request
    String crlf = String(char(13)) + String(char(10));
    String apiKeyHeader = "Authorization: " + apiKey + crlf;
    String contentTypeHeader = "Content-Type: application/json" + crlf;
    String contentLengthHeader = "Content-Length: " + String(json.length()) + crlf;
    String hostHeader = "Host: " + domain + crlf;
    String headers = hostHeader + apiKeyHeader + contentTypeHeader + contentLengthHeader;
    String httpRequest =
        "POST " + apiUrl + " HTTP/1.1" + crlf +
        headers +
        crlf +   // Empty line between headers and body!
        json;

    // 5. Send CIPSEND (wait for > prompt)
    modemSendAT("AT+CIPSEND", modem_cmdTimeout, ">");
    delay(50); // Allow time for prompt

    // 6. Send HTTP request, then Ctrl+Z (0x1A)
    debugPrint("Sending HTTP Request:\n" + httpRequest, DEBUG_VERBOSE);
    if (modemSerial) {
        modemSerial->print(httpRequest);
        modemSerial->write(0x1A);
    }

    // 7. Wait for the full server response (till timeout or till "}" after HTTP/1.1)
    String response = "";
    bool sawSendOk = false;
    bool sawIPD = false;
    unsigned long startTime = millis();
    while (millis() - startTime < modem_httpTimeout) {
    while (modemSerial && modemSerial->available()) {
            char c = modemSerial->read();
            response += c;
            if (!sawSendOk && response.indexOf("SEND OK") != -1) {
                sawSendOk = true;
                response = ""; // Clear the buffer—start fresh!
            }
            if (sawSendOk && !sawIPD && response.indexOf("+IPD,") != -1) {
                sawIPD = true;
            }
            // If after HTTP/1.1 we see a }, break inner loop to check outer loop
            if (sawIPD && response.indexOf("HTTP/1.1") != -1) {
                int headerPos = response.indexOf("HTTP/1.1");
                int bracePos = response.indexOf('}', headerPos);
                if (bracePos != -1) {
                    goto gotFullResponse; // Use goto for immediate break out of both loops
                }
            }
        }
        delay(10);
    }
    gotFullResponse:


    // 8. Extract HTTP status code from the true server response (after +IPD)
    int code = 0;
    int httpPos = response.indexOf("HTTP/1.1");
    if (httpPos >= 0 && response.length() > httpPos + 12) {
        code = response.substring(httpPos + 9, httpPos + 12).toInt();
    }
    httpStatus = code;
    httpResponse = response;
    debugPrint("HTTP status: " + String(httpStatus), DEBUG_BASIC);
    debugPrint("Server raw response:\n" + response, DEBUG_VERBOSE);

    modemSendAT("AT+CIPCLOSE", modem_cmdTimeout, "CLOSE");
    modemSendAT("AT+CIPSHUT", modem_cmdTimeout, "SHUT OK");

    return code;
}

String GeoLinker::modemSendAT(const String& cmd, uint32_t timeout, const char* expect) {
    if (!modemSerial) return "";
    while (modemSerial->available()) modemSerial->read(); // Clear RX
    modemSerial->println(cmd);
    if (debugLevel >= DEBUG_VERBOSE) debugPrint(">> " + cmd, DEBUG_VERBOSE);
    unsigned long start = millis(); String resp;
    while (millis() - start < timeout) {
        while (modemSerial->available()) {
            char c = modemSerial->read();
            resp += c;
            if (debugLevel >= DEBUG_VERBOSE) Serial.write(c);
        }
        if (expect && resp.indexOf(expect) != -1) break;
        delay(10);
    }
    if (debugLevel >= DEBUG_VERBOSE && resp.length()) debugPrint("<< " + resp, DEBUG_VERBOSE);
    return resp;
}
bool GeoLinker::modemWaitForResponse(const char* expect, uint32_t timeoutMs, String* respOut) {
    if (!modemSerial) return false;
    unsigned long start = millis(); String resp;
    while (millis() - start < timeoutMs) {
        while (modemSerial->available()) {
            char c = modemSerial->read(); resp += c;
            if (debugLevel >= DEBUG_VERBOSE) Serial.write(c);
        }
        if (resp.indexOf(expect) != -1) {
            if (respOut) *respOut = resp;
            return true;
        }
        delay(10);
    }
    if (respOut) *respOut = resp;
    return false;
}
bool GeoLinker::decodeModemResponse(const String& response) {
    if (response.indexOf("OK") != -1) return true;
    if (response.indexOf("ERROR") != -1) return false;
    return false; // Add module-specific error handling as needed
}

// ======= DEBUG HELPERS =======
void GeoLinker::debugPrint(const String& msg, uint8_t level) {
    if (debugLevel >= level) { Serial.println("[GeoLinker] " + msg); }
}
void GeoLinker::debugPrintNoNewline(const String& msg, uint8_t level) {
    if (debugLevel >= level) { Serial.print(msg); }
}

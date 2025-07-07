#include <GeoLinker.h>


// ==================================================================
//                   GPS SERIAL CONFIGURATION
// ==================================================================

#define gpsSerial Serial2  // UART1
#define GPS_RX 4
#define GPS_TX 5
#define GPS_BAUD 9600   // Standard NMEA baud rate

// ==================================================================
//                   GSM SERIAL CONFIGURATION
// ==================================================================

#define gsmSerial Serial1  // UART0
#define GSM_RX 0
#define GSM_TX 1
#define GSM_BAUD 9600   // Standard modem baud rate

// ==============================
//         LED DEFINITIONS
// ==============================
#define LED_GREEN 21   // Success indicator
#define LED_YELLOW 20  // GPS error indicator
#define LED_RED 19     // GSM error indicator

// ==============================
//       TIMING AND STATE
// ==============================
bool greenLedOn = false;
unsigned long greenLedTimer = 0;

bool gpsErrorActive = false;
bool gsmErrorActive = false;

unsigned long lastGPSStatusTime = 0;
unsigned long lastGSMStatusTime = 0;

const unsigned long errorTimeout = 8000; // 8 seconds to auto-clear error

// ==============================
//     NETWORK CONFIGURATION
// ==============================
const char* apn = "gprs"; // Cellular APN
const char* gsmUser = nullptr;  // APN username if required
const char* gsmPass = nullptr;  // APN password if required

// ==============================
//    GEO LINKER CONFIGURATION
// ==============================
const char* apiKey = "xxxxxxx";  // Your GeoLinker API key
const char* deviceID = "GeoLinker_tracker_test1"; // Unique device identifier
const uint16_t updateInterval = 3;  // Data upload interval (seconds)
const bool enableOfflineStorage = true; // Enable offline data storage
const uint8_t offlineBufferLimit = 20;  // Max stored offline record, Keep it minimal for MCUs with less RAM
const bool enableAutoReconnect = true;  // Enable auto-reconnect Only for WiFi, Ignored with GSM
const int8_t timeOffsetHours = 5; // UTC + Hours 
const int8_t timeOffsetMinutes = 30;  // UTC + Minutes 

GeoLinker geo;  //GeoLinker instance

void setup() {
  Serial.begin(115200);
  delay(1000);

  // GPS UART
  gpsSerial.setTX(GPS_RX);
  gpsSerial.setRX(GPS_TX);
  gpsSerial.begin(GPS_BAUD);

  // GSM UART
  gsmSerial.setTX(GSM_RX);
  gsmSerial.setRX(GSM_TX);
  gsmSerial.begin(GSM_BAUD);

  // LED Setup
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  // GeoLinker Init
  geo.begin(Serial2);
  geo.setApiKey(apiKey);
  geo.setDeviceID(deviceID);
  geo.setUpdateInterval_seconds(updateInterval);
  geo.setDebugLevel(DEBUG_BASIC);
  geo.enableOfflineStorage(enableOfflineStorage);
  geo.setOfflineBufferLimit(offlineBufferLimit);
  geo.enableAutoReconnect(enableAutoReconnect);
  geo.setTimeOffset(timeOffsetHours, timeOffsetMinutes);

  geo.setNetworkMode(GEOLINKER_CELLULAR);
  geo.setModemCredentials(apn, gsmUser, gsmPass);
  geo.beginModem(Serial1);
  geo.setModemTimeouts(5000, 10000);

  Serial.println("GeoLinker setup complete.");
}

void loop() {
  unsigned long now = millis();

  // Example payload
  geo.setPayloads({
    {"temperature", 27.5},
    {"humidity", 65.3}
  });

  geo.setBatteryLevel(90);

  uint8_t status = geo.loop();

  // =========================================
  //  SUCCESS: Green LED for 1 second
  // =========================================
  if (status == STATUS_SENT) {
    Serial.println("Data sent successfully!");

    if (!greenLedOn) {
      digitalWrite(LED_GREEN, HIGH);
      delay(500);
      greenLedOn = true;
      greenLedTimer = now;
    }

    gpsErrorActive = false;
    gsmErrorActive = false;
  }

  if (greenLedOn && (now - greenLedTimer >= 1000)) {
    digitalWrite(LED_GREEN, LOW);
    greenLedOn = false;
  }


  // =========================================
  //  GPS ERROR LED Handling
  // =========================================
  if (status == STATUS_GPS_ERROR || status == STATUS_PARSE_ERROR) {
    gpsErrorActive = true;
    lastGPSStatusTime = now;
  } else if (gpsErrorActive && (status == STATUS_SENT || status == 0)) {
    if (now - lastGPSStatusTime > errorTimeout) {
      gpsErrorActive = false;
    }
  }

  digitalWrite(LED_YELLOW, gpsErrorActive ? HIGH : LOW);

  // =========================================
  //  GSM ERROR LED Handling
  // =========================================
  bool gsmError = (
    status == STATUS_NETWORK_ERROR ||
    status == STATUS_CELLULAR_NOT_REGISTERED ||
    status == STATUS_CELLULAR_CTX_ERROR ||
    status == STATUS_CELLULAR_DATA_ERROR ||
    status == STATUS_CELLULAR_TIMEOUT ||
    status == STATUS_BAD_REQUEST_ERROR ||
    status == STATUS_INTERNAL_SERVER_ERROR
  );

  if (gsmError) {
    gsmErrorActive = true;
    lastGSMStatusTime = now;
  } else if (gsmErrorActive && (now - lastGSMStatusTime > errorTimeout)) {
    gsmErrorActive = false;
  }

  digitalWrite(LED_RED, gsmErrorActive ? HIGH : LOW);
}

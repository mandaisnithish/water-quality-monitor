// Cloud upload test with SIMULATED data (no sensors are read).
// ESP8266 pushes random water quality values to Firebase RTDB every 15 s,
// shows the same values on an SSD1306 OLED, and drives a relay from /relay1.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "secrets.h"  // WIFI_SSID, WIFI_PASSWORD, FIREBASE_API_KEY, FIREBASE_DATABASE_URL

const int RELAY_PIN = 5;  // GPIO5 (D1 on NodeMCU)
const unsigned long UPLOAD_INTERVAL_MS = 15000;
const unsigned long RELAY_POLL_INTERVAL_MS = 1000;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_I2C_ADDRESS = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

FirebaseData relayReadData;
FirebaseData uploadData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

bool firebaseSignupOk = false;
unsigned long lastUploadMs = 0;
unsigned long lastRelayPollMs = 0;
bool firstUploadDone = false;

struct WaterReadings {
  float temperatureC;
  float tdsPpm;
  float ec;
  float orpMv;
  float ph;
};

// random values in plausible ranges, for testing the cloud pipeline only
WaterReadings makeSimulatedReadings() {
  WaterReadings readings;
  readings.temperatureC = random(200, 300) / 10.0;
  readings.tdsPpm = random(1000, 5000) / 10.0;
  readings.ec = random(10, 30) / 10.0;
  readings.orpMv = random(2000, 4000) / 10.0;
  readings.ph = random(60, 80) / 10.0;
  return readings;
}

void showReadings(const WaterReadings &readings) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SIMULATED DATA");
  display.println("pH: " + String(readings.ph, 1));
  display.println("Temp: " + String(readings.temperatureC, 1) + " C");
  display.println("ORP: " + String(readings.orpMv, 1) + " mV");
  display.println("TDS: " + String(readings.tdsPpm, 1) + " ppm");
  display.println("EC: " + String(readings.ec, 1));
  display.display();
}

void uploadFloat(const char* path, float value) {
  if (Firebase.RTDB.setFloat(&uploadData, path, value)) Serial.printf("%s PASSED\n", path);
  else Serial.printf("%s FAILED: %s\n", path, uploadData.errorReason().c_str());
}

void uploadReadings(const WaterReadings &readings) {
  uploadFloat("Values/Temperature", readings.temperatureC);
  uploadFloat("Values/TDS", readings.tdsPpm);
  uploadFloat("Values/EC", readings.ec);
  uploadFloat("Values/ORP", readings.orpMv);
  uploadFloat("Values/Ph", readings.ph);
}

// "0" -> pin HIGH, "1" -> pin LOW (same mapping as the original)
void syncRelayFromFirebase() {
  if (!Firebase.RTDB.getString(&relayReadData, "/relay1")) return;
  if (relayReadData.dataType() != "string") return;
  String relayValue = relayReadData.stringData();
  if (relayValue == "0") digitalWrite(RELAY_PIN, HIGH);
  else if (relayValue == "1") digitalWrite(RELAY_PIN, LOW);
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) Serial.println("OLED not found");
  display.clearDisplay();
  display.display();

  connectWifi();

  firebaseConfig.api_key = FIREBASE_API_KEY;
  firebaseConfig.database_url = FIREBASE_DATABASE_URL;
  // anonymous sign-up, needs Anonymous auth enabled in the Firebase project
  if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) firebaseSignupOk = true;
  else Serial.println(firebaseConfig.signer.signupError.message.c_str());

  firebaseConfig.token_status_callback = tokenStatusCallback;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (millis() - lastRelayPollMs > RELAY_POLL_INTERVAL_MS) {
    lastRelayPollMs = millis();
    syncRelayFromFirebase();
  }

  bool uploadDue = !firstUploadDone || millis() - lastUploadMs > UPLOAD_INTERVAL_MS;
  if (Firebase.ready() && firebaseSignupOk && uploadDue) {
    lastUploadMs = millis();
    firstUploadDone = true;
    WaterReadings readings = makeSimulatedReadings();
    showReadings(readings);
    uploadReadings(readings);
  }
}

// Earlier prototype: two relays controlled from Firebase RTDB paths /relay1 and /relay2.

#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "secrets.h"  // WIFI_SSID, WIFI_PASSWORD, FIREBASE_API_KEY, FIREBASE_DATABASE_URL

const int RELAY1_PIN = 5;  // GPIO5
const int RELAY2_PIN = 4;  // GPIO4
const unsigned long RELAY_POLL_INTERVAL_MS = 1000;

FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// "0" = relay on (pin HIGH), "1" = relay off (pin LOW), as in the original
void syncRelayFromFirebase(const char* path, int relayPin) {
  if (!Firebase.RTDB.getString(&firebaseData, path)) {
    Serial.printf("Error reading %s: %s\n", path, firebaseData.errorReason().c_str());
    return;
  }
  if (firebaseData.dataType() != "string") {
    Serial.printf("%s missing or not a string\n", path);
    return;
  }
  String value = firebaseData.stringData();
  Serial.printf("%s = %s\n", path, value.c_str());
  if (value == "0") digitalWrite(relayPin, HIGH);
  else if (value == "1") digitalWrite(relayPin, LOW);
  else Serial.println("Invalid data received");
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());

  firebaseConfig.api_key = FIREBASE_API_KEY;
  firebaseConfig.database_url = FIREBASE_DATABASE_URL;
  if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) Serial.println("Firebase sign-up ok");
  else Serial.println(firebaseConfig.signer.signupError.message.c_str());

  firebaseConfig.token_status_callback = tokenStatusCallback;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
}

void loop() {
  syncRelayFromFirebase("/relay1", RELAY1_PIN);
  syncRelayFromFirebase("/relay2", RELAY2_PIN);
  delay(RELAY_POLL_INTERVAL_MS);
}

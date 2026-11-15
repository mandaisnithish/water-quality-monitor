#!/usr/bin/env bash
# Install the board cores and libraries this repo needs.
# Requires arduino-cli (https://arduino.github.io/arduino-cli/).

set -euo pipefail

URLS="https://arduino.esp8266.com/stable/package_esp8266com_index.json"

arduino-cli core update-index --additional-urls "$URLS"
arduino-cli core install arduino:avr esp8266:esp8266 --additional-urls "$URLS"

arduino-cli lib install "OneWire" "DallasTemperature" "Adafruit GFX Library" "Adafruit SSD1306" "Firebase Arduino Client Library for ESP8266 and ESP32"

# GravityTDS is not in the Library Manager index
ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true \
  arduino-cli lib install --git-url https://github.com/DFRobot/GravityTDS.git

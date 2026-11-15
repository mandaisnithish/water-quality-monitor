# Water Quality Monitor

Firmware for measuring pH, TDS, ORP and water temperature with low-cost analog probes, and for getting readings into a Firebase Realtime Database where a dashboard can show them and switch a relay (a pump, for example).

The project is in two halves that are not joined up yet: the sensor side reads real probes, and the cloud side currently uploads generated test values. See [Status](#status).

## Sketches

| Sketch | Board | What it does |
|---|---|---|
| `firmware/sensor_node_uno` | Uno | Reads pH (A0), TDS (A1), ORP (A3) and a DS18B20 (D2) and prints them. pH is filtered; TDS is temperature compensated |
| `firmware/sensor_node_nano_csv` | Nano | Reads the same sensors and prints one CSV line every 5 s on the UART, as the link to a Wi-Fi board |
| `firmware/cloud_upload_simulator_esp8266` | ESP8266 (NodeMCU) | Signs in to Firebase, uploads a set of values every 15 s, shows them on an SSD1306 OLED and follows a relay state stored in the database |
| `iterations/firebase_relay_control_esp8266` | ESP8266 | Earlier sketch: two relays controlled from the database, no sensor data |

## Measurement details

**pH** (`sensor_node_uno`): ten ADC samples 10 ms apart are sorted, the two lowest and two highest are dropped, and the middle six are averaged. That takes out the spikes these probe boards produce without the lag of a long moving average. The result is converted with a fixed slope of 3.5 pH/V taken from the probe board's reference code.

**TDS**: the DFRobot GravityTDS library does the conversion. It is given the water temperature measured by the DS18B20 so the reading is compensated; if the DS18B20 is missing it falls back to 25 °C.

**ORP**: the ADC voltage in millivolts minus a calibration offset (0 by default).

**CSV format** (`sensor_node_nano_csv`): `ph,temp_c,orp_mv,tds_ppm,ec`

## Firebase layout

| Path | Type | Written by |
|---|---|---|
| `Values/Temperature`, `Values/TDS`, `Values/EC`, `Values/ORP`, `Values/Ph` | float | ESP8266, every 15 s |
| `/relay1` | string `"0"` or `"1"` | Dashboard; polled by the ESP8266 once per second |

`"0"` drives the relay pin HIGH and `"1"` drives it LOW. The ESP8266 uses anonymous sign-in, so Anonymous auth has to be enabled in the Firebase project.

## Pins

| Sketch | Signal | Pin |
|---|---|---|
| sensor_node_uno | pH / TDS / ORP | A0 / A1 / A3 |
| sensor_node_nano_csv | pH / TDS / ORP | A0 / A1 / A2 |
| both sensor nodes | DS18B20 data | D2 |
| cloud_upload_simulator_esp8266 | Relay | GPIO5 (D1) |
| cloud_upload_simulator_esp8266 | OLED (I2C, 0x3C) | default SDA / SCL |

## Build and upload

Libraries: **OneWire**, **DallasTemperature**, **GravityTDS** (from [DFRobot's GitHub](https://github.com/DFRobot/GravityTDS), not in the Library Manager), **Adafruit SSD1306**, **Adafruit GFX Library**, **Firebase Arduino Client Library for ESP8266 and ESP32**.

For the ESP8266 sketches, copy `secrets.example.h` to `secrets.h` and fill in the Wi-Fi name and password, the Firebase web API key and the database URL.

```bash
arduino-cli compile --fqbn arduino:avr:uno  firmware/sensor_node_uno
arduino-cli compile --fqbn arduino:avr:nano firmware/sensor_node_nano_csv
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/cloud_upload_simulator_esp8266
```

## Tested versions

Last compiled with `arduino-cli` 1.6.0 against:

| Package | Version |
|---|---|
| Arduino AVR Boards (`arduino:avr`) | 1.8.8 |
| ESP8266 (`esp8266:esp8266`) | 3.1.2 |
| OneWire | 2.3.8 |
| DallasTemperature | 4.0.6 |
| Adafruit GFX Library | 1.13.0 |
| Adafruit SSD1306 | 2.5.17 |
| Firebase Arduino Client Library for ESP8266 and ESP32 | 4.4.17 |
| GravityTDS (DFRobot, from GitHub) | untagged |

`scripts/install_deps.sh` installs all of these and `scripts/compile_all.sh` builds every sketch in the repo. Both run in CI on each push.

## Status

- `cloud_upload_simulator_esp8266` uploads **random values in plausible ranges**, not sensor readings. It exists to prove the Wi-Fi, Firebase and relay path, and the OLED says "SIMULATED DATA" while it runs.
- The ESP8266 does not read the Nano's CSV line yet. That link is the missing piece between the two halves.
- `sensor_node_nano_csv` uses simple linear placeholder conversions for pH, ORP, TDS and EC. `sensor_node_uno` has the better pH and TDS handling.
- Nothing is calibrated. pH needs a two-point buffer calibration and ORP an offset against a reference solution before the numbers mean anything.
- Both sensor sketches assume a 5 V ADC reference.

## Ideas for later

- Parse the CSV line on the ESP8266 and upload real readings.
- Move the Uno's pH filter and compensated TDS into the Nano sketch so there is one sensor node.
- Store pH calibration in EEPROM.
- Replace the once-per-second relay polling with a Firebase stream.

## License

MIT, see [LICENSE](LICENSE).

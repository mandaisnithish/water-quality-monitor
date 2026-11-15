// Water quality sensor node (Nano): reads pH, ORP, TDS (analog) and temperature (DS18B20)
// and sends one CSV line every 5 s over the hardware UART, intended for an ESP8266.
// CSV format: ph,temp_c,orp_mv,tds_ppm,ec
// NOTE: the pH, ORP, TDS and EC conversions below are placeholder formulas, not calibrated.

#include <OneWire.h>
#include <DallasTemperature.h>

const int PH_SENSOR_PIN = A0;
const int TDS_SENSOR_PIN = A1;
const int ORP_SENSOR_PIN = A2;
const int ONE_WIRE_BUS_PIN = 2;

const float ADC_REF_VOLTS = 5.0;
const float ADC_MAX_COUNT = 1023.0;
const unsigned long SEND_INTERVAL_MS = 5000;

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature temperatureSensors(&oneWire);

float analogPinToVolts(int pin) {
  return analogRead(pin) * (ADC_REF_VOLTS / ADC_MAX_COUNT);
}

float readPhValue() {
  // placeholder linear model centred on 2.5 V
  return (analogPinToVolts(PH_SENSOR_PIN) - 2.5) * -13.3;
}

float readWaterTempC() {
  temperatureSensors.requestTemperatures();
  return temperatureSensors.getTempCByIndex(0);
}

float readOrpMillivolts() {
  return (analogPinToVolts(ORP_SENSOR_PIN) - 2.5) * 1000;  // placeholder
}

float readTdsPpm() {
  return analogPinToVolts(TDS_SENSOR_PIN) * 500;  // placeholder
}

float tdsToEc(float tdsPpm) {
  return tdsPpm / 500.0;  // placeholder
}

void setup() {
  Serial.begin(9600);
  temperatureSensors.begin();
}

void loop() {
  float phValue = readPhValue();
  float waterTempC = readWaterTempC();
  float orpMillivolts = readOrpMillivolts();
  float tdsPpm = readTdsPpm();
  float ecValue = tdsToEc(tdsPpm);

  String csvLine = String(phValue) + "," + String(waterTempC) + "," + String(orpMillivolts) + "," +
                   String(tdsPpm) + "," + String(ecValue);
  Serial.println(csvLine);

  delay(SEND_INTERVAL_MS);
}

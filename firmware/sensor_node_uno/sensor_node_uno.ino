// Water quality sensor node: pH, TDS, ORP (analog) and water temperature (DS18B20),
// printed to the serial monitor. TDS is temperature compensated with the measured temperature.
// Board: Arduino UNO, 5 V ADC reference, 10-bit ADC.

#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "GravityTDS.h"

const int PH_SENSOR_PIN = A0;
const int TDS_SENSOR_PIN = A1;
const int ORP_SENSOR_PIN = A3;
const int ONE_WIRE_BUS_PIN = 2;

const float ADC_REF_VOLTS = 5.0;
const int ADC_RESOLUTION_COUNTS = 1024;

const int PH_SAMPLE_COUNT = 10;
const float PH_SLOPE = 3.5;            // uncalibrated, from sensor example code
const float ORP_OFFSET_MV = 0.0;       // calibrate against a reference solution
const float DEFAULT_WATER_TEMP_C = 25.0;  // used if DS18B20 is missing

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature temperatureSensors(&oneWire);
GravityTDS gravityTds;

// median-style filter: sort 10 samples, average the middle 6
float readPhValue() {
  int samples[PH_SAMPLE_COUNT];
  for (int i = 0; i < PH_SAMPLE_COUNT; i++) {
    samples[i] = analogRead(PH_SENSOR_PIN);
    delay(10);
  }
  for (int i = 0; i < PH_SAMPLE_COUNT - 1; i++) {
    for (int j = i + 1; j < PH_SAMPLE_COUNT; j++) {
      if (samples[i] > samples[j]) {
        int swap = samples[i];
        samples[i] = samples[j];
        samples[j] = swap;
      }
    }
  }
  unsigned long middleSum = 0;
  for (int i = 2; i < 8; i++) middleSum += samples[i];

  float sensorVolts = (float)middleSum * ADC_REF_VOLTS / ADC_RESOLUTION_COUNTS / 6;
  return PH_SLOPE * sensorVolts;
}

float readWaterTempC() {
  temperatureSensors.requestTemperatures();
  float tempC = temperatureSensors.getTempCByIndex(0);
  if (tempC == DEVICE_DISCONNECTED_C) return DEFAULT_WATER_TEMP_C;
  return tempC;
}

float readTdsPpm(float waterTempC) {
  gravityTds.setTemperature(waterTempC);
  gravityTds.update();
  return gravityTds.getTdsValue();
}

float readOrpMillivolts() {
  int adcCount = analogRead(ORP_SENSOR_PIN);
  float sensorVolts = adcCount * (ADC_REF_VOLTS / (ADC_RESOLUTION_COUNTS - 1));
  return sensorVolts * 1000 - ORP_OFFSET_MV;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");

  gravityTds.setPin(TDS_SENSOR_PIN);
  gravityTds.setAref(ADC_REF_VOLTS);
  gravityTds.setAdcRange(ADC_RESOLUTION_COUNTS);
  gravityTds.begin();

  temperatureSensors.begin();
}

void loop() {
  float waterTempC = readWaterTempC();
  float tdsPpm = readTdsPpm(waterTempC);
  float orpMillivolts = readOrpMillivolts();
  float phValue = readPhValue();

  Serial.print("tds : ");
  Serial.print(tdsPpm, 0);
  Serial.println("ppm");
  Serial.print("ORP Value: ");
  Serial.print(orpMillivolts);
  Serial.println(" mV");
  Serial.print(" pH:");
  Serial.println(phValue, 2);
  Serial.print("Temperature: ");
  Serial.print(waterTempC);
  Serial.println(" C");

  delay(800);
}

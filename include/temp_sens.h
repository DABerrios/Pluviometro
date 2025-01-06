#ifndef TMP102_H
#define TMP102_H
#include <Arduino.h>
#include <Wire.h>
#include <SparkFunTMP102.h>
#include <SparkFunBME280.h>


const int alertPin = 33; // Pin number for the alert pin

float temp_102_read();
void temp_102_init();
void temp_BME280_init();
float BME280_read_temp();
float BME280_read_pressure();
float BME280_read_humidity();
extern TMP102 sensor_tmp102;
extern BME280 sensor_bme208;
extern BME280_SensorMeasurements sensor_BME280_measurements;







#endif /*TMP102_H */
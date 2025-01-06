#include <temp_sens.h>
#include <main.h>

TMP102 sensor_tmp102;
BME280 sensor_bme208;
BME280_SensorMeasurements sensor_BME280_measurements;

/**
 * @brief Initializes the TMP102 temperature sensor.
 * 
 * This function initializes the TMP102 temperature sensor by setting up the 
 * necessary configurations. It checks if the sensor can connect, and if not, 
 * it prints an error message and enters an infinite loop. The function sets 
 * the number of consecutive faults before triggering an alert, the alert 
 * polarity to active HIGH, the alert mode to comparator mode, and the low 
 * and high temperature settings. Finally, it wakes up the TMP102 sensor.
 * 
 * @note The low temperature setting is 60°C and the high temperature setting is 61°C.
 */
void temp_102_init(){
    if (!sensor_tmp102.begin())
    {
        Serial.println("TMP102 cannot connect. Freezing...");
        while (1);
    }
    sensor_tmp102.setFault(0); // Set the number of consecutive faults before triggering an alert
    sensor_tmp102.setAlertPolarity(1); // Active HIGH
    sensor_tmp102.setAlertMode(0); // Set to comparator mode
    sensor_tmp102.setLowTempC(60); // Low temp setting
    sensor_tmp102.setHighTempC(61); // High temp setting
    sensor_tmp102.wakeup(); // Wake up the TMP102

}
/**
 * @brief Reads the temperature from the TMP102 sensor.
 * 
 * This function reads the temperature in degrees Celsius from the TMP102 sensor
 * and returns it as a float value.
 * 
 * @return float The temperature in degrees Celsius.
 */
float temp_102_read(){
    return sensor_tmp102.readTempC();
}

/**
 * @brief Initializes the BME280 sensor with specific settings.
 * 
 * This function sets up the BME280 sensor by configuring various parameters such as 
 * reference pressure, filter, mode, oversampling rates for pressure, humidity, and temperature, 
 * and standby time. If the sensor cannot be connected, the function will print an error message 
 * and enter an infinite loop to halt further execution.
 * 
 * @note The function assumes that the sensor_bme208 object has been properly instantiated 
 * and is accessible within the scope of this function.
 */
void temp_BME280_init(){
    if (!sensor_bme208.begin())
    {
        Serial.println("BME280 cannot connect. Freezing...");
        while (1);
    }
    sensor_bme208.setReferencePressure(1013.25);
    sensor_bme208.setFilter(0);
    sensor_bme208.setMode(0);
    sensor_bme208.setPressureOverSample(16);
    sensor_bme208.setHumidityOverSample(16);
    sensor_bme208.setStandbyTime(0);
    sensor_bme208.setTempOverSample(16);
    sensor_bme208.setHumidityOverSample(1);
    sensor_bme208.setPressureOverSample(1);
    sensor_bme208.setMode(MODE_NORMAL);
}
/**
 * @brief Reads the temperature from the BME280 sensor.
 *
 * This function waits until the BME280 sensor has finished measuring,
 * then reads all measurements from the sensor and returns the temperature.
 *
 * @return The temperature measured by the BME280 sensor.
 */
float BME280_read_temp(){
    float temp;
    while (sensor_bme208.isMeasuring()){
    }   
    sensor_bme208.readAllMeasurements(&sensor_BME280_measurements);
    temp = sensor_BME280_measurements.temperature;
    return temp;
}
/**
 * @brief Reads the pressure from the BME280 sensor.
 *
 * This function waits until the BME280 sensor has finished measuring,
 * then reads all measurements from the sensor and extracts the pressure value.
 *
 * @return The pressure value read from the BME280 sensor.
 */
float BME280_read_pressure(){
    float pressure;
    while (sensor_bme208.isMeasuring()){
    } 
    sensor_bme208.readAllMeasurements(&sensor_BME280_measurements);
    pressure = sensor_BME280_measurements.pressure;
    return pressure;
}
/**
 * @brief Reads the humidity from the BME280 sensor.
 *
 * This function waits until the BME280 sensor has finished measuring,
 * then reads all measurements from the sensor and returns the humidity value.
 *
 * @return The humidity value read from the BME280 sensor.
 */
float BME280_read_humidity(){
    float humidity;
    while (sensor_bme208.isMeasuring()){
    } 
    sensor_bme208.readAllMeasurements(&sensor_BME280_measurements);
    humidity = sensor_BME280_measurements.humidity;
    return humidity;
}
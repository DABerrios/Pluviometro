#include <air_sens.h>
#include <main.h>
/**
 * @var ccs811
 * @brief CCS811 sensor object instantiated with the I2C address.
 */
CCS811 ccs811(CCS811_ADDR);
/**
 * @brief Stops the CCS811 sensor by setting its drive mode to 0.
 * 
 * This function first checks if the CCS811 sensor is properly initialized.
 * If the sensor is not found, it prints an error message to the serial monitor
 * and enters an infinite loop to halt further execution. If the sensor is found,
 * it sets the drive mode of the CCS811 sensor to 0, effectively stopping it.
 */
void ccs811_stop(){
    if(!ccs811.begin()){
        Serial.println("CCS811 sensor not found. Freezing...");
        while(1);
    }
    ccs811.setDriveMode(0);
    
}
/**
 * @brief Initializes the CCS811 sensor.
 * 
 * This function initializes the CCS811 sensor by checking if the sensor is 
 * connected and setting its drive mode. If the sensor is not found, the 
 * function will print an error message and enter an infinite loop.
 * 
 */
void ccs811_init(){
    if(!ccs811.begin()){
        Serial.println("CCS811 sensor not found. Freezing...");
        while(1);
    }
    ccs811.setDriveMode(1);
}
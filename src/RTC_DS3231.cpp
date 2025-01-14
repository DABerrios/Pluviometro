#include <Arduino.h>
#include <RTClib.h>
#include <main.h>

/* Create an RTC object */
RTC_DS3231 rtc;
/**
 * @brief Initializes the RTC (Real-Time Clock) module.
 * 
 * This function attempts to initialize the RTC module. If the initialization
 * fails, it prints an error message to the serial monitor and halts the 
 * execution indefinitely.
 */
void RTC_init(){
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1); // Stop execution here
    }
}
/**
 * @brief Checks if the RTC (Real-Time Clock) has lost power and adjusts the time if necessary.
 *
 * This function checks if the RTC has lost power using the `lostPower()` method. If the RTC has lost power,
 * it prints a message to the serial monitor (for debugging purposes) and sets the RTC to the compile time
 * using the `adjust()` method with the current date and time.
 */
void RTC_power_loss(){
    if (rtc.lostPower()) {
        Serial.println("RTC lost power, setting the time...");// to be removed for final version
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile time
    }
}

/**
 * @brief Retrieves the current time from the RTC and stores it in the provided buffer.
 * 
 * @param time A character array where the current time will be stored as a string.
 */
void RTC_get_time(char* time){
    DateTime now = rtc.now();
    rtc.now().toString(time);
}
/**
 * @brief Retrieves the current date from the RTC and formats it as a string.
 * 
 * @param date A character array where the formatted date string will be stored.
 *             The date will be formatted as "DD/MM/YYYY".
 */
void RTC_get_date(char* date, size_t size){
    DateTime now = rtc.now();
    snprintf(date,sizeof(size),"%02d/%02d/%04d",now.day(),now.month(),now.year());    
}

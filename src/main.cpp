#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <esp_sleep.h>
#include "FS.h"
#include "SD.h"


// put function declarations here:

RTC_DS3231 rtc;
void logData(const char *filename, const String &data,bool serialout);
const int LED_BUILTIN = 2;
#define WAKEUP_PIN GPIO_NUM_34 
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5
//global variable
int adc_output = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);  // Give time for Serial monitor to connect
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1); // Stop execution here
    }
    
    if (rtc.lostPower()) {
        Serial.println("RTC lost power, setting the time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile time
    }
    
    // Check the wake-up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wakeup reason: ");
    Serial.println(wakeup_reason);
    if (!SD.begin(CS)) {
        Serial.println("SD Card initialization failed!");
        return;
    }
    Serial.println("SD Card initialized successfully!");
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke up from GPIO HIGH!");
        // Perform actions upon wake-up
        DateTime now = rtc.now();
        char date[10] = "hh:mm:ss";
        rtc.now().toString(date);
        
        Serial.print(date);
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
        char result[50];
        snprintf(result, sizeof(result), "0.0409 %s %s %d", buffer, date, 12345);  
        Serial.println(result);
        Serial.println(buffer);
        logData("/rain_data.txt", result, true);
        
    } else {
        Serial.println("Initial boot or not a GPIO wake-up...");
    }

    // Configure the wake-up pin
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 1);  // Wake up when pin is HIGH

    // Prepare to go back to deep sleep
    Serial.println("Entering deep sleep...");
    delay(1000);  // Allow time for Serial logs to complete
    esp_deep_sleep_start();
}

void loop() {
  adc_output = analogRead(34);
  Serial.println(adc_output);
  delay(1000);
  // put your main code here, to run repeatedly:
  /*digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(100);*/                        // wait for a second
}

// put function definitions here:
void logData(const char *filename, const String &data,bool serialout) {
  File file = SD.open(filename, FILE_APPEND);
  if (!file && serialout) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file)
  {
    file.println(data);
    file.close();
  }
  else if (serialout)
  {
    Serial.println("Error opening file for writing");
  }
  if(serialout){
    Serial.println("Data written to SD: " + data);
  }
  
}
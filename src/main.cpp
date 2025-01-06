#include <Arduino.h>

/** 
 * @file main.cpp
 * @brief Main file for the Pluviometro project.
 */

/* Include libraries*/
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <esp_sleep.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <preferences.h>
//#include <DallasTemperature.h>

//#include <OneWire.h>


/* Include the header files*/
#include <lora.h>
#include <main.h>
#include <temp_sens.h>
#include <wifi_ap_serv.h>
#include <SD_adp.h>
#include <air_sens.h>
#include <RTC_DS3231.h>


// Create a SPI object
SPIClass SPI2(HSPI);
//OneWire oneWire(33);

/** 
 * @brief Global variables
 */
int RTC_DATA_ATTR num_id = 0;
int RTC_DATA_ATTR bucket_tips_counter = 0;
int bucket_tips_counter_log=0;
int RTC_DATA_ATTR sec_to_micro = 1000000;
int RTC_DATA_ATTR sleep_interval = 60 ;;
uint32_t RTC_DATA_ATTR rtimer = 0;
int RTC_DATA_ATTR sleep_counter = 0;
int RTC_DATA_ATTR sleep_counter_limit = 10;

float temp102;



/* Preferences object to store data in the ESP32 flash memory*/
Preferences preferences;
/* Create a DallasTemperature object*/
//DallasTemperature ds18b20(&oneWire);

/**
 * @brief Setup function for initializing the ESP32 and peripherals.
 * 
 * This function performs the following tasks:
 * - Initializes the I2C communication on specified pins.
 * - Disables Bluetooth and WiFi to save power.
 * - Initializes the Serial communication for debugging purposes.
 * - Sets the CPU  to 80MHz.
 * - Configures wakeup sources for deep sleep.
 * - Checks the wakeup reason and handles accordingly:
 *   - If woken up by GPIO pin, checks which pin caused the wakeup and handles sensor or WiFi server.
 *   - If woken up by timer, initializes RTC, BME280 sensor, and SD card, then logs data.
 *   - If initial boot or not a GPIO wakeup, scans I2C devices and initializes RTC and CCS811 sensor.
 * - Initializes preferences to store and retrieve data.
 * - Logs wakeup reason and other debug information to Serial (to be removed in final version).
 */
void setup() {
    Wire.begin(21, 22);
    /*disable bluetooth and wifi*/
    btStop();
    esp_wifi_stop();

    

    /*initialize serial for debugging*/// to be removed for final version
    Serial.begin(115200);
    delay(1000);  // Give time for Serial monitor to connect

    /*set esp32 frequency to 80MHz*/
    setCpuFrequencyMhz(80);
    
    /*Spi config*/
    
    SPI2.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    Serial.println("sd");
    SD_init();
    Serial.println("sd_in");
    /*wakeup sources*/ 
    esp_sleep_enable_ext1_wakeup((1ULL<<WAKEUP_PIN)|(1ULL<<WAKEUP_PIN_2_wifi), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_timer_wakeup(sleep_interval * sec_to_micro);


    /*Check the wake-up reason*/ 
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wakeup reason: ");// to be removed for final version
    Serial.println(wakeup_reason);// to be removed for final version

    /*initialize the preferences objects*/
    preferences.begin("serial_num", false);
    preferences.begin("sleep_interval", false); 
    preferences.begin("rtimer", false);

    /*get data from the preferences objects*/
    num_id = preferences.getInt("num_id", 0);
    sleep_interval = preferences.getInt("sleep_interval", 60);

    /*Check if the wake-up was caused by the GPIO pin*/
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
      uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();// Get the wake-up pin
      /*check if wake up pin is the sensor pin*/
      if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN)){
        bucket_tips();
        goToSleep();
      }
      /*check if wake up pin is the wifi button pin*/
      else if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN_2_wifi)){
        //start the wifi server
        handleWiFiServer();
      }
      
    }
    else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER){
      Serial.println("Woke up from timer...");// to be removed for final version
      attachInterrupt(34, ISR, HIGH);
      // check if the RTC is running 
      RTC_init();
      // check if the RTC lost power and if so, set the time
      RTC_power_loss();
      // Initialize the BME280 sensor
      temp_BME280_init();
            
      Serial.println("BME280 sensor initialized successfully!");// to be removed for final version
      // Initialize SD card
      
      SD_init();
      
      //log the sensor data and time on the sd card
      handleDataLogging();
      bucket_tips_counter=bucket_tips_counter_log;
      sleep_counter++;
      if(sleep_counter>=sleep_counter_limit){
        sleep_counter=0;
        loraWAN_config_and_transmition();
      }
      detachInterrupt(34);
      //set the ESP32 to deep sleep
      digitalWrite(SD_CS, HIGH);
      goToSleep();
    
        
      }
     else {
            // This is a fresh boot
        Serial.println("Initial boot or not a GPIO wake-up...");// to be removed for final version
        RTC_init();
        // check if the RTC lost power and if so, set the time
        RTC_power_loss();
        // Initialize rain reset timer
        rtimer = preferences.getUInt("rtimer", 0);
        DateTime now = rtc.now();
        rtimer = now.unixtime();
        preferences.putInt("rtimer", rtimer);

        ccs811_stop();
        
        //sendDataFromFile("/rain_data.txt");
        temp_102_init();
        Serial.println("Initialization complete!");// to be removed for final version
        temp102 = temp_102_read();
        Serial.print("Temperature: ");
        Serial.println(temp102);
        temp_BME280_init();
        Serial.print("Temperature: ");
        Serial.println(BME280_read_temp());
        SD_init();
        loraWAN_config_and_transmition();
         
    }


    
}

/**
 * @brief Main loop function that handles server activity and power management.
 * 
 * This function continuously checks the status of the server and manages power consumption.
 * If the server is active, it checks for a timeout or a wakeup pin press to stop the server and put the device to sleep.
 * If the server is not active, it puts the device to sleep immediately.
 * 
 */
void loop() {
  if (serverActive) {
    // Check for timeout
        if (millis() - lastActivityTime > TIMEOUT_PERIOD) {
            Serial.println("Timeout reached. Stopping server and going to sleep...");
            server.end();
            serverActive = false;
            WiFi.softAPdisconnect(true);
            goToSleep();
        }
    // Check if the wifi wakeup pin is pressed again to stop the server and sleep
    if (digitalRead(WAKEUP_PIN_2_wifi) == HIGH) {
            Serial.println("Stopping server and going to sleep...");// to be removed for final version
            server.end();
            serverActive = false;
            WiFi.softAPdisconnect(true);
            goToSleep();
        }     
  }
  else if(loraWANActive){
    os_runloop_once();
    loraWANActive = false;
  }
  else{
    // If the server is not active, go to sleep
  goToSleep();
  }                    
}


/**
 * @brief Logs data to an SD card file and optionally outputs to the serial monitor.
 * 
 * @param filename The name of the file to write to on the SD card.
 * @param data The data to be written to the file.
 * @param serialout If true, outputs status messages to the serial monitor.
 */
void logData(const char *filename, const String &data,bool serialout) {
  SD_init();
  File file = SD_open(filename, FILE_APPEND);// Open the file in append mode
  if (file)
  {
    file.println(data);
    file.close();
  }
  Serial.println("Data written to SD: " + data);// to be removed for final version
}




/**
 * @brief Handles the data logging process.
 *
 * This function is responsible for logging data when the system wakes up.
 * Retrieves the current date and time from the RTC, formats the date
 * and time, and logs the datam to a specified file.
 *
 * The logged data includes thetemperatur,the amount of rain, the current date, time, and a
 * numerical identifier.
 */
void handleDataLogging() {
    Serial.println("Woke up for data logging...");// to be removed for final version
    char time[10] = "hh:mm:ss";
    RTC_get_time(time);
    char date[12] = "dd/mm/yyyy";
    RTC_get_date(date);

    float rain=0.0;
    rain=bucket_tips_counter*0.0409;
    check_reset_timer();
    
    char result1[50];
    snprintf(result1, sizeof(result1), "%.4f,%s,%s,%d", rain, date, time, num_id);
    char result2[100];
    temp_102_init();
    float temp= temp_102_read();
    snprintf(result2, sizeof(result2), "%.2f,%s", temp, result1);

    //ds18b20.begin();
    //ds18b20.requestTemperatures();
    //float temp = ds18b20.getTempCByIndex(0);
    //Serial.println(temp);
    
    Serial.println(result2);// to be removed for final version
    logData("/rain_data.txt", result2, true);
}

/**
 * @brief Puts the device into deep sleep mode.
 * 
 * This function performs the necessary steps to put the device into deep sleep mode.
 * It turns off the Wi-Fi to save power, stops the Wi-Fi driver, and
 * starts the deep sleep mode.
 */
void goToSleep() {
    Serial.println("Entering deep sleep...");// to be removed for final version
    WiFi.mode(WIFI_OFF); // Ensure Wi-Fi is off
    esp_wifi_stop();
    delay(1000); // to be removed for final version
    esp_deep_sleep_start();
}
/**
 * @brief counts the number of bucket tips of the rain gauge.
 * 
 * this function increments the bucket_tips_counter variable by 1.
 * 
 */
void bucket_tips(){
  bucket_tips_counter++;  
}
/**
 * @brief counts the number of bucket tips of the rain gauge.
 * 
 * this function increments the bucket_tips_counter variable by 1.
 * 
 */
void bucket_tips_log(){
  bucket_tips_counter_log++;  
}
/**
 * @brief Interrupt Service Routine (ISR) for handling bucket tips.
 * 
 * This function is marked with IRAM_ATTR to ensure it is placed in 
 * the IRAM (Instruction RAM) for faster execution. It is triggered 
 * by an interrupt and calls the bucket_tips_log() function to log 
 * the bucket tips.
 */
void IRAM_ATTR ISR() {
    bucket_tips_log();
}
void check_reset_timer(){
  DateTime now = rtc.now();
  uint32_t current_time = now.unixtime();
  if (current_time - rtimer >= 86400){
    rtimer = current_time;
    preferences.putUInt("rtimer", rtimer);
    bucket_tips_counter = 0;

  }
}

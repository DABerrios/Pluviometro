#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <esp_sleep.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <esp_wifi.h>
#include <SparkFunBME280.h>
#include <preferences.h>
#include <SparkFun_ENS160.h>
#include <SparkFunCCS811.h>

/* Pin definitions*/
#define WAKEUP_PIN GPIO_NUM_34
#define WAKEUP_PIN_2_wifi GPIO_NUM_35 
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5
#define CCS811_ADDR 0x5B

const int LED_BUILTIN = 2;

/* Create an RTC object */
RTC_DS3231 rtc;

/* Create a BME280 object */
BME280 bme;
BME280_SensorMeasurements sensor_measurements;
CCS811 ccs811(CCS811_ADDR);
/*function declarations*/
void logData(const char *filename, const String &data,bool serialout);
void handleWiFiServer();
void handleDataLogging();
void goToSleep();
void bucket_tips();
void IRAM_ATTR ISR();


/* Network credentials*/
const char* ssid = "ESP32_AP";
const char* password = "12345678";
/*global variable*/
int RTC_DATA_ATTR num_id = 0;
int RTC_DATA_ATTR bucket_tips_counter = 0;
int bucket_tips_counter_log=0;
int RTC_DATA_ATTR sec_to_micro = 1000000;
int RTC_DATA_ATTR sleep_interval = 60 ;;
unsigned long lastActivityTime = 0; // Timestamp of the last activity
const unsigned long TIMEOUT_PERIOD = 5 * 60 * 1000; // 5 minutes in milliseconds
/* Create an AsyncWebServer object on port 80*/
AsyncWebServer server(80);
/* Variable to store received data*/
String receivedData = "";
/* Flag to keep the ESP32 awake when the server is active*/
bool serverActive = false;
/* Preferences object to store data in the ESP32 flash memory*/
Preferences preferences;
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
    
    /*wakeup sources*/ 
    esp_sleep_enable_ext1_wakeup((1ULL<<WAKEUP_PIN)|(1ULL<<WAKEUP_PIN_2_wifi), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_timer_wakeup(sleep_interval* sec_to_micro);

    /*Check the wake-up reason*/ 
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wakeup reason: ");// to be removed for final version
    Serial.println(wakeup_reason);// to be removed for final version
    /*initialize the preferences objects*/
    preferences.begin("serial_num", false);
    preferences.begin("sleep_interval", false); 
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
      if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1); // Stop execution here
      }
      // check if the RTC lost power and if so, set the time
      if (rtc.lostPower()) {
        Serial.println("RTC lost power, setting the time...");// to be removed for final version
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile time
      }
      // Initialize the BME280 sensor
      if (bme.beginI2C() == false) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
      }
      bme.setTempOverSample(1);
      bme.setHumidityOverSample(1);
      bme.setPressureOverSample(1);
      
      Serial.println("BME280 sensor initialized successfully!");// to be removed for final version
      // Initialize SD card
      if (!SD.begin(CS)) {
        Serial.println("SD Card initialization failed!");// to be removed for final version
        return;
      }
      Serial.println("SD Card initialized successfully!");// to be removed for final version
      //log the sensor data and time on the sd card
      handleDataLogging();
      bucket_tips_counter=bucket_tips_counter_log;
      detachInterrupt(34);
      //set the ESP32 to deep sleep
      goToSleep();
    
        
      }
     else {
      Serial.println("\nI2C Scanner");
      for (byte address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        if (error == 0) {
          Serial.print("I2C device found at address 0x");
          if (address < 16) Serial.print("0");
          Serial.println(address, HEX);
        } else if (error == 4) {
          Serial.print("Unknown error at address 0x");
          if (address < 16) Serial.print("0");
          Serial.println(address, HEX);
        }
      }
      Serial.println("Done");
      // This is a fresh boot
        Serial.println("Initial boot or not a GPIO wake-up...");// to be removed for final version
        if (!rtc.begin()) {
          Serial.println("Couldn't find RTC");
          while (1); // Stop execution here
        }
        // check if the RTC lost power and if so, set the time
        if (rtc.lostPower()) {
          Serial.println("RTC lost power, setting the time...");// to be removed for final version
          rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile time
        }
        if(!ccs811.begin()){
          Serial.println("CCS811 sensor not found. Please check wiring.");
          while(1);
        }
        ccs811.setDriveMode(0);

           
    }


    
}

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
  File file = SD.open(filename, FILE_APPEND);// Open the file in append mode
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

/**
 * @brief Initializes and handles the Wi-Fi server.
 * 
 * This function sets up the ESP32 as a Wi-Fi access point and starts an asynchronous web server.
 * It defines routes for handling HTTP GET requests to serve an HTML form, receive data, and send a file from the SD card.
 * 
 * Routes:
 * - "/" : Serves an HTML form for rain gauge configuration.
 * - "/submit" : Receives data from the HTML form and prints it to the Serial monitor.
 * - "/request_file" : Sends the content of a specified file from the SD card to the client.
 * - "/set_sleep_interval" : Sets the sleep interval for the ESP32.
 * 
 * The function also enables Wi-Fi low-power mode and prints the access point IP address to the Serial monitor.
 */
void handleWiFiServer() {
    Serial.println("Woke up to start Wi-Fi server...");// to be removed for final version
    WiFi.softAP(ssid, password);
    WiFi.setSleep(true); // Enable Wi-Fi low-power mode
    Serial.print("Access Point IP: ");// to be removed for final version
    Serial.println(WiFi.softAPIP());// to be removed for final version
    lastActivityTime = millis();// Update the last activity time
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = "<!DOCTYPE html><html><body>";
        html += "<h1>Rain Gauge Configuration</h1>";

        if (num_id == 0) {
            // Show the serial input form if num_id is not set
            html += "<form action='/submit' method='GET'>";
            html += "Enter serial: <input type='text' name='data'>";
            html += "<input type='submit' value='Send'>";
            html += "</form>";
        } else {
            // Display num_id if it's already set
            html += "<p>Serial ID is already set: " + String(num_id) + "</p>";
        }

        html += "<h2>Set Sleep Interval (seconds)</h2>";
        html += "<form action='/set_sleep_interval' method='GET'>";
        html += "Sleep Interval: <input type='text' name='interval' value='" + String(sleep_interval) + "'>";
        html += "<input type='submit' value='Set'>";
        html += "</form>";

        html += "<br><form action='/request_file' method='GET'>";
        html += "<button type='submit'>Request File</button>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html);
    });

    server.on("/submit", HTTP_GET, [](AsyncWebServerRequest* request) {
        lastActivityTime = millis(); // Reset timeout on activity
        if (request->hasParam("data")) {
            receivedData = request->getParam("data")->value();
            Serial.println("Received Data: " + receivedData);
            if((receivedData.toInt())==0){
              Serial.println("Invalid serial number");
              request->send(200, "text/plain", "Invalid serial number");
            }
            else{
              num_id = receivedData.toInt();
            }
            preferences.putInt("num_id", num_id);
        }
        request->send(200, "text/plain", "Data received: " + receivedData);
    });

    server.on("/set_sleep_interval", HTTP_GET, [](AsyncWebServerRequest* request) {
        lastActivityTime = millis(); // Reset timeout on activity
        if (request->hasParam("interval")) {
            String intervalStr = request->getParam("interval")->value();
            Serial.println("Received Sleep Interval: " + intervalStr);
            if((intervalStr.toInt())==0){
              Serial.println("Invalid sleep interval");
              request->send(200, "text/plain", "Invalid sleep interval");
            }
            else{
            sleep_interval = intervalStr.toInt();
            }
            Serial.println("Sleep Interval set to: " + String(sleep_interval) + " seconds");
            preferences.putInt("sleep_interval", sleep_interval); // Save to flash
        }
        request->send(200, "text/plain", "Sleep interval set to: " + String(sleep_interval) + " seconds");
    });

    server.on("/request_file", HTTP_GET, [](AsyncWebServerRequest* request) {
        lastActivityTime = millis(); // Reset timeout on activity
        if (!SD.begin(CS)) {
            Serial.println("SD Card initialization failed!");
            return;
        }
        const char* fileName = "/rain_data.txt";
        File file = SD.open(fileName);
        if (!file) {
            request->send(500, "text/plain", "Failed to open file");
            return;
        }

        String fileContent;
        while (file.available()) {
            fileContent += (char)file.read();
        }
        file.close();

        request->send(200, "text/plain", fileContent);
    });

    server.begin();
    serverActive = true;
    lastActivityTime = millis(); // Reset activity timestamp
}
/**
 * @brief Handles the data logging process.
 *
 * This function is responsible for logging data when the system wakes up.
 * Retrieves the current date and time from the RTC, formats the date
 * and time, and logs the datamto a specified file.
 *
 * The logged data includes a fixed value, the current date, time, and a
 * numerical identifier.
 */
void handleDataLogging() {
    Serial.println("Woke up for data logging...");// to be removed for final version
    DateTime now = rtc.now();
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    float rain=0.0;
    rain=bucket_tips_counter*0.0409;
    bucket_tips_counter=0;
    char result1[50];
    snprintf(result1, sizeof(result1), "%.4f,%s,%s,%d", rain, buffer, date, num_id);

    while (bme.isMeasuring())
    {
    }
    char result2[100];
    bme.readAllMeasurements(&sensor_measurements);
    snprintf(result2, sizeof(result2), "%.2f,%s", sensor_measurements.temperature, result1);

    
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
void bucket_tips_log(){
  bucket_tips_counter_log++;  
}
void IRAM_ATTR ISR() {
    bucket_tips_log();
}
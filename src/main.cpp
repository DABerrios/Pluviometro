#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <esp_sleep.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include <esp_wifi.h>

/* Pin definitions*/
#define WAKEUP_PIN GPIO_NUM_34
#define WAKEUP_PIN_2_wifi GPIO_NUM_35 
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5
const int LED_BUILTIN = 2;

/* Create an RTC object */
RTC_DS3231 rtc;
/*function declarations*/
void logData(const char *filename, const String &data,bool serialout);
void handleWiFiServer();
void handleDataLogging();
void goToSleep();


/* Network credentials*/
const char* ssid = "ESP32_AP";
const char* password = "12345678";
/*global variable*/
int RTC_DATA_ATTR num_id = 0;
/* Create an AsyncWebServer object on port 80*/
AsyncWebServer server(80);
/* Variable to store received data*/
String receivedData = "";
/* Flag to keep the ESP32 awake when the server is active*/
bool serverActive = false;
void setup() {
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

    /*Check the wake-up reason*/ 
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wakeup reason: ");// to be removed for final version
    Serial.println(wakeup_reason);// to be removed for final version
    
    /*Check if the wake-up was caused by the GPIO pin*/
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
      uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();// Get the wake-up pin
      /*check if wake up pin is the sensor pin*/
      if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN)){
        
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
        // Initialize SD card
        if (!SD.begin(CS)) {
          Serial.println("SD Card initialization failed!");// to be removed for final version
          return;
        }
        Serial.println("SD Card initialized successfully!");// to be removed for final version
        //log the sensor data and time on the sd card
        handleDataLogging();
        //set the ESP32 to deep sleep
        goToSleep();
      }
      /*check if wake up pin is the wifi button pin*/
      else if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN_2_wifi)){
        //start the wifi server
        handleWiFiServer();
      }
    } else {
      // This is a fresh boot
        Serial.println("Initial boot or not a GPIO wake-up...");// to be removed for final version
    }


    
}

void loop() {
  if (serverActive) {
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
 * 
 * The function also enables Wi-Fi low-power mode and prints the access point IP address to the Serial monitor.
 */
void handleWiFiServer() {
    Serial.println("Woke up to start Wi-Fi server...");// to be removed for final version
    WiFi.softAP(ssid, password);
    WiFi.setSleep(true); // Enable Wi-Fi low-power mode
    Serial.print("Access Point IP: ");// to be removed for final version
    Serial.println(WiFi.softAPIP());// to be removed for final version

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = "<!DOCTYPE html><html><body>";
        html += "<h1>Rain gauge config</h1>";
        html += "<form action='/submit' method='GET'>";
        html += "Enter serial: <input type='text' name='data'>";
        html += "<input type='submit' value='Send'>";
        html += "</form>";
        html += "<br>";
        html += "<form action='/request_file' method='GET'>";
        html += "<button type='submit'>Request File</button>";
        html += "</form>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    server.on("/submit", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("data")) {
            receivedData = request->getParam("data")->value();
            Serial.println("Received Data: " + receivedData);// to be removed for final version
            num_id=receivedData.toInt();
            Serial.println(num_id);// to be removed for final version
        }
        request->send(200, "text/plain", "Data received: " + receivedData);
    });

  server.on("/request_file", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!SD.begin(CS)) {
          Serial.println("SD Card initialization failed!");
          return;
        }
        const char* fileName = "/rain_data.txt"; // File to send
        File file = SD.open(fileName);
        if (!file) {
            Serial.println("Failed to open file for sending"); // to be removed for final version
            request->send(500, "text/plain", "Failed to open file");
            return;
        }

        String fileContent;
        while (file.available()) {
            fileContent += (char)file.read();
        }
        file.close();

        Serial.println("File sent to client:");// to be removed for final version
        Serial.println(fileContent);// to be removed for final version
        request->send(200, "text/plain", fileContent);
    });

    server.begin();
    serverActive = true; // Set flag to keep the ESP32 awake
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
    Serial.println("Current num_id: " + String(num_id)); // to be removed for final version
    DateTime now = rtc.now();
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    char result[50];
    snprintf(result, sizeof(result), "0.0409 %s %s %d", buffer, date, num_id);

    Serial.println(result);// to be removed for final version
    logData("/rain_data.txt", result, true);
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
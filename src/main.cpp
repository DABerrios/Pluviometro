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

#define WAKEUP_PIN GPIO_NUM_34
#define WAKEUP_PIN_2_wifi GPIO_NUM_35 
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5

// put function declarations here:

RTC_DS3231 rtc;
void logData(const char *filename, const String &data,bool serialout);
void handleWiFiServer();
void handleDataLogging();
void goToSleep();

const int LED_BUILTIN = 2;
// Network credentials
const char* ssid = "ESP32_AP";
const char* password = "12345678";
//global variable
int adc_output = 0;
int RTC_DATA_ATTR num_id = 0;
// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);
// Variable to store received data
String receivedData = "";
// Flag to keep the ESP32 awake when the server is active
bool serverActive = false;
void setup() {
    btStop();
    esp_wifi_stop();
    Serial.begin(115200);
    delay(1000);  // Give time for Serial monitor to connect
    setCpuFrequencyMhz(80);
    
    //wakeup sources 
    esp_sleep_enable_ext1_wakeup((1ULL<<WAKEUP_PIN)|(1ULL<<WAKEUP_PIN_2_wifi), ESP_EXT1_WAKEUP_ANY_HIGH);

    // Check the wake-up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wakeup reason: ");
    Serial.println(wakeup_reason);
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
      uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
      if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN)){
        
        if (!rtc.begin()) {
          Serial.println("Couldn't find RTC");
          while (1); // Stop execution here
        }
    
        if (rtc.lostPower()) {
          Serial.println("RTC lost power, setting the time...");
          rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile time
        }
        if (!SD.begin(CS)) {
          Serial.println("SD Card initialization failed!");
          return;
        }
        Serial.println("SD Card initialized successfully!");
        handleDataLogging();
        goToSleep();
      }
      else if(wakeup_pin_mask & (1ULL<<WAKEUP_PIN_2_wifi)){
        handleWiFiServer();
      }
    } else {
        Serial.println("Initial boot or not a GPIO wake-up...");
    }


    
}

void loop() {
  if (serverActive) {
    // Check if the wakeup pin is pressed again to stop the server and sleep
    if (digitalRead(WAKEUP_PIN_2_wifi) == HIGH) {
            Serial.println("Stopping server and going to sleep...");
            server.end();
            serverActive = false;
            WiFi.softAPdisconnect(true);
            goToSleep();
        }     
  }
  else{   
  goToSleep();
  }                    
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

void handleWiFiServer() {
    Serial.println("Woke up to start Wi-Fi server...");
    WiFi.softAP(ssid, password);
    WiFi.setSleep(true); // Enable Wi-Fi low-power mode
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());

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
            Serial.println("Received Data: " + receivedData);
            num_id=receivedData.toInt();
            Serial.println(num_id);
        }
        request->send(200, "text/plain", "Data received: " + receivedData);
    });

  server.on("/request_file", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!SD.begin(CS)) {
          Serial.println("SD Card initialization failed!");
          return;
        }
        const char* fileName = "/rain_data.txt"; // Replace with your desired file
        File file = SD.open(fileName);
        if (!file) {
            Serial.println("Failed to open file for sending");
            request->send(500, "text/plain", "Failed to open file");
            return;
        }

        String fileContent;
        while (file.available()) {
            fileContent += (char)file.read();
        }
        file.close();

        Serial.println("File sent to client:");
        Serial.println(fileContent);
        request->send(200, "text/plain", fileContent);
    });

    server.begin();
    serverActive = true; // Set flag to keep the ESP32 awake
}
void handleDataLogging() {
    Serial.println("Woke up for data logging...");
    Serial.println("Current num_id: " + String(num_id)); // Debug print
    DateTime now = rtc.now();
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    char result[50];
    snprintf(result, sizeof(result), "0.0409 %s %s %d", buffer, date, num_id);

    Serial.println(result);
    logData("/rain_data.txt", result, true);
}

void goToSleep() {
    Serial.println("Entering deep sleep...");
    WiFi.mode(WIFI_OFF); // Ensure Wi-Fi is off
    esp_wifi_stop();
    delay(1000); // Allow time for Serial logs to complete
    esp_deep_sleep_start();
}
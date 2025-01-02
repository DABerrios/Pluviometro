#include <wifi_ap_serv.h>

#include <main.h>
#include <SD_adp.h>
#include <wifi_ap_serv.h>
/* Create an AsyncWebServer object on port 80*/
AsyncWebServer server(80);

String receivedData = "";
bool serverActive = false;

/* Network credentials*/
const char* ssid = "ESP32_AP";
const char* password = "12345678";

unsigned long lastActivityTime = 0; // Timestamp of the last activity
const unsigned long TIMEOUT_PERIOD = 5 * 60 * 1000; // 5 minutes in milliseconds

/**
 * @brief Initializes and handles the Wi-Fi server.
 * 
 * This function sets up the Wi-Fi access point, configures the server routes, and handles incoming HTTP requests.
 * It also enables Wi-Fi low-power mode and updates the last activity time.
 * 
 * Routes:
 * - "/" (GET): Displays the main configuration page with options to set the serial ID and sleep interval.
 * - "/submit" (GET): Receives and processes the serial ID input from the user.
 * - "/set_sleep_interval" (GET): Receives and sets the sleep interval input from the user.
 * - "/request_file" (GET): Reads and sends the content of the "rain_data.txt" file from the SD card.
 * 
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
        SD_init();
        // Open file
        const char* fileName = "/rain_data.txt";
        File file = SD_open(fileName, FILE_READ);
        
        if (!file) {
            request->send(500, "text/plain", "Failed to open file");
            return;
        }
        Serial.println("Reading file: " + String(fileName));// to be removed for final version

        // Stream file to response to prevent watchdog from triggering
        AsyncWebServerResponse *response = request->beginResponse(SD, fileName, "text/plain");
        request->send(response);

        file.close();
        Serial.println("File sent");
    });

    server.begin();
    serverActive = true;
    lastActivityTime = millis(); // Reset activity timestamp
}
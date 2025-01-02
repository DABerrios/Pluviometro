#ifndef WIFI_AP_SERV_H
#define WIFI_AP_SERV_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

/* Create an AsyncWebServer object on port 80*/
/**
 * @var server
 * @brief AsyncWebServer object instantiated on port 80.
 */
extern AsyncWebServer server;

/* Variable to store received data*/
extern String receivedData;

/* Flag to keep the ESP32 awake when the server is active*/
extern bool serverActive;

/* Network credentials*/
extern const char* ssid;
extern const char* password;

/* Timestamp of the last activity*/
extern unsigned long lastActivityTime;
extern const unsigned long TIMEOUT_PERIOD;

void handleWiFiServer();

#endif /*WIFI_AP_SERV_H */
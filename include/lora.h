#ifndef LORA_H
#define LORA_H
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <SD.h>
#include <preferences.h>

#define SX1276_SCK 18
#define SX1276_MISO 19
#define SX1276_MOSI 23
#define SX1276_NSS 5
#define SX1276_RST 12
#define SX1276_DIO0 32
#define SX1276_DIO1 13

extern bool loraWANActive;
extern Preferences preferences;
void loraWAN_test();
void do_send(osjob_t* j);
void onEvent(ev_t ev);
size_t Lineprocessing(const String &line,uint8_t* data);
void sendDataFromFile(const char* filename);
void saveLastPosition(unsigned long position);
void readLastPosition();
void loraWAN_config_and_transmition();
size_t data_processing(const char* filename, uint8_t* data);

#endif /*LORA_H */
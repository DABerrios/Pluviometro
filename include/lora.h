#ifndef LORA_H
#define LORA_H
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <SD.h>
#include <preferences.h>


extern bool loraWANActive;
extern Preferences preferences;
void loraWAN_test();

void do_send(osjob_t* j);

void onEvent(ev_t ev);

size_t Lineprocessing(const String &line,uint8_t* data);

#endif /*LORA_H */
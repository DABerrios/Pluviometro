#ifndef LORA_OTAA_H
#define LORA_OTAA_H
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <SD.h>
#include <preferences.h>

#define SX1276_SCK 18
#define SX1276_MISO 19
#define SX1276_MOSI 23
#define SX1276_NSS 2
#define SX1276_RST 25
#define SX1276_DIO0 32
#define SX1276_DIO1 26

extern bool loraWANActive;
extern uint8_t mydata[];
extern osjob_t sendjob;
void do_send(osjob_t* j);
void onEvent (ev_t ev);
#endif /*LORA_OTAA_H */
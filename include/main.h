#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>

/* Pin definitions*/
#define WAKEUP_PIN GPIO_NUM_34
#define WAKEUP_PIN_2_wifi GPIO_NUM_35 
#define SD_SCK 25
#define SD_MISO 26
#define SD_MOSI 27
#define SD_CS 14
#define CCS811_ADDR 0x5B



/*function declarations*/
void logData(const char *filename, const String &data,bool serialout);
void handleDataLogging();
void goToSleep();
void bucket_tips();
void IRAM_ATTR ISR();
void check_reset_timer();

extern SPIClass vspi;
extern Preferences preferences;
extern int RTC_DATA_ATTR num_id;
extern int RTC_DATA_ATTR sleep_interval;

#endif /*MAIN_H */
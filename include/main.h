#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>

/*function declarations*/
void logData(const char *filename, const String &data,bool serialout);
void handleWiFiServer();
void handleDataLogging();
void goToSleep();
void bucket_tips();
void IRAM_ATTR ISR();


#endif /*MAIN_H */
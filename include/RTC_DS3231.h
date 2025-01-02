#ifndef RTC_DS3231_H
#define RTC_DS3231_H
#include <Arduino.h>
#include <RTClib.h>

extern RTC_DS3231 rtc;

void RTC_init();
void RTC_power_loss();
void RTC_get_time(char* time);
void RTC_get_date(char* date);
#endif
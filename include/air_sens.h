#ifndef AIR_SENS_H
#define AIR_SENS_H
#include <Arduino.h>
#include <SparkFunCCS811.h>

extern CCS811 ccs811;
void ccs811_stop();
void ccs811_init();
#endif /*AIR_SENS_H */
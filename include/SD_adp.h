#ifndef SD_ADP_H
#define SD_ADP_H
#include <SD.h>
#include <SPI.h>
#include <FS.h>

void SD_init();
File SD_open(const char* fileName, const char *mode);

#endif /*SD_ADP_H */
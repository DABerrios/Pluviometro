#ifndef SD_ADP_H
#define SD_ADP_H
#include <SD.h>
#include <SPI.h>
#include <FS.h>

#define SD_SCK 14
#define SD_MISO 27
#define SD_MOSI 13
#define SD_CS 15


void SD_init();
File SD_open(const char* fileName, const char *mode);

#endif /*SD_ADP_H */
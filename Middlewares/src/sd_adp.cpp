
#include "SD.h"
#include "SPI.h"
#include "sd_adp.h"

void logData(const char *filename, const String &data,bool serialout) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file && serialout) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file)
  {
    file.println(data);
    file.close();
  }
  else if (serialout)
  {
    Serial.println("Error opening file for writing");
  }
  if(serialout){
    Serial.println("Data written to SD: " + data);
  }
  
}
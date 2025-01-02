#include <SD_adp.h>
#include <main.h>

/**
 * @brief Initializes the SD card and mounts it.
 * 
 * This function sets up the SPI communication for the SD card, configures the 
 * chip select (CS) pin, and attempts to mount the SD card. If the SD card 
 * fails to mount, an error message is printed to the Serial monitor.
 * 
 * @note Ensure that the SD card is properly connected before calling this function.
 * 
 * @return void
 */
void SD_init(){
    vspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    if (!SD.begin(SD_CS, vspi))
    {
        Serial.println("SD Card Mount Failed");
        return;
    }
    Serial.println("SD Card Mounted");// to be removed for final version
}
/**
 * @brief Opens a file on the SD card with the specified mode.
 * 
 * @param fileName The name of the file to open.
 * @param mode The mode in which to open the file (e.g., "r" for read, "w" for write).
 * @return File The file object representing the opened file. If the file could not be opened, an invalid file object is returned.
 * 
 * @note If the file cannot be opened, a message "Failed to open file for writing" is printed to the Serial monitor.
 */
File SD_open(const char* fileName, const char *mode){
    File file = SD.open(fileName,mode);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
    }
    return file;
    
    
}
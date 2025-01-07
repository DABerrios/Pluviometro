#include <lora.h>
#include <main.h>
#include <SD_adp.h>



/*LoraWAN*/
/*Dev address, NwkSKey, AppSKey*/
static const PROGMEM u1_t NWKSKEY[16] = {0x73, 0x19, 0xD0, 0xCE, 0x95, 0xBD, 0x84, 0xA8, 0xCD, 0x17, 0xBB, 0xEA, 0x43, 0xA8, 0xD6, 0x9F};
static const PROGMEM u1_t APPSKEY[16] = {0x9F, 0x4C, 0x2C, 0xAD, 0x66, 0x28, 0x44, 0xA6, 0x0A, 0x32, 0x96, 0x7A, 0xCA, 0x8A, 0xC2, 0x2C};
static const u4_t DEVADDR = 0x260C0482;

/*
static const PROGMEM u1_t NWKSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const PROGMEM u1_t APPSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u4_t DEVADDR = 0x00000000;
*/

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
static uint8_t mydata[] = "Hello, test";
static osjob_t sendjob;
const unsigned TX_INTERVAL = 30;

const lmic_pinmap lmic_pins = {
    .nss = SX1276_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = SX1276_RST,
    .dio = {SX1276_DIO0, SX1276_DIO1, LMIC_UNUSED_PIN},
};

bool loraWANActive = false;

int RTC_DATA_ATTR last_line = 0;


/**
 * @brief Configures and initiates the LoRaWAN transmission.
 *
 * This function initializes the LoRaWAN stack, sets up the session keys,
 * configures the channels, and sends data from a specified file.
 *
 * @note The debug print statements are included for development purposes
 *       and should be removed in the final version.
 *
 * The function performs the following steps:
 * - Initializes the LoRaWAN stack.
 * - Resets the LMIC (LoRaMAC-in-C) library.
 * - Sets up the session keys for communication.
 * - Configures the channels based on the region (e.g., CFG_au915).
 * - Disables link check mode.
 * - Sets the data rate and transmission power.
 * - Sends data from the specified file.
 *
 * @param None
 * @return None
 */
void loraWAN_config_and_transmition() {
    Serial.println(F("Starting LoRaWAN..."));// to be removed for final version
    loraWANActive = true;
    os_init();
    Serial.println(F("LMIC init done!"));// to be removed for final version
    LMIC_reset();
    Serial.println(F("LMIC reset done!"));// to be removed for final version
    #ifdef PROGMEM
    Serial.println(F("Using PROGMEM"));// to be removed for final version
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
    Serial.println(F("LMIC set session done!"));// to be removed for final version
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_au915)
    LMIC_setupChannel(0, 916800000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(1, 917000000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(2, 917200000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(3, 917400000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(4, 917600000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(5, 917800000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(6, 918000000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(7, 918200000, DR_RANGE_MAP(DR_SF12, DR_SF7), 1);
    LMIC_setupChannel(8, 917500000, DR_RANGE_MAP(DR_SF8, DR_SF8), 1);
    LMIC_selectSubBand(1);
    #endif

    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7, 14);
    sendDataFromFile("/rain_data.txt");
    //do_send(&sendjob);

    Serial.println(F("LoRaWAN ended!"));
    
}
/**
 * @brief Sends data using the LoRaWAN protocol.
 *
 * This function checks if there is a current TX/RX job running. If there is no
 * ongoing transmission or reception, it prepares an upstream data transmission
 * at the next possible time.
 *
 * @param j Pointer to the osjob_t structure representing the job to be executed.
 *
 * The function performs the following actions:
 * - If a TX/RX job is currently running, it logs a message indicating that the
 *   transmission is pending and does not send data.
 * - If no TX/RX job is running, it queues a packet for transmission and logs
 *   a message indicating that the packet has been queued.
 *
 * The next transmission is scheduled after the TX_COMPLETE event.
 */
void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

/**
 * @brief Event handler for LoRa events.
 * 
 * This function is called whenever a LoRa event occurs. It prints the event
 * time and type to the serial monitor and handles specific events accordingly.
 * 
 * @param ev The event type (ev_t) that occurred.
 * 
 * Event types handled:
 * - EV_TXSTART: Transmission started.
 * - EV_SCAN_TIMEOUT: Scan timed out.
 * - EV_BEACON_FOUND: Beacon found.
 * - EV_BEACON_MISSED: Beacon missed.
 * - EV_BEACON_TRACKED: Beacon tracked.
 * - EV_JOINING: Joining network.
 * - EV_JOINED: Joined network.
 * - EV_RFU1: Reserved for future use.
 * - EV_JOIN_FAILED: Joining network failed.
 * - EV_REJOIN_FAILED: Rejoining network failed.
 * - EV_TXCOMPLETE: Transmission complete (includes waiting for RX windows).
 * - EV_LOST_TSYNC: Lost time synchronization.
 * - EV_RESET: Reset event.
 * - EV_RXCOMPLETE: Data received in ping slot.
 * - EV_LINK_DEAD: Link dead.
 * - EV_LINK_ALIVE: Link alive.
 * - Default: Unknown event.
 */
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.println(ev);
    switch(ev) {
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}
/**
 * @brief Sends data from a specified file over LoRa.
 *
 * This function initializes the SD card, opens the specified file, and reads data from it.
 * If the file has been read before, it seeks to the last read position. If the next character
 * in the file is a newline, the function puts the device to sleep. Otherwise, it processes the
 * data and sends it using LoRa. The function waits until the transmission is complete before returning.
 *
 * @param filename The name of the file to read data from.
 */
void sendDataFromFile(const char* filename) {
    /*
    SD_init();
    
    File file = SD_open(filename, FILE_READ);
    unsigned long position = last_line;
    if (position > 0) {
        file.seek(position);
            }
    if (file.peek() == '\n') {
        goToSleep();
    }*/
    uint8_t transmit_data[9];
    size_t linedata = data_processing(filename, transmit_data);
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
    LMIC_setTxData2(1, transmit_data, linedata, 0);
    }
    while (LMIC.opmode & OP_TXRXPEND) {
        os_runloop_once();
    }
    loraWANActive = false;
    /*
    while (file.available()) {
        uint8_t data[51]={0};
        size_t dataSize =0;
        while (file.available() && dataSize < 51)
        {               
            String line =file.readStringUntil('\n');
            uint8_t tempData[10];
            size_t lineSize = Lineprocessing(line, tempData);
            
            // Send data if it fits within 51 bytes
            if (dataSize + lineSize > 51) {
                file.seek(file.position() - line.length() - 1); // Roll back to last unsent line
                break;
            }

            memcpy(&data[dataSize], tempData, lineSize);
            dataSize += lineSize;
        }
        
    
        if (dataSize > 0) {
            LMIC_setTxData2(1, data, dataSize, 0);
            Serial.println(F("Packet queued"));// to be removed for final version
        }
        // Wait for TX_COMPLETE event before sending next packet
        while (LMIC.opmode & OP_TXRXPEND) {
            os_runloop_once();
        }
        saveLastPosition(file.position());
    }
    file.close();
    */
}

/**
 * @brief Processes a line of data and converts it into a binary format.
 * 
 * This function takes a line of data in the form of a string, parses it to extract
 * temperature, rainfall, date, time, and ID information, and then converts these
 * values into a binary format stored in the provided data array.
 * 
 * @param line The input string containing the data to be processed.
 * @param data A pointer to the array where the binary data will be stored.
 * @param avrg_temp The average temperature to be used in the processing.
 * @return The number of bytes written to the data array.
 */
size_t Lineprocessing(const String &line, uint8_t* data, float avrg_temp) {
    float temp;
    float rain;
    int day, month, year,hour, minute,second;
    int id;
    sscanf(line.c_str(),"%f,%f,%d/%d/%d,%d:%d,:%d,%d",&temp,&rain,&day,&month,&year,&hour,&minute,&second,&id);   
    temp=avrg_temp;
    uint16_t tempInt = (uint16_t)(temp * 100);   
    uint16_t rainInt = (uint16_t)(rain * 1000);  
    uint16_t dateInt = (year - 2000) * 365 + (month - 1) * 30 + day; 
    uint16_t timeInt = hour * 3600 + minute * 60 + second;           //
    uint16_t idInt = (uint16_t)id; 

    /*Serial.print("Temp: ");Serial.println(tempInt);// to be removed for final version
    Serial.print("Rain: ");Serial.println(rainInt);// to be removed for final version
    Serial.print("Date: ");Serial.println(dateInt);// to be removed for final version
    Serial.print("Time: ");Serial.println(timeInt);// to be removed for final version
    Serial.print("ID: ");Serial.println(idInt);// to be removed for final version
    */

    size_t index = 0;
    data[index++] = tempInt >> 8;     
    data[index++] = tempInt & 0xFF;  
    data[index++] = rainInt >> 8;
    data[index++] = rainInt & 0xFF;         
    data[index++] = dateInt >> 8;    
    data[index++] = dateInt & 0xFF;  
    data[index++] = timeInt >> 8;    
    data[index++] = timeInt & 0xFF;  

    Serial.print("Index: ");Serial.println(index);// to be removed for final version
    for (size_t i = 0; i < index; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    return index;

}
/**
 * @brief Saves the last position to persistent storage.
 *
 * This function stores the given position value in a persistent storage
 * using the preferences API. The position is saved under the key "last_line".
 *
 * @param position The position value to be saved.
 */
void saveLastPosition(unsigned long position) {
    last_line = position;
    preferences.putInt("last_line", last_line);
}
/**
 * @brief Reads the last saved position from preferences.
 *
 * This function initializes the preferences with the namespace "last_line"
 * and retrieves the integer value associated with the key "last_line".
 * If the key does not exist, it defaults to 0.
 */
void readLastPosition() {
    preferences.begin("last_line", false);
    //last_line = preferences.getInt("last_line", 0);
}
/**
 * @brief Processes data from a file and calculates the average temperature.
 *
 * This function reads data from the specified file starting from the last read position,
 * calculates the average temperature from the data, and processes the last line of data.
 *
 * @param filename The name of the file to read data from.
 * @param data A pointer to a buffer where processed data will be stored.
 * @return The size of the processed data.
 */
size_t data_processing(const char* filename, uint8_t* data){
    //readLastPosition();
    SD_init();
    File file = SD_open(filename, FILE_READ);
    unsigned long position = last_line;
    float  avrg_temp = 0;
    int lines_read = 0;
    String line_f;
    //if (position > 0) {
     //   file.seek(position);
    //}
    while (file.available()) {
        String line = file.readStringUntil('\n');
        //float temp;
        //sscanf(line.c_str(), "%f", &temp);
        //avrg_temp += temp;
        //lines_read++;
        line_f = line;
    }
    Serial.println(line_f);
    //avrg_temp /= lines_read;
    avrg_temp=20.04;
    Serial.print("Average temperature: ");Serial.println(avrg_temp);
    size_t linedata=Lineprocessing(line_f, data, avrg_temp);
    //saveLastPosition(file.position());
    file.close();
    return linedata;
}
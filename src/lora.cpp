


#include <lora.h>
#include <main.h>

#define SX1276_SCK 25
#define SX1276_MISO 26
#define SX1276_MOSI 27
#define SX1276_NSS 14
#define SX1276_RST 12
#define SX1276_DIO0 32
#define SX1276_DIO1 13

/*LoraWAN*/

static const PROGMEM u1_t NWKSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const PROGMEM u1_t APPSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u4_t DEVADDR = 0x00000000;
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


void loraWAN_test() {
    Serial.println(F("Starting LoRaWAN..."));
    loraWANActive = true;
    SPI.begin(SX1276_SCK,SX1276_MISO ,SX1276_MOSI );
    os_init();
    Serial.println(F("LMIC init done!"));
    LMIC_reset();
    Serial.println(F("LMIC reset done!"));
    #ifdef PROGMEM
    Serial.println(F("Using PROGMEM"));
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
    Serial.println(F("LMIC set session done!"));
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
    do_send(&sendjob);
    Serial.println(F("LoRaWAN ended!"));
    
}
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
void sendDataFromFile(const char* filename) {
    File file = SD.open(filename);
    if (!file) {
        Serial.println(F("Failed to open file"));
        return;
    }
    unsigned long position = last_line;
    if (position > 0) {
        file.seek(position);
            }
    if (file.peek() == '\n') {
    goToSleep();
    }
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
    }
    file.close();
}

size_t Lineprocessing(const String &line, uint8_t* data){
    float temp;
    float rain;
    int day, month, year,hour, minute,second;
    int id;
    sscanf(line.c_str(),"%f,%f,%d/%d/%d,%d:%d,:%d,%d",&temp,&rain,&day,&month,&year,&hour,&minute,&second,&id);      
    
    uint16_t tempInt = (uint16_t)(temp * 100);   
    uint16_t rainInt = (uint16_t)(rain * 1000);  
    uint16_t dateInt = (year - 2000) * 365 + (month - 1) * 30 + day; 
    uint16_t timeInt = hour * 3600 + minute * 60 + second;           //
    uint16_t idInt = (uint16_t)id; 

    Serial.print("Temp: ");Serial.println(tempInt);// to be removed for final version
    Serial.print("Rain: ");Serial.println(rainInt);// to be removed for final version
    Serial.print("Date: ");Serial.println(dateInt);// to be removed for final version
    Serial.print("Time: ");Serial.println(timeInt);// to be removed for final version
    Serial.print("ID: ");Serial.println(idInt);// to be removed for final version

    size_t index = 0;
    data[index++] = tempInt >> 8;     
    data[index++] = tempInt & 0xFF;  
    data[index++] = rainInt;         
    data[index++] = dateInt >> 8;    
    data[index++] = dateInt & 0xFF;  
    data[index++] = timeInt >> 8;    
    data[index++] = timeInt & 0xFF;  
    data[index++] = idInt >> 8;      
    data[index++] = idInt & 0xFF;

    return index;

}
void saveLastPosition(unsigned long position) {
    last_line = position;
    preferences.putInt("last_line", last_line);
}
void readLastPosition() {
    preferences.begin("last_line", false);
    last_line = preferences.getInt("last_line", 0);
}
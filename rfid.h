#ifndef RFID_H
#define RFID_H

#include <SPI.h>
#include <MFRC522.h>

// Define pins for the MFRC522
#define RST_PIN 22
#define SS_PIN  21

class RFID {
public:
    RFID();
    void initialize();
    bool isCardPresent();
    String readCardUID();         // Return formatted UID string
    byte* getUidPointer();        // Raw UID pointer
    byte getUidLength();          // Length of UID in bytes

private:
    MFRC522 rfid;
};

#endif // RFID_H

#include "rfid.h"
#include "UidFormatter.h"

RFID::RFID() : rfid(SS_PIN, RST_PIN) {}

void RFID::initialize() {
    SPI.begin();
    rfid.PCD_Init();
    rfid.PCD_SetRegisterBitMask(rfid.TxControlReg, 0x03);
    rfid.PCD_SetRegisterBitMask(rfid.RFCfgReg, 0x70);
    rfid.PCD_WriteRegister(rfid.TxASKReg, 0x40);
    Serial.println("RFID reader initialized with max gain.");
}

bool RFID::isCardPresent() {
    return rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
}

String RFID::readCardUID() {
    String uidString = formatUid(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return uidString;
}

byte* RFID::getUidPointer() {
    return rfid.uid.uidByte;
}

byte RFID::getUidLength() {
    return rfid.uid.size;
}

/*
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <SPI.h>
#include <MFRC522.h>

const byte bonUID[4] = {245,100,55,70};
byte buzz = 8;
const int pinLEDVerte = 12; // LED verte
const int pinLEDRouge = 11; // LED rouge

// setup des pin.s de la carte pour lea connexion avec le module RFID
const int pinRST = 5;  // pin RST du module RC522
const int pinSDA = 53; // pin SDA du module RC522
MFRC522 rfid(pinSDA, pinRST);

void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();
    pinMode(pinLEDVerte, OUTPUT);
    pinMode(pinLEDRouge, OUTPUT);
    pinMode(buzz, OUTPUT);
}

void loop() {
    int refus = 0; // quand cette variable n'est pas nulle, c'est que le code est refusé

    if (rfid.PICC_IsNewCardPresent()) { // on a dédecté un tag
        if (rfid.PICC_ReadCardSerial()) { // on a lu avec succès son contenu
            for (byte i = 0; i < rfid.uid.size; i++) { // comparaison avec le bon UID
                Serial.print(rfid.uid.uidByte[i], HEX);
                if (rfid.uid.uidByte[i] != bonUID[i]) {
                    refus++;
                }
            }
            if (refus == 0) {// UID accepté
                // on allume la LED verte pendant trois secondes
                digitalWrite(pinLEDVerte, HIGH);
                tone(buzz,523,50);
                delay(50);
                tone(buzz, 783, 50);
                delay(50);
                tone(buzz, 1046, 50);
                delay(50);
                tone(buzz, 1568, 50);
                delay(50);
                tone(buzz, 2093, 70);
                delay(250);
                delay(2550);
                digitalWrite(pinLEDVerte, LOW);
            }

            else {  // UID refusé
                // on allume la LED rouge pendant trois secondes
                digitalWrite(pinLEDRouge, HIGH);
                tone(buzz,370,50);
                delay(100);
                tone(buzz, 370, 300);
                delay(1000);
                delay(1900);
                digitalWrite(pinLEDRouge, LOW);
            }
        }
    }
    delay(100);
}

/* EEPROM from internet */
// //Libraries
// #include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
// //Constants
// #define EEPROM_SIZE 12
// void setup() {
//  //Init Serial USB
//  Serial.begin(115200);
//  Serial.println(F("Initialize System"));
//  //Init EEPROM
//  EEPROM.begin(EEPROM_SIZE);
//  //Write data into eeprom
//  int address = 0;
//  int boardId = 18;
//  EEPROM.put(address, boardId);
//  address += sizeof(boardId); //update address value
//  float param = 26.5;
//  EEPROM.put(address, param);
//  EEPROM.commit();
//  //Read data from eeprom
//  address = 0;
//  int readId;
//  EEPROM.get(address, readId);
//  Serial.print("Read Id = ");
//  Serial.println(readId);
//  address += sizeof(readId); //update address value
//  float readParam;
//  EEPROM.get(address, readParam); //readParam=EEPROM.readFloat(address);
//  Serial.print("Read param = ");
//  Serial.println(readParam);
//  EEPROM.end();
// }
// void loop() {}
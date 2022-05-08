// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
#include <Arduino.h> 
#include <ESP8266WiFi.h>   // permet la connexion du module ESP8266 à la WiFi
#include <PubSubClient.h>  // permet d'envoyer et de recevoir des messages MQTT

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h> //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h

#include "Wire.h"
#include "Adafruit_INA219.h"
#include <Adafruit_NeoPixel.h>



#define EEPROM_SIZE 2<<7
// /* FabLab Network */
// #define PROJECT_NAME "TEST_MQTT"                   // nom du projet // à enlever ?
// #define SSID "ESME-FABLAB"                         // indiquer le SSID de votre réseau
// #define PASSWORD "ESME-FABLAB"                     // indiquer le mdp de votre réseau
// #define IP_RASPBERRY "192.168.1.202"               // adresse du serveur MQTT auquel vous etes connecté
// #define PORT_RASPBERRY 5678
// /* Raps Access Point Network */
#define SSID "raspi-webgui"                     // indiquer le SSID de votre réseau
#define PASSWORD "ChangeMe"                     // indiquer le mdp de votre réseau
#define IP_RASPBERRY "10.3.141.1"               // adresse du serveur MQTT auquel vous etes connecté
// #define PORT_RASPBERRY 5678
#define N8N_PORT 5678
#define MQTT_PORT 1883

#define NOMBRE_CASIER 1
#define PRINT true
#define INTERNET false
#define RFID_ACTIVE true
#define CASIER true
#define RESET_EEPROM false
#define MUSIC_ACTIVE true
#define INA2019 false
#define MQTT_active false

#define ALPHA true
#define DEBUG true
#define SS_PIN D8
#define RST_PIN D3
#define NUM_LEDS 5
#define NUM_DEBUG_LED 3

#define TEMP_music_switch true

Adafruit_INA219 ina219;


/* roue de couleur */
byte * Wheel(byte WheelPos) {
    static byte c[3];
  
    if(WheelPos < 85) {
        c[0] = WheelPos * 3;
        c[1] = 255 - WheelPos * 3;
        c[2] = 0;
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        c[0] = 255 - WheelPos * 3;
        c[1] = 0;
        c[2] = WheelPos * 3;
    } else {
        WheelPos -= 170;
        c[0] = 0;
        c[1] = WheelPos * 3;
        c[2] = 255 - WheelPos * 3;
    }

    return c;
}


const int succes_song[][3] = {
    {523 , 50, 50 },
    {783 , 50, 50 },
    {1046, 50, 50 },
    {1568, 50, 50 },
    {2093, 70, 250},
};

const int failure_song[][3] = {
    {370 , 50, 100 },
    {370 , 300, 1000 },
    {0 , 0, 0 },
    {0 , 0, 0 },
    {0 , 0, 0 },
};




/** WIFI **/
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
// boolean wifi_connected = false; //deprecated

WiFiClient html_client;

/* MQTT */
#define TPC_NAME_SIZE 80
char inTopic[TPC_NAME_SIZE];
char outTopic[TPC_NAME_SIZE];


/* setup des pin.s de la carte pour la connexion avec le module RFID */
const int pinSDA    = D8; // pin SDA du module RC522
const int pinSCK    = D5; // pin SCK du module RC522
const int pinMOSI   = D7; // pin MOSI du module RC522
const int pinMISO   = D6; // pin MISO du module RC522
const int pinRST    = D3; // pin RST du module RC522

const int pinSS     = pinSDA; // pin SS du module RC522
const int buzz = D2;
const int pinLEDrgb = D4;
const int relai = D1; 


/* RFID */
// const byte bonUID[NOMBRE_CASIER][4] = {{245,100,55,70}};
// const byte listeUID[4] = {245,100,55,70};
// Init array that will store new NUID 
byte nuidPICC[NOMBRE_CASIER][4];
// MFRC522 rfid(pinSDA, pinRST);
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 


/* LEDs */
Adafruit_NeoPixel pixels(NUM_LEDS, pinLEDrgb, NEO_GRB + NEO_KHZ800);


/* VARIABLES TEMPORAIRES */
#define MSG_BUFFER_SIZE	50
char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;
int value = 0;
bool casier_disponible[4] = {true, true, true, true};


#if PRINT 
#define Print(a) Serial.print(a)
#define Printf(a) Serial.printf(a)
#define Println(a) Serial.println(a)
#define Printb(a,b) Serial.print(a,b)
#define Printbf(a,b) Serial.printf(a,b)
#define Printbln(a,b) Serial.println(a,b)
#else
#define Print(a) 
#define Printf(a) 
#define Println(a)
#define Printb(a,b)
#define Printbf(a,b)
#define Printbln(a,b)
#endif

#define LED_init() pixels.begin()
#define LED(i, r, g, b) {pixels.setPixelColor(i, pixels.Color(r, g, b)); pixels.show();}
#define LED_temp(i, r, g, b) pixels.setPixelColor(i, pixels.Color(r, g, b))
#define LED_show() pixels.show()
#define LED_clear()     \
    {                   \
        pixels.clear(); \
        pixels.show();  \
    }

#if DEBUG
#define DEBUG_LED(t, r, g, b)           \
    {                                   \
        LED(NUM_DEBUG_LED, r, g, b);    \
        delay(t);                       \
    }
#define DEBUG_Print(a)     \
    {                      \
        Serial.print("[DEBUG] "); \
        Serial.println(a); \
    }
#define DEBUG_Print(a,b)     \
    {                      \
        Serial.print("[DEBUG] "); \
        Serial.println(a,b); \
    }
#else
#define DEBUG_LED(i, r, g, b)
#define DEBUG_Print(a)
#define DEBUG_Print(a,b)
#endif

#if CASIER
#define Ouvrir_casier(indice, rfia) {                                                                   \
                /* on assigne le casier à la carte */                                                   \
                digitalWrite(relai, HIGH); /* ouverture locket {&indice} */                               \
                casier_disponible[indice] = !casier_disponible[indice];                                 \
                /* on stock le code de la nouvelle carte si le casier n'est pas pris */                 \
                for (byte i = 0; i < 4; i++) {                                                          \
                    nuidPICC[indice][i] = rfia;                                                         \
                    EEPROM_write(4*i + 16 * indice, nuidPICC[indice][i]);/* devrais écrire sur des 0 */ \
                }                                                                                       \
                delay(2000);                                                                            \
                digitalWrite(relai, LOW);/* fermeture locket &indice */                                 \
}
#else
#define Ouvrir_casier(indice)
#endif

#if MUSIC_ACTIVE
#if TEMP_music_switch 
#define MUSIC(song) {   \
    for (int i = 0; i < sizeof(song)-1; i++) {  \
        tone(buzz, song[i][0], song[i][1]);   \
        delay(song[i][2]); \
    } \
}
#else
#define MUSIC(song) {   \
    for (int notes = 0, notes < 5; notes++){ \
        tone(buzz, succes_song[0], succes_song[1]); \
        delay(succes_song[2]); \
    } \
}
#endif
#else
#define MUSIC(song) 
#endif

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
Adafruit_INA219 ina219;

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
#define PORT_RASPBERRY 5678
#define NOMBRE_CASIER 2

#define SS_PIN D8
#define RST_PIN D3
#define NUM_LEDS 5

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


const int play[][3] = {
    {523 , 50, 50 },
    {783 , 50, 50 },
    {1046, 50, 50 },
    {1568, 50, 50 },
    {2093, 70, 250},
};


/** WIFI **/
WiFiClient espClient;
PubSubClient client(espClient);
boolean wifi_connected = false;


/* MQTT */
#define TPC_NAME_SIZE 80
char inTopic[TPC_NAME_SIZE];
char outTopic[TPC_NAME_SIZE];


/* setup des pin.s de la carte pour la connexion avec le module RFID */
// const int pinRST    = D3; // pin RST du module RC522
// const int pinSS     = D8; // pin SS du module RC522
// const int pinMOSI   = D7; // pin MOSI du module RC522
// const int pinMISO   = D6; // pin MISO du module RC522
// const int pinSCK    = D5; // pin SCK du module RC522
// const int pinSDA    = pinSS; // pin SDA du module RC522
//const int buzz = D0;
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

/* Print Pretty Hexadecimal */
void printHex(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/* Print Pretty Decimal */
void printDec(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}

/* make LEDs rainbow */
void rainbowCycle(int SpeedDelay) {
    byte *c;
    uint16_t i, j;

    for (j = 0; j < 151*5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 151 / NUM_LEDS) + j) & 150);
            pixels.setPixelColor(i, pixels.Color(*c, *(c+1), *(c+2)));
        }
        pixels.show();
        delay(SpeedDelay);
        pixels.clear();
    }
}

/* Fonction de paramètrage du WiFi */
void setup_wifi() {
    delay(10);
    // Nous affichons le nom du réseau WiFi sur lequel nous souhaitons nous connecter
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);

    // Configuration du WiFi pour faire une connexion à une borne WiFi
    WiFi.mode(WIFI_STA);

    // Connexion au réseau WiFi "SSID" avec le mot de passe contenu dans "password"
    WiFi.begin(SSID, PASSWORD);
    
    // Tant que le WiFi n'est pas connecté, on attends!
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Affichage de l'adresse IP du module
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

/* Fonction appelé lors de la réception de donnée via le MQTT */
void callback(char* topic, byte* payload, unsigned int length) {

    // Afficher le message reçu
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    //********************************//
    // TRAITEMENT DES DONNEES RECUES
    //********************************//
    
}

/* Fonction de reconnexion au broker MQTT */
void reconnect() {
    // Tant que le client n'est pas connecté...
    // while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        // Génération d'un identifiant unique
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        
        // Tentative de connexion
        if (client.connect(clientId.c_str())) {

            // Connexion réussie
            Serial.println("connected");

            // Abonnement aux topics au près du broker MQTT
            snprintf(inTopic, TPC_NAME_SIZE, "ESME/#");
            
            // inTopic => /ESME/COMPTEUR/inTopic
            client.subscribe(inTopic);

        } else {

            // Tentative échouée
            Serial.print("failed, rc=");
            Serial.print(client.state());
            // Serial.println(" try again in 5 seconds");

            // Attente de 5 secondes avant une nouvelle tentative
            // delay(5000);
        }
    // }
}

void setup_pins() {
    // Paramètrage de la pin BUILTIN_LED en sortie
    pinMode(LED_BUILTIN, OUTPUT);

    // pinMode(pinLEDVerte, OUTPUT);
    // pinMode(pinLEDRouge, OUTPUT);
    // pinMode(buzz, OUTPUT);
    pinMode(pinLEDrgb, OUTPUT);
    // pinMode(buzz, OUTPUT);
    pinMode(relai, OUTPUT);
}

void send_MQTT(float &my_value, const char * my_topic ) {
    snprintf(msg, MSG_BUFFER_SIZE,"%f", my_value);
    // Serial.print("[MQTT] Publish message: ");
    // Serial.print(msg);
    // Serial.print(" topic: ");
    // Serial.println(my_topic);
    
    // Construction du topic d'envoi
    snprintf(outTopic, TPC_NAME_SIZE, my_topic);
    // outTopic => /ESME/COMPTEUR/my_topic

    // Envoi de la donnée
    client.publish(outTopic, msg);
}

String HTTP_connect_send_and_print(String parameters) {// deprecated
    String returned = "";
    WiFiClient client_local;
    Serial.printf("\n[Connecting to %s ... ", IP_RASPBERRY);
    if (client_local.connect(IP_RASPBERRY, PORT_RASPBERRY)) {
        Serial.println("connected]");

        // Serial.println("[Sending a request]");
        String message = String("GET /webhook/innov?") + parameters + " HTTP/1.1\r\n" +
                        "Host: " + IP_RASPBERRY + "\r\n" +
                        "Connection: close\r\n" +
                        "\r\n";
        // Serial.println(message);
        client_local.print(message);

        // Serial.println("[Response:]");
        while (client_local.connected() || client_local.available()) {
            if (client_local.available()) {
                String line = client_local.readStringUntil('\n');
                returned = line;
                // Serial.println(line);
            }
        }
        client_local.stop();
        // Serial.println("\n[Disconnected]");
        // Serial.println(returned);
    } else {
        Serial.println("connection failed!]");
        client_local.stop();
    }
    return returned;
}

void EEPROM_write(int adresse, float param) {
    // Init EEPROM
    EEPROM.begin(EEPROM_SIZE);

    //Write data into eeprom
    int write_address = adresse;
    Serial.print("WRITE");

    Serial.print(" | size = ");
    Serial.print(sizeof(param));

    Serial.print(" | address = ");
    Serial.print(write_address);

    EEPROM.put(write_address, param);
    write_address += sizeof(param);

    Serial.print("..");
    Serial.print(write_address);
    
    Serial.print(" | param = ");
    Serial.print(param);
    Serial.println();

    EEPROM.commit();
    EEPROM.end();
}

#define EEPROM_READ(address, type) ({type tmp; _EEPROM_read(address, sizeof(tmp)) ; })
float _EEPROM_read(int adresse, int sizeofparam) {
    //Init EEPROM
    EEPROM.begin(EEPROM_SIZE);

    //Read data from eeprom
    int read_address = adresse;
    Serial.print("READ ");
    
    Serial.print(" | size = ");
    Serial.print(sizeofparam);

    Serial.print(" | address = ");
    Serial.print(read_address);

    float readParam;
    EEPROM.get(read_address, readParam); //readParam=EEPROM.readFloat(address);
    
    Serial.print("..");
    read_address += sizeof(readParam); //update address value
    Serial.print(read_address);
    
    Serial.print(" | val = ");
    Serial.print(readParam);

    Serial.println();
    EEPROM.end();

    return readParam;
}

bool RFID_read_print_and_recognize() {
    u8 currentRFID[4] = {0, 0, 0, 0};
    Serial.println();
    if (rfid.PICC_IsNewCardPresent()) { // on a dédecté un tag
        if (rfid.PICC_ReadCardSerial()) { // on a lu avec succès son contenu
            for (u8 i = 0; i < 4; i ++) { currentRFID[i] = rfid.uid.uidByte[i]; }
            for (u8 i = 0; i < 4; i ++) { Serial.print(currentRFID[i]); Serial.print(" "); }
            Serial.println();
        }
    }
    return false;
}

void MQTT_communication_info() {
    // Construction du message à envoyer
    float current_mA = 0;
    float voltage_V = 0;
    float shunt_voltage_mV = 0;
    current_mA = ina219.getCurrent_mA();
    voltage_V = ina219.getBusVoltage_V();
    shunt_voltage_mV = ina219.getShuntVoltage_mV();
    // Serial.println(current_mA);
    // Serial.println(voltage_V);
    // Serial.println(shunt_voltage_mV);
    Serial.println();
    send_MQTT(current_mA, "ESME/COMPTEUR_AMP");
    send_MQTT(voltage_V, "ESME/COMPTEUR_VOL");
    send_MQTT(shunt_voltage_mV, "ESME/COMPTEUR_SmV");
}

void init_leds() {
    
    pixels.begin();
    pixels.show();
    pixels.clear();

    /* ANIMATION LED DÉBUT */

    for (int i = 0; i < NUM_LEDS; i++){
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
        pixels.show();
        delay(300);
    }

    rainbowCycle(5);

    pixels.clear();
    pixels.show();

    delay(200);

    for (int i = 0; i < NUM_LEDS; i++){
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
        pixels.show();
    }
    delay(200);
    pixels.clear();
    pixels.show();

    // play_sound(play);

    /* ANIMATION LED FIN */
}

bool check_internet(byte code_rfid[4]) {
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(150, 0, 150));// LED 0 en rouge
    pixels.show();
    String id = "";
    for (int i = 0; i < 4; i ++) {
        id += code_rfid[i];
        id += ".";
    }
    String answer = HTTP_connect_send_and_print((String)"id=" + id);
    Serial.print("[RESPONSE] ");
    Serial.println(answer);
    if (answer == "not in sheet") {
        return true;
    }
    return false;
}
int RFID() {
    int address = -1;

    // rainbowCycle(5); // Arc-en-cieel

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! rfid.PICC_IsNewCardPresent()) {return -2;}

    // Verify if the NUID has been readed
    if ( ! rfid.PICC_ReadCardSerial()) {return -3;}

    Serial.print("PICC type: ");
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

    // Check is the PICC of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return -4;
    }
    // on allume la LED 0 en bleu, indique que la carte a été détecté
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(0, 0, 150));// LED 0 en bleu
    pixels.show();

    // if (check_internet(rfid.uid.uidByte)) {
    //     // on allume la LED 0 en rouge, indique que la carte n'est pas reconnu
    //     pixels.clear();
    //     pixels.setPixelColor(0, pixels.Color(150, 0, 0));// LED 0 en rouge
    //     pixels.show();
    //     return -5;
    // }
    for (int indice = 0; indice < NOMBRE_CASIER; indice++) {
        if (rfid.uid.uidByte[0] != nuidPICC[indice][0] ||
            rfid.uid.uidByte[1] != nuidPICC[indice][1] ||
            rfid.uid.uidByte[2] != nuidPICC[indice][2] ||
            rfid.uid.uidByte[3] != nuidPICC[indice][3]) {

            Serial.println("A new card has been detected.");
            Serial.println(F("The NUID tag is:"));
            Serial.print(F("In hex: "));
            printHex(rfid.uid.uidByte, rfid.uid.size);
            Serial.println();
            Serial.print(F("In dec: "));
            printDec(rfid.uid.uidByte, rfid.uid.size);
            Serial.println();

            if (casier_disponible[indice]) {// si le casier n'est pas pris
                if (check_internet(rfid.uid.uidByte)) {
                    // on allume la LED 0 en rouge, indique que la carte n'est pas reconnu
                    pixels.clear();
                    pixels.setPixelColor(0, pixels.Color(150, 0, 0));// LED 0 en rouge
                    pixels.show();
                    delay(2000);
                    pixels.clear();
                    pixels.show();
                    return -5;
                }
                pixels.setPixelColor(indice+1, pixels.Color(0, 150, 0));// LED &indice+1 en vert
                pixels.show();

                // play_sound(play);

                // on assigne le casier à la carte
                digitalWrite(relai, HIGH);// ouverture locket &indice
                
                casier_disponible[indice] = !casier_disponible[indice];
                // on stock le code de la nouvelle carte si le casier n'est pas pris
                for (byte i = 0; i < 4; i++) {
                    nuidPICC[indice][i] = rfid.uid.uidByte[i];
                    EEPROM_write(4*i + 16 * indice, nuidPICC[indice][i]);// devrais écrire sur des 0
                }
                address = 16 * indice;
                delay(2000);
                digitalWrite(relai, LOW);// fermeture locket &indice
                break;
            } else { // si le casiers est pris
                pixels.setPixelColor(indice+1, pixels.Color(150, 0, 0));// LED &indice+1 en rouge
                pixels.show();
                Serial.print("code RFID:");
                for (byte i = 0; i < 4; i++) {
                    Serial.print(nuidPICC[indice][i]);
                    Serial.print(".");
                }
                Serial.print(" | ");
                Serial.println(casier_disponible[indice]);
                delay(1000);
                // delay(1000);
                // tone(buzz,370,50);
                // delay(100);
                // tone(buzz, 370, 300);
            }
        } else {// on lit la même carte que celle du casier &indice

            Serial.println(F("Card read previously."));
            // Retrait de son téléphone
            if (not casier_disponible[indice]) {// check que le casier est bien pris // sûrement inutile
                pixels.setPixelColor(indice+1, pixels.Color(0, 150, 0));// LED &indice+1 en vert
                pixels.show();

                // play_music(play);

                digitalWrite(relai, HIGH);// ouverture locket &indice
                casier_disponible[indice] = !casier_disponible[indice];
                // on efface le code de l'ancienne carte
                for (byte i = 0; i < 4; i++) {
                    nuidPICC[indice][i] = 0;
                    EEPROM_write(4*i + 16 * indice, 0);
                }
                // address = -10;
                delay(2000);
                digitalWrite(relai, LOW);// fermeture locket &indice
                break;
            } else {// devrais jamais arriver // on lit la carte du casier et le casier n'est pas pris
                if (check_internet(rfid.uid.uidByte)) {
                    // on allume la LED 0 en rouge, indique que la carte n'est pas reconnu
                    pixels.clear();
                    pixels.setPixelColor(0, pixels.Color(150, 0, 0));// LED 0 en rouge
                    pixels.show();
                    delay(2000);
                    pixels.clear();
                    pixels.show();
                    return -5;
                }
                pixels.setPixelColor(indice+1, pixels.Color(0, 150, 0));// LED &indice+1 en vert
                pixels.show();

                // play_music(play);

                digitalWrite(relai, HIGH);// ouverture locket &indice
                casier_disponible[indice] = !casier_disponible[indice];
                // on stock le code de la nouvelle carte si le casier n'est pas pris
                for (byte i = 0; i < 4; i++) {
                    nuidPICC[indice][i] = rfid.uid.uidByte[i];
                    EEPROM_write(4*i + 16 * indice, nuidPICC[indice][i]);// devrais écrire sur des 0
                }
                address = 16 * indice;
                delay(2000);
                digitalWrite(relai, LOW);// fermeture locket &indice
                break;
            }
        }
    }
    pixels.clear();
    pixels.show();
    rfid.PICC_HaltA(); // Halt PICC
    rfid.PCD_StopCrypto1(); // Stop encryption on PCD
    return address;
}

void init_EEPROM() {
    for (int indice = 0; indice < NOMBRE_CASIER; indice ++) {
        for (byte i = 0; i < 4; i++) {
            nuidPICC[indice][i] = EEPROM_READ(4*i + 16 * indice, int);
        }
        Serial.println(nuidPICC[indice][0] + nuidPICC[indice][1] + nuidPICC[indice][2] + nuidPICC[indice][3]);
        if (nuidPICC[indice][0] + nuidPICC[indice][1] + nuidPICC[indice][2] + nuidPICC[indice][3] != 0) {
            Serial.print("well init code RFID:");
            for (byte i = 0; i < 4; i++) {
                Serial.print(nuidPICC[indice][i]);
                Serial.print(".");
            }
            Serial.print(" | ");
            Serial.println(casier_disponible[indice]);
            casier_disponible[indice] != casier_disponible[indice];
        }
    }
}

void init_custom_EEPROM() {
    for (int indice = 0; indice < 4; indice ++) {
        for (byte i = 0; i < 4; i++) {
            EEPROM_write(4*i + 16 * indice, 0);
        }
    }
}


void setup() {
    delay(1000);
    setup_pins();           // assignation des pins de l'ESP

    Serial.begin(115200);   // Configuration de la communication série à 115200 Mbps
    Serial.println();
    SPI.begin();            // Initialistaion du SPI (dépendances)
    rfid.PCD_Init();        // Configuration du RFID

    // // initialisation de la communication avec l'INA219
    // if (! ina219.begin()) {
    //     Serial.println("Erreur pour trouver le INA219");
    //     while (1) { delay(10); }
    // }

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    init_leds();                            // initialisation du bandeau leds
    setup_wifi();                           // Connexion au WiFi
    client.setServer(IP_RASPBERRY, 1883);   // Configuration de la connexion au broker MQTT
    client.setCallback(callback);           // Déclaration de la fonction de récupération des données reçues du broker MQTT
    init_custom_EEPROM(); // permet d'enregistrer une carte prédéfine dans l'ESP
    init_EEPROM();

    // wifi_connected = connect_serveur_HTML(client_someone);
}

void loop() {
    // if (!client.connected()) { reconnect(); } // Si perte de connexion MQTT, on essaye une reconnexion!
    // client.loop(); // Appel de fonction pour redonner la main au process de communication MQTT

    int user_RFID_address = RFID();
    Serial.print("address:");
    Serial.println(user_RFID_address);
    // unsigned long now = millis();
    // if (now - lastMsg > 2000) {
    //     lastMsg = now;
    //     MQTT_communication_info();
    //     Serial.println(HTTP_connect_send_and_print("id=hello"));
    //     Serial.println(HTTP_connect_send_and_print("id=WDXFGHKML"));
    //     Serial.println(HTTP_connect_send_and_print("id=WDXFGHKM"));
    // }
    delay(100);
}

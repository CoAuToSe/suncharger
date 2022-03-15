// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

#include <Arduino.h> 
#include <ESP8266WiFi.h>   // permet la connexion du module ESP8266 à la WiFi
#include <PubSubClient.h>  // permet d'envoyer et de recevoir des messages MQTT

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h> //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h

#include "Wire.h"
#include "Adafruit_INA219.h"
Adafruit_INA219 ina219;

#define EEPROM_SIZE 2<<7
#define PROJECT_NAME "TEST_MQTT"                   // nom du projet
#define SSID "ESME-FABLAB"                         // indiquer le SSID de votre réseau
#define PASSWORD "ESME-FABLAB"                     // indiquer le mdp de votre réseau
#define IP_RASPBERRY "192.168.1.203"                 // adresse du serveur MQTT auquel vous etes connecté
#define PORT_RASPBERRY 80
#define NOMBRE_CASIER 4

/** WIFI **/
WiFiClient espClient;
PubSubClient client(espClient);

/* MQTT */
#define TPC_NAME_SIZE 80
char inTopic[TPC_NAME_SIZE];
char outTopic[TPC_NAME_SIZE];

/* RFID */
const byte bonUID[NOMBRE_CASIER][4] = {245,100,55,70};

/* setup des pin.s de la carte pour la connexion avec le module RFID */
const int pinRST = D3; // pin RST du module RC522
const int pinSS = D8; // pin SS du module RC522
const int pinMOSI = D7; // pin MOSI du module RC522
const int pinMISO = D6; // pin MISO du module RC522
const int pinSCK = D5; // pin SCK du module RC522
const int pinSDA = pinSS; // pin SDA du module RC522
MFRC522 rfid(pinSDA, pinRST);

/* VARIABLES TEMPORAIRES */
#define MSG_BUFFER_SIZE	50
char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;
int value = 0;

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
    Serial.println("IP address: ");
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
    while (!client.connected()) {
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
            Serial.println(" try again in 5 seconds");

            // Attente de 5 secondes avant une nouvelle tentative
            delay(5000);
        }
    }
}

void setup_pins() {
    // Paramètrage de la pin BUILTIN_LED en sortie
    pinMode(LED_BUILTIN, OUTPUT);

    // pinMode(pinLEDVerte, OUTPUT);
    // pinMode(pinLEDRouge, OUTPUT);
    // pinMode(buzz, OUTPUT);
}

void setup() {
    setup_pins();

    // Configuration de la communication série à 115200 Mbps
    Serial.begin(115200);
    
    Serial.println();

    // 
    if (! ina219.begin()) {
        Serial.println("Erreur pour trouver le INA219");
        while (1) { delay(10); }
    }

    // Connexion au WiFi
    setup_wifi();

    // Configuration de la connexion au broker MQTT
    client.setServer(IP_RASPBERRY, 1883);

    // Initialistaion du SPI (dépendances)
    SPI.begin();

    // Configuration du RFID
    rfid.PCD_Init();

    // Déclaration de la fonction de récupération des données reçues du broker MQTT
    client.setCallback(callback);
}

void send_MQTT(float &my_value, const char * my_topic ) {
    snprintf(msg, MSG_BUFFER_SIZE,"%f", my_value);
    Serial.print("[MQTT]Publish message: ");
    Serial.print(msg);
    Serial.print(" topic: ");
    Serial.println(my_topic);
    
    // Construction du topic d'envoi
    snprintf(outTopic, TPC_NAME_SIZE, my_topic);
    // outTopic => /ESME/COMPTEUR/my_topic

    // Envoi de la donnée
    client.publish(outTopic, msg);
}


void loop() {
    
    // Si perte de connexion, reconnexion!
    if (!client.connected()) { reconnect(); }

    // Appel de fonction pour redonner la main au process de communication MQTT
    client.loop();

    // Sous programme de test pour un envoi périodique
    unsigned long now = millis();
    if (now - lastMsg > 2000) {
        // Enregistrement de l'action réalisée
        lastMsg = now;
        MQTT_communication_info();
        RFID_read_print_and_recognize();
        HTTP_send_connect_and_print();
        EEPROM_use();
    }
    delay(1000);
}
    
String HTTP_send_connect_and_print() {
    
    WiFiClient client_local;
    Serial.printf("\n[Connecting to %s ... ", IP_RASPBERRY);
    if (client_local.connect(IP_RASPBERRY, PORT_RASPBERRY)) {
        Serial.println("connected]");

        Serial.println("[Sending a request]");
        String message = String("GET /") + " HTTP/1.1\r\n" +
                         "Host: " + IP_RASPBERRY + "\r\n" +
                         "Connection: close\r\n" +
                         "\r\n";
        Serial.println(message);
        client_local.print(message);

        Serial.println("[Response:]");
        while (client_local.connected() || client_local.available()) {
            if (client_local.available()) {
                String line = client_local.readStringUntil('\n');
                Serial.println(line);
            }
        }
        client_local.stop();
        Serial.println("\n[Disconnected]");
    } else {
        Serial.println("connection failed!]");
        client_local.stop();
    }
}

void EEPROM_use() {
    //Init Serial USB
    Serial.begin(115200);
    Serial.println(F("Initialize System"));

    //Init EEPROM
    EEPROM.begin(EEPROM_SIZE);

    //Write data into eeprom
    int address = 0;
    int boardId = 18;
    EEPROM.put(address, boardId);
    address += sizeof(boardId); //update address value
    float param = 26.5;
    EEPROM.put(address, param);
    EEPROM.commit();

    //Read data from eeprom
    address = 0;
    int readId;
    EEPROM.get(address, readId);
    Serial.print("Read Id = ");
    Serial.println(readId);
    address += sizeof(readId); //update address value
    float readParam;
    EEPROM.get(address, readParam); //readParam=EEPROM.readFloat(address);
    Serial.print("Read param = ");
    Serial.println(readParam);
    EEPROM.end();
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
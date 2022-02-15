// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

#include <Arduino.h> 
#include <ESP8266WiFi.h>   // permet la connexion du module ESP8266 à la WiFi
#include <PubSubClient.h>  // permet d'envoyer et de recevoir des messages MQTT

#include "Wire.h"
#include "Adafruit_INA219.h"
Adafruit_INA219 ina219;

#define PROJECT_NAME "TEST_MQTT"                          // nom du projet
const char* ssid = "ESME-FABLAB";                         // indiquer le SSID de votre réseau
const char* password = "ESME-FABLAB";                     // indiquer le mdp de votre réseau
const char* mqtt_server = "192.168.1.203";                 // adresse du serveur MQTT auquel vous etes connecté

/** WIFI **/
WiFiClient espClient;
PubSubClient client(espClient);

/* MQTT */
#define TPC_NAME_SIZE   80
char inTopic[TPC_NAME_SIZE];
char outTopic[TPC_NAME_SIZE];

/* VARIABLES TEMPORAIRES */
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;
int value = 0;

/* Fonction de paramètrage du WiFi */
void setup_wifi() {
    delay(10);
    // Nous affichons le nom du réseau WiFi sur lequel nous souhaitons nous connecter
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Configuration du WiFi pour faire une connexion à une borne WiFi
    WiFi.mode(WIFI_STA);

    // Connexion au réseau WiFi "ssid" avec le mot de passe contenu dans "password"
    WiFi.begin(ssid, password);
    
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
    for (int i = 0; i < length; i++) {
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
            snprintf(inTopic, TPC_NAME_SIZE, "ESME/COMPTEUR");
            
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

void setup() {
    // Paramètrage de la pin BUILTIN_LED en sortie
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Configuration de la communication série à 115200 Mbps
    Serial.begin(115200);
    // Serial.println();
    // Serial.begin(9600);
    if (! ina219.begin()) {
        Serial.println("Erreur pour trouver le INA219");
        while (1) { delay(10); }
    }
    Serial.print("Courant"); 
    Serial.print("\t");

    // Connexion au WiFi
    setup_wifi();

    // Configuration de la connexion au broker MQTT
    client.setServer(mqtt_server, 1883);

    // Déclaration de la fonction de récupération des données reçues du broker MQTT
    client.setCallback(callback);
}

void loop() {

    // Si perte de connexion, reconnexion!
    if (!client.connected()) {
      reconnect();
    }

    // Appel de fonction pour redonner la main au process de communication MQTT
    client.loop();

    // Sous programme de test pour un envoi périodique
    unsigned long now = millis();
    if (now - lastMsg > 2000) {
        // Enregistrement de l'action réalisée
        lastMsg = now;

        // Construction du message à envoyer
        float current_mA = 0;
        float voltage_V = 0;
        float shunt_voltage_mV = 0;
        current_mA = ina219.getCurrent_mA();
        voltage_V = ina219.getBusVoltage_V();
        shunt_voltage_mV = ina219.getShuntVoltage_mV();
        Serial.println(current_mA);
        Serial.println(voltage_V);
        Serial.println(shunt_voltage_mV);
        // delay(1000);
        value = current_mA*10;
        snprintf (msg, MSG_BUFFER_SIZE,"%d", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        
        // Construction du topic d'envoi
        snprintf(outTopic, TPC_NAME_SIZE, "ESME/COMPTEUR");
        // outTopic => /ESME/COMPTEUR/outTopic

        // Envoi de la donnée
        client.publish(outTopic, msg);
    }
}

// #include "Wire.h"
// #include "Adafruit_INA219.h"
// Adafruit_INA219 ina219;

// void setup() {
//     Serial.println();
//     Serial.begin(9600);
//     if (! ina219.begin()) {
//         Serial.println("Erreur pour trouver le INA219");
//         while (1) { delay(10); }
//     }
//     Serial.print("Courant"); 
//     Serial.print("\t");
// }

// void loop() {
//     float current_mA = 0;
//     current_mA = ina219.getCurrent_mA();
//     Serial.print(current_mA); 
//     Serial.print("\t");
//     delay(1000);
// }
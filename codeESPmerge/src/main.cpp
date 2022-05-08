#include "main.h"


void play_music(const int song[][3]) {
    for (int i = 0; i < sizeof(song)-1; i++) { 
        tone(buzz, song[i][0], song[i][1]);  
        delay(song[i][2]);
    }
}

/* Print Pretty Hexadecimal */
void PrintHex(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Print(buffer[i] < 0x10 ? " 0" : " ");
        Printb(buffer[i], HEX);
    }
}

/* Print Pretty Decimal */
void PrintDec(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Print(buffer[i] < 0x10 ? " 0" : " ");
        Printb(buffer[i], DEC);
    }
}

/* make LEDs rainbow */
void rainbowCycle(int SpeedDelay) {
    byte *c;
    uint16_t i, j;

    for (j = 0; j < 151*5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 151 / NUM_LEDS) + j) & 150);
            LED_temp(i,*c, *(c+1), *(c+2));
        }
        LED_show();
        delay(SpeedDelay);
        LED_clear();
    }
}


// unsigned long last = 0;
// unsigned char stateLed = 0;
/* make LEDs rainbow */
void rainbowCycleAsyncWifi(int SpeedDelay) {
    // // loop
    // if (millis() - last > 500) {
    //   last = millis();
    //   stateLed = (stateLed + 1 ) %2;
    //   digitalWrite(LED_BUILTIN, stateLed);
    //   Serial.print("LED : ");
    //   Serial.println(stateLed);
    // }
    byte *c;
    uint16_t i, j;

    
    // while 
    for (j = 0; j < 151*5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 151 / NUM_LEDS) + j) & 150);
            LED_temp(i,*c, *(c+1), *(c+2));
        }
        LED_show();
        delay(SpeedDelay);
        LED_clear();
    }
}


void HTML_send(String parameters, byte code_rfid_to_check[4]) {
    if (html_client.connected()) {
        Println("[Sending HTML request]");
        String message = String("GET /webhook/innov?") + parameters + " HTTP/1.1\r\n" +
                        "Host: " + IP_RASPBERRY + "\r\n" +
                        "Connection: close\r\n" +
                        "\r\n";
        html_client.print(message);
    } else {
        Println("[HTML not connected]");
    }
}
/* Fonction appelé lors de la réception de donnée via MQTT */
void callback_MQTT(char* topic, byte* payload, unsigned int length) {

    // Afficher le message reçu
    Print("Message arrived [");
    Print(topic);
    Print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Print((char)payload[i]);
    }
    Println();

    //********************************//
    // TRAITEMENT DES DONNEES RECUES
    //********************************//
    
}
/* Fonction appelé lors de la réception de donnée via HTML */
void callback_HTML(char* topic, byte* payload, unsigned int length) {

    String answer = "";
    // Afficher le message reçu
    Print("reponse de n8n: [");
    Print(topic);
    Print("] ");
    for (unsigned int i = 0; i < length; i++) {
        answer += (char)payload[i];
        Print((char)payload[i]);
    }
    Println();
    Println(answer);
    if (answer != "done") { 
        LED_clear();                        
        LED(0, 150, 0, 0);                  
        delay(2000);                        
        LED_clear()    
        // return false;
    }
    // return true;
}

/* Fonction de paramètrage du WiFi */
void setup_MQTT_wifi() {
    #if MQTT_active
    mqtt_client.setServer(IP_RASPBERRY, MQTT_PORT); // Configuration de la connexion au broker MQTT
    mqtt_client.setCallback(callback_MQTT);         // Déclaration de la fonction de récupération des données reçues du broker MQTT
    #endif
}

void setup_HTML_wifi() {
    // html_client.setServer(IP_RASPBERRY, N8N_PORT);  // Configuration de la connexion au broker HTML
    // html_client.setCallback(callback_HTML);         // Déclaration de la fonction de récupération des données reçues du broker HTM
}

void setup_wifi() {
    #if INTERNET
    delay(10);
    // Nous affichons le nom du réseau WiFi sur lequel nous souhaitons nous connecter
    Println();
    Print("Connecting to ");
    Println(SSID);

    // Configuration du WiFi pour faire une connexion à une borne WiFi
    WiFi.mode(WIFI_STA);

    // Connexion au réseau WiFi "SSID" avec le mot de passe "PASSWORD"
    WiFi.begin(SSID, PASSWORD);
    
    // Tant que le WiFi n'est pas connecté, on attends!
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Print(".");
    }
    Println("");
    Println("WiFi connected");
    // Affichage de l'adresse IP du module ESP
    Print("ESP IP: ");
    Println(WiFi.localIP());

    setup_MQTT_wifi();
    setup_HTML_wifi();
    #endif
}
String HTML_manage_com(String para, byte local_code_rfid[4]) {
    String returned = "";
    LED(0, 150, 0, 150);
    HTML_send(para, local_code_rfid);
    return returned;
}

/* Fonction de reconnexion au broker MQTT */
void reconnect() {
    #if MQTT_active
    // Tant que le client n'est pas connecté...
    // while (!client.connected()) {
        Print("Attempting MQTT connection...");
        
        // Génération d'un identifiant unique
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        
        // Tentative de connexion
        if (mqtt_client.connect(clientId.c_str())) {

            // Connexion réussie
            Println("connected");

            // Abonnement aux topics au près du broker MQTT
            snprintf(inTopic, TPC_NAME_SIZE, "ESME/#");
            
            // inTopic => /ESME/COMPTEUR/inTopic
            mqtt_client.subscribe(inTopic);

        } else {

            // Tentative échouée
            Print("failed, rc=");
            Print(mqtt_client.state());
            // Println(" try again in 5 seconds");

            // Attente de 5 secondes avant une nouvelle tentative
            // delay(5000);
        }
    // }
    #endif
}

void setup_pins() {
    pinMode(LED_BUILTIN, OUTPUT); // LED sur de l'ESP8266

    pinMode(pinLEDrgb, OUTPUT);
    pinMode(buzz, OUTPUT);
    pinMode(relai, OUTPUT);
}

void send_MQTT(float &my_value, const char * my_topic ) {
    #if MQTT_active
    snprintf(msg, MSG_BUFFER_SIZE,"%f", my_value);
    // Print("[MQTT] Publish message: ");
    // Print(msg);
    // Print(" topic: ");
    // Println(my_topic);
    
    // Construction du topic d'envoi
    snprintf(outTopic, TPC_NAME_SIZE, my_topic);
    // outTopic => /ESME/COMPTEUR/my_topic
    #endif

    // Envoi de la donnée
    mqtt_client.publish(outTopic, msg);
}

// deprecated
bool healthy_internet() {
    #if INTERNET
    bool connected = client_global.connected();
    if (!connected) {
        Printbf("\n[Connecting to %s ... ", IP_RASPBERRY);
        if (client_global.connect(IP_RASPBERRY, N8N_PORT)) {
            Println("connected]");
        } else {
            Println("connection failed!]");
            client_global.stop();
        }
    }
    connected = client_global.connected();
    return connected;
    #else
    return false;
    #endif
}

String HTTP_connect_send_and_Print(String parameters) {// deprecated
    String returned = "";
    WiFiClient client_local;
    Printbf("\n[Connecting to %s ... ", IP_RASPBERRY);
    if (client_local.connect(IP_RASPBERRY, N8N_PORT)) {
        Println("connected]");

        // Println("[Sending a request]");
        String message = String("GET /webhook/innov?") + parameters + " HTTP/1.1\r\n" +
                        "Host: " + IP_RASPBERRY + "\r\n" +
                        "Connection: close\r\n" +
                        "\r\n";
        // Println(message);
        client_local.print(message);

        // Println("[Response:]");
        while (client_local.connected() || client_local.available()) {
            if (client_local.available()) {
                String line = client_local.readStringUntil('\n');
                returned = line;
                // Println(line);
            }
        }
        client_local.stop();
        // Println("\n[Disconnected]");
        // Println(returned);
    } else {
        Println("connection failed!]");
        client_local.stop();
    }
    return returned;
}

void EEPROM_write(int adresse, float param) {
    // Init EEPROM
    EEPROM.begin(EEPROM_SIZE);

    //Write data into eeprom
    int write_address = adresse;
    Print("WRITE");

    Print(" | size = ");
    Print(sizeof(param));

    Print(" | address = ");
    Print(write_address);

    EEPROM.put(write_address, param);
    write_address += sizeof(param);

    Print("..");
    Print(write_address);
    
    Print(" | param = ");
    Print(param);
    Println();

    EEPROM.commit();
    EEPROM.end();
}

#define EEPROM_READ(address, type) ({type tmp; _EEPROM_read(address, sizeof(tmp)) ; })
float _EEPROM_read(int adresse, int sizeofparam) {
    //Init EEPROM
    EEPROM.begin(EEPROM_SIZE);

    //Read data from eeprom
    int read_address = adresse;
    Print("READ ");
    
    Print(" | size = ");
    Print(sizeofparam);

    Print(" | address = ");
    Print(read_address);

    float readParam;
    EEPROM.get(read_address, readParam); //readParam=EEPROM.readFloat(address);
    
    Print("..");
    read_address += sizeof(readParam); //update address value
    Print(read_address);
    
    Print(" | val = ");
    Print(readParam);

    Println();
    EEPROM.end();

    return readParam;
}

// code mort
bool RFID_read_Print_and_recognize() {
    u8 currentRFID[4] = {0, 0, 0, 0};
    Println();
    if (rfid.PICC_IsNewCardPresent()) { // on a dédecté un tag
        if (rfid.PICC_ReadCardSerial()) { // on a lu avec succès son contenu
            for (u8 i = 0; i < 4; i ++) { currentRFID[i] = rfid.uid.uidByte[i]; }
            for (u8 i = 0; i < 4; i ++) { Print(currentRFID[i]); Print(" "); }
            Println();
        }
    }
    return false;
}

void send_battery_info() {
    #if MQTT_active
    // Construction du message à envoyer
    float current_mA = 0;
    float voltage_V = 0;
    float shunt_voltage_mV = 0;
    #if INA2019
    current_mA = ina219.getCurrent_mA();
    voltage_V = ina219.getBusVoltage_V();
    shunt_voltage_mV = ina219.getShuntVoltage_mV();
    #endif
    // Println(current_mA);
    // Println(voltage_V);
    // Println(shunt_voltage_mV);
    Println();
    send_MQTT(current_mA, "ESME/COMPTEUR_AMP");
    send_MQTT(voltage_V, "ESME/COMPTEUR_VOL");
    send_MQTT(shunt_voltage_mV, "ESME/COMPTEUR_SmV");
    #endif
}

void init_leds() {

    LED_init();//library init
    LED_clear();

    /* ANIMATION LED DÉBUT */

    for (int i = 0; i < NUM_LEDS; i++){
        LED(i, 0, 150, 0);
        delay(300);
    }

    rainbowCycle(5);

    LED_clear();

    delay(200);

    for (int i = 0; i < NUM_LEDS; i++){
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
        pixels.show();
    }
    delay(200);
    LED_clear();

    // play_sound(play);

    /* ANIMATION LED FIN */
}

bool internet_accept(byte code_rfid[4]) {
    #if INTERNET
    LED_clear();
    LED(0, 150, 0, 150);// LED 0 en violet
    String id = "";
    for (int i = 0; i < 4; i ++) {
        id += code_rfid[i];
        id += ".";
    }
    String answer = HTTP_connect_send_and_Print((String)"id=" + id);
    Print("[RESPONSE] ");
    Println(answer);
    if (answer != "done") { 
        LED_clear();                        
        LED(0, 150, 0, 0);                  
        delay(2000);                        
        LED_clear()    
        return false;
    }
    return true;
    #else
    return true; // on considère qu'internet accepte la demande
    #endif
}

int RFID() {
    int address = -1;
    #if RFID_ACTIVE
    DEBUG_LED(400, 150, 150, 150);
    DEBUG_LED(100, 0, 0, 0);
    // rainbowCycle(5); // Arc-en-cieel

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! rfid.PICC_IsNewCardPresent()) {return -2;}
    DEBUG_LED(250, 0, 150, 150);

    // Verify if the NUID has been readed
    if ( ! rfid.PICC_ReadCardSerial()) {
        Print(rfid.PICC_GetType(rfid.uid.sak));
        return -3;
    }
    DEBUG_LED(250, 0, 0, 150);

    Print("PICC type: ");
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Println(rfid.PICC_GetTypeName(piccType));

    // Check is the PICC of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Println(F("Your tag is not of type MIFARE Classic."));
        return -4;
    }
    // on allume la LED 0 en bleu, indique que la carte a été détecté et accepté
    LED_clear();
    LED(0, 0, 0, 150);// LED 0 en bleu

    for (int indice = 0; indice < NOMBRE_CASIER; indice++) {
        if (rfid.uid.uidByte[0] != nuidPICC[indice][0] ||
            rfid.uid.uidByte[1] != nuidPICC[indice][1] ||
            rfid.uid.uidByte[2] != nuidPICC[indice][2] ||
            rfid.uid.uidByte[3] != nuidPICC[indice][3]) {

            Println("A new card has been detected.");
            Println(F("The NUID tag is:"));
            Print(F("In hex: "));
            PrintHex(rfid.uid.uidByte, rfid.uid.size);
            Println();
            Print(F("In dec: "));
            PrintDec(rfid.uid.uidByte, rfid.uid.size);
            Println();

            if (casier_disponible[indice]) {// si le casier n'est pas pris
                if (!internet_accept(rfid.uid.uidByte))
                    return -5;

                LED(indice+1, 0, 150, 0);// LED &indice+1 en vert
                MUSIC(succes_song);
                address = 16 * indice;
                Ouvrir_casier(indice, rfid.uid.uidByte[i]);
                break;
            } else { // si le casiers est pris
                LED(indice+1, 150, 0, 0);// LED &indice+1 en rouge
                #if DEBUG // permet d'afficher l'identifiant de la carte que le casier contient
                Print("code RFID:");
                for (byte i = 0; i < 4; i++) {
                    Print(nuidPICC[indice][i]);
                    Print(".");
                }
                Print(" | ");
                Println(casier_disponible[indice]);
                #endif
                MUSIC(failure_song);
            }
        } else {// on lit la même carte que celle du casier &indice

            Println(F("Card read previously."));
            // Retrait de son téléphone
            if (!casier_disponible[indice]) {// check que le casier est bien pris // sûrement inutile
                LED(indice+1, 0, 150,0);
                MUSIC(succes_song);
                Ouvrir_casier(indice, 0);
                // address = -10;
                break;
            } else {// devrais jamais arriver // on lit la carte du casier et le casier n'est pas pris
                if (!internet_accept(rfid.uid.uidByte))
                    return -5;
                LED(indice+1, 0, 150,0);
                MUSIC(succes_song);
                Ouvrir_casier(indice, rfid.uid.uidByte[i]);
                address = 16 * indice;
                break;
            }
        }
    }
    LED_clear();
    rfid.PICC_HaltA(); // Halt PICC
    rfid.PCD_StopCrypto1(); // Stop encryption on PCD
    #endif
    DEBUG_LED(250, 150, 150, 0);
    return address;
}

void init_EEPROM() {
    for (int indice = 0; indice < NOMBRE_CASIER; indice ++) {
        for (byte i = 0; i < 4; i++) {
            nuidPICC[indice][i] = EEPROM_READ(4*i + 16 * indice, int);
        }
        Println(nuidPICC[indice][0] + nuidPICC[indice][1] + nuidPICC[indice][2] + nuidPICC[indice][3]);
        if (nuidPICC[indice][0] + nuidPICC[indice][1] + nuidPICC[indice][2] + nuidPICC[indice][3] != 0) {
            Print("well init code RFID:");
            for (byte i = 0; i < 4; i++) {
                Print(nuidPICC[indice][i]);
                Print(".");
            }
            Print(" | ");
            Println(casier_disponible[indice]);
            casier_disponible[indice] != casier_disponible[indice];
        }
    }
}

void init_custom_EEPROM() {
    #if RESET_EEPROM
    for (int indice = 0; indice < 4; indice ++) {
        for (byte i = 0; i < 4; i++) {
            EEPROM_write(4*i + 16 * indice, 0);
        }
    }
    #endif
}

void setup() {
    delay(500);
    setup_pins();           // assignation des pins de l'ESP

    #if PRINT
    Serial.begin(115200);   // Configuration de la communication série à 115200 Mbps
    #endif
    Println();
    SPI.begin();            // Initialistaion du SPI (dépendances)
    rfid.PCD_Init();        // Configuration du RFID

    #if INA2019
    // initialisation de la communication avec l'INA219
    if (! ina219.begin()) {
        Println("Erreur pour trouver le INA219");
        while (1) { delay(10); }
    }
    #endif

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    init_leds();                            // initialisation du bandeau leds
    setup_wifi();
    setup_MQTT_wifi();                      // Connexion au WiFi MQTT
    // setup_HTML_wifi();                      // Connexion au WiFi HTML
    init_custom_EEPROM();                   // permet d'enregistrer une carte prédéfine dans l'ESP
    init_EEPROM();

    // wifi_connected = connect_serveur_HTML(client_someone);
}

void loop() {
    #if MQTT_active
    if (!client.connected()) { reconnect(); } // Si perte de connexion MQTT, on essaye une reconnexion!
    client.loop(); // Appel de fonction pour redonner la main au process de communication MQTT
    #endif

    #if ALPHA
    byte lol[4] = {0, 0, 0, 0};
    HTML_send((String) "id=0.0.0.0", lol);
    #endif
    int user_RFID_address = RFID();
    Print("address:");
    Println(user_RFID_address);
    // unsigned long now = millis();
    // if (now - lastMsg > 2000) {
    //     lastMsg = now;
    //     send_battery_info();
    //     Println(HTTP_connect_send_and_Print("id=hello"));
    //     Println(HTTP_connect_send_and_Print("id=WDXFGHKML"));
    //     Println(HTTP_connect_send_and_Print("id=WDXFGHKM"));
    // }
    delay(100);
}

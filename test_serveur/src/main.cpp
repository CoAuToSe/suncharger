
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#else
#include <ESP31BWiFi.h>
#endif
#include "ESPAsyncTCP.h"
#include "SyncClient.h"

// #include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>

// extern "C" {
// #include <osapi.h>
// #include <os_type.h>
// }

#include "config.h"

// static os_timer_t intervalTimer;
// #define SSID "raspi-webgui"                     // indiquer le SSID de votre réseau
// #define PASSWORD "ChangeMe"                     // indiquer le mdp de votre réseau
// #define IP_RASPBERRY "10.3.141.1"               // adresse du serveur MQTT auquel vous etes connecté
// // #define PORT_RASPBERRY 5678
// #define N8N_PORT 5678
// #define MQTT_PORT 1883
// static void replyToServer(void* arg) {
// 	AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

// 	// send reply
// 	if (client->space() > 256 && client->canSend()) {
// 		char message[256];
// 		// sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
// 		String some = String("GET /webhook/innov?") + "id=0.0.0.0" + " HTTP/1.1\r\n" +
//                         "Host: " + IP_RASPBERRY + "\r\n" +
//                         "Connection: close\r\n" +
//                         "\r\n";
// 		sprintf(message, "%s", some);
		 
// 		client->add(message, strlen(message));
// 		client->send();
// 	}
// }

// /* event callbacks */
// static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
// 	Serial.printf("\n data received from %s \n", client->remoteIP().toString().c_str());
// 	Serial.write((uint8_t*)data, len);

// 	os_timer_arm(&intervalTimer, 2000, true); // schedule for reply to server at next 2s
// }

// void onConnect(void* arg, AsyncClient* client) {
// 	Serial.printf("\n client has been connected to %s on port %d \n", SERVER_HOST_NAME, TCP_PORT);
// 	replyToServer(client);
// }


// void setup() {
// 	Serial.begin(115200);
// 	delay(20);

// 	// connects to access point
// 	WiFi.mode(WIFI_STA);
// 	WiFi.begin(SSID, PASSWORD);
// 	while (WiFi.status() != WL_CONNECTED) {
// 		Serial.print('.');
// 		delay(500);
// 	}

// 	AsyncClient* client = new AsyncClient;
// 	client->onData(&handleData, client);
// 	client->onConnect(&onConnect, client);
// 	client->connect(SERVER_HOST_NAME, TCP_PORT);

// 	os_timer_disarm(&intervalTimer);
// 	os_timer_setfn(&intervalTimer, &replyToServer, client);
// }
#define SSID "raspi-webgui"                     // indiquer le SSID de votre réseau
#define PASSWORD "ChangeMe"                     // indiquer le mdp de votre réseau
#define IP_RASPBERRY "10.3.141.1"               // adresse du serveur MQTT auquel vous etes connecté
// #define PORT_RASPBERRY 5678
#define N8N_PORT 5678
#define MQTT_PORT 1883
void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.printf("WiFi Connected!\n");
  Serial.println(WiFi.localIP());
#ifdef ESP8266
  ArduinoOTA.begin();
#endif
  
  SyncClient client;
  if(!client.connect(SERVER_HOST_NAME, TCP_PORT)){
    Serial.println("Connect Failed");
    return;
  }
//   client.setTimeout(2);
  const char* message = "GET /webhook/innov?id=hello2 HTTP/1.1\r\nHost: 10.3.141.1\r\nConnection: close\r\n\r\n";
  if(client.printf( message) > 0){
    while(client.connected() && client.available() == 0){
      delay(1);
    }
    while(client.available()){
      Serial.write(client.read());
    }
    if(client.connected()){
      client.stop();
    }
  } else {
    client.stop();
    Serial.println("Send Failed");
    while(client.connected()) delay(0);
  }
}

void loop() {
#ifdef ESP8266
  ArduinoOTA.handle();
#endif
  
    Serial.println("looping");
	delay(100);
}
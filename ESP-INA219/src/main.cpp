#include "Wire.h"
#include "Adafruit_INA219.h"
Adafruit_INA219 ina219;

void setup() {
    Serial.begin(9600);
    if (! ina219.begin()) {
        Serial.println("Erreur pour trouver le INA219");
        while (1) { delay(10); }
    }
    Serial.print("Courant"); 
    Serial.print("\t");
}

void loop() {
    float current_mA = 0;
    current_mA = ina219.getCurrent_mA();
    Serial.print(current_mA); 
    Serial.print("\t");
    delay(1000);
}
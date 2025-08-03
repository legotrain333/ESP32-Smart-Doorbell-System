#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting MAC address test...");
  WiFi.mode(WIFI_STA);
  delay(1000);
  String mac = WiFi.macAddress();
  Serial.println("MAC Address: " + mac);
}

void loop() {
  delay(5000);
}
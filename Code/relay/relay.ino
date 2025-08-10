#include <ESP8266WiFi.h>
#include <espnow.h>

#define RELAY_PIN 0  // GPIO0 controls the relay on this board

void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len > 0) {
    if (data[0] == 1) {
      digitalWrite(RELAY_PIN, LOW); // Relay ON (active LOW)
      Serial.println("Relay ON");
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
      Serial.println("Relay OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start with relay OFF

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Relay receiver ready.");
}

void loop() {
  // Nothing needed here
}
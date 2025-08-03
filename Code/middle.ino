#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

// Pin definitions
#define DND_BUTTON_PIN 19   // DND button GPIO
#define RESET_BUTTON_PIN 18 // RESET button GPIO
#define DND_LED_PIN 2       // Onboard status LED (optional)

// MAC addresses
uint8_t senderMac[] = {0x9C, 0x9E, 0x6E, 0x43, 0x8A, 0xF0};
uint8_t relayMac[] = {0xF0, 0x08, 0xD1, 0x05, 0xF9, 0x60};

// Message Types
typedef enum {
  DOORBELL_REQUEST,
  DND_RESPONSE,
  LIGHT_ON_CONFIRM,
  DOOR_OPEN_SIGNAL,
  RESET_SIGNAL
} MessageType;

typedef struct {
  MessageType type;
} DoorbellMessage;

// States and debounce vars
bool dndMode = false;     // DND mode ON/OFF
unsigned long lastDndPress = 0;
unsigned long lastResetPress = 0;
const unsigned long debounceDelay = 300;
int previousDndState = HIGH;
int previousResetState = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(DND_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DND_LED_PIN, OUTPUT);

  EEPROM.begin(1);
  dndMode = EEPROM.read(0);
  digitalWrite(DND_LED_PIN, dndMode ? HIGH : LOW);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, senderMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer (sender)");
    while (1);
  }
  memcpy(peerInfo.peer_addr, relayMac, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer (relay)");
    while (1);
  }

  Serial.println("Middle device ready.");
  Serial.print("DND mode: ");
  Serial.println(dndMode ? "ON" : "OFF");
}

void loop() {
  int dndBtn = digitalRead(DND_BUTTON_PIN);
  int resetBtn = digitalRead(RESET_BUTTON_PIN);

  if (previousDndState == HIGH && dndBtn == LOW && millis() - lastDndPress > debounceDelay) {
    lastDndPress = millis();
    dndMode = !dndMode;
    EEPROM.write(0, dndMode);
    EEPROM.commit();
    digitalWrite(DND_LED_PIN, dndMode ? HIGH : LOW);
    Serial.print("DND mode toggled to: ");
    Serial.println(dndMode ? "ON" : "OFF");
  }

  if (previousResetState == HIGH && resetBtn == LOW && millis() - lastResetPress > debounceDelay) {
    lastResetPress = millis();

    // 1. Turn OFF relay
    uint8_t offSignal = 0;
    esp_now_send(relayMac, &offSignal, 1);
    Serial.println("RESET button pressed — Sent OFF signal to relay.");

    // 2. Tell sender to turn off its light (send RESET_SIGNAL)
    DoorbellMessage msg = {RESET_SIGNAL};
    esp_now_send(senderMac, (uint8_t *)&msg, sizeof(msg));
    Serial.println("RESET button pressed — Sent RESET_SIGNAL to sender.");

    // 3. Reset any internal state (add your own resets here if needed)
    // Example: lightOn = false;
  }

  previousDndState = dndBtn;
  previousResetState = resetBtn;
  delay(100);
}

// Whenever the code would have turned the LED ON or OFF, send ON/OFF to relay
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  DoorbellMessage msg;
  memcpy(&msg, incomingData, sizeof(msg));

  switch (msg.type) {
    case DOORBELL_REQUEST: {
      Serial.println("Received: DOORBELL_REQUEST");
      if (dndMode) {
        DoorbellMessage resp = {DND_RESPONSE};
        esp_now_send(senderMac, (uint8_t *)&resp, sizeof(resp));
        Serial.println("DND ON — sent DND_RESPONSE to sender.");
      } else {
        // LED would have turned ON here, so send ON to relay
        uint8_t onSignal = 1;
        esp_now_send(relayMac, &onSignal, 1);
        Serial.println("DND OFF — Sent ON signal to relay, LIGHT_ON_CONFIRM sent.");
        DoorbellMessage resp = {LIGHT_ON_CONFIRM};
        esp_now_send(senderMac, (uint8_t *)&resp, sizeof(resp));
      }
      break;
    }
    case DOOR_OPEN_SIGNAL: {
      Serial.println("Received: DOOR_OPEN_SIGNAL");
      // LED would have turned OFF here, so send OFF to relay
      uint8_t offSignal = 0;
      esp_now_send(relayMac, &offSignal, 1);
      Serial.println("Door opened — Sent OFF signal to relay.");
      break;
    }
    default:
      Serial.println("Unknown message received.");
      break;
  }
}

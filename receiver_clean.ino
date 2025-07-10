#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

// Pin definitions
#define DND_BUTTON_PIN 19   // DND button GPIO
#define RESET_BUTTON_PIN 18 // RESET button GPIO
#define DND_LED_PIN 2       // LED indicates DND
#define IR_SEND_PIN 15      // Instead of IR, test LED here

// Sender MAC address — replace with your sender MAC
uint8_t senderMac[] = {Mac Here};

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
bool irLightOn = false;   // Simulated IR light ON/OFF

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
  pinMode(IR_SEND_PIN, OUTPUT); // Now acts as a normal LED pin

  EEPROM.begin(1);
  dndMode = EEPROM.read(0);
  digitalWrite(DND_LED_PIN, dndMode ? HIGH : LOW);
  digitalWrite(IR_SEND_PIN, LOW); // Ensure test LED is off

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
    Serial.println("Failed to add ESP-NOW peer");
    while (1);
  }

  Serial.println("Receiver ready.");
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

    if (irLightOn) {
      digitalWrite(IR_SEND_PIN, LOW); // Turn OFF test LED
      irLightOn = false;

      DoorbellMessage msg = {RESET_SIGNAL};
      esp_now_send(senderMac, (uint8_t *)&msg, sizeof(msg));

      Serial.println("RESET: Test LED turned OFF, RESET_SIGNAL sent.");
    } else {
      Serial.println("RESET button pressed, but test LED is already OFF. Nothing sent.");
    }
  }

  previousDndState = dndBtn;
  previousResetState = resetBtn;

  delay(100);
}

void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  DoorbellMessage msg;
  memcpy(&msg, incomingData, sizeof(msg));

  switch (msg.type) {
    case DOORBELL_REQUEST:
      Serial.println("Received: DOORBELL_REQUEST");
      if (dndMode) {
        DoorbellMessage resp = {DND_RESPONSE};
        esp_now_send(senderMac, (uint8_t *)&resp, sizeof(resp));
        Serial.println("DND ON — sent DND_RESPONSE to sender.");
      } else {
        digitalWrite(IR_SEND_PIN, HIGH); // Turn ON test LED
        irLightOn = true;

        DoorbellMessage resp = {LIGHT_ON_CONFIRM};
        esp_now_send(senderMac, (uint8_t *)&resp, sizeof(resp));
        Serial.println("DND OFF — Test LED ON, LIGHT_ON_CONFIRM sent.");
      }
      break;

    case DOOR_OPEN_SIGNAL:
      Serial.println("Received: DOOR_OPEN_SIGNAL");
      if (irLightOn) {
        digitalWrite(IR_SEND_PIN, LOW); // Turn OFF test LED
        irLightOn = false;
        Serial.println("Door opened — Test LED OFF.");
      } else {
        Serial.println("Door opened, but test LED was already OFF.");
      }
      break;

    default:
      Serial.println("Unknown message received.");
      break;
  }
}

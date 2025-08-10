#include <esp_now.h>
#include <WiFi.h>

// Pin definitions
const int BUTTON_PIN = 0;      // Self-reset momentary switch (connects to GND when pressed)
const int RGB_RED_PIN = 5;     // RGB LED Red pin (in button)
const int RGB_GREEN_PIN = 6;   // RGB LED Green pin (in button)
const int RGB_BLUE_PIN = 7;    // RGB LED Blue pin (in button)
const int TRIG_PIN = 19;       // Ultrasonic sensor TRIG pin
const int ECHO_PIN = 20;       // Ultrasonic sensor ECHO pin

// Receiver MAC address (replace with your actual receiver MAC)
uint8_t receiverMac[] = {0x9C, 0x9E, 0x6E, 0x43, 0x6E, 0xFC};

// Message structure for ESP-NOW communication
// These enums and structs must match on both sender and receiver
typedef enum {
  DOORBELL_REQUEST,   // Sent when button is pressed
  DND_RESPONSE,       // Sent by receiver if DND is ON
  LIGHT_ON_CONFIRM,   // Sent by receiver if DND is OFF and IR light is turned on
  DOOR_OPEN_SIGNAL,   // Sent by sender when door is opened (to turn off IR light)
  RESET_SIGNAL        // Sent by receiver when reset button is pressed
} MessageType;

typedef struct {
  MessageType type;
} DoorbellMessage;

// State variables
bool ledOn = false;                // True if IR light is ON (RGB LED shows status)
unsigned long lastButtonPress = 0; // For button debounce
const unsigned long debounceDelay = 200; // Debounce time in ms
bool waitingForResponse = false;   // True if waiting for receiver response
unsigned long bootTime = 0;        // For ignoring button at startup
int previousButtonState = HIGH;    // Used for edge detection (button released)

// Function prototype for ESP-NOW receive callback
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len);

// RGB LED control functions
void setRGBColor(int red, int green, int blue) {
  digitalWrite(RGB_RED_PIN, red > 0 ? LOW : HIGH);    // Red pin - LOW = ON, HIGH = OFF
  digitalWrite(RGB_GREEN_PIN, green > 0 ? LOW : HIGH); // Green pin - LOW = ON, HIGH = OFF
  digitalWrite(RGB_BLUE_PIN, blue > 0 ? LOW : HIGH);   // Blue pin - LOW = ON, HIGH = OFF
}

void turnOffRGB() {
  setRGBColor(0, 0, 0);
}

void setRGBGreen() {
  setRGBColor(0, 255, 0);  // Green for IR light ON
}

void setRGBRed() {
  setRGBColor(255, 0, 0);  // Red for DND/error
}

void setRGBBlue() {
  setRGBColor(0, 0, 255);  // Blue for testing
}

// Function to wake up and reinitialize
void wakeUpAndInitialize() {
  Serial.println("Waking up from deep sleep...");
  
  // Reinitialize WiFi and ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    return;
  }
  
  Serial.println("Sender reinitialized and ready.");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting sender...");
  delay(100); // Let the pin stabilize

  // Initialize hardware pins
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button to GND, uses internal pull-up
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Test RGB LED sequence to verify it's working
  Serial.println("Testing RGB LED...");
  setRGBRed();
  delay(500);
  setRGBGreen();
  delay(500);
  setRGBBlue();
  delay(500);
  turnOffRGB();
  Serial.println("RGB LED test complete");
  
  // Check if we're waking up from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println("Woke up from button press");
    wakeUpAndInitialize();
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up from timer");
    wakeUpAndInitialize();
  } else {
    // First boot - initialize normally
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }
  Serial.println("ESP-NOW initialized.");
  esp_now_register_recv_cb(onDataRecv);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    Serial.println("ESP-NOW peer added.");
  }

  bootTime = millis();  // Track boot time for startup ignore
}

void loop() {
  // Ignore button presses for the first 1 second after boot
  if (millis() - bootTime < 1000) {
    previousButtonState = digitalRead(BUTTON_PIN);  // Keep updating to avoid false edge detection
    return;
  }

  int currentButtonState = digitalRead(BUTTON_PIN);

  // Debug button state
  static unsigned long lastButtonDebug = 0;
  if (millis() - lastButtonDebug > 5000) { // Print every 5 seconds
    Serial.print("Button state: ");
    Serial.print(currentButtonState);
    Serial.print(", Previous: ");
    Serial.print(previousButtonState);
    Serial.print(", LED: ");
    Serial.print(ledOn ? "ON" : "OFF");
    Serial.print(", Waiting: ");
    Serial.println(waitingForResponse ? "YES" : "NO");
    lastButtonDebug = millis();
  }

    // Detect falling edge (HIGH to LOW) with debounce
  // Only trigger when button is pressed after being released
  if (previousButtonState == HIGH && currentButtonState == LOW &&
      millis() - lastButtonPress > debounceDelay && !waitingForResponse) {
    lastButtonPress = millis();
    Serial.println("=== BUTTON PRESSED ===");
    
    // Check if door is already open before sending request
    if (doorOpened()) {
      Serial.println("Door is already open. Not sending DOORBELL_REQUEST.");
      return;
    }
    
    Serial.println("Button pressed. Sending DOORBELL_REQUEST.");
    sendDoorbellRequest();
    waitingForResponse = true;
  }

  previousButtonState = currentButtonState;

    // Only check door status when LED is GREEN (waiting for door to open)
  // Don't check during red blink or when LED is off
  if (ledOn && doorOpened()) {
    Serial.println("Door opened detected by ultrasonic. Sending DOOR_OPEN_SIGNAL.");
    sendDoorOpenSignal();
    ledOn = false;
    turnOffRGB();
    Serial.println("RGB LED turned OFF after door opened.");
  }
}

// Send DOORBELL_REQUEST to receiver
void sendDoorbellRequest() {
  DoorbellMessage msg = {DOORBELL_REQUEST};
  esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&msg, sizeof(msg));
  if (result == ESP_OK) {
    Serial.println("DOORBELL_REQUEST sent successfully.");
  } else {
    Serial.print("Error sending DOORBELL_REQUEST: ");
    Serial.println(result);
  }
}

// Send DOOR_OPEN_SIGNAL to receiver
void sendDoorOpenSignal() {
  DoorbellMessage msg = {DOOR_OPEN_SIGNAL};
  esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&msg, sizeof(msg));
  if (result == ESP_OK) {
    Serial.println("DOOR_OPEN_SIGNAL sent successfully.");
  } else {
    Serial.print("Error sending DOOR_OPEN_SIGNAL: ");
    Serial.println(result);
  }
}

// ESP-NOW receive callback
// Handles responses from receiver (DND, light on, reset)
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Serial.println("ESP-NOW message received.");
  DoorbellMessage msg;
  memcpy(&msg, incomingData, sizeof(msg));
  switch (msg.type) {
    case LIGHT_ON_CONFIRM:
      ledOn = true;
      setRGBGreen();  // Green for IR light ON
      Serial.println("LIGHT_ON_CONFIRM received. RGB LED turned GREEN.");
      waitingForResponse = false;
      break;
    case DND_RESPONSE:
      Serial.println("DND_RESPONSE received. Blinking RED LED 5 times.");
      blinkRedLED(5);
      waitingForResponse = false;
      break;
    case RESET_SIGNAL:
      ledOn = false;
      turnOffRGB();
      Serial.println("RESET_SIGNAL received. RGB LED turned OFF.");
      break;
    default:
      Serial.println("Unknown message type received.");
      break;
  }
}

// Blink RED LED n times (used for DND denial feedback)
void blinkRedLED(int n) {
  for (int i = 0; i < n; i++) {
    setRGBRed();
    delay(200);
    turnOffRGB();
    delay(200);
  }
  Serial.print("RED LED blinked ");
  Serial.print(n);
  Serial.println(" times.");
}

// Detect if the door is open using the ultrasonic sensor
// Returns true if the measured distance is greater than 20cm (door open)
bool doorOpened() {
  long duration, distance;
  
  // Send trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Measure echo duration
  duration = pulseIn(ECHO_PIN, HIGH, 20000);
  
  // Calculate distance
  distance = duration * 0.034 / 2;
  
  // Debug information
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print(" μs, Distance: ");
  Serial.print(distance);
  Serial.print(" cm");
  
  // Check if distance is valid
  if (distance > 0 && distance < 400) {
    Serial.print(" (Valid reading)");
    if (distance > 20 && distance < 300) {
      Serial.println(" -> DOOR OPENED!");
      return true;
    } else {
      Serial.println(" -> Door closed");
      return false;
    }
  } else {
    Serial.println(" (Invalid reading - no echo or out of range)");
    return false;
  }
}
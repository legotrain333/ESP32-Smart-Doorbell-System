// Simple RGB LED Test for ESP32-C6
// Tests the RGB LED on pins 2, 3, 4

// Pin definitions
const int RGB_RED_PIN = 5;     // RGB LED Red pin
const int RGB_GREEN_PIN = 6;   // RGB LED Green pin  
const int RGB_BLUE_PIN = 7;    // RGB LED Blue pin

void setup() {
  Serial.begin(115200);
  Serial.println("RGB LED Test Starting...");
  
  // Initialize RGB LED pins
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  
  // Start with all LEDs off
  turnOffRGB();
  
  Serial.println("RGB LED pins initialized");
  Serial.println("Starting color cycle test...");
}

void loop() {
  // Test each color individually
  Serial.println("RED");
  setRGBRed();
  delay(1000);
  
  Serial.println("GREEN");
  setRGBGreen();
  delay(1000);
  
  Serial.println("BLUE");
  setRGBBlue();
  delay(1000);
  
  // Test some color combinations
  Serial.println("YELLOW (Red + Green)");
  setRGBColor(255, 255, 0);
  delay(1000);
  
  Serial.println("CYAN (Green + Blue)");
  setRGBColor(0, 255, 255);
  delay(1000);
  
  Serial.println("MAGENTA (Red + Blue)");
  setRGBColor(255, 0, 255);
  delay(1000);
  
  Serial.println("WHITE (All colors)");
  setRGBColor(255, 255, 255);
  delay(1000);
  
  Serial.println("OFF");
  turnOffRGB();
  delay(2000);
  
  Serial.println("--- Cycle complete, starting again ---");
}

// RGB LED control functions
void setRGBColor(int red, int green, int blue) {
  digitalWrite(RGB_RED_PIN, red > 0 ? LOW : HIGH);   // LOW = ON, HIGH = OFF
  digitalWrite(RGB_GREEN_PIN, green > 0 ? LOW : HIGH); // LOW = ON, HIGH = OFF
  digitalWrite(RGB_BLUE_PIN, blue > 0 ? LOW : HIGH);   // LOW = ON, HIGH = OFF
}

void turnOffRGB() {
  setRGBColor(0, 0, 0);
}

void setRGBRed() {
  setRGBColor(255, 0, 0);
}

void setRGBGreen() {
  setRGBColor(0, 255, 0);
}

void setRGBBlue() {
  setRGBColor(0, 0, 255);
}

#include <WiFi.h>

void setup() {
  // Start serial communication
  Serial.begin(115200);
  
  // Wait for serial to be ready
  delay(2000);
  
  Serial.println("Starting MAC address test...");
  Serial.println("If you see this, serial is working!");
  
  // Try to get MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);
  
  String mac = WiFi.macAddress();
  
  Serial.println("MAC Address: " + mac);
  Serial.println("MAC Length: " + String(mac.length()));
  
  // Also try printing each character
  Serial.println("MAC characters:");
  for (int i = 0; i < mac.length(); i++) {
    Serial.print("Char ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(mac.charAt(i));
  }
  
  Serial.println("Test complete!");
}

void loop() {
  // Keep printing to show the board is alive
  Serial.println("Board is running...");
  delay(5000);
} 
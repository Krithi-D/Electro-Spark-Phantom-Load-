#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define CURRENT_SENSOR_PIN 34
#define RELAY_PIN 15
#define BUZZER_PIN 4

// ThingSpeak details
const char* ssid = "Wokwi-GUEST";   // Wokwi default WiFi
const char* password = "";          // No password
String apiKey = "45UMVD1UTL0CWA98";  // Replace with your key
const char* server = "http://api.thingspeak.com/update";

const float phantomThreshold = 0.3; // 0.3A for simulation
const float activeThreshold = 0.5;  // 0.5A for reactivation
unsigned long lowCurrentStart = 0;
bool relayOn = true;
float phantomLoadPower = 5.0; // 5W assumed standby power
unsigned long totalCutoffTime = 0;
unsigned long lastCutoffStart = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  // WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  relayOn = true;

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");
  delay(1000);
  lcd.clear();
}

void loop() {
  int raw = analogRead(CURRENT_SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float current = voltage / 3.3 * 1.0; // Simulated: 0 to 1.0A

  Serial.print("Current: ");
  Serial.print(current, 2);
  Serial.println(" A");

  lcd.setCursor(0, 0);
  lcd.print("Curr: ");
  lcd.print(current, 2);
  lcd.print("A  ");

  // Send data to ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(current, 2);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("ThingSpeak Update Success");
    } else {
      Serial.println("ThingSpeak Update Failed");
    }
    http.end();
  }

  // Phantom load detection
  if (current < phantomThreshold) {
    if (lowCurrentStart == 0) {
      lowCurrentStart = millis();
      lastCutoffStart = millis();
    }

    int countdown = 10 - (millis() - lowCurrentStart) / 1000;
    if (countdown > 0) {
      lcd.setCursor(0, 1);
      lcd.print("Cut in ");
      lcd.print(countdown);
      lcd.print("s    ");
    }

    if ((millis() - lowCurrentStart) >= 10000 && relayOn) {
      Serial.println("Phantom load detected. Cutting off.");
      digitalWrite(RELAY_PIN, LOW);
      relayOn = false;

      digitalWrite(BUZZER_PIN, HIGH);
      delay(5000);
      digitalWrite(BUZZER_PIN, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phantom Load!");
      lcd.setCursor(0, 1);
      lcd.print("Power OFF");
      delay(1500);
    }
  } 
  else if (current > activeThreshold) { // Hysteresis margin
    if (!relayOn) {
      totalCutoffTime += (millis() - lastCutoffStart);
    }

    lowCurrentStart = 0;
    if (!relayOn) {
      Serial.println("Device reactivated. Power ON.");
      digitalWrite(RELAY_PIN, HIGH);
      relayOn = true;
      lcd.clear();
    }
    lcd.setCursor(0, 1);
    lcd.print("Device ON       ");
  }

  delay(500);
}

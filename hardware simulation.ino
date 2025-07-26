#include <WiFi.h>
#include <HTTPClient.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// WiFi credentials
#define WIFI_SSID     " "
#define WIFI_PASSWORD " "

// SinricPro credentials
#define APP_KEY       "d235f169-966c-45f2-aec1-77627ffdd48a"
#define APP_SECRET    "bf9ee42f-520c-4535-bbcc-26ac2c5fb8ad-3dcf3ede-4adb-474a-ad8e-f8cee692b533"
#define DEVICE_ID     "6880df0d7104f15ae52dbac7"

// Pins
#define LED_PIN 2
#define SWITCH_PIN 18  // Flip-flop switch

bool ledState = false;

// Telegram Bot Token & Chat ID
String botToken = "8473726465:AAFIeyU--hlD0t5eV0cBJ15UJpDx7vuCnSc";  // From BotFather
String chatID = "7143095919";               // From userinfobot

// Function to send Telegram message
void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url);
    int httpResponseCode = http.GET();
    Serial.print("Telegram Response: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

// Callback for Google Home commands
bool onPowerState(const String &deviceId, bool &state) {
  if (deviceId == DEVICE_ID) {
    ledState = state;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    String msg = ledState ? "LED turned ON (Google Home)" : "LED turned OFF (Google Home)";
    Serial.println(msg);
    sendTelegramMessage(msg);
  }
  return true;
}

// WiFi setup
void setupWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

// Setup
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);  // Switch is active LOW

  setupWiFi();

  SinricProSwitch &mySwitch = SinricPro[DEVICE_ID];
  mySwitch.onPowerState(onPowerState);

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

// Main loop
void loop() {
  SinricPro.handle();

  static bool lastSwitchState = HIGH;
  bool currentSwitchState = digitalRead(SWITCH_PIN);

  // Detect switch toggle
  if (currentSwitchState != lastSwitchState) {
    delay(50);  // debounce
    if (currentSwitchState == LOW) {  // switch flipped
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);

      String msg = ledState ? "LED turned ON (manual switch)" : "LED turned OFF (manual switch)";
      Serial.println(msg);

      SinricProSwitch &mySwitch = SinricPro[DEVICE_ID];
      mySwitch.sendPowerStateEvent(ledState);  // Update Google Home
      sendTelegramMessage(msg);                // Notify on Telegram
    }
    lastSwitchState = currentSwitchState;
  }
}

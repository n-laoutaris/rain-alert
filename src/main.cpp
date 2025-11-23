#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // for parsing JSON easily
#include <WiFiManager.h>

// Defining attached components
const int LED = 2;
const int prg = 0;
const int buzzer = 22;
const int rainSensor_analog = 37;
const int rainSensor_power = 13;

String message = "Run! Your clothes are getting wet!";  

// Telegram bot credentials
const char* botToken = "8575076240:AAF18Uj6Bjru1OzL6gxV1mHUnm4JGvv6hX0";
const char* chatID   = "8292217044";

// Wifi manager object initialization
WiFiManager wifiManager;

// Function declaration
void connectToNetwork();
void sendTelegramMessage(String message);
void playTone(int frequency, int durationMs);
void playAlarm();

void setup() {
  // Setup pins
  pinMode(LED, OUTPUT);
  pinMode(rainSensor_power, OUTPUT);
  pinMode(prg, INPUT_PULLUP);
  pinMode(rainSensor_analog, INPUT);
  
  pinMode(buzzer, OUTPUT);
  ledcAttachPin(buzzer, 0);
  ledcSetup(0, 2000, 8);
  playTone(0,0); // Start muted

  Serial.begin(115200);

  // Begin detection routine
  digitalWrite(rainSensor_power, HIGH); // Power the rain sensor
  delay(100); // Wait for sensor to stabilize (necessary?)
  int analogValue = analogRead(rainSensor_analog);
  Serial.print("Rain Sensor Analog Value: ");
  Serial.print(analogValue);
  
  if (analogValue < 4000) {  // Rain detected!
    while (true) {     // Stay in this loop until deactivated
      // playAlarm(); // Maybe someday
      // Connect and send message
      if (!WiFi.isConnected()) {
        connectToNetwork();
      }
      sendTelegramMessage(message);

      // Give a time window of 5 seconds to deactivate station
      unsigned long startTime = millis();
      while (millis() - startTime < 5000) {
        int state = digitalRead(prg);
        if (state == LOW) {  // Button pressed to deactivate station
          // Sleep (until power is cut manually from the user) ~30 minutes
          digitalWrite(rainSensor_power, LOW); // Power down the rain sensor
          Serial.println("Going to sleep forever.");
          esp_deep_sleep(30 * 60 * 1000000ULL);
        }  
        delay(25); // Small delay to avoid busy looping
      }
    }
  }
  // Go to sleep for one minute
  Serial.println("Going to sleep.");
  digitalWrite(rainSensor_power, LOW); // Power down the rain sensor
  esp_deep_sleep(10 * 1000000ULL);
}

void loop() {
  

}

void connectToNetwork() {
  // Connects to the Network via WiFi 
  Serial.print("Connecting to WiFi ");
  // This starts the portal if no WiFi credentials are stored
  wifiManager.autoConnect("Vroxoulis_AP", "tarouxamou");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("..connected! IP Address: ");
  Serial.println(WiFi.localIP());
}

void sendTelegramMessage(String message) {

  String url = "https://api.telegram.org/bot";
  url += botToken;
  url += "/sendMessage?chat_id=";
  url += chatID;
  url += "&text=";
  url += message;

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("Notification sent!");
    Serial.println(http.getString());
  } else {
    Serial.print("Failed to send message. Error: ");
    Serial.println(httpCode);
  }

  http.end();

}

void playTone(int frequency, int durationMs) {
  if (frequency == 0) {
    ledcWriteTone(0, 0);  // silence
  } else {
    ledcWriteTone(0, frequency);
  }
  delay(durationMs);
  ledcWriteTone(0, 0); // stop tone after duration
}

void playAlarm() {
  const int highTone = 1000;  // Hz (you can change)
  const int lowTone  = 600;   // Hz
  const int toneLength = 200; // ms per tone
  const int repetitions = 2;  // how many cycles

  for (int i = 0; i < repetitions; i++) {
    playTone(highTone, toneLength);
    delay(80);  // pause for effect
    playTone(lowTone, toneLength);
    delay(120); // slight pause before next cycle
  }

  // Silence the buzzer at the end
  ledcWriteTone(0, 0);
}
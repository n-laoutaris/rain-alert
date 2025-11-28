#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>  
#include "secrets.h"

// Defining attached components
const int PRG = 0;
const int BUZZER = 22;
const int RAIN_SENSOR_ANALOG = 37;
const int RAIN_SENSOR_POWER = 13;

// Telegram bot 
const char* BOT_TOKEN = SECRET_BOT_TOKEN;
const char* CHAT_ID   = SECRET_CHAT_ID;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Wifi manager object
WiFiManager wifiManager;
const char* AP_PASS = SECRET_AP_PASS;

// Rain detection threshold
int RAIN_THRESHOLD = 3700; 

// A RTC variable to remember deep sleep cycles
RTC_DATA_ATTR int bootCount = 0;

// Function declaration
void connectToNetwork();
String getLastCommand();
void playTone(int frequency, int durationMs);
void playAlarm();

void setup() {
  // Setup pins
  pinMode(RAIN_SENSOR_POWER, OUTPUT);
  pinMode(RAIN_SENSOR_ANALOG, INPUT);
  pinMode(PRG, INPUT_PULLUP);
  
  pinMode(BUZZER, OUTPUT);
  ledcAttachPin(BUZZER, 0);
  ledcSetup(0, 2000, 8);
  playTone(0,0); // Start muted

  Serial.begin(115200);

  // Begin detection routine
  digitalWrite(RAIN_SENSOR_POWER, HIGH); // Power the rain sensor
  delay(100); // Wait for sensor to stabilize 
  int analogValue = analogRead(RAIN_SENSOR_ANALOG);
  digitalWrite(RAIN_SENSOR_POWER, LOW); // Save power immediately
  Serial.print("Rain Sensor Analog Value: ");  Serial.println(analogValue);
  
  if (analogValue < RAIN_THRESHOLD) {  // Rain detected!
    while (true) {     // Stay in this loop until deactivated
      // Connect and send message
      if (WiFi.status() != WL_CONNECTED) {
        connectToNetwork();        
      }

      // Check for deactivation command
      String lastCommand = getLastCommand();
      if (lastCommand == "/ok") {
        bot.sendMessage(CHAT_ID, "Understood! Going to sleep. 😴");
        esp_deep_sleep(30 * 60 * 1000000ULL); // Sleep (until power is cut manually from the user) 
      }

      bot.sendMessage(CHAT_ID, "Run! Your clothes are getting wet! ☔️ (Send /ok to deactivate.)");
      // playAlarm(); // Maybe someday

      // Give a time window of 5 seconds to deactivate station through a button press
      unsigned long startTime = millis();
      while (millis() - startTime < 5000) {
        int state = digitalRead(PRG);
        if (state == LOW) {  // Button pressed 
          bot.sendMessage(CHAT_ID, "Manual override. Going to sleep. 😴");
          esp_deep_sleep(30 * 60 * 1000000ULL); // Sleep (until power is cut manually from the user) 
        }
        delay(50); // Small delay to avoid busy looping
      }
    }
  }

  // Every 10 minutes, connect to WiFi to receive any commands and reply accordingly
  bootCount++;
  if (bootCount >= 60) {
    bootCount = 0; // Reset counter
    connectToNetwork();
    String lastCommand = getLastCommand();
    if (lastCommand == "/status") {
      bot.sendMessage(CHAT_ID, "All clear! No rain detected. ☀️");
    }
  }

  // No rain? Go to sleep for 10 seconds
  Serial.println("Going to sleep.");
  esp_deep_sleep(10 * 1000000ULL);

}

void loop() {
  // Nothing here when using deep sleep

}

void connectToNetwork() {
  Serial.print("Connecting to WiFi ");

  // If you can't connect to WiFi, open the AP portal but only for 60 seconds
  wifiManager.setConfigPortalTimeout(60); 

  // This starts the AP portal if no WiFi credentials are stored, for 60 seconds
  wifiManager.autoConnect("Vroxoulis_AP", AP_PASS);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Failed to connect and hit timeout. Going to sleep.");
    esp_deep_sleep(10 * 1000000ULL); // Sleep for 10 seconds
  }

  Serial.print("..connected! IP Address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure(); // Necessary for Telegram (skips certificate validation for speed/ease)
}

String getLastCommand() {
  // Get the messages
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  if (numNewMessages > 0) {
    // Grab the text of the very last message in the batch
    String lastText = bot.messages[numNewMessages - 1].text; 
    String lastChatId = bot.messages[numNewMessages - 1].chat_id;

    // Force a quick update to tell Telegram: "I have seen up to this message."
    bot.getUpdates(bot.last_message_received + 1);

    // Security Check
    if (lastChatId == SECRET_CHAT_ID) {
      Serial.println("Received & Flushed command: " + lastText);
      return lastText;
    }
  }
  
  return "";
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


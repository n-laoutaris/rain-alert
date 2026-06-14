#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>  
#include <secrets.h>
#include <pins_config.h>

// Define attached components Through pins_config, for multi-board support

// Telegram bot 
const char* BOT_TOKEN = SECRET_BOT_TOKEN;
const char* CHAT_ID   = SECRET_CHAT_ID;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Wifi manager object
WiFiManager wifiManager;
const char* AP_PASS = SECRET_AP_PASS;

// Function declaration
void connectToNetwork();
String getLastCommand();
void playTone(int frequency, int durationMs);
void playAlarm();
float getBatteryPercentage();

void setup() {
  // Setup pins
  pinMode(RAIN_SENSOR_POWER, OUTPUT);
  pinMode(RAIN_SENSOR_ANALOG, INPUT);
  pinMode(BATTERY, INPUT);
  pinMode(PRG, INPUT_PULLUP);
  
  // pinMode(BUZZER, OUTPUT);
  // ledcAttachPin(BUZZER, 0);
  // ledcSetup(0, 2000, 8);
  // playTone(0,0); // Start muted

  Serial.begin(115200);  
}

void loop() {
  // Begin detection routine
  digitalWrite(RAIN_SENSOR_POWER, HIGH); // Power the rain sensor
  delay(100); // Wait for sensor to stabilize 
  int analogValue = analogRead(RAIN_SENSOR_ANALOG);
  digitalWrite(RAIN_SENSOR_POWER, LOW); // Save power immediately
  Serial.print("Rain Sensor Analog Value: ");  Serial.println(analogValue);
  
  if (analogValue < RAIN_THRESHOLD) {  // Rain detected!

      // Connect and send message
      if (WiFi.status() != WL_CONNECTED) {
        connectToNetwork();        
      }
      bot.sendMessage(CHAT_ID, "Run! Your clothes are getting wet! ☔️ (Send /ok to deactivate.) Sensor value: " + String(analogValue));
  }

  float battery = getBatteryPercentage();
  Serial.print("Battery percentage:");  Serial.println(battery);

  // No rain? Go to sleep for 10 seconds
  Serial.println("Going to sleep. (2s)");
  delay(2000);


}

void connectToNetwork() {
  Serial.print("Connecting to WiFi ");
  wifiManager.setConnectTimeout(120);

  // If you can't connect to WiFi, open the AP portal 
  wifiManager.setConfigPortalTimeout(120); 

  // This starts the AP portal if no WiFi credentials are stored,
  wifiManager.autoConnect("Vroxoulis_AP", AP_PASS);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Failed to connect and hit timeout.");
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

float getBatteryPercentage() {
  uint32_t Vbatt_mv = 0;
  for (int i = 0; i < 16; i++) {
    Vbatt_mv = Vbatt_mv + analogReadMilliVolts(BATTERY); // ADC with correction   
  }
  float Vbatt_true_mv = (VOLTAGE_MULTIPLIER * Vbatt_mv / 16) + BATTERY_OFFSET;     
  float percentage = map(Vbatt_true_mv, 3300, 4200, 0, 100);
  Serial.print("Battery voltage:");  Serial.println(Vbatt_true_mv);
  return percentage;
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


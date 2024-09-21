#include <WiFi.h>
#include <ESP32Firebase.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

const char* ssid[] = {"Smart Green Garden", "WIFIKU"};
const char* password[] = {"Sukasukakami", "1234567890"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);

#define FIREBASE_URL "https://nextgenhidroponik-default-rtdb.asia-southeast1.firebasedatabase.app/"
Firebase firebase(FIREBASE_URL);

#define PIN_RELAY_PENGURASAN_PIPA 4
#define PIN_RELAY_POMPA_IRIGASI 5
#define PIN_RELAY_DINAMO_PENGADUK 12
#define PIN_RELAY_NUTRISI_AB 13
#define PIN_RELAY_PH_UP 14
#define PIN_RELAY_PH_DOWN 15
#define PIN_RELAY_PESTISIDA 16
#define PIN_RELAY_SUMBER_AIR 17
#define PIN_RELAY_GROW_LIGHT 18
#define LDR_PIN 19  

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  bool connected = false;
  for (int i = 0; i < sizeof(ssid) / sizeof(ssid[0]); i++) {
    Serial.print("Sistem Next-Gen Hydroponics\n\n");
    Serial.print("Menghubungkan ke jaringan ");
    Serial.println(ssid[i]);
    WiFi.begin(ssid[i], password[i]);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      Serial.print(".");
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.println("\nTerhubung! Tunggu sebentar, sedang menyiapkan controls relay...");
      break;
    } else {
      Serial.println("\nGagal menghubungkan.");
    }
  }
  if (!connected) {
    Serial.println("Tidak ada WiFi yang bisa terhubung.");
  }

  pinMode(PIN_RELAY_PENGURASAN_PIPA, OUTPUT);
  digitalWrite(PIN_RELAY_PENGURASAN_PIPA, LOW);
  pinMode(PIN_RELAY_POMPA_IRIGASI, OUTPUT);
  digitalWrite(PIN_RELAY_POMPA_IRIGASI, LOW);
  pinMode(PIN_RELAY_DINAMO_PENGADUK, OUTPUT);
  digitalWrite(PIN_RELAY_DINAMO_PENGADUK, LOW);
  pinMode(PIN_RELAY_NUTRISI_AB, OUTPUT);
  digitalWrite(PIN_RELAY_NUTRISI_AB, LOW);
  pinMode(PIN_RELAY_PH_UP, OUTPUT);
  digitalWrite(PIN_RELAY_PH_UP, LOW);
  pinMode(PIN_RELAY_PH_DOWN, OUTPUT);
  digitalWrite(PIN_RELAY_PH_DOWN, LOW);
  pinMode(PIN_RELAY_PESTISIDA, OUTPUT);
  digitalWrite(PIN_RELAY_PESTISIDA, LOW);
  pinMode(PIN_RELAY_SUMBER_AIR, OUTPUT);
  digitalWrite(PIN_RELAY_SUMBER_AIR, LOW);
  pinMode(PIN_RELAY_GROW_LIGHT, OUTPUT);
  digitalWrite(PIN_RELAY_GROW_LIGHT, LOW);
  pinMode(LDR_PIN, INPUT);
  
  initializeRelayDataInFirebase();
}

void loop() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  int year = ptm->tm_year + 1900;
  int month = ptm->tm_mon + 1;
  int day = ptm->tm_mday;
  int hour = ptm->tm_hour;
  int minute = ptm->tm_min;
  int second = ptm->tm_sec;

  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", year, month, day);
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hour, minute);

  sendDataToFirebase(dateStr, timeStr);
}

void initializeRelayDataInFirebase() {
  String relayDataPath = "/esp32controls/relay_data";
  int relayData = firebase.getInt(relayDataPath);
  Serial.print("relayData: ");
  Serial.println(relayData);
  if (relayData == 0) {
    String defaultRelayData = "0,0,0,0,0,0,0,0,0";
    bool result = firebase.setString(relayDataPath, defaultRelayData);
    Serial.println(result ? "relay_data initialized successfully" : "Failed to initialize relay_data");
  } else {
    Serial.println("relay_data already exists in Firebase");
  }
}

void sendDataToFirebase(const char* dateStr, const char* timeStr) {
  controlRelaysFromFirebase();

  int controls_action = firebase.getInt("esp32controls/controls_action");
  int val = analogRead(LDR_PIN);

  if (controls_action == 1) {
    Serial.println("Pestisida otomatis");
    String status_hama_path = "/esp32cam/" + String(dateStr) + "/" + String(timeStr) + "/status_hama";
    String status_hama = firebase.getString(status_hama_path);
    if (status_hama == "true") {
      firebase.setInt("esp32controls/relay_pompa_pestisida", 1);
    } else {
      firebase.setInt("esp32controls/relay_pompa_pestisida", 0);
    }
    Serial.println("Grow Light otomatis");
    int Status_grow_light = val >= 2000 ? 1 : 0;
    if (Status_grow_light == 1) {
      digitalWrite(PIN_RELAY_GROW_LIGHT, HIGH);
    } else {
      digitalWrite(PIN_RELAY_GROW_LIGHT, LOW);
    }
    bool result = firebase.setInt("/esp32controls/relay_grow_light", Status_grow_light);
  } else {
    Serial.println("Pestisida manual");
    checkRelay(PIN_RELAY_PESTISIDA, "RELAY_PESTISIDA", 7);
    Serial.println("Grow Light manual");
    int Status_grow_light_manual = digitalRead(PIN_RELAY_GROW_LIGHT);
    if (Status_grow_light_manual == HIGH) {
      digitalWrite(PIN_RELAY_GROW_LIGHT, HIGH);
    } else {
      digitalWrite(PIN_RELAY_GROW_LIGHT, LOW);
    }
  }
}

void controlRelaysFromFirebase() {
  String relayData = firebase.getString("/esp32controls/relay_data");
  if (relayData.length() > 0) {
    int relayStates[9];
    int index = 0;
    char* token = strtok((char*)relayData.c_str(), ",");
    while (token != NULL) {
      relayStates[index++] = atoi(token);
      token = strtok(NULL, ",");
    }
    checkRelay(PIN_RELAY_PENGURASAN_PIPA, "RELAY_PENGURASAN_PIPA", relayStates[0]);
    checkRelay(PIN_RELAY_POMPA_IRIGASI, "RELAY_POMPA_IRIGASI", relayStates[1]);
    checkRelay(PIN_RELAY_DINAMO_PENGADUK, "RELAY_DINAMO_PENGADUK", relayStates[2]);
    checkRelay(PIN_RELAY_NUTRISI_AB, "RELAY_NUTRISI_AB", relayStates[3]);
    checkRelay(PIN_RELAY_PH_UP, "RELAY_PH_UP", relayStates[4]);
    checkRelay(PIN_RELAY_PH_DOWN, "RELAY_PH_DOWN", relayStates[5]);
    checkRelay(PIN_RELAY_PESTISIDA, "RELAY_PESTISIDA", relayStates[6]);
    checkRelay(PIN_RELAY_SUMBER_AIR, "RELAY_SUMBER_AIR", relayStates[7]);
    checkRelay(PIN_RELAY_GROW_LIGHT, "RELAY_GROW_LIGHT", relayStates[8]);
  }
}

void checkRelay(int relayPin, const char* relayName, int relayState) {
  if (relayState == 1) {
    digitalWrite(relayPin, HIGH);
    Serial.print(relayName);
    Serial.println(" menyala");
  } else if (relayState == 0) {
    digitalWrite(relayPin, LOW);
    Serial.print(relayName);
    Serial.println(" mati");
  }
}

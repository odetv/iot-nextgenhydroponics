#include "esp_camera.h"
#include <WiFi.h>
#include <ESP32Firebase.h>
#include "Base64.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid[] = {"Smart Green Garden","WIFIKU"};
const char* password[] = {"Sukasukakami","66993259"};

const char* status_ulat_global;
const char* photo_original_global;
const char* photo_detected_global;

#define FIREBASE_URL "https://nextgenhidroponik-default-rtdb.asia-southeast1.firebasedatabase.app/"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);
void startCameraServer();
void setupLedFlash(int pin);
Firebase firebase(FIREBASE_URL);

#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

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
      Serial.println("\nTerhubung! Tunggu sebentar, sedang menyiapkan kamera...");
      break;
    } else {
      Serial.println("\nGagal menghubungkan.");
    }
  }
  if (!connected) {
    Serial.println("Tidak ada WiFi yang bisa terhubung.");
  }
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(2000);
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_HVGA;
  // config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  // config.jpeg_quality = 12;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_HVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_HVGA;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }
#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    esp_restart();
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_HVGA);
  }
#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif
  startCameraServer();
  Serial.print("Kamera siap! Gunakan 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' untuk akses kamera!\n");
  delay(1000);
}

void loop() {
  Serial.println("\n=== PROSES KAMERA BERJALAN ===");
  uploadPhotoToFastAPI();
  Serial.println("=== PROSES KAMERA SELESAI ===");
  delay(5000);
}

void reconnectWiFi() {
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
      Serial.println("\nTerhubung kembali ke WiFi!");
      Serial.println("Update waktu dari server NTP...");
      timeClient.begin();
      while (!timeClient.update()) {
        timeClient.forceUpdate();
        delay(1000);
      }
      Serial.println("Waktu telah diperbarui!");
      break;
    } else {
      Serial.println("\nGagal menghubungkan.");
    }
  }
}

void uploadPhotoToFastAPI() {
  Serial.println("Mengecek konektivitas jaringan...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Koneksi WiFi terputus. Mencoba untuk tersambung kembali...");
    reconnectWiFi();
  }
  Serial.println("Konektivitas aman.");

  Serial.println("Mengecek waktu NTP...");
  time_t now = timeClient.getEpochTime();
  struct tm * timeinfo;
  char currentDate[11]; // YYYY-MM-DD
  char currentTime[7]; // HH:MM
  char timeStamp[20]; // YYYY-MM-DD HH:MM:SS
  timeinfo = localtime(&now);
  strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
  strftime(currentTime, sizeof(currentTime), "%H:%M", timeinfo);
  strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", timeinfo);
  Serial.println("Waktu NTP aman.");

  Serial.println("Memproses foto...");
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Pengambilan foto gagal");
    return;
  }
  Serial.println("Foto berhasil diproses.");
  String base64Photo = "data:image/png;base64," + encodebase64(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  String boundary = "--------------------------" + String(millis(), HEX);
  String payload = "--" + boundary + "\r\n";
  payload += "Content-Disposition: form-data; name=\"image_url\"\r\n\r\n";
  payload += base64Photo + "\r\n";
  payload += "--" + boundary + "--\r\n";
  HTTPClient http;
  http.begin("http://nextgen.dev.smartgreenovation.com/upload");
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("Response HTTP API: ");
    Serial.println(httpResponseCode);

    String response = http.getString();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("deserializeJson() gagal: ");
      Serial.println(error.c_str());
      return;
    }

    const char* status_ulat = doc["status_ulat"];
    const char* photo_original = doc["photo_original"];
    const char* photo_detected = doc["photo_detected"];
    status_ulat_global = status_ulat;
    photo_original_global = photo_original;
    photo_detected_global = photo_detected;

    String jsonData = "{";
    jsonData += "\"timestamp\":\"" + String(timeStamp) + "\",";
    jsonData += "\"status_hama\":\"" + String(status_ulat_global) + "\",";
    jsonData += "\"photo_original\":\"" + String(photo_original_global) + "\",";
    jsonData += "\"photo_hama\":\"" + String(photo_detected_global) + "\"";
    jsonData += "}";
    String photoPath = "/esp32cam/" + String(currentDate) + "/" + String(currentTime);
    if (firebase.setNum(photoPath, jsonData)) {
      Serial.println("Foto berhasil diunggah ke Firebase.");
    } else {
      Serial.println("Gagal mengunggah.");
    }
  } else {
    Serial.print("Response HTTP API: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

String encodebase64(uint8_t* data, size_t len) {
  size_t encodedLen = base64_enc_len(len);
  char* encodedString = (char*)malloc(encodedLen + 1);
  if (encodedString == NULL) {
    Serial.println("Memory allocation for base64 encoding failed");
    return String();
  }
  base64_encode(encodedString, (char*)data, len);
  String result(encodedString);
  free(encodedString);
  return result;
}
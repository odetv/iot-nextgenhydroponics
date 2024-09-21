#include <WiFi.h>
#include <ESP32Firebase.h>
#include <DHT.h> // Library untuk sensor DHT
#include <NewPing.h> // Library untuk sensor ultrasonik
#include <OneWire.h> //Memanggil library OneWire yang diperlukan sebagai dependensi library Dallas Temperature
#include <DallasTemperature.h> // Memanggil library Dallas Temperature
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

//inisiasi firebase
#define FIREBASE_URL "https://nextgenhidroponik-default-rtdb.asia-southeast1.firebasedatabase.app/"
Firebase firebase(FIREBASE_URL);

//inisiasi sensor suhu air 
#define ONE_WIRE_BUS 4  //  sensor DS18B20  suhu air
OneWire oneWire(ONE_WIRE_BUS);    
DallasTemperature sensor(&oneWire);


//inisiasi Sensor Ultrasonik 
#define TRIGGER_PIN1  12 // Pin trigger ultrasonik 
#define ECHO_PIN1     13  // Pin echo ultrasonik 
#define MAX_DISTANCE 200 // Jarak maksimum yang diukur dalam sentimeter
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE); // Inisialisasi objek sonar1
#define TRIGGER_PIN2  14  // Pin trigger ultrasonik 
#define ECHO_PIN2     15  // Pin echo ultrasonik 
NewPing sonar2(TRIGGER_PIN2, ECHO_PIN2, MAX_DISTANCE); // Inisialisasi objek sonar2
#define TRIGGER_PIN3  16  // Pin trigger ultrasonik 
#define ECHO_PIN3     17  // Pin echo ultrasonik 
NewPing sonar3(TRIGGER_PIN3, ECHO_PIN3, MAX_DISTANCE); // Inisialisasi objek sonar3
#define TRIGGER_PIN4  18  // Pin trigger ultrasonik 
#define ECHO_PIN4     19  // Pin echo ultrasonik 
NewPing sonar4(TRIGGER_PIN4, ECHO_PIN4, MAX_DISTANCE); // Inisialisasi objek sonar4
#define TRIGGER_PIN5  5  // Pin trigger ultrasonik 
#define ECHO_PIN5     23  // Pin echo ultrasonik 
NewPing sonar5(TRIGGER_PIN5, ECHO_PIN5, MAX_DISTANCE); // Inisialisasi objek sonar5
#define TRIGGER_PIN6  25  // Pin trigger ultrasonik 
#define ECHO_PIN6     26 // Pin echo ultrasonik 
NewPing sonar6(TRIGGER_PIN6, ECHO_PIN6, MAX_DISTANCE); // Inisialisasi objek sonar6

//inisiasi Sensor Suhu DHT22
#define DHT_PIN 27      // Pin data sensor DHT terhubung ke pin 27
#define DHT_TYPE DHT22   // Tipe sensor DHT (DHT11, DHT21, DHT22)
DHT dht(DHT_PIN, DHT_TYPE);

//inisiasi sensor TDS
// Pin sensor TDS
const int TDS_SENSOR_PIN = 34;

// Konstanta untuk kalibrasi TDS
const float TDS_FACTOR = 0.5; // Faktor konversi dari sensor
const float VREF = 3.3; // Tegangan referensi ADC (ESP32 menggunakan 3.3V)
const float ADC_RESOLUTION = 4096; // Resolusi ADC (12-bit)

//inisiasi sensor PH
const int ph_pin = 35;
float po = 0;
float PH_step;
int nilai_analog_ph;
double teganganph;
float PH4 = 10.654;
float PH6 = 8.500;


//inisiasi waktu 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);  // 28800 seconds = 8 hours

//inisiasi Wifi
const char* ssid = "Smart Green Garden";
const char* password = "Sukasukakami";

// Variabel untuk melacak waktu pengiriman terakhir
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // Interval pengiriman 1 menit (60000 ms)


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  
   //Inisialisasi sensor DHT
    dht.begin();
  
  //Inisiasi Sensor Ultrasonik
  pinMode(TRIGGER_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  //Inisiasi Sensor Ultrasonik2
  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
  //Inisiasi Sensor Ultrasonik3
  pinMode(TRIGGER_PIN3, OUTPUT);
  pinMode(ECHO_PIN3, INPUT);
  //Inisiasi Sensor Ultrasonik4
  pinMode(TRIGGER_PIN4, OUTPUT);
  pinMode(ECHO_PIN4, INPUT);
  //Inisiasi Sensor Ultrasonik5
  pinMode(TRIGGER_PIN5, OUTPUT);
  pinMode(ECHO_PIN5, INPUT);
  //Inisiasi Sensor Ultrasonik6
  pinMode(TRIGGER_PIN6, OUTPUT);
  pinMode(ECHO_PIN6, INPUT);
  
  //Menginisiasikan sensor One-Wire DS18B20
  sensor.begin();
  
  //inisiasi Ph
  pinMode(ph_pin, INPUT);
  
  //inisiasi TDS
  //pinMode(TdsSensorPin,INPUT);
  // Inisialisasi komunikasi I2C
  Wire.begin();
  
  
  //inisiasi waktu 
  timeClient.begin();
}

void loop() {

  // Dapatkan waktu saat ini dalam format "HH:MM:SS"
timeClient.update(); // Update time client to get the latest time

// Dapatkan waktu saat ini
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  int year = ptm->tm_year + 1900;
  int month = ptm->tm_mon + 1;
  int day = ptm->tm_mday;
  int hour = ptm->tm_hour;
  int minute = ptm->tm_min;
  int second = ptm->tm_sec;

  // Format tanggal dan waktu
  char dateStr[11]; // YYYY-MM-DD
  sprintf(dateStr, "%04d-%02d-%02d", year, month, day);
  char timeStr[6]; // HH:MM
  sprintf(timeStr, "%02d:%02d", hour, minute);


  //if (millis() - DataMillis > 15000) {
    // Baca jarak dari sensor ultrasonik
    int distance1 = sonar1.ping_cm();
    String status_nutrisi_a;
      if (distance1 > 50) {
          status_nutrisi_a = "full";
      } else if (distance1 > 30 && distance1 <= 50) {
          status_nutrisi_a = "medium";
      } else if (distance1 < 20 && distance1 > 0) {
          status_nutrisi_a = "low";
      } else if (distance1 == 0) {
          status_nutrisi_a = "empty";
      }
    
    int distance2 = sonar2.ping_cm();
    String status_nutrisi_b;
      if (distance2 > 50) {
          status_nutrisi_b = "full";
      } else if (distance2 > 30 && distance2 <= 50) {
          status_nutrisi_b = "medium";
      } else if (distance2 < 20 && distance2 > 0) {
          status_nutrisi_b = "low";
      } else if (distance2 == 0) {
          status_nutrisi_b = "empty";
      }
    
    int distance3 = sonar3.ping_cm();
    String status_ph_up;
      if (distance3 > 50) {
          status_ph_up = "full";
      } else if (distance3 > 30 && distance3 <= 50) {
          status_ph_up = "medium";
      } else if (distance3 < 20 && distance3 > 0) {
          status_ph_up = "low";
      } else if (distance3 == 0) {
          status_ph_up = "empty";
      }
      
    int distance4 = sonar4.ping_cm();
    String status_ph_down;
      if (distance4 > 50) {
          status_ph_down = "full";
      } else if (distance4 > 30 && distance4 <= 50) {
          status_ph_down = "medium";
      } else if (distance4 < 20 && distance4 > 0) {
          status_ph_down = "low";
      } else if (distance4 == 0) {
          status_ph_down = "empty";
      }
      
    int distance5 = sonar5.ping_cm();
    String status_pestisida;
      if (distance5 > 50) {
          status_pestisida = "full";
      } else if (distance5 > 30 && distance5 <= 50) {
          status_pestisida = "medium";
      } else if (distance5 < 20 && distance5 > 0) {
          status_pestisida = "low";
      } else if (distance5 == 0) {
          status_pestisida = "empty";
      }
      
    int distance6 = sonar6.ping_cm();

    // Baca suhu dari sensor DS18B20
    //sensor.setResolution(10);
    sensor.requestTemperatures();
    float suhuDS18B20 = sensor.getTempCByIndex(0);

    // Baca nilai sensor PH
    nilai_analog_ph = analogRead(ph_pin);
    teganganph = 3.3 / 4095.0 * nilai_analog_ph;
    PH_step = (PH4 - PH6) / 4.0 - 6.0;
    po = 7.00 + ((PH6 - teganganph) / PH_step);

    // Baca nilai sensor TDS
    float tdsValue = readTdsSensor();


   // Baca suhu dan kelembaban dari sensor DHT
   float temperature = dht.readTemperature();
   float humidity = dht.readHumidity();

    

    // Jalur baru berdasarkan tanggal dan waktu
    String Path = "/esp32info/" + String(dateStr) + "/" + String(timeStr);

  // Data JSON
  String jsonData = "{\"sensor_suhu_udara\":\"" + String(temperature, 2) + 
                      "\", \"sensor_kelembaban_udara\":\"" + String(humidity, 2) +
                      "\", \"kapasitas_nutrisi_a\":\"" + String(distance1) +
                      "\", \"status_nutrisi_a\":\"" + String(status_nutrisi_a) +
                      "\", \"kapasitas_nutrisi_b\":\"" + String(distance2) +
                      "\", \"status_nutrisi_b\":\"" + String(status_nutrisi_b) +
                      "\", \"kapasitas_ph_up\":\"" + String(distance3) +
                      "\", \"status_ph_up\":\"" + String(status_ph_up) +
                      "\", \"kapasitas_ph_down\":\"" + String(distance4) +
                      "\", \"status_ph_down\":\"" + String(status_ph_down) +
                      "\", \"kapasitas_pestisida\":\"" + String(distance5) +
                      "\", \"status_pestisida\":\"" + String(status_pestisida) +
                      "\", \"kapasitas_tandon_pencampuran\":\"" + String(distance6) +
                      "\", \"sensor_suhu_air\":\"" + String(suhuDS18B20, 2) +
                      "\", \"sensor_ph\":\"" + String(po, 2) +
                      "\", \"sensor_tds\":\"" + String(tdsValue, 2) + "\"}";


// Periksa apakah waktu pengiriman sudah lewat 1 menit
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis; // Update waktu pengiriman terakhir

  Serial.print("Sending JSON data to path: ");
  Serial.println(Path); // Print the path being used

  Serial.print("Sending JSON data: ");
  Serial.println(jsonData); // Print the JSON data being sent

            if (firebase.setNum(Path, jsonData)) {
              Serial.println("Uploaded to Firebase");
            } else {
              Serial.println("Upload failed");
              Serial.print("Error: ");
            }

 }


  
  int kontrol_aksi = firebase.getInt("esp32controls/controls_action");
  
  if (kontrol_aksi == 1 ){
// Periksa apakah sudah jam 8 pagi
  if (hour == 8 && minute == 0 && second == 0) {
    unsigned long startTime = millis(); // Catat waktu saat ini
    unsigned long elapsedTime = 0;

    // Loop selama 30 menit (30 * 60 * 1000 milidetik)
    while (elapsedTime < 30 * 60 * 1000) {
      // Perintah firebase dan logika kontrol Anda
      firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // selenoid valve

      // Baca jarak dari sensor ultrasonik6
      int distance6 = sonar6.ping_cm();
      
      if (distance6 >= 20) {
        firebase.setInt("esp32controls/relay_dinamo_pengaduk", 1); // fan 

          if (po > 6.5) {
            firebase.setInt("esp32controls/relay_ph_down", 1); // down on 
            firebase.setInt("esp32controls/relay_ph_up", 0);   // up off
          } else if (po < 6) {
            firebase.setInt("esp32controls/relay_ph_up", 1);   // up on
            firebase.setInt("esp32controls/relay_ph_down", 0); // down off
          } else {
            firebase.setInt("esp32controls/relay_ph_up", 0);   // up off
            firebase.setInt("esp32controls/relay_ph_down", 0); // down off
            //Serial.println("Nilai pH berada dalam rentang 6 - 6.5, tidak ada aksi.");
          }

          if (tdsValue > 1100) {
            firebase.setInt("esp32controls/relay_sumber_air", 1); // Hidupkan relay sumber air
            firebase.setInt("esp32controls/relay_nutrisi_ab", 0); // Matikan relay nutrisi AB
          } else if (tdsValue < 1000) {
            firebase.setInt("esp32controls/relay_sumber_air", 0); // Hidupkan relay sumber air
            firebase.setInt("esp32controls/relay_nutrisi_ab", 1); // Hidupkan relay nutrisi AB
          } else {
            // Rentang 1000 - 1100, tidak ada aksi
            firebase.setInt("esp32controls/relay_sumber_air", 0); // Matikan relay sumber air
            // Matikan relay nutrisi AB jika TDS berada di rentang 1000 - 1100
            firebase.setInt("esp32controls/relay_nutrisi_ab", 0); // Matikan relay nutrisi AB
            //Serial.println("Nilai TDS berada dalam rentang 1000 - 1100, tidak ada aksi.");
          }
          }

          if (po >= 6 && po <= 6.5 && tdsValue >= 1000 && tdsValue <= 1100) {
            firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // pompa on 
          }


      // Hitung berapa lama sudah berlalu sejak startTime
      elapsedTime = millis() - startTime;
    }
  }



                    // Periksa apakah sudah jam 12 sore
                    if (hour == 12 && minute == 0 && second == 0) {
                      unsigned long startTime = millis(); // Catat waktu saat ini
                      unsigned long elapsedTime = 0;
                  
                      // Loop selama 30 menit (30 * 60 * 1000 milidetik)
                      while (elapsedTime < 30 * 60 * 1000) {


                          // Baca suhu dari sensor DS18B20
                          //sensor.setResolution(10);
                          sensor.requestTemperatures();
                          float suhuDS18B20 = sensor.getTempCByIndex(0);
                          // Buka pengurasan pipa jika suhu air lebih dari 31 derajat Celsius
                          if (suhuDS18B20 > 31) {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // Selenoid valve on
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // Pompa on
                          } else {
                            //tidak ada hal yang terjadi
                          }
                          }
                        // Hitung berapa lama sudah berlalu sejak startTime
                        elapsedTime = millis() - startTime;
                      }
                      // Periksa apakah sudah jam 12 sore
                    if (hour == 13 && minute == 0 && second == 0) {
                      unsigned long startTime = millis(); // Catat waktu saat ini
                      unsigned long elapsedTime = 0;
                  
                      // Loop selama 30 menit (30 * 60 * 1000 milidetik)
                      while (elapsedTime < 30 * 60 * 1000) {


                          // Baca suhu dari sensor DS18B20
                          //sensor.setResolution(10);
                          sensor.requestTemperatures();
                          float suhuDS18B20 = sensor.getTempCByIndex(0);
                          // Buka pengurasan pipa jika suhu air lebih dari 31 derajat Celsius
                          if (suhuDS18B20 > 31) {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // Selenoid valve on
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // Pompa on
                          } else {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 0); // Selenoid valve off
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 0); // Pompa off
                          }
                          }
                        // Hitung berapa lama sudah berlalu sejak startTime
                        elapsedTime = millis() - startTime;
                      }
                      // Periksa apakah sudah jam 12 sore
                    if (hour == 14 && minute == 0 && second == 0) {
                      unsigned long startTime = millis(); // Catat waktu saat ini
                      unsigned long elapsedTime = 0;
                  
                      // Loop selama 30 menit (30 * 60 * 1000 milidetik)
                      while (elapsedTime < 30 * 60 * 1000) {


                          // Baca suhu dari sensor DS18B20
                          //sensor.setResolution(10);
                          sensor.requestTemperatures();
                          float suhuDS18B20 = sensor.getTempCByIndex(0);
                          // Buka pengurasan pipa jika suhu air lebih dari 31 derajat Celsius
                          if (suhuDS18B20 > 31) {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // Selenoid valve on
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // Pompa on
                          } else {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 0); // Selenoid valve off
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 0); // Pompa off
                          }
                          }
                        // Hitung berapa lama sudah berlalu sejak startTime
                        elapsedTime = millis() - startTime;
                      }
                      // Periksa apakah sudah jam 12 sore
                    if (hour == 15 && minute == 0 && second == 0) {
                      unsigned long startTime = millis(); // Catat waktu saat ini
                      unsigned long elapsedTime = 0;
                  
                      // Loop selama 30 menit (30 * 60 * 1000 milidetik)
                      while (elapsedTime < 30 * 60 * 1000) {


                          // Baca suhu dari sensor DS18B20
                          //sensor.setResolution(10);
                          sensor.requestTemperatures();
                          float suhuDS18B20 = sensor.getTempCByIndex(0);
                          // Buka pengurasan pipa jika suhu air lebih dari 31 derajat Celsius
                          if (suhuDS18B20 > 31) {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // Selenoid valve on
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // Pompa on
                          } else {
                            firebase.setInt("esp32controls/relay_pengurasan_pipa", 0); // Selenoid valve off
                            firebase.setInt("esp32controls/relay_pompa_irigasi", 0); // Pompa off
                          }
                          }
                        // Hitung berapa lama sudah berlalu sejak startTime
                        elapsedTime = millis() - startTime;
                      } 
                    
  
                                // Periksa apakah sudah jam 4 sore
                                if (hour == 16 && minute == 0 && second == 0) {
                                  unsigned long startTime = millis(); // Catat waktu saat ini
                                  unsigned long elapsedTime = 0;
                              
                                  // Loop selama 30 menit (30 * 60 * 1000 milidetik)
                                  while (elapsedTime < 30 * 60 * 1000) {
                              
                                   firebase.setInt("esp32controls/relay_pengurasan_pipa", 1); // selenoid valve
                              
                                    // Baca jarak dari sensor ultrasonik6
                                    int distance6 = sonar6.ping_cm();
                              
                                    if(distance6 >= 20){
                                      firebase.setInt("esp32controls/relay_dinamo_pengaduk", 1); // fan 
                                      
                                      if (po > 6.5) {
                                        firebase.setInt("esp32controls/relay_ph_down", 1); // down on 
                                        firebase.setInt("esp32controls/relay_ph_up", 0);   // up off
                                      } else if (po < 6) {
                                        firebase.setInt("esp32controls/relay_ph_up", 1);   // up on
                                        firebase.setInt("esp32controls/relay_ph_down", 0); // down off
                                      } else {
                                        firebase.setInt("esp32controls/relay_ph_up", 0);   // up off
                                        firebase.setInt("esp32controls/relay_ph_down", 0); // down off
                                        //Serial.println("Nilai pH berada dalam rentang 6 - 6.5, tidak ada aksi.");
                                      }
                                      
                                      if (tdsValue > 1100) {
                                      firebase.setInt("esp32controls/relay_sumber_air", 1); // Hidupkan relay sumber air
                                      firebase.setInt("esp32controls/relay_nutrisi_ab", 0); // Matikan relay nutrisi AB
                                    } else if (tdsValue < 1000) {
                                      firebase.setInt("esp32controls/relay_sumber_air", 0); // Hidupkan relay sumber air
                                      firebase.setInt("esp32controls/relay_nutrisi_ab", 1); // Hidupkan relay nutrisi AB
                                    } else {
                                      // Rentang 1000 - 1100, tidak ada aksi
                                      firebase.setInt("esp32controls/relay_sumber_air", 0); // Matikan relay sumber air
                                      // Matikan relay nutrisi AB jika TDS berada di rentang 1000 - 1100
                                      firebase.setInt("esp32controls/relay_nutrisi_ab", 0); // Matikan relay nutrisi AB
                                      //Serial.println("Nilai TDS berada dalam rentang 1000 - 1100, tidak ada aksi.");
                                    }
                                    }
                              
                                    if (po >= 6 && po <= 6.5 && tdsValue >= 1000 && tdsValue <= 1100) {
                                      firebase.setInt("esp32controls/relay_pompa_irigasi", 1); // pompa on 
                                    }
                              
                                    // Hitung berapa lama sudah berlalu sejak startTime
                                    elapsedTime = millis() - startTime;
                                  }
                                }

                                

  }else {
    //aksi di kontrol manual
  }

    
}

float readTdsSensor() {
  // Baca nilai analog dari pin sensor TDS
  int analogValue = analogRead(TDS_SENSOR_PIN);

  // Hitung tegangan yang dibaca (voltase)
  float voltage = analogValue * (VREF / ADC_RESOLUTION);

  // Konversi voltase menjadi nilai TDS
  float tdsValue = (voltage * TDS_FACTOR) * 1000;

  return tdsValue;
}

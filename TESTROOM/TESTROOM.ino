#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ElegantOTA.h>
#include <WiFiManager.h>  // Voor WiFi setup
#include <DHT.h>  // Voor DHT22
#include <DallasTemperature.h>  // Voor DS18B20
#include <OneWire.h>  // Voor OneWire bus
#include <Adafruit_TSL2561_U.h>  // Voor TSL2561
#include <Adafruit_NeoPixel.h>  // Voor WS2812 Neopixels

// Pin definitions gebaseerd op jouw lintkabel en tabel
#define DHT_PIN 18       // D6 -> GPIO18 (DHT22 data)
#define ONE_WIRE_PIN 4   // D3 -> GPIO4 (DS18B20)
#define TSL_SDA 21       // D0 -> GPIO21 (I2C SDA)
#define TSL_SCL 22       // D1 -> GPIO22 (I2C SCL)
#define PIR_MOV1 17      // D5 -> GPIO17
#define PIR_MOV2 26      // A5 -> GPIO26
#define SHARP_LED 19     // D7 -> GPIO19 (Sharp dust LED out)
#define SHARP_ANALOG 32  // A2 -> GPIO32 (Sharp dust analog in)
#define LDR_ANALOG 33    // A3 -> GPIO33 (LDR analog in)
#define CO2_PWM 25       // A4 -> GPIO25 (CO2 PWM input)
#define TSTAT_PIN 27     // A6 -> GPIO27 (TSTAT switch to GND)
#define OPTION_LDR 14    // A7 -> GPIO14 (Option LDR for BEAMalert / MOV2)
#define NEOPIXEL_PIN 16  // D4 -> GPIO16 (Neopixels data)
#define NEOPIXEL_NUM 1   // Stel in op aantal LEDs in je strip, bv. 1 voor test

// Sensor objecten
DHT dht(DHT_PIN, DHT22);  // DHT22
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Variabelen voor JSON data
float temp_dht = 0.0;     // g: Temp1 DHT
float temp_ds = 0.0;      // h: Temp2 DS18B20
float humi = 0.0;         // d: Humidity
float dew = 0.0;          // c: Dew point (bereken uit DHT)
int light_ldr = 0;        // e: LDR light 0-100
int sun_light = 0;        // f: TSL2561 lux
int dust = 0;             // b: Sharp dust
int co2 = 0;              // a: CO2 PPM from PWM
int tstat_on = 0;         // k: TSTAT 0/1 (inverted, low = on)
int mov1_triggers = 0;    // i: MOV1 triggers per min
int mov2_triggers = 0;    // j: MOV2 triggers per min
int mov1_light = 0;       // l: MOV1 light 0/1 (logic: dark + motion)
int mov2_light = 0;       // m: MOV2 light 0/1
int beam_alert = 0;       // n: BEAMalert 0/1 (from OPTION_LDR low = alert?)
int beam_light = 0;       // o: BEAMlight 0/1 (logic if alert)
int night_mode = 0;       // p: Night unused
int reserve1 = 0;         // q: Bed unused
int reserve2 = 0;         // r: Unused
int rgb_r = 0;            // s: Neopixel R
int rgb_g = 0;            // t: G
int rgb_b = 0;            // u: B

// Timers en counters
unsigned long last_min = 0;  // Voor trigger counters reset elke min
unsigned long last_motion1 = 0;  // Voor light timeout
unsigned long last_motion2 = 0;
const unsigned long LIGHT_TIMEOUT = 60000;  // 1 min light on na motion

// Webserver op poort 80
AsyncWebServer server(80);

// Functie om dew point te berekenen (eenvoudige approx)
float calculateDewPoint(float t, float h) {
  return t - ((100 - h) / 5.0);  // Simpele formule, pas aan als je exacte hebt
}

// Functie om LDR te schalen 0-100 (pas aan op basis van je 22k divider)
int scaleLDR(int raw) {
  return map(raw, 0, 4095, 0, 100);  // Aanpassen op basis van calibratie
}

// Functie om Sharp dust te lezen (simpel gemiddelde)
int readDust() {
  digitalWrite(SHARP_LED, LOW);  // LED on
  delayMicroseconds(280);
  int val = analogRead(SHARP_ANALOG);
  delayMicroseconds(40);
  digitalWrite(SHARP_LED, HIGH);  // LED off
  delayMicroseconds(9680);
  return val;  // Kalibreer naar µg/m³ later
}

// Functie om CO2 PPM van PWM te lezen
int readCO2() {
  unsigned long high = pulseIn(CO2_PWM, HIGH, 2000000);
  unsigned long low = pulseIn(CO2_PWM, LOW, 2000000);
  if (high == 0 || low == 0) return 0;
  float ppm = 5000.0 * (high - 2.0) / (high + low - 4.0);
  return (int)ppm;  // Aanpassen als je MH-Z19 exacte formule hebt
}

// Functie om JSON string te bouwen
String getJSON() {
  String json = "{";
  json += "\"a\":" + String(co2) + ",";
  json += "\"b\":" + String(dust) + ",";
  json += "\"c\":" + String(dew) + ",";
  json += "\"d\":" + String((int)humi) + ",";
  json += "\"e\":" + String(light_ldr) + ",";
  json += "\"f\":" + String(sun_light) + ",";
  json += "\"g\":" + String(temp_dht, 1) + ",";
  json += "\"h\":" + String(temp_ds, 1) + ",";
  json += "\"i\":" + String(mov1_triggers) + ",";
  json += "\"j\":" + String(mov2_triggers) + ",";
  json += "\"k\":" + String(tstat_on) + ",";
  json += "\"l\":" + String(mov1_light) + ",";
  json += "\"m\":" + String(mov2_light) + ",";
  json += "\"n\":" + String(beam_alert) + ",";
  json += "\"o\":" + String(beam_light) + ",";
  json += "\"p\":" + String(night_mode) + ",";
  json += "\"q\":" + String(reserve1) + ",";
  json += "\"r\":" + String(reserve2) + ",";
  json += "\"s\":" + String(rgb_r) + ",";
  json += "\"t\":" + String(rgb_g) + ",";
  json += "\"u\":" + String(rgb_b) + ",";
  json += "\"v\":" + String(WiFi.RSSI()) + ",";
  int quality = 2 * (WiFi.RSSI() + 100);  // Simpele calc, pas aan
  json += "\"w\":" + String(quality) + ",";
  int free_heap = (ESP.getFreeHeap() * 100) / ESP.getHeapSize();  // %
  json += "\"x\":" + String(free_heap);
  json += "}";
  return json;
}

void setup() {
  Serial.begin(115200);

  // WiFiManager voor setup
  WiFiManager wm;
  wm.autoConnect("RoomController-AP");  // Maakt AP als geen WiFi

  // Pin modes
  pinMode(PIR_MOV1, INPUT);
  pinMode(PIR_MOV2, INPUT);
  pinMode(SHARP_LED, OUTPUT);
  digitalWrite(SHARP_LED, HIGH);  // LED off init
  pinMode(TSTAT_PIN, INPUT_PULLUP);  // Pull-up, low = on
  pinMode(OPTION_LDR, INPUT);  // Voor BEAMalert analog? Of digital?
  pinMode(CO2_PWM, INPUT);

  // Sensor init
  dht.begin();
  ds18b20.begin();
  tsl.begin();
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  pixels.begin();
  pixels.clear();

  // Webserver setup
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Room Controller OK");
  });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getJSON());
  });
  ElegantOTA.begin(&server);  // OTA op /update
  server.begin();

  last_min = millis();
}

void loop() {
  ElegantOTA.loop();  // OTA handler

  // Lees sensoren elke 2 sec (pas aan)
  static unsigned long last_read = 0;
  if (millis() - last_read > 2000) {
    last_read = millis();

    // DHT22
    humi = dht.readHumidity();
    temp_dht = dht.readTemperature();
    dew = calculateDewPoint(temp_dht, humi);

    // DS18B20
    ds18b20.requestTemperatures();
    temp_ds = ds18b20.getTempCByIndex(0);

    // TSL2561
    sensors_event_t event;
    tsl.getEvent(&event);
    sun_light = event.light;  // lux

    // LDR
    int ldr_raw = analogRead(LDR_ANALOG);
    light_ldr = scaleLDR(ldr_raw);

    // Sharp dust
    dust = readDust();

    // CO2
    co2 = readCO2();

    // TSTAT
    tstat_on = !digitalRead(TSTAT_PIN);  // Low = 1 (on)

    // OPTION LDR for BEAM
    int beam_raw = analogRead(OPTION_LDR);  // Pas drempel aan
    beam_alert = (beam_raw < 2000) ? 1 : 0;  // Voorbeeld drempel
    beam_light = beam_alert;  // Simpele logic

    // Neopixel voorbeeld (pas aan)
    rgb_r = 219; rgb_g = 159; rgb_b = 61;  // Test waarden
    pixels.setPixelColor(0, pixels.Color(rgb_r, rgb_g, rgb_b));
    pixels.show();
  }

  // Motion detection en counters
  if (digitalRead(PIR_MOV1) == HIGH) {
    mov1_triggers++;
    last_motion1 = millis();
  }
  if (digitalRead(PIR_MOV2) == HIGH) {
    mov2_triggers++;
    last_motion2 = millis();
  }

  // Light logic: on if motion en dark (light_ldr < 20)
  mov1_light = (millis() - last_motion1 < LIGHT_TIMEOUT && light_ldr < 20) ? 1 : 0;
  mov2_light = (millis() - last_motion2 < LIGHT_TIMEOUT && light_ldr < 20) ? 1 : 0;

  // Reset counters elke min
  if (millis() - last_min > 60000) {
    last_min = millis();
    mov1_triggers = 0;
    mov2_triggers = 0;
  }
}

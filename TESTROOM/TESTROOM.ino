// ESP32_TESTROOM_Grok.ino = Transition from Photon based to ESP32 based Home automation system
// Developed together with ChatGPT in december '25.
// Bereikbaar op http://testroom.local of http://192.168.1.36 => Andere controller: Naam (sectie DNS/MDNS) + static IP aanpassen!
// 16dec25: Zarlardinge: Met Grok + ChatGPT: Logica & consistentie van HVAC auto/manueel modes in orde! Enkel REFRESH blijft een issue!

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>  // Voor OTA
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>  // Voor sin()

#define DHT_PIN       18
#define ONE_WIRE_PIN   4
#define PIR_MOV1      17
#define PIR_MOV2      26
#define SHARP_LED     19
#define SHARP_ANALOG  32
#define LDR_ANALOG    33
#define CO2_PWM       25
#define TSTAT_PIN     27
#define OPTION_LDR    14
#define NEOPIXEL_PIN  16
#define NEOPIXEL_NUM   8

// Initialize libraries
DHT dht(DHT_PIN, DHT22);
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);


// Logingegevens voor wifi
const char* WIFI_SSID = "Delannoy";
const char* WIFI_PASS = "kampendaal,34";


// HVAC Variabelen
float room_temp = 0.0;      // Berekende kamertemp: primair temp_ds, backup temp_dht
String temp_melding = "";   // Melding bij defect (bijv. "DS18B20 defect – DHT22 gebruikt")
int heating_setpoint = 20;  // aa: Gewenste temp (integer, default 20)
int heating_on = 0;         // y: Verwarming aan (0/1, auto of manueel)
int vent_percent = 0;       // z: Ventilatie % (0-100, auto of manueel)
int heating_mode = 0;       // 0 = AUTO, 1 = MANUEEL voor heating
int vent_mode = 0;          // 0 = AUTO, 1 = MANUEEL voor ventilation (slider zet naar 1)


// Sensor variabelen
float temp_dht = 0, temp_ds = 0, humi = 0, dew = 0;
int light_ldr = 0, sun_light = 0, dust = 0, co2 = 0;
int tstat_on = 0, mov1_triggers = 0, mov2_triggers = 0;
int mov1_light = 0, mov2_light = 0;
unsigned long uptime_sec = 0;
int dew_alert = 0;       // Voor k: DewAlert (temp_ds < dew ? 1 : 0)  
int night = 0;           // Voor q: Night (light_ldr > 50 ? 1 : 0, aangezien light_ldr donker=100, helder=0)  
int bed = 0;             // Voor r: Bed switch (0/1, hardcoded initieel 0, later togglebaar)  
int beam_value = 0;      // Voor o: BEAMvalue (geschaald 0-100 van analogRead(OPTION_LDR))  
int beam_alert_new = 0;  // Voor p: BEAMalert (beam_value > 50 ? 1 : 0)  
uint8_t neo_r = 255;       // Voor s: R waarde (hardcoded 255)  
uint8_t neo_g = 255;       // Voor t: G waarde (hardcoded 255)  
uint8_t neo_b = 255;       // Voor u: B waarde (hardcoded 255)  

// Pixel specifieke arrays
int pixel_mode[2] = {0, 0};  // Voor pixel 0 en 1: 0 = AUTO (PIR), 1 = MANUEEL ON (RGB)
bool pixel_on[NEOPIXEL_NUM] = {false};  // Huidige aan/uit status voor alle 8 pixels (voor display/JSON)

// PIR op 3.3V: beweging = LOW
unsigned long mov1_off_time = 0;
unsigned long mov2_off_time = 0;
const unsigned long LIGHT_ON_DURATION = 30000;
const int LDR_DARK_THRESHOLD = 40;

#define MOV_WINDOW 60000
#define MOV_BUF_SIZE 50
unsigned long mov1Times[MOV_BUF_SIZE] = {0};
unsigned long mov2Times[MOV_BUF_SIZE] = {0};

// Pixel Fade engine
uint8_t currR[NEOPIXEL_NUM], currG[NEOPIXEL_NUM], currB[NEOPIXEL_NUM];
uint8_t targetR[NEOPIXEL_NUM], targetG[NEOPIXEL_NUM], targetB[NEOPIXEL_NUM];
uint8_t startR[NEOPIXEL_NUM], startG[NEOPIXEL_NUM], startB[NEOPIXEL_NUM];  // Missing, nu toegevoegd
unsigned long lastFadeStep = 0;
unsigned long fade_interval_ms = 15;        // Dynamische interval, initieel 15 ms (wordt herberekend)
int fade_duration = 2;                      // Dimsnelheid in seconden (1-10, default 2)
const int FADE_NUM_STEPS = 20;              // 20 stappen = heel smooth, maar nog snel genoeg
float fade_progress[NEOPIXEL_NUM] = {0.0};  // Per pixel progress (0.0-1.0) voor sine-easing



void initFadeEngine() {
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    uint32_t c = pixels.getPixelColor(i);
    currR[i] = (c >> 16) & 0xFF;
    currG[i] = (c >> 8)  & 0xFF;
    currB[i] = c & 0xFF;
    targetR[i] = currR[i]; targetG[i] = currG[i]; targetB[i] = currB[i];
    startR[i] = currR[i]; startG[i] = currG[i]; startB[i] = currB[i];  // Toegevoegd
  }
}


void setTargetColor(int idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < 0 || idx >= NEOPIXEL_NUM) return;
  if (targetR[idx] != r || targetG[idx] != g || targetB[idx] != b) {
    targetR[idx] = r;
    targetG[idx] = g;
    targetB[idx] = b;
    startR[idx] = currR[idx];
    startG[idx] = currG[idx];
    startB[idx] = currB[idx];
    fade_progress[idx] = 0.0;  // Altijd resetten bij verandering
  }
}


void updateFades() {
  unsigned long now = millis();
  if (now - lastFadeStep < fade_interval_ms) return;
  lastFadeStep = now;
  bool changed = false;

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    // Skip als fade al klaar is
    if (fade_progress[i] >= 1.0f && 
        currR[i] == targetR[i] && 
        currG[i] == targetG[i] && 
        currB[i] == targetB[i]) {
      continue;
    }

    // Verhoog progress
    fade_progress[i] += 1.0f / FADE_NUM_STEPS;
    if (fade_progress[i] > 1.0f) fade_progress[i] = 1.0f;

    float ease = sin(fade_progress[i] * PI / 2.0f);  // 0.0 → 1.0 smooth accel

    int newR = startR[i] + (int)round((targetR[i] - startR[i]) * ease);
    int newG = startG[i] + (int)round((targetG[i] - startG[i]) * ease);
    int newB = startB[i] + (int)round((targetB[i] - startB[i]) * ease);

    currR[i] = constrain(newR, 0, 255);
    currG[i] = constrain(newG, 0, 255);
    currB[i] = constrain(newB, 0, 255);

    // Belangrijk: forceer exacte target bij einde (wegens mogelijke afrondingsfout)
    if (fade_progress[i] >= 1.0f) {
      currR[i] = targetR[i];
      currG[i] = targetG[i];
      currB[i] = targetB[i];
    }

    pixels.setPixelColor(i, currR[i], currG[i], currB[i]);
    changed = true;
  }

  if (changed) pixels.show();
}



// Dynamische aanpassing van updateinterval (sneller dimmen)
void updateFadeInterval() {
  fade_duration = constrain(fade_duration, 1, 10);              // Clamp 1-10 s
  fade_interval_ms = (fade_duration * 1000UL) / FADE_NUM_STEPS; // ms per stap, voor fixed aantal
  if (fade_interval_ms < 10) fade_interval_ms = 10;             // Veilig minimum, voorkomt CPU-overload
}

// Helpers (identiek, untouched)
void pushEvent(unsigned long *buf, int size) {
  unsigned long now = millis();
  for (int i = 0; i < size; i++) {
    if (buf[i] == 0 || (now - buf[i] > MOV_WINDOW)) { buf[i] = now; return; }
  }
  int oldest = 0; unsigned long old = buf[0];
  for (int i = 1; i < size; i++) if (buf[i] < old) { old = buf[i]; oldest = i; }
  buf[oldest] = now;
}

int countRecent(unsigned long *buf, int size) {
  unsigned long now = millis(); int c = 0;
  for (int i = 0; i < size; i++) if (buf[i] && (now - buf[i] <= MOV_WINDOW)) c++;
  return c;
}

float calculateDewPoint(float t, float h) { return isnan(t) || isnan(h) ? 0 : t - ((100 - h) / 5.0); } // Berekening dauwpunt met DHT22 data
int scaleLDR(int r) { return map(constrain(r, 100, 3800), 100, 3800, 100, 0); }
int readDust() { digitalWrite(SHARP_LED, LOW); delayMicroseconds(280); int v = analogRead(SHARP_ANALOG); delayMicroseconds(40); digitalWrite(SHARP_LED, HIGH); delayMicroseconds(9680); return v; }
int readCO2() {
  unsigned long h = pulseIn(CO2_PWM, HIGH, 200000);  // Timeout 0.2s i.p.v. 0.1s (i.p.v. 2s vroeger: blocking!)
  unsigned long l = pulseIn(CO2_PWM, LOW, 200000);
  return (h < 100 || l < 100) ? 0 : (int)(5000.0 * (h - 2.0) / (h + l - 4.0));
}



String getJSON() {
  String pixel_on_str = "";
  String pixel_mode_str = String(pixel_mode[0]) + String(pixel_mode[1]);  // Voor toggle checkboxes

  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    pixel_on_str += pixel_on[i] ? "1" : "0";
  }

  return "{\"a\":" + String(co2) +
         ",\"b\":" + String(dust) +
         ",\"c\":" + String(dew,1) +
         ",\"d\":" + String((int)round(humi)) +
         ",\"e\":" + String(light_ldr) +
         ",\"f\":" + String(sun_light) +
         ",\"g\":" + String(temp_dht,1) +
         ",\"h\":" + String(temp_ds,1) +
         ",\"i\":" + String(mov1_triggers) +
         ",\"j\":" + String(mov2_triggers) +
         ",\"k\":" + String(dew_alert) +
         ",\"l\":" + String(tstat_on) +
         ",\"m\":" + String(mov1_light) +
         ",\"n\":" + String(mov2_light) +
         ",\"o\":" + String(beam_value) +
         ",\"p\":" + String(beam_alert_new) +
         ",\"q\":" + String(night) +
         ",\"r\":" + String(bed) +
         ",\"s\":" + String(neo_r) +
         ",\"t\":" + String(neo_g) +
         ",\"u\":" + String(neo_b) +
         ",\"v\":" + String(WiFi.RSSI()) +
         ",\"w\":" + String(constrain(2*(WiFi.RSSI()+100),0,100)) +
         ",\"x\":" + String((ESP.getFreeHeap()*100)/ESP.getHeapSize()) +
         ",\"y\":" + String(heating_on) +
         ",\"z\":" + String(vent_percent) +
         ",\"aa\":" + String(heating_setpoint) +
         ",\"ab\":" + String(fade_duration) +
         ",\"ac\":" + String(uptime_sec) +
         ",\"ad\":\"" + pixel_on_str + "\"" +
         ",\"ae\":\"" + pixel_mode_str + "\"}";
}



// Voor date - time stempel
String getFormattedDateTime() {
  time_t now;
  time(&now);

  if (now < 1700000000) {
    return "tijd nog niet gesynchroniseerd";
  }

  struct tm tm;
  localtime_r(&now, &tm);

  char buf[32];
  strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tm);
  return String(buf);
}




void setup() {

  Serial.begin(115200);
  pinMode(PIR_MOV1, INPUT_PULLUP);  // Voor 3.3V HC-SR501: beweging = LOW
  pinMode(PIR_MOV2, INPUT_PULLUP);
  pinMode(SHARP_LED, OUTPUT); digitalWrite(SHARP_LED, HIGH);
  pinMode(TSTAT_PIN, INPUT_PULLUP);
  pinMode(OPTION_LDR, INPUT);

  dht.begin();
  ds18b20.begin();
  if (!tsl.begin()) Serial.println("TSL2561 niet gevonden");
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  pixels.begin(); pixels.clear(); pixels.show();
  initFadeEngine();
  updateFadeInterval();

  WiFi.mode(WIFI_STA);

  // Kies met of zonder DNS (moet met voor datum-tijd)
  //WiFi.config(IPAddress(192,168,1,36), IPAddress(192,168,1,1), IPAddress(255,255,255,0)); // Zonder DNS
  WiFi.config(
  IPAddress(192,168,1,36),
  IPAddress(192,168,1,1),
  IPAddress(255,255,255,0),
  IPAddress(192,168,1,1)
);   // Mét DNS


  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }


  // ===== TIJDINITIALISATIE (ESP32 native SNTP) =====
  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();

  configTzTime(
  "CET-1CEST,M3.5.0/02,M10.5.0/03",
  "pool.ntp.org",
  "time.nist.gov"
  );




  Serial.println("\nIP: " + WiFi.localIP().toString());
  if (MDNS.begin("testroom")) Serial.println("mDNS: http://testroom.local");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "0");




  // === HOME PAGE ===
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html;
    html.reserve(8000);  // Voldoende voor grote HTML + data
    html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Testroom Status & Control</title>


  <style>
  body {font-family:Arial,Helvetica,sans-serif;background:#ffffff;margin:0;padding:0;}
  .header {
    display:flex;background:#ffcc00;color:black;padding:10px 15px;
    font-size:18px;font-weight:bold;align-items:center;box-sizing:border-box;
  }
  .header-left {flex:1;text-align:left;}
  .header-right {flex:1;text-align:right;font-size:15px;}
  .container {display:flex;flex-direction:row;min-height:calc(100vh - 60px);}
  .sidebar {
    width:80px;padding:10px 5px;background:#ffffff;
    border-right:3px solid #cc0000;box-sizing:border-box;flex-shrink:0;
  }
  .sidebar a {
    display:block;background:#336699;color:white;padding:8px;
    margin:8px 0;text-decoration:none;font-weight:bold;
    font-size:12px;border-radius:6px;text-align:center;
    line-height:1.3;width:60px;box-sizing:border-box;margin-left:auto;margin-right:auto;
  }
  .sidebar a:hover {background:#003366;}
  .sidebar a.active {background:#cc0000;}
  .main {flex:1;padding:15px;overflow-y:auto;box-sizing:border-box;}
  .group-title {
    font-size:17px;font-style:italic;font-weight:bold;
    color:#336699;margin:20px 0 8px 0;
  }
  table {width:100%;border-collapse:collapse;margin-bottom:15px;}
  td.label {
    color:#336699;font-size:13px;padding:8px 5px;width:30%;
    border-bottom:1px solid #ddd;text-align:left;vertical-align:middle;
  }
  td.value {
    background:#e6f0ff;font-size:13px;padding:8px 5px;
    width:100px;border-bottom:1px solid #ddd;text-align:center;
    vertical-align:middle;
  }
  td.control {
    font-size:13px;padding:8px 5px;width:auto;
    border-bottom:1px solid #ddd;text-align:right;vertical-align:middle;
  }
  .slider {width:150px;height:28px;}
  .switch {
    position:relative;display:inline-block;width:50px;height:28px;
    vertical-align:middle;
  }
  .switch input {opacity:0;width:0;height:0;}
  .slider-switch {
    position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
    background:#ccc;transition:.4s;border-radius:28px;
  }
  .slider-switch:before {
    position:absolute;content:"";height:20px;width:20px;
    left:4px;bottom:4px;background:white;transition:.4s;
    border-radius:50%;
  }
  input:checked + .slider-switch {background:#336699;}
  input:checked + .slider-switch:before {transform:translateX(22px);}

  /* Responsive voor mobiel < 600px */
  @media (max-width: 600px) {
    .container {flex-direction:column;}
    .sidebar {
      width:100%;border-right:none;border-bottom:3px solid #cc0000;
      padding:10px 0;display:flex;justify-content:center;
    }
    .sidebar a {width:80px;margin:0 5px;}
    .main {padding:10px;}
    .group-title {font-size:16px;margin:15px 0 6px 0;}
    td.label {font-size:12px;padding:6px 4px;width:40%;}
    td.value {font-size:12px;padding:6px 4px;width:auto;}
    td.control {padding:6px 4px;}
    .slider {width:100%;max-width:200px;}
    table {font-size:12px;}
  }
  </style>


</head>
<body>
  <div class="header">
    <div class="header-left">Testroom</div>
    <div class="header-right">
      )rawliteral";
    html += String(uptime_sec) + " s &nbsp;&nbsp; " + getFormattedDateTime();  // Pas datum aan (NTP)
    html += R"rawliteral(
    </div>
  </div>
  <div class="container">

    <div class="sidebar">
      <a href="/" class="active">Status</a>
      <a href="/update">OTA</a>
      <a href="/status.json">JSON</a>
    </div>



    <div class="main">


  <div class="group-title">HVAC</div>
  <table>
    <tr><td class="label">Room temp</td>
    <td class="value">)rawliteral" + String(room_temp, 1) + " °C<br><small>(" + String(temp_dht, 1) + ", " + String(temp_ds, 1) + ")</small>" + R"rawliteral(</td>
    <td class="control"></td></tr>
    <tr><td class="label">Humidity</td><td class="value">)rawliteral" + String(humi, 1) + " %" + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">Dauwpunt</td><td class="value">)rawliteral" + String(dew, 1) + " °C" + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">DewAlert</td><td class="value">)rawliteral" + String(dew_alert ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">CO₂ ppm</td><td class="value">)rawliteral" + String(co2) + " ppm" + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">Stof (Dust)</td><td class="value">)rawliteral" + String(dust) + R"rawliteral(</td><td class="control"></td></tr>

    <tr><td class="label">Heating setpoint</td><td class="value">)rawliteral" + String(heating_setpoint) + " °C" + R"rawliteral(</td>
      <td class="control"><form action="/set_setpoint" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="setpoint" min="10" max="30" value=")rawliteral" + String(heating_setpoint) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
    <tr><td class="label">Heating Auto</td><td class="value">)rawliteral" + String(heating_mode == 0 ? "AUTO" : "MANUEEL") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_heating_auto" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (heating_mode == 0 ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>

    <tr><td class="label">Ventilatie %</td><td class="value">)rawliteral" + String(vent_percent) + " %" + R"rawliteral(</td>
      <td class="control"><form action="/set_vent" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="vent" min="0" max="100" value=")rawliteral" + String(vent_percent) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
    <tr><td class="label">Vent Auto</td><td class="value">)rawliteral" + String(vent_mode == 0 ? "AUTO" : "MANUEEL") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_vent_auto" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (vent_mode == 0 ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>

    <tr><td class="label">Heating aan</td><td class="value">)rawliteral" + String(heating_on ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
  </table>



  <div class="group-title">VERLICHTING</div>
  <table>
    <tr><td class="label">Zonlicht (TSL2561)</td><td class="value">)rawliteral" + String(sun_light) + " lux" + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">LDR (donker=100)</td><td class="value">)rawliteral" + String(light_ldr) + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">Night mode</td><td class="value">)rawliteral" + String(night ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">MOV1 licht aan</td><td class="value">)rawliteral" + String(mov1_light ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">MOV2 licht aan</td><td class="value">)rawliteral" + String(mov2_light ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">NeoPixel RGB</td><td class="value">)rawliteral" + String(neo_r) + ", " + String(neo_g) + ", " + String(neo_b) + R"rawliteral(</td>
      <td class="control"><a href="/neopixel" style="color:#336699;text-decoration:underline;">Kleur kiezen</a></td></tr>
    <tr><td class="label">Bed switch</td><td class="value">)rawliteral" + String(bed ? "AAN" : "UIT") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_bed" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (bed ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>
    <tr><td class="label">Dim snelheid (s)</td><td class="value">)rawliteral" + String(fade_duration) + R"rawliteral(</td>
      <td class="control"><form action="/set_fade_duration" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="duration" min="1" max="10" value=")rawliteral" + String(fade_duration) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
)rawliteral";

    // Dynamische pixels loop
    for (int i = 0; i < NEOPIXEL_NUM; i++) {
      String label = "Pixel " + String(i);
      if (i < 2) label += " (MOV)";
      String value = pixel_on[i] ? "On" : "Off";
      String checked = pixel_on[i] ? "checked" : "";
      String action = (i < 2) ? "/toggle_pixel_mode" + String(i) : "/toggle_pixel" + String(i);  // Verschil AUTO/ON vs ON/OFF
      html += "<tr><td class=\"label\">" + label + "</td><td class=\"value\">" + value + "</td>";
      html += "<td class=\"control\"><form action=\"" + action + "\" method=\"get\" onsubmit=\"event.preventDefault(); submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" " + checked + " onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>";
    }



    html += R"rawliteral(
      </table>

      <div class="group-title">BEWEGING</div>
      <table>
        <tr><td class="label">MOV1 triggers/min</td><td class="value">)rawliteral" + String(mov1_triggers) + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">MOV2 triggers/min</td><td class="value">)rawliteral" + String(mov2_triggers) + R"rawliteral(</td><td class="control"></td></tr>
      </table>

      <div class="group-title">BEWAKING</div>
      <table>
        <tr><td class="label">Beam waarde (0-100)</td><td class="value">)rawliteral" + String(beam_value) + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">Beam alert</td><td class="value">)rawliteral" + String(beam_alert_new ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>      
      </table>

      <div class="group-title">CONTROLLER</div>
      <table>
        <tr><td class="label">WiFi RSSI</td><td class="value">)rawliteral" + String(WiFi.RSSI()) + " dBm" + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">WiFi kwaliteit</td><td class="value">)rawliteral" + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + " %" + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">Free heap</td><td class="value">)rawliteral" + String((ESP.getFreeHeap() * 100) / ESP.getHeapSize()) + " %" + R"rawliteral(</td><td class="control"></td></tr>
      

      </table>

        <div style="text-align:center; margin:20px 0;">
          <button class="button" onclick="updateValues()">Refresh Data</button>
      </div>

      <p style="font-size:12px; color:#666; text-align:center; margin-top:10px;">
        Tip: Op iPhone kun je ook naar beneden swipen om te verversen.
      </p>

    </div>
  </div>


<script>
  // Live update zonder volledige reload
  function updateValues() {
    fetch('/status.json', {
      method: 'GET',
      cache: 'no-store'
    })
      .then(response => response.json())
      .then(data => {
        // Update alle waarden
        document.querySelectorAll('td.value').forEach(td => {
          const labelTd = td.previousElementSibling;
          if (!labelTd) return;
          const label = labelTd.textContent.trim();

          if (label.includes("Room temp")) {
            td.innerHTML = data.h.toFixed(1) + " °C<br><small>(" + data.g.toFixed(1) + ", " + data.h.toFixed(1) + ")</small>";
          } else if (label.includes("Humidity")) {
            td.textContent = Math.round(data.d) + " %";
          } else if (label.includes("Dauwpunt")) {
            td.textContent = data.c.toFixed(1) + " °C";
          } else if (label.includes("DewAlert")) {
            td.textContent = data.k ? "JA" : "NEE";
          } else if (label.includes("CO₂ ppm")) {
            td.textContent = data.a + " ppm";
          } else if (label.includes("Stof")) {
            td.textContent = data.b;
          } else if (label.includes("Heating setpoint")) {
            td.textContent = data.aa + " °C";
          } else if (label.includes("Ventilatie %")) {
            td.textContent = data.z + " %";
          } else if (label.includes("HVAC Auto")) {
            td.textContent = data.af == 0 ? "AUTO" : "MANUEEL";
          } else if (label.includes("Heating aan")) {
            td.textContent = data.y ? "JA" : "NEE";
          } else if (label.includes("Zonlicht")) {
            td.textContent = data.f + " lux";
          } else if (label.includes("LDR")) {
            td.textContent = data.e;
          } else if (label.includes("Night mode")) {
            td.textContent = data.q ? "JA" : "NEE";
          } else if (label.includes("MOV1 licht aan")) {
            td.textContent = data.m ? "JA" : "NEE";
          } else if (label.includes("MOV2 licht aan")) {
            td.textContent = data.n ? "JA" : "NEE";
          } else if (label.includes("NeoPixel RGB")) {
            td.textContent = data.s + ", " + data.t + ", " + data.u;
          } else if (label.includes("Bed switch")) {
            td.textContent = data.r ? "AAN" : "UIT";
          } else if (label.includes("Dim snelheid (s)")) {
            td.textContent = data.ab;
          } else if (label.includes("MOV1 triggers/min")) {
            td.textContent = data.i;
          } else if (label.includes("MOV2 triggers/min")) {
            td.textContent = data.j;
          } else if (label.includes("Beam waarde")) {
            td.textContent = data.o;
          } else if (label.includes("Beam alert")) {
            td.textContent = data.p ? "JA" : "NEE";
          } else if (label.includes("WiFi RSSI")) {
            td.textContent = data.v + " dBm";
          } else if (label.includes("WiFi kwaliteit")) {
            td.textContent = data.w + " %";
          } else if (label.includes("Free heap")) {
            td.textContent = data.x + " %";
          } else if (label.includes("Pixel")) {
            const pixelIdx = parseInt(label.match(/\d+/)[0]);
            td.textContent = data.ad[pixelIdx] === '1' ? "On" : "Off";
          }
        });

        // Update HVAC Auto toggle (checked = AUTO)
        const hvacToggle = document.querySelector('td.control form[action="/toggle_hvac_auto"] input[type="checkbox"]');
        if (hvacToggle) {
          hvacToggle.checked = (data.af == 0);
        }

        // Update pixel toggles (checked = manueel)
        document.querySelectorAll('td.control form[action^="/toggle_pixel_mode"] input[type="checkbox"]').forEach((checkbox, idx) => {
          checkbox.checked = (data.ae.charAt(idx) === '1');
        });

        // Update header uptime + tijd
        const headerRight = document.querySelector('.header-right');
        if (headerRight) {
          const now = new Date();
          const timeStr = now.toLocaleDateString('nl-BE') + ' ' + now.toLocaleTimeString('nl-BE');
          headerRight.innerHTML = data.ac + " s &nbsp;&nbsp; " + timeStr;
        }
      })
      .catch(err => console.error('Update error:', err));
  }

  function submitAjax(form) {
    const params = new URLSearchParams();
    for (const element of form.elements) {
      if (element.name && element.value !== undefined) {
        params.append(element.name, element.value);
      }
    }
    const queryString = params.toString();
    const url = queryString ? form.action + '?' + queryString : form.action;

    fetch(url, { method: 'GET' })
      .then(() => updateValues())
      .catch(err => console.error('Submit error:', err));
  }

  window.addEventListener('load', () => {
    updateValues();
    setInterval(updateValues, 3000);
  });
</script>



</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });



  // === JSON ENDPOINT ===
  server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "application/json", getJSON());
  });



  // === OTA UPDATE PAGE ===
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
      String html;
    html.reserve(4000);
    html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>OTA Update</title>
  <style>
    body {font-family:Arial,Helvetica,sans-serif;background:#ffffff;margin:0;padding:0;}
    .header {
      display:flex;background:#ffcc00;color:black;padding:10px 20px;
      font-size:18px;font-weight:bold;align-items:center;
    }
    .header-left {flex:1;text-align:left;}
    .header-right {flex:1;text-align:right;font-size:16px;}
    .container {display:flex;min-height:calc(100vh - 60px);}
    .sidebar {
      width:120px;padding:20px 10px;background:#ffffff;
      border-right:3px solid #cc0000;box-sizing:border-box;
    }
    .sidebar a {
      display:block;background:#336699;color:white;padding:10px;
      margin:10px 0;text-decoration:none;font-weight:bold;
      font-size:14px;border-radius:8px;text-align:center;
      line-height:1.4;width:70px;box-sizing:border-box;
    }
    .sidebar a:hover {background:#003366;}
    .sidebar a.active {background:#cc0000;}
    .main {flex:1;padding:40px;text-align:center;}
    .button {
      background:#336699;color:white;padding:12px 24px;border:none;
      border-radius:8px;cursor:pointer;font-size:16px;margin:10px;
    }
    .button:hover {background:#003366;}
    .reboot {background:#cc0000;}
    .reboot:hover {background:#990000;}
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">Testroom</div>
    <div class="header-right">
      )rawliteral" + String(uptime_sec) + " s &nbsp;&nbsp; " + getFormattedDateTime() + R"rawliteral(
    </div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status &<br>Control</a>
      <a href="/update" class="active">OTA<br>Update</a>
      <a href="/status.json">JSON<br>Data</a>
    </div>
    <div class="main">
      <h1 style="color:#336699;">OTA Firmware Update</h1>
      <p>Selecteer een .bin bestand</p>
      <form method="POST" action="/update" enctype="multipart/form-data">
        <input type="file" name="update" accept=".bin"><br><br>
        <button class="button" type="submit">Upload Firmware</button>
      </form>
      <br>
      <button class="button reboot" onclick="location.href='/reboot'">Reboot</button>
      <br><br><a href="/" style="color:#336699;text-decoration:underline;">← Terug naar Status</a>
    </div>
  </div>
</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });



  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool success = !Update.hasError();
    request->send(200, "text/html", success 
      ? "<h2 style='color:#0f0'>Update succesvol!</h2><p>Rebooting...</p>" 
      : "<h2 style='color:#f00'>Update mislukt!</h2><p>Probeer opnieuw.</p><a href='/update'>Terug</a>");
    if (success) { delay(1000); ESP.restart(); }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.println("\n=== OTA UPDATE GESTART ===");
      Serial.printf("Bestand: %s (%u bytes)\n", filename.c_str(), request->contentLength());
      Update.begin(UPDATE_SIZE_UNKNOWN);
    }
    size_t written = Update.write(data, len);
    if (written != len) Serial.printf("Fout bij schrijven: %u/%u\n", written, len);
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      lastPrint = millis();
      Serial.printf("Geüpload: %u/%u bytes\n", index + len, request->contentLength());
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("OTA succesvol: %u bytes\n", index + len);
        Serial.println("Rebooting...\n");
      } else {
        Serial.println("OTA fout: " + String(Update.errorString()));
      }
    }
  });

  // === REBOOT ===
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h2>Rebooting ESP32...</h2>");
    delay(500);
    ESP.restart();
  });


  // Toggle voor Bed (r)
  server.on("/toggle_bed", HTTP_GET, [](AsyncWebServerRequest *request) {
    bed = !bed;  // Toggle 0/1
    request->send(200, "text/plain", "OK");
  });


  // Toggles voor pixels 0-1 (mode AUTO/ON) en 2-7 (on/off)
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    String path = (i < 2) ? "/toggle_pixel_mode" + String(i) : "/toggle_pixel" + String(i);
    server.on(path.c_str(), HTTP_GET, [i](AsyncWebServerRequest *request) {  // Capture i
      if (i < 2) {
        pixel_mode[i] = 1 - pixel_mode[i];  // Toggle 0/1 (AUTO/ON)
      } else {
        pixel_on[i] = !pixel_on[i];  // Toggle on/off
      }
      request->send(200, "text/plain", "OK");
    });
  }


  // === NEOPIXEL KLEURKIEZER PAGE ===
  server.on("/neopixel", HTTP_GET, [](AsyncWebServerRequest *request) {
  String html;
  html.reserve(4000);
  html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>NeoPixel Kleur</title>
  <style>
    body {font-family:Arial,Helvetica,sans-serif;background:#ffffff;margin:0;padding:0;}
    .header {
      display:flex;background:#ffcc00;color:black;padding:10px 20px;
      font-size:18px;font-weight:bold;align-items:center;
    }
    .header-left {flex:1;text-align:left;}
    .header-right {flex:1;text-align:right;font-size:16px;}
    .container {display:flex;min-height:calc(100vh - 60px);}
    .sidebar {
      width:120px;padding:20px 10px;background:#ffffff;
      border-right:3px solid #cc0000;box-sizing:border-box;
    }
    .sidebar a {
      display:block;background:#336699;color:white;padding:10px;
      margin:10px 0;text-decoration:none;font-weight:bold;
      font-size:14px;border-radius:8px;text-align:center;
      line-height:1.4;width:70px;box-sizing:border-box;
    }
    .sidebar a:hover {background:#003366;}
    .sidebar a.active {background:#cc0000;}
    .main {flex:1;padding:40px;text-align:center;}
    input[type=range] {width:80%;height:30px;}
    .button {
      background:#336699;color:white;padding:12px 24px;border:none;
      border-radius:8px;cursor:pointer;font-size:16px;margin-top:20px;
    }
    .button:hover {background:#003366;}
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">Testroom</div>
    <div class="header-right">
      )rawliteral" + String(uptime_sec) + " s &nbsp;&nbsp; " + getFormattedDateTime() + R"rawliteral(
    </div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status &<br>Control</a>
      <a href="/update">OTA<br>Update</a>
      <a href="/status.json">JSON<br>Data</a>
    </div>
    <div class="main">
      <h1 style="color:#336699;">NeoPixel Kleur Instellen</h1>
      <form action="/setcolor" method="get“ onsubmit="event.preventDefault(); submitAjax(this);">
        <p>R: <input type="range" name="r" min="0" max="255" value=")rawliteral" + String(neo_r) + R"rawliteral("><br><br></p>
        <p>G: <input type="range" name="g" min="0" max="255" value=")rawliteral" + String(neo_g) + R"rawliteral("><br><br></p>
        <p>B: <input type="range" name="b" min="0" max="255" value=")rawliteral" + String(neo_b) + R"rawliteral("><br><br></p>
        <button class="button" type="submit">Kleur instellen</button>
      </form>
      <br><br><a href="/" style="color:#336699;text-decoration:underline;">← Terug naar Status</a>
    </div>
  </div>
</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });



  // === SET COLOR HANDLER ===
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("r")) neo_r = request->getParam("r")->value().toInt();
    if (request->hasParam("g")) neo_g = request->getParam("g")->value().toInt();
    if (request->hasParam("b")) neo_b = request->getParam("b")->value().toInt();

  // Beperk waarden tot 0-255
  neo_r = constrain(neo_r, 0, 255);
  neo_g = constrain(neo_g, 0, 255);
  neo_b = constrain(neo_b, 0, 255);
  request->send(200, "text/plain", "OK");
  });


  // Handler voor Instelbare fade duration (slider)
  server.on("/set_fade_duration", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("duration")) {
      fade_duration = request->getParam("duration")->value().toInt();
      fade_duration = constrain(fade_duration, 1, 10);
      updateFadeInterval();  // Herbereken de fade-interval direct
    }
    request->send(200, "text/plain", "OK");
  });



  // Slider Heating setpoint (wijzig alleen setpoint, geen mode-switch)
  server.on("/set_setpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("setpoint")) {
      heating_setpoint = request->getParam("setpoint")->value().toInt();
      heating_setpoint = constrain(heating_setpoint, 10, 30);
    }
    request->send(200, "text/plain", "OK");
  });


  // Slider Ventilation % (gebruik zet MANUEEL)
  server.on("/set_vent", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("vent")) {
      vent_percent = request->getParam("vent")->value().toInt();
      vent_percent = constrain(vent_percent, 0, 100);
      vent_mode = 1;  // Slider gebruik → MANUEEL voor ventilation
    }
    request->send(200, "text/plain", "OK");
  });


  // Toggle Heating AUTO/MANUEEL (aparte toggle)
  server.on("/toggle_heating_auto", HTTP_GET, [](AsyncWebServerRequest *request) {
    heating_mode = 1 - heating_mode;  // Schakelt tussen 0 (AUTO) en 1 (MANUEEL)
    request->send(200, "text/plain", "OK");
  });

  // Toggle Ventilation AUTO/MANUEEL (aparte toggle)
  server.on("/toggle_vent_auto", HTTP_GET, [](AsyncWebServerRequest *request) {
    vent_mode = 1 - vent_mode;  // Schakelt tussen 0 (AUTO) en 1 (MANUEEL)
    request->send(200, "text/plain", "OK");
  });



  // Afsluiting webserver (alle handlers hiervoor!)
  server.begin();
  Serial.println("HTTP server gestart – http://testroom.local");
}

unsigned long lastSerial = 0;
unsigned long last_slow = 0;





void loop() {

  updateFades();

  // PIR op 3.3V: beweging = LOW → rising edge van HIGH naar LOW
  static bool last1 = HIGH, last2 = HIGH;
  bool p1 = digitalRead(PIR_MOV1);
  bool p2 = digitalRead(PIR_MOV2);

  if (!p1 && last1) { 
    mov1_off_time = millis() + LIGHT_ON_DURATION; 
    pushEvent(mov1Times, MOV_BUF_SIZE); 
  }
  if (!p2 && last2) { 
    mov2_off_time = millis() + LIGHT_ON_DURATION; 
    pushEvent(mov2Times, MOV_BUF_SIZE); 
  }

  last1 = p1; last2 = p2;


  // NeoPixel aansturing per pixel
  for (int i = 0; i < NEOPIXEL_NUM; i++) {
    if (i < 2) {  // Pixel 0 en 1: speciale logica
      if (bed == 1) {
        // Bed AAN: alles uit, geen PIR, reset mode naar AUTO
        setTargetColor(i, 0, 0, 0);
        pixel_mode[i] = 0;  // Reset naar AUTO
        pixel_on[i] = false;  // Expliciet updaten voor UI/JSON
        if (i == 0) mov1_light = 0;
        if (i == 1) mov2_light = 0;
      } else {  // Bed UIT: normale logica (altijd AUTO na reset)
        if (pixel_mode[i] == 1) {  // MANUEEL ON: gebruik RGB, altijd aan, geen timer
          setTargetColor(i, neo_r, neo_g, neo_b);
          pixel_on[i] = true;
          if (i == 0) mov1_light = 1;
          if (i == 1) mov2_light = 1;
        } else {  // AUTO: PIR logica
          bool dark = (light_ldr > LDR_DARK_THRESHOLD);
          bool movement = (millis() < (i == 0 ? mov1_off_time : mov2_off_time));
          bool l = dark && movement;
          setTargetColor(i, 0, l ? 220 : 0, 0);  // Groen bij beweging
          pixel_on[i] = l;
          if (i == 0) mov1_light = l;
          if (i == 1) mov2_light = l;
        }
      }
    } else {  // Pixels 2-7: simpel manueel ON/OFF met RGB
      if (pixel_on[i]) {
        setTargetColor(i, neo_r, neo_g, neo_b);
      } else {
        setTargetColor(i, 0, 0, 0);
      }
    }
  }



  if (millis() - last_slow < 2000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;



  // Sensoren
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  ds18b20.requestTemperatures(); temp_ds = ds18b20.getTempCByIndex(0);
  sensors_event_t e; tsl.getEvent(&e); sun_light = (int)e.light;
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));
  dust = readDust();
  co2 = readCO2();
  tstat_on = !digitalRead(TSTAT_PIN);

  dew_alert = (temp_ds < dew) ? 1 : 0;  
  night = (light_ldr > 50) ? 1 : 0;  
  // bed blijft voorlopig hardcoded 0, tot toggle in webserver  
  beam_value = map(analogRead(OPTION_LDR), 0, 4095, 0, 100);  // Schaal ruwe ADC naar 0-100 voor o  
  beam_alert_new = (beam_value > 50) ? 1 : 0;  // Voor p, vervangt oude beam_alert


  // Room temp logica: primair temp_ds (Temp2), backup temp_dht (Temp1)
  room_temp = temp_ds;
  temp_melding = "";
  if (isnan(temp_ds) || temp_ds < 5.0 || temp_ds > 40.0) {  // Falen detectie
    room_temp = temp_dht;
    temp_melding = "DS18B20 defect – DHT22 gebruikt";
    if (isnan(temp_dht) || temp_dht < 5.0 || temp_dht > 40.0) {
      room_temp = 0.0;
      temp_melding = "Beide temp sensoren defect!";
    }
  }



  // Heating logica (slider wijzigt setpoint, geen mode-switch)
  if (heating_mode == 0) {  // AUTO
    heating_on = (room_temp < (heating_setpoint - 0.5)) ? 1 : 0;  // Hysteresis
  } else {  // MANUEEL
    heating_on = 1;  // Altijd aan
  }

  // Ventilation logica (slider zet mode = 1)
  if (vent_mode == 0) {  // AUTO
    vent_percent = map(constrain(co2, 400, 800), 400, 800, 0, 100);
  }  // MANUEEL: vent_percent = slider-waarde (gezet in handler)



  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
  mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);

  // Serial rapport (identiek)
  if (millis() - lastSerial > 3000) {
    lastSerial = millis();

    Serial.println("\nTESTROOM – " + String(uptime_sec) + " s");
    Serial.println("─────────────────────────────────────");
    Serial.printf("DHT22 Temp2          : %.2f °C\n", temp_dht);
    Serial.printf("DHT22 Humidity       : %.1f %%\n", humi);
    Serial.printf("Dauwpunt             : %.1f °C\n", dew);
    Serial.printf("DewAlert (Temp2<Dew) : %s\n", dew_alert ? "JA" : "NEE");
    Serial.printf("DS18B20 Temp1        : %.2f °C\n", temp_ds);
    Serial.printf("Room temp            : %.1f °C %s\n", room_temp, temp_melding.c_str());
    Serial.printf("Heating setpoint     : %d °C\n", heating_setpoint);
    Serial.printf("Heating mode         : %s\n", heating_mode == 0 ? "AUTO" : "MANUEEL");
    Serial.printf("Heating aan          : %s\n", heating_on ? "JA" : "NEE");
    Serial.printf("TSTAT aan (wired)    : %s\n", tstat_on ? "JA" : "NEE");
    Serial.printf("Stof (Sharp)         : %d\n", dust);
    Serial.printf("CO₂ ppm              : %d ppm\n", co2);
    Serial.printf("Ventilatie %         : %d %%\n", vent_percent);
    Serial.printf("Ventilation mode     : %s\n", vent_mode == 0 ? "AUTO" : "MANUEEL");
    Serial.printf("Zonlicht (TSL2561)   : %d lux\n", sun_light);
    Serial.printf("LDR (donker=100)     : %d\n", light_ldr);
    Serial.printf("MOV1 triggers/min    : %d\n", mov1_triggers);
    Serial.printf("MOV2 triggers/min    : %d\n", mov2_triggers);
    Serial.printf("MOV1 licht aan       : %s\n", mov1_light ? "JA" : "NEE");
    Serial.printf("MOV2 licht aan       : %s\n", mov2_light ? "JA" : "NEE");
    Serial.printf("Night mode (donker)  : %s\n", night ? "JA" : "NEE");
    Serial.printf("Bed switch           : %s\n", bed ? "AAN" : "UIT");
    Serial.printf("Beam waarde (0-100)  : %d\n", beam_value);
    Serial.printf("Beam alert (donker)  : %s\n", beam_alert_new ? "JA" : "NEE");
    Serial.printf("NeoPixel RGB         : %d, %d, %d\n", neo_r, neo_g, neo_b);
    Serial.printf("Dim snelheid (s)     : %d s\n", fade_duration);
    Serial.printf("Fade interval        : %lu ms\n", fade_interval_ms);  // Debug toegevoegd
    Serial.print("Pixel modes (0-1)    : ");
    Serial.printf("%d, %d\n", pixel_mode[0], pixel_mode[1]);
    Serial.print("Pixels on (0-7)      : ");
      for (int i = 0; i < NEOPIXEL_NUM; i++) {
        Serial.print(pixel_on[i] ? "1" : "0");
        if (i < 7) Serial.print(", ");
      }
    Serial.println();
    Serial.printf("WiFi RSSI            : %d dBm\n", WiFi.RSSI());
    Serial.printf("WiFi kwaliteit       : %d %%\n", constrain(2 * (WiFi.RSSI() + 100), 0, 100));
    Serial.printf("Free heap            : %d %%\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    Serial.println("─────────────────────────────────────\n");
  }
}

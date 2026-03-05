// ESP32-C6_ROOM.ino = Photon based distributed Home automation system, converted to ESP32C6 controllers.
// Developed by Filip Delannoy in december '25.
// Bereikbaar op (bijvb) http://eetplaats.local of http://192.168.0.80 => Andere controller: Naam (sectie DNS/MDNS) + static IP aanpassen!

// 05mar26 20:30 v. 2.1 Verwarmingslogica verbeterd volgens onze verwachtingen..
// 05mar26 19:30 v. 2.0 Teruggedraaid: DS18B20 async + esp_int_wdt_deinit verwijderd (veroorzaakten instabiliteit). Enkel echte fixes behouden: CO2/dust guards, serial interval, heap monitoring, MAC in settings.
// 05mar26 18:30 v. 1.9 WDT fix: dust guard (default false), yield() na OneWire, JSON labels ag/ah/ai/aj.
// 05mar26 18:00 v. 1.8 Serial interval instelbaar in /settings (5-30s), direct actief. Heap monitoring v1.6 hersteld (was verloren bij v1.7 rebase).
// 05mar26 17:30 v. 1.7 WDT-crash fixes: DS18B20 async (geen delay(750) meer), CO2 read bewaakt met if(co2_enabled). Default co2_enabled=false.
// 05mar26 16:00 v. 1.6 Heap monitoring op statuspagina: largest free block + kleurcode. /json uitgebreid met heap_largest + heap_min_ever.
// 05mar26 12:00 v. 1.5 Lux meting in orde gemaakt: I2C pins gewijzigd naar de voorziene pins. Geen errors meer in serial.
// 04mar26 12:00 v. 1.4 Lichter gemaakt en vereenvoudigd om heap size maximaal te maken voor matter integratie. (25 => 67% over!)
// 27feb26 17:30 v. 1.3 C6 compatibel: OneWireNg, pin updates, multi DS18B20 discovery + rescan + /config page expanded & simplified textboxes (Claude)
// 26feb26 19:00 v. 1.2 Fixed IP geintroduceerd. Set zoals in tabel op google drive: vb: EETPL	(Mac = 58:8C:81:32:2F:48)	=> IP = 192.168.0.80
// 21dec25 23:00 v. 1.1 Pixel nicknames werken VOLLEDIG in /settings en in / (hoofdpagina)! Ga terug naar deze versie als je vastloopt!
// 22dec25 18:00 Captive portal geimplementeerd en gans factory reset proces verbeterd! Thuis getest, werkt nog niet.
// 02jan26 21:00 Pixels persistent gemaakt! (voor Mireille) De UI labels van pixel 0 & 1 worden niet geupdated, tenzij ze refreshed worden! Noch ChatGPT noch Grok slaagden erin dit betrouwbaar op te lossen zonder nevenschade. Laat dit zo!
// 12jan26 20:00 Endpoint voor JSON string veranderd van /status.json => /json zoals de andere controllers.
// 13jan26 20:00 MAC address toegevoegd om Static IP adres in router te kunnen vastleggen.

// Volgende opdrachten voor Grok of chatGPT: 
//                1) Nicknames voor sensors die in Matter gebruikt worden: Standaard = Roomname+Sensor, Option: Make own nickname. (zoals de pixels)


#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>        // Toegevoegd om captive portal toe te voegen
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>           // Voor OTA update
#include <DHT.h>
#include <OneWireNg_CurrentPlatform.h>  // C6 compatibel (vervangt OneWire + DallasTemperature)
#include <Adafruit_TSL2561_U.h>

#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>            // Voor sin() in dimmer engine
#include <Preferences.h>     // NVS library voor Preferences
#include <string.h>          // Voor memset()
Preferences preferences;     // Globale Preferences instantie

#define Serial Serial0  // Fix: ESP32-C6 gebruikt Serial0 als hardware serial

// ============== PIN DEFINITIONS (ESP32-C6 via Photon Shield) ==============
#define DHT_PIN        6   // IO6  - DHT22 data         (was GPIO18)
#define ONE_WIRE_PIN   3   // IO3  - DS18B20 OneWire     (was GPIO4)
#define PIR_MOV1       5   // IO5  - MOV1 PIR            (was GPIO17)
#define PIR_MOV2      19   // IO19 - MOV2 PIR            (was GPIO26)
#define SHARP_LED     12   // IO12 - Sharp dust LED out  (was GPIO19)
#define SHARP_ANALOG   7   // IO7  - Sharp dust analog   (was GPIO32)
#define LDR_ANALOG     1   // IO1  - LDR1 analog         (was GPIO33, 10k pull-up naar 3V3!)
#define CO2_PWM       18   // IO18 - CO2 PWM input       (was GPIO25)
#define TSTAT_PIN     10   // IO10 - TSTAT switch        (was GPIO27)
#define OPTION_LDR     2   // IO2  - LDR2 analog         (was GPIO14)
#define NEOPIXEL_PIN   4   // IO4  - Pixels data         (was GPIO16)


// NVS keys (const voor netheid en veiligheid)
const char* NVS_ROOM_ID             = "room_id";
const char* NVS_WIFI_SSID           = "wifi_ssid";
const char* NVS_WIFI_PASS           = "wifi_password";
const char* NVS_STATIC_IP           = "static_ip";
const char* NVS_HEATING_SETPOINT    = "heat_setpoint";
const char* NVS_VENT_REQUEST        = "vent_request";
const char* NVS_DEW_MARGIN          = "dew_margin";
const char* NVS_HOME_MODE           = "home_mode";
const char* NVS_LIGHT_THRESHOLD     = "light_thresh";
const char* NVS_MOV_WINDOW          = "mov_window";
const char* NVS_LDR_DARK            = "ldr_dark";
const char* NVS_BEAM_THRESHOLD      = "beam_thresh";
const char* NVS_CO2_ENABLED         = "co2_en";
const char* NVS_DUST_ENABLED        = "dust_en";
const char* NVS_SUN_ENABLED         = "sun_en";
const char* NVS_MOV2_ENABLED        = "mov2_en";
const char* NVS_TSTAT_ENABLED       = "tstat_en";
const char* NVS_BEAM_ENABLED        = "beam_en";
const char* NVS_NEO_R               = "neo_r";
const char* NVS_NEO_G               = "neo_g";
const char* NVS_NEO_B               = "neo_b";
const char* NVS_PIXELS_NUM          = "pixels_num";
const char* NVS_BED_STATE           = "bed_state";       // bool: bed AAN/UIT
const char* NVS_SERIAL_VERBOSE      = "serial_verbose";  // bool: statusrapport aan/uit
const char* NVS_SERIAL_INTERVAL     = "serial_intv";     // int: interval serial rapport in seconden
const char* NVS_CURRENT_SETPOINT    = "curr_setpoint";   // int: huidige gekozen temperatuur
const char* NVS_FADE_DURATION       = "fade_duration";   // int: dim-snelheid in seconden (1-10)
const char* NVS_HOME_MODE_STATE     = "home_mode_state"; // int: 0 = Uit, 1 = Thuis
const char* NVS_PIXEL_NICK_BASE     = "pixel_nick_";     // Pixel nicknames: keys "pixel_nick_0" t/m "pixel_nick_29"
const char* NVS_PIXEL_ON_BASE       = "pixel_on_";       // Voor pixel_on[0..29]
const char* NVS_PIXEL_USER_ON_BASE  = "pixel_user_on_";  // Voor pixel_user_on[0..29]
const char* NVS_PIXEL_MODE_0        = "pixel_mode_0";    // AUTO/MANUEEL voor MOV1
const char* NVS_PIXEL_MODE_1        = "pixel_mode_1";    // AUTO/MANUEEL voor MOV2 (alleen als mov2_enabled)
// DS18B20 multi-sensor (v1.3)
const char* NVS_DS_COUNT            = "ds_count";        // Aantal gevonden sensoren
const char* NVS_DS_PRIMARY          = "ds_primary";      // Index van primaire sensor (room_temp)



// Initialize libraries
DHT dht(DHT_PIN, DHT22);
OneWireNg_CurrentPlatform ow(ONE_WIRE_PIN, false);  // C6 compatibel 1-Wire
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);  // Tijdelijk 1, lengte wordt in setup() gezet
AsyncWebServer server(80);



// Room-specifieke instellingen (worden uit NVS geladen)
String room_id              = "Testroom";      // Default bij eerste flash
String mdns_name            = "Testroom";      // Identiek aan room_id
String wifi_ssid            = "netwerknaam";
String wifi_pass            = "paswoord";
String static_ip_str        = "192.168.xx.xx"; // Wordt omgezet naar IPAddress
String pixel_nicknames[30];                    // Runtime Array voor pixel nicknames
String mac_address          = "";              // Voor display in settings



// Configureerbare defaults
int heating_setpoint_default = 20;
int vent_request_default     = 0;
float dew_safety_margin      = 2.0;
int home_mode_default        = 0;              // 0 = Weg
int light_dark_threshold     = 50;
unsigned long mov_window_ms  = 60000;
int ldr_dark_threshold       = 50;
int beam_alert_threshold     = 50;



// Optionele feature enables (default 1 = aan)
bool co2_enabled   = false;  // v1.7: default false — voorkomt WDT crash bij niet-aangesloten sensor
bool dust_enabled  = false;  // v1.9: default false — delayMicroseconds(9680) kan WDT triggeren als sensor niet aanwezig
bool sun_light_enabled = true;
bool mov2_enabled  = true;
bool tstat_enabled = true;
bool beam_enabled  = true;
bool serial_verbose = true;   // Statusrapport elke 15s via serial
int  serial_interval = 15;   // v1.8: interval serial rapport in seconden (instelbaar in /settings, 5-30s)
int pixels_num     = 8;     // Default. Configureerbaar via NVS (1-30)
int num_mov_pixels = 2;     // Wordt in setup() aangepast op basis van mov2_enabled


// AP mode (Access Point)
bool ap_mode_active = false;  // Track of we in AP fallback zitten
DNSServer dnsServer;
const byte DNS_PORT = 53;


bool mdns_running = false;
wl_status_t last_wifi_status = WL_IDLE_STATUS;



// HVAC Variabelen
float room_temp = 0.0;      // Berekende kamertemp: primair temp_ds, backup temp_dht
String temp_melding = "";   // Melding bij defect (bijv. "DS18B20 defect – DHT22 gebruikt")
int heating_setpoint = 20;  // aa: Gewenste temp (integer, default 20)
int heating_on = 0;         // y: Verwarming aan (0/1, auto of manueel)
int vent_percent = 0;       // z: Ventilatie % (0-100, auto of manueel)
int heating_mode = 0;       // 0 = AUTO, 1 = MANUEEL voor heating
int vent_mode = 0;          // 0 = AUTO, 1 = MANUEEL voor ventilation (slider zet naar 1)
int home_mode = 1;          // 1 = Thuis (hardware thermostaat prioriteit), 0 = Uit (ESP regelt met anti-condens)


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

// DS18B20 multi-sensor (v1.3) - max 4 sensors op 1 bus
#define DS_MAX_SENSORS 4
int ds_count = 0;                          // Aantal gevonden sensoren
OneWireNg::Id ds_addrs[DS_MAX_SENSORS];    // 8-byte adressen
float temp_ds_arr[DS_MAX_SENSORS];         // Temperaturen per sensor
String ds_nicknames[DS_MAX_SENSORS];       // Nicknames per sensor
int ds_primary = 0;                        // Index van primaire sensor → room_temp


// Pixel specifieke arrays
int pixel_mode[2] = {0, 0};  // Voor pixel 0 en 1: 0 = AUTO (PIR), 1 = MANUEEL ON (RGB)
bool pixel_on[30] = {false}; // Voor alle pixels
bool pixel_user_on[30] = {false};   // Manuele intentie (persistent)


uint8_t currR[30], currG[30], currB[30];
uint8_t targetR[30], targetG[30], targetB[30];
uint8_t startR[30], startG[30], startB[30];
float fade_progress[30] = {0.0};


// PIR op 3.3V: beweging = LOW
unsigned long mov1_off_time = 0;
unsigned long mov2_off_time = 0;
const unsigned long LIGHT_ON_DURATION = 30000;
int LDR_DARK_THRESHOLD = 40;  // Wordt overschreven door NVS waarde
unsigned long MOV_WINDOW_MS = 60000;  // Wordt overschreven door NVS (in ms)


#define MOV_BUF_SIZE 50
unsigned long mov1Times[MOV_BUF_SIZE] = {0};
unsigned long mov2Times[MOV_BUF_SIZE] = {0};


// Pixel Fade engine
unsigned long lastFadeStep = 0;
unsigned long fade_interval_ms = 15;        // Dynamische interval, initieel 15 ms (wordt herberekend)
int fade_duration = 2;                      // Dimsnelheid in seconden (1-10, default 2)
const int FADE_NUM_STEPS = 20;              // 20 stappen = heel smooth, maar nog snel genoeg


void initFadeEngine() {
  for (int i = 0; i < pixels_num; i++) {
    uint32_t c = pixels.getPixelColor(i);
    currR[i] = (c >> 16) & 0xFF;
    currG[i] = (c >> 8)  & 0xFF;
    currB[i] = c & 0xFF;
    targetR[i] = currR[i]; targetG[i] = currG[i]; targetB[i] = currB[i];
    startR[i] = currR[i]; startG[i] = currG[i]; startB[i] = currB[i];  // Toegevoegd
  }
}


void setTargetColor(int idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < 0 || idx >= pixels_num) return;
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

  for (int i = 0; i < pixels_num; i++) {
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
    if (buf[i] == 0 || (now - buf[i] > MOV_WINDOW_MS)) { buf[i] = now; return; }
  }
  int oldest = 0; unsigned long old = buf[0];
  for (int i = 1; i < size; i++) if (buf[i] < old) { old = buf[i]; oldest = i; }
  buf[oldest] = now;
}

int countRecent(unsigned long *buf, int size) {
  unsigned long now = millis(); int c = 0;
  for (int i = 0; i < size; i++) if (buf[i] && (now - buf[i] <= MOV_WINDOW_MS)) c++;
  return c;
}

float calculateDewPoint(float t, float h) { return isnan(t) || isnan(h) ? 0 : t - ((100 - h) / 5.0); } // Berekening dauwpunt met DHT22 data

int scaleLDR(int r) { return map(constrain(r, 100, 3800), 100, 3800, 100, 0); }

int readDust() { digitalWrite(SHARP_LED, LOW); delayMicroseconds(280); int v = analogRead(SHARP_ANALOG); delayMicroseconds(40); digitalWrite(SHARP_LED, HIGH); delayMicroseconds(9680); return v; }

// ============== DS18B20 MULTI-SENSOR (v1.3) ==============

void scanDS18B20() {
  Serial.println("DS18B20: scanning 1-Wire bus...");
  ds_count = 0;
  for (int i = 0; i < DS_MAX_SENSORS; i++) temp_ds_arr[i] = 0.0;

  OneWireNg::Id id;
  ow.searchReset();
  while (ds_count < DS_MAX_SENSORS) {
    if (ow.search(id) != OneWireNg::EC_SUCCESS) break;
    if (id[0] != 0x28) continue;
    memcpy(ds_addrs[ds_count], id, 8);
    Serial.printf("  Sensor %d: ", ds_count + 1);
    for (int j = 0; j < 8; j++) Serial.printf("%02X ", id[j]);
    Serial.println();
    ds_count++;
  }
  Serial.printf("DS18B20: %d sensor(s) gevonden\n", ds_count);

  preferences.putInt(NVS_DS_COUNT, ds_count);
  for (int i = 0; i < ds_count; i++) {
    String akey = "ds_addr_" + String(i);
    preferences.putBytes(akey.c_str(), ds_addrs[i], 8);
    String nkey = "ds_nick_" + String(i);
    if (preferences.getString(nkey.c_str(), "").isEmpty()) {
      String defnick = room_id + " DS " + String(i + 1);
      preferences.putString(nkey.c_str(), defnick);
      ds_nicknames[i] = defnick;
    } else {
      ds_nicknames[i] = preferences.getString(nkey.c_str(), "DS " + String(i + 1));
    }
  }
  ds_primary = constrain(preferences.getInt(NVS_DS_PRIMARY, 0), 0, max(ds_count - 1, 0));
  preferences.putInt(NVS_DS_PRIMARY, ds_primary);
}

void loadDS18B20fromNVS() {
  ds_count = preferences.getInt(NVS_DS_COUNT, 0);
  ds_primary = constrain(preferences.getInt(NVS_DS_PRIMARY, 0), 0, max(ds_count - 1, 0));
  for (int i = 0; i < ds_count; i++) {
    String akey = "ds_addr_" + String(i);
    preferences.getBytes(akey.c_str(), ds_addrs[i], 8);
    String nkey = "ds_nick_" + String(i);
    ds_nicknames[i] = preferences.getString(nkey.c_str(), room_id + " DS " + String(i + 1));
    temp_ds_arr[i] = 0.0;
  }
}

void readDS18B20temps() {
  if (ds_count == 0) return;

  // Stap 1: Start conversie voor ALLE sensoren tegelijk via SKIP ROM (broadcast)
  // Slechts 1x interrupt-blokkering voor de conversie-opdracht i.p.v. N keer
  ow.reset();
  ow.writeByte(0xCC);  // SKIP ROM - alle sensoren tegelijk
  ow.writeByte(0x44);  // CONVERT T
  delay(750);          // Een wacht voor alle sensoren - FreeRTOS-friendly

  // Stap 2: Lees elke sensor individueel via MATCH ROM
  for (int i = 0; i < ds_count; i++) {
    ow.reset();
    ow.writeByte(0x55);  // MATCH ROM
    for (int j = 0; j < 8; j++) ow.writeByte(ds_addrs[i][j]);
    ow.writeByte(0xBE);  // READ SCRATCHPAD

    uint8_t data[9];
    for (int j = 0; j < 9; j++) data[j] = ow.touchByte(0xFF);

    int16_t raw = (int16_t)((data[1] << 8) | data[0]);
    float t = raw / 16.0f;
    if (t >= -55.0f && t <= 125.0f) temp_ds_arr[i] = t;
  }
  temp_ds = temp_ds_arr[ds_primary];
}

// ============== EINDE DS18B20 MULTI-SENSOR ==============

int readCO2() {
  unsigned long h = pulseIn(CO2_PWM, HIGH, 200000);  // Timeout 0.2s i.p.v. 0.1s (i.p.v. 2s vroeger: blocking!)
  unsigned long l = pulseIn(CO2_PWM, LOW, 200000);
  return (h < 100 || l < 100) ? 0 : (int)(5000.0 * (h - 2.0) / (h + l - 4.0));
}


void handleSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("reset_nvs")) {
      Serial.println("\n=== FACTORY RESET NVS UITGEVOERD ===");
      preferences.clear();
      Serial.println("NVS gewist – reboot...");
      delay(500);
      ESP.restart();
    }
  }
}



String getJSON() {
  String pixel_on_str = "";
  String pixel_mode_str = String(pixel_mode[0]) + String(pixel_mode[1]);

  for (int i = 0; i < pixels_num; i++) {
    pixel_on_str += pixel_on[i] ? "1" : "0";
  }

  // DS18B20 extra sensoren
  String ds_json = "";
  for (int i = 0; i < ds_count; i++) {
    ds_json += ",\"ds" + String(i) + "\":" + String(temp_ds_arr[i], 1);
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
         ",\"ae\":\"" + pixel_mode_str + "\"" +
         ",\"af\":" + String(home_mode) +
         ",\"ag\":" + String(ds_count) +
         ",\"ah\":" + String(ds_primary) +
         ",\"ai\":" + String(ESP.getMaxAllocHeap()) +
         ",\"aj\":" + String(ESP.getMinFreeHeap()) +
         ds_json +
         "}";
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
  delay(1500);
  while (Serial.available()) Serial.read();  // flush
  Serial.println("\n\n=== ROOM Controller v3.1 ===");
  Serial.println("Commando's: 'R' = NVS reset, 'reset_nvs', 'status'");
  Serial.println("\nType 'R' binnen 5 sec voor NVS reset...");
  unsigned long boot_start = millis();
  while (millis() - boot_start < 5000) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'R' || c == 'r') {
        Serial.println("-> NVS reset!");
        preferences.begin("room-config", false);
        preferences.clear();
        preferences.end();
        delay(300);
        ESP.restart();
      }
    }
    delay(10);
  }
  Serial.println("(geen reset)");

  // === NVS INITIALISATIE ===
  preferences.begin("room-config", false);  // read/write mode


  // ===== BOOT: restore pixel states from NVS =====
  for (int i = 0; i < pixels_num; i++) {

    if (i < num_mov_pixels) {
      // Pixel 0–1: mode (AUTO / ON)
      const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
      pixel_mode[i] = preferences.getInt(key, 0);   // default = AUTO
    } else {
      // Pixels 2+
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      pixel_on[i] = preferences.getBool(key.c_str(), false);
    }
  }
  

  // Detecteer eerste boot / lege NVS → zet defaults + melding
  bool first_boot = preferences.getString(NVS_ROOM_ID, "").isEmpty();
  
  if (first_boot) {
    Serial.println("\n*** EERSTE BOOT GEDTECTEERD – DEFAULTS TOEPASSEN ***");
    
    // Zet alle defaults in NVS
    preferences.putString(NVS_ROOM_ID, "Testroom");
    preferences.putString(NVS_WIFI_SSID, "netwerknaam");
    preferences.putString(NVS_WIFI_PASS, "paswoord");
    preferences.putString(NVS_STATIC_IP, "192.168.xx.xx");
    
    preferences.putInt(NVS_HEATING_SETPOINT, 20);
    preferences.putInt(NVS_VENT_REQUEST, 0);
    preferences.putFloat(NVS_DEW_MARGIN, 2.0);
    preferences.putInt(NVS_HOME_MODE, 0);
    preferences.putInt(NVS_LIGHT_THRESHOLD, 50);
    preferences.putULong(NVS_MOV_WINDOW, 60000UL);
    preferences.putInt(NVS_LDR_DARK, 50);
    preferences.putInt(NVS_BEAM_THRESHOLD, 50);
    
    // Alle optionele features default aan
    preferences.putBool(NVS_CO2_ENABLED, true);
    preferences.putBool(NVS_DUST_ENABLED, true);
    preferences.putBool(NVS_SUN_ENABLED, true);
    preferences.putBool(NVS_MOV2_ENABLED, true);
    preferences.putBool(NVS_TSTAT_ENABLED, true);
    preferences.putBool(NVS_BEAM_ENABLED, true);
    preferences.putUChar(NVS_NEO_R, 255);
    preferences.putUChar(NVS_NEO_G, 255);
    preferences.putUChar(NVS_NEO_B, 255);
    preferences.putInt(NVS_PIXELS_NUM, 8);
    // Pixel states defaults: alles uit, modes AUTO
    for (int i = 0; i < 30; i++) {
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      preferences.putBool(key.c_str(), false);
    }
    preferences.putInt(NVS_PIXEL_MODE_0, 0);  // AUTO
    preferences.putInt(NVS_PIXEL_MODE_1, 0);  // AUTO


    
    Serial.println("Defaults opgeslagen in NVS. Configureer via webinterface /settings");
  }
  
  // Laad alles uit NVS (ook na eerste boot)
  room_id = preferences.getString(NVS_ROOM_ID, "Testroom");
    mdns_name = room_id;              // Kopieer room_id
    mdns_name.toLowerCase();          // Alles lowercase
    mdns_name.replace(" ", "-");      // Spaties vervangen door -
  wifi_ssid             = preferences.getString(NVS_WIFI_SSID, "netwerknaam");
  wifi_pass             = preferences.getString(NVS_WIFI_PASS, "paswoord");
  static_ip_str         = preferences.getString(NVS_STATIC_IP, "192.168.xx.xx");
  
  heating_setpoint_default = preferences.getInt(NVS_HEATING_SETPOINT, 20);
  vent_request_default     = preferences.getInt(NVS_VENT_REQUEST, 0);
  dew_safety_margin        = preferences.getFloat(NVS_DEW_MARGIN, 2.0);
  home_mode_default        = preferences.getInt(NVS_HOME_MODE, 0);
  light_dark_threshold     = preferences.getInt(NVS_LIGHT_THRESHOLD, 50);
  mov_window_ms            = preferences.getULong(NVS_MOV_WINDOW, 60000UL);
  ldr_dark_threshold       = preferences.getInt(NVS_LDR_DARK, 50);
  beam_alert_threshold     = preferences.getInt(NVS_BEAM_THRESHOLD, 50);
  
  co2_enabled      = preferences.getBool(NVS_CO2_ENABLED, true);
  dust_enabled     = preferences.getBool(NVS_DUST_ENABLED, true);
  sun_light_enabled= preferences.getBool(NVS_SUN_ENABLED, true);
  mov2_enabled     = preferences.getBool(NVS_MOV2_ENABLED, true);
  int num_mov_pixels = 1 + (mov2_enabled ? 1 : 0);  // 1 of 2 MOV-pixels
  tstat_enabled    = preferences.getBool(NVS_TSTAT_ENABLED, true);
  beam_enabled     = preferences.getBool(NVS_BEAM_ENABLED, true);
  serial_verbose   = preferences.getBool(NVS_SERIAL_VERBOSE, true);
  serial_interval  = constrain(preferences.getInt(NVS_SERIAL_INTERVAL, 15), 5, 30);
  neo_r = preferences.getUChar(NVS_NEO_R, 255);
  neo_g = preferences.getUChar(NVS_NEO_G, 255);
  neo_b = preferences.getUChar(NVS_NEO_B, 255);
  pixels_num = preferences.getInt(NVS_PIXELS_NUM, 8);
  pixels_num = constrain(pixels_num, 1, 30);          // Laad aantal pixels uit NVS. (limiet)


  // === PIXEL NICKNAMES INITIALISEREN ===
  for (int i = 0; i < 30; i++) {
    String key = String(NVS_PIXEL_NICK_BASE) + String(i);
    pixel_nicknames[i] = preferences.getString(key.c_str(), "");
    
    // Als leeg (eerste boot of na factory reset) → genereer default
    if (pixel_nicknames[i].isEmpty()) {
      pixel_nicknames[i] = room_id + " Pixel " + String(i);
      preferences.putString(key.c_str(), pixel_nicknames[i]);
    }
  }
  


  // === PIXEL STATES LADEN UIT NVS ===
  for (int i = 0; i < pixels_num; i++) {
    String key = String(NVS_PIXEL_USER_ON_BASE) + String(i);
    pixel_user_on[i] = preferences.getBool(key.c_str(), false);
    pixel_on[i] = pixel_user_on[i];   // startwaarde, auto-logica kan dit overschrijven
  }



  // pixel_mode alleen laden als i < 2 (MOV pixels)
  pixel_mode[0] = preferences.getInt(NVS_PIXEL_MODE_0, 0);
  if (mov2_enabled) {
    pixel_mode[1] = preferences.getInt(NVS_PIXEL_MODE_1, 0);
  } else {
    pixel_mode[1] = 0;  // Forceer AUTO als MOV2 uitgeschakeld
  }



  // Als aantal pixels gewijzigd is, zorg dat nieuwe pixels een default krijgen
  for (int i = pixels_num; i < 30; i++) {
    pixel_nicknames[i] = "";  // Niet gebruiken
  }



  
  // Pas runtime variabelen aan met geladen waarden
  heating_setpoint = heating_setpoint_default;
  vent_percent     = vent_request_default;
  home_mode        = home_mode_default;
  LDR_DARK_THRESHOLD = ldr_dark_threshold;      // Const vervangen door variabele



  // Bed modus persistent maken
  bed = preferences.getBool(NVS_BED_STATE, false);  // default: UIT


  // Laad pixel_on[] persistent uit NVS
  for (int i = 0; i < pixels_num; i++) {
    String key = String(NVS_PIXEL_ON_BASE) + String(i);
    pixel_on[i] = preferences.getBool(key.c_str(), false);
  }


  // Thuis/Uit modus persistent maken
  home_mode = preferences.getInt(NVS_HOME_MODE_STATE, home_mode_default);

  // Heating Setpoint persistent maken
  heating_setpoint = preferences.getInt(NVS_CURRENT_SETPOINT, heating_setpoint_default);

  // Fade duration persistent maken
  fade_duration = preferences.getInt(NVS_FADE_DURATION, 2);
  fade_duration = constrain(fade_duration, 1, 10);
  updateFadeInterval();


  
  Serial.printf("Room ID: %s\n", room_id.c_str());
  Serial.printf("mDNS naam: %s.local\n", mdns_name.c_str());
  
  if (first_boot) {
    Serial.println("Typ 'reset_nvs' in serial monitor voor factory reset");
  }


  pinMode(PIR_MOV1, INPUT_PULLUP);  // Voor 3.3V HC-SR501: beweging = LOW
  pinMode(PIR_MOV2, INPUT_PULLUP);
  pinMode(SHARP_LED, OUTPUT); digitalWrite(SHARP_LED, HIGH);
  pinMode(TSTAT_PIN, INPUT_PULLUP);
  pinMode(OPTION_LDR, INPUT);

  // TSL2561:
  dht.begin();
  if (sun_light_enabled) {
    Wire.begin(13, 11);  // SDA=IO13, SCL=IO11
    if (!tsl.begin()) Serial.println("TSL2561 niet gevonden");
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  }

  // DS18B20: laad uit NVS of scan bij eerste boot
  if (ds_count == 0 && preferences.getInt(NVS_DS_COUNT, 0) == 0) {
    Serial.println("Eerste boot of geen sensoren in NVS → scan uitvoeren...");
    scanDS18B20();
  } else {
    loadDS18B20fromNVS();
    Serial.printf("DS18B20: %d sensor(s) geladen uit NVS\n", ds_count);
  }


  pixels.begin();
    pixels.updateLength(pixels_num);  // Nu correct na laden pixels_num
    pixels.clear();
    pixels.show();
    initFadeEngine();
    updateFadeInterval();



  // === FORCEER PIXEL STATES NA NVS LADEN (DIRECT ZICHTBAAR BIJ POWER-ON) ===
  for (int i = 0; i < pixels_num; i++) {
    uint8_t r = 0, g = 0, b = 0;
    bool is_on = false;

    if (i < num_mov_pixels) {  // MOV-pixel
      if (pixel_mode[i] == 1) {  // MANUEEL ON
        r = neo_r; g = neo_g; b = neo_b;
        is_on = true;
      } else {  // AUTO → uit bij start
        r = 0; g = 0; b = 0;
        is_on = false;
      }
      if (i == 0) mov1_light = is_on ? 1 : 0;
      if (i == 1) mov2_light = is_on ? 1 : 0;
    } else {  // Normale pixel (incl. pixel 1 als mov2 uit)
      is_on = pixel_on[i];
      r = is_on ? neo_r : 0;
      g = is_on ? neo_g : 0;
      b = is_on ? neo_b : 0;
    }

    pixels.setPixelColor(i, r, g, b);
    currR[i] = r; currG[i] = g; currB[i] = b;
    targetR[i] = r; targetG[i] = g; targetB[i] = b;
    startR[i] = r; startG[i] = g; startB[i] = b;
    fade_progress[i] = 1.0f;
  }
  pixels.show();  // Toon meteen de juiste staat





  WiFi.mode(WIFI_STA);

  // === DYNAMISCHE WIFI + STATIC IP UIT NVS ===
  IPAddress local_ip;
  if (local_ip.fromString(static_ip_str)) {
    // Valide IP gevonden in NVS → gebruik static config
    IPAddress gateway;
    IPAddress subnet(255, 255, 255, 0);

    // Simpele heuristiek: gateway is meestal .1 in hetzelfde subnet
    gateway = local_ip;
    gateway[3] = 1;

    WiFi.config(local_ip, gateway, subnet, gateway);  // DNS = gateway
    Serial.printf("Static IP ingesteld: %s (gateway %s)\n", local_ip.toString().c_str(), gateway.toString().c_str());
  } else {
    Serial.println("Geen geldig static IP in NVS → DHCP gebruiken");
  }

  // Verbind met WiFi uit NVS
  Serial.print("Verbinden met WiFi SSID: ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

  // Haal MAC op NA WiFi.begin voor betere compatibiliteit
  mac_address = WiFi.macAddress();
  Serial.println("MAC adres: " + mac_address);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {  // 20 seconden timeout
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // WiFi successfully connected!
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi verbonden!");
    Serial.println("IP adres: " + WiFi.localIP().toString());
    ap_mode_active = false;  // Normale mode
  } else {

    Serial.println("\nWiFi verbinding mislukt! Starten Access Point voor configuratie...");
    
    WiFi.mode(WIFI_AP_STA);
    
    String ap_ssid = "ROOM-" + room_id;           // bijv. ROOM-Testroom
    WiFi.softAP(ap_ssid.c_str()); // Open AP (geen wachtwoord) voor eenvoudige configuratie

    IPAddress ap_ip(192, 168, 4, 1);
    WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
    
    Serial.println("\n=== ACCESS POINT GESTART ===");
    Serial.printf("SSID: %s\n", ap_ssid.c_str());
    Serial.println("Wachtwoord: roomconfig");
    Serial.println("IP: http://192.168.4.1");
    Serial.println("Ga naar http://192.168.4.1/settings om je WiFi in te stellen");
    Serial.println("Na opslaan reboot de controller automatisch");
    Serial.println("=======================================\n");
    
    // Belangrijk: geef de iPhone tijd om het netwerk te zien
    delay(1000);
    ap_mode_active = true;

    Serial.println("AP-mode actief → webserver en DNS blijven actief");
    ap_mode_active = true;
    
    dnsServer.start(DNS_PORT, "*", ap_ip);
    Serial.println("DNS captive portal actief (alle domeinen → 192.168.4.1)");

  }


  // ===== TIJDINITIALISATIE (ESP32 native SNTP) =====
  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();

  configTzTime(
  "CET-1CEST,M3.5.0/02,M10.5.0/03",
  "pool.ntp.org",
  "time.nist.gov"
  );




  Serial.println("\nIP: " + WiFi.localIP().toString());

  // mDNS alleen starten bij geldige STA-verbinding
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    if (MDNS.begin(room_id.c_str())) {
      Serial.print("mDNS gestart: http://");
      Serial.print(room_id);
      Serial.println(".local");
    } else {
      Serial.println("mDNS start mislukt");
    }
  } else {
    Serial.println("mDNS niet gestart (geen geldige STA-IP)");
  }


  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");



  // === HOME PAGE ===
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html;
    html.reserve(12000);
    html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)rawliteral" + room_id + R"rawliteral( Status</title>


  <style>
  body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}
  .header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}
  .header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}
  .container{display:flex;min-height:calc(100vh - 60px);}
  .sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}
  .sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}
  .sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}
  .main{flex:1;padding:15px;overflow-y:auto;}
  .group-title{font-size:17px;font-style:italic;font-weight:bold;color:#369;margin:20px 0 8px 0;}
  table{width:100%;border-collapse:collapse;margin-bottom:15px;}
  td.label{color:#369;font-size:13px;padding:8px 5px;width:30%;border-bottom:1px solid #ddd;vertical-align:middle;}
  td.value{background:#e6f0ff;font-size:13px;padding:8px 5px;width:100px;border-bottom:1px solid #ddd;text-align:center;vertical-align:middle;}
  td.control{font-size:13px;padding:8px 5px;border-bottom:1px solid #ddd;text-align:right;vertical-align:middle;}
  .slider{width:150px;height:28px;}
  .switch{position:relative;display:inline-block;width:50px;height:28px;vertical-align:middle;}
  .switch input{opacity:0;width:0;height:0;}
  .slider-switch{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.4s;border-radius:28px;}
  .slider-switch:before{position:absolute;content:"";height:20px;width:20px;left:4px;bottom:4px;background:#fff;transition:.4s;border-radius:50%;}
  input:checked + .slider-switch{background:#369;}
  input:checked + .slider-switch:before{transform:translateX(22px);}
  @media(max-width:600px){
    .container{flex-direction:column;}
    .sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;padding:10px 0;display:flex;justify-content:center;}
    .sidebar a{width:80px;margin:0 5px;}
    .main{padding:10px;}
    .group-title{font-size:16px;margin:15px 0 6px 0;}
    td.label{font-size:12px;padding:6px 4px;width:40%;}
    td.value{font-size:12px;padding:6px 4px;width:auto;}
    td.control{padding:6px 4px;}
    .slider{width:100%;max-width:200px;}
  }
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
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
      <a href="/json">JSON</a>
      <a href="/settings">Settings</a>
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
    
    )rawliteral";
    if (co2_enabled) {
      html += R"rawliteral(
    <tr><td class="label">CO₂</td><td class="value">)rawliteral" + String(co2) + " ppm" + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    }
    if (dust_enabled) {
      html += R"rawliteral(
    <tr><td class="label">Stof</td><td class="value">)rawliteral" + String(dust) + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    }
    html += R"rawliteral(
    <tr><td class="label">Heating setpoint</td><td class="value">)rawliteral" + String(heating_setpoint) + " °C" + R"rawliteral(</td>

      <td class="control"><form action="/set_setpoint" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="setpoint" min="10" max="30" value=")rawliteral" + String(heating_setpoint) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
    <tr><td class="label">Heating Auto</td><td class="value">)rawliteral" + String(heating_mode == 0 ? "AUTO" : "MANUEEL") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_heating_auto" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (heating_mode == 0 ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>

    <tr><td class="label">Ventilatie snelheid %</td><td class="value">)rawliteral" + String(vent_percent) + " %" + R"rawliteral(</td>
      <td class="control"><form action="/set_vent" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="vent" min="0" max="100" value=")rawliteral" + String(vent_percent) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
    <tr><td class="label">Vent Auto</td><td class="value">)rawliteral" + String(vent_mode == 0 ? "AUTO" : "MANUEEL") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_vent_auto" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (vent_mode == 0 ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>


    <tr><td class="label">Thuis/Uit</td><td class="value">)rawliteral" + String(home_mode ? "Thuis" : "Uit") + R"rawliteral(</td>
       <td class="control"><form action="/toggle_home" method="get" onsubmit="event.preventDefault(); submitAjax(this);">
       <label class="switch"><input type="checkbox" )rawliteral" + (home_mode ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);">
       <span class="slider-switch"></span></label>
    </form></td></tr>
    )rawliteral";
    if (tstat_enabled) {
      html += R"rawliteral(
    <tr><td class="label">Hardware thermostaat</td><td class="value">)rawliteral" + String(tstat_on ? "AAN" : "UIT") + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    }
    html += R"rawliteral(
    <tr><td class="label">Heating aan</td><td class="value">)rawliteral" + String(heating_on ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
  </table>


  <div class="group-title">VERLICHTING</div>
  <table>
    )rawliteral";
    if (sun_light_enabled) {
      html += R"rawliteral(
    <tr><td class="label">Zonlicht</td><td class="value">)rawliteral" + String(sun_light) + " lux" + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    }
    
    
    html += R"rawliteral(
    <tr><td class="label">LDR (donker=100)</td><td class="value">)rawliteral" + String(light_ldr) + R"rawliteral(</td><td class="control"></td></tr>
    <tr><td class="label">MOV1 PIR licht aan</td><td class="value">)rawliteral" + String(mov1_light ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    if (mov2_enabled) {
      html += R"rawliteral(
    <tr><td class="label">MOV2 PIR licht aan</td><td class="value">)rawliteral" + String(mov2_light ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
    )rawliteral";
    }
    html += R"rawliteral(


    <tr><td class="label">NeoPixel RGB</td><td class="value">)rawliteral" + String(neo_r) + ", " + String(neo_g) + ", " + String(neo_b) + R"rawliteral(</td>
      <td class="control"><a href="/neopixel" style="color:#336699;text-decoration:underline;">Kleur kiezen</a></td></tr>
    <tr><td class="label">Bed switch</td><td class="value">)rawliteral" + String(bed ? "AAN" : "UIT") + R"rawliteral(</td>
      <td class="control"><form action="/toggle_bed" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><label class="switch"><input type="checkbox" )rawliteral" + (bed ? "checked" : "") + R"rawliteral( onchange="submitAjax(this.form);"><span class="slider-switch"></span></label></form></td></tr>
    <tr><td class="label">Dim snelheid (s)</td><td class="value">)rawliteral" + String(fade_duration) + R"rawliteral(</td>
      <td class="control"><form action="/set_fade_duration" method="get" onsubmit="event.preventDefault(); submitAjax(this);"><input type="range" class="slider" name="duration" min="1" max="10" value=")rawliteral" + String(fade_duration) + R"rawliteral(" onchange="submitAjax(this.form);"></form></td></tr>
)rawliteral";


    // Dynamische pixels loop met nicknames
    for (int i = 0; i < pixels_num; i++) {
      String label = pixel_nicknames[i];  // Kopieer eerst
      label.trim();                       // Verwijder spaties voor/achter
      
      if (label.isEmpty()) {
        // Fallback als geen nickname ingesteld
        label = "Pixel " + String(i);
        if (i < 2) {
          if (i == 0) {
            label += " (MOV1)";
          } else if (mov2_enabled) {
            label += " (MOV2)";
          }
        }
      }
      String value = pixel_on[i] ? "On" : "Off";
      String checked = pixel_on[i] ? "checked" : "";


      String action;
      if (i == 0 || (i == 1 && mov2_enabled)) {
        action = "/toggle_pixel_mode" + String(i);   // Mode-toggle alleen voor actieve MOV-pixels
      } else {
        action = "/toggle_pixel" + String(i);        // On/off-toggle voor alle andere (incl. pixel 1 als MOV2 uit)
      }


        html += "<tr><td class=\"label\">" + label + "</td><td class=\"value\">" + value + "</td>";
        html += "<td class=\"control\"><form action=\"" + action + "\" method=\"get\" onsubmit=\"event.preventDefault(); submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" " + checked + " onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>";
    }


    html += R"rawliteral(
      </table>

      <div class="group-title">BEWEGING</div>
      
      <table>
        <tr><td class="label">MOV1 PIR trig/min</td><td class="value">)rawliteral" + String(mov1_triggers) + R"rawliteral(</td><td class="control"></td></tr>
        )rawliteral";
    if (mov2_enabled) {
      html += R"rawliteral(
        <tr><td class="label">MOV2 PIR trig/min</td><td class="value">)rawliteral" + String(mov2_triggers) + R"rawliteral(</td><td class="control"></td></tr>
        )rawliteral";
    }
    html += R"rawliteral(


    </table>
      )rawliteral";
    if (beam_enabled) {
      html += R"rawliteral(
      <div class="group-title">BEWAKING</div>
      <table>
        <tr><td class="label">Beam sensor waarde</td><td class="value">)rawliteral" + String(beam_value) + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">Beam sensor alert</td><td class="value">)rawliteral" + String(beam_alert_new ? "JA" : "NEE") + R"rawliteral(</td><td class="control"></td></tr>
      </table>
      )rawliteral";
    }
    html += R"rawliteral(


      <div class="group-title">CONTROLLER</div>
      <table>
        <tr><td class="label">WiFi RSSI</td><td class="value">)rawliteral" + String(WiFi.RSSI()) + " dBm" + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">WiFi kwaliteit</td><td class="value">)rawliteral" + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + " %" + R"rawliteral(</td><td class="control"></td></tr>
        <tr><td class="label">Free heap</td><td class="value" id="heap-pct">)rawliteral" + String((ESP.getFreeHeap() * 100) / ESP.getHeapSize()) + " %" + R"rawliteral(</td>
        <td class="control" id="heap-lb" style="font-size:12px;">)rawliteral" +
          [&]() -> String {
            uint32_t lb = ESP.getMaxAllocHeap();
            const char* col = lb > 35000 ? "#0a0" : lb > 25000 ? "#f80" : "#c00";
            return String("largest: <b style='color:") + col + "'>" + String(lb/1024) + " KB</b>";
          }()
        + R"rawliteral(</td></tr>
      

      </table>
      <div style="text-align:center;margin:10px 0;">
        <button class="button" onclick="updateValues()">Refresh</button>
      </div>
      <div id="status" style="text-align:center;margin:8px 0;font-weight:bold;color:#369;"></div>
    </div>
  </div>
<script>
function updateValues(){
  fetch('/json?'+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(data=>{
    document.querySelectorAll('td.value').forEach(td=>{
      const l=td.previousElementSibling;if(!l)return;
      const lbl=l.textContent.trim();
      if(lbl.includes("Room temp")) td.innerHTML=data.h.toFixed(1)+" °C<br><small>("+data.g.toFixed(1)+", "+data.h.toFixed(1)+")</small>";
      else if(lbl.includes("Humidity")) td.textContent=Math.round(data.d)+" %";
      else if(lbl.includes("Dauwpunt")) td.textContent=data.c.toFixed(1)+" °C";
      else if(lbl.includes("DewAlert")) td.textContent=data.k?"JA":"NEE";
      else if(lbl.includes("CO₂")) td.textContent=data.a+" ppm";
      else if(lbl.includes("Stof")) td.textContent=data.b;
      else if(lbl.includes("Heating setpoint")) td.textContent=data.aa+" °C";
      else if(lbl.includes("Ventilatie snelheid")) td.textContent=data.z+" %";
      else if(lbl.includes("HVAC Auto")) td.textContent=data.af==0?"AUTO":"MANUEEL";
      else if(lbl.includes("Hardware thermostaat")) td.textContent=data.l?"AAN":"UIT";
      else if(lbl.includes("Heating aan")) td.textContent=data.y?"JA":"NEE";
      else if(lbl.includes("Zonlicht")) td.textContent=data.f+" lux";
      else if(lbl.includes("LDR")) td.textContent=data.e;
      else if(lbl.includes("Night mode")) td.textContent=data.q?"JA":"NEE";
      else if(lbl.includes("MOV1 PIR licht")) td.textContent=data.m?"JA":"NEE";
      else if(lbl.includes("MOV2 PIR licht")) td.textContent=data.n?"JA":"NEE";
      else if(lbl.includes("NeoPixel RGB")) td.textContent=data.s+", "+data.t+", "+data.u;
      else if(lbl.includes("Bed switch")) td.textContent=data.r?"AAN":"UIT";
      else if(lbl.includes("Dim snelheid")) td.textContent=data.ab;
      else if(lbl.includes("MOV1 PIR trig")) td.textContent=data.i;
      else if(lbl.includes("MOV2 PIR trig")) td.textContent=data.j;
      else if(lbl.includes("Beam waarde")) td.textContent=data.o;
      else if(lbl.includes("Beam alert")) td.textContent=data.p?"JA":"NEE";
      else if(lbl.includes("WiFi RSSI")) td.textContent=data.v+" dBm";
      else if(lbl.includes("WiFi kwaliteit")) td.textContent=data.w+" %";
      else if(lbl.includes("Free heap")){
        td.textContent=data.x+" %";
        var lb=data.ai||0;
        var lbKB=Math.round(lb/1024);
        var col=lb>35000?"#0a0":lb>25000?"#f80":"#c00";
        var detail=document.getElementById("heap-lb");
        if(detail) detail.innerHTML="largest: <b style='color:"+col+"'>"+lbKB+" KB</b>";
      }
      else if(lbl.includes("Pixel")){
        const idx=parseInt(lbl.match(/\d+/)[0]);
        if(idx===0) td.textContent=data.m?"On":"Off";
        else if(idx===1) td.textContent=data.n?"On":"Off";
        else td.textContent=data.ad.charAt(idx)==='1'?"On":"Off";
      }
    });
    const hvacT=document.querySelector('td.control form[action="/toggle_hvac_auto"] input');
    if(hvacT) hvacT.checked=(data.af==0);
    const homeT=document.querySelector('td.control form[action="/toggle_home"] input');
    if(homeT) homeT.checked=(data.af==1);
    document.querySelectorAll('td.control form[action^="/toggle_pixel_mode"] input').forEach((cb,i)=>{cb.checked=(data.ae.charAt(i)==='1');});
    const hdr=document.querySelector('.header-right');
    if(hdr){const now=new Date();hdr.innerHTML=data.ac+" s &nbsp;&nbsp; "+now.toLocaleDateString('nl-BE')+" "+now.toLocaleTimeString('nl-BE');}
  }).catch(e=>console.error(e));
}
function submitAjax(form){
  const p=new URLSearchParams();let px=null;
  for(const el of form.elements){
    if(el.name){p.append(el.name,el.value);
      if(el.name==='pixel'&&el.type==='hidden') px=parseInt(el.value);
      if(el.name==='state') px={idx:px,state:el.value==='1'};
    }
  }
  const url=p.toString()?form.action+'?'+p.toString():form.action;
  if(px&&px.idx!==null) document.querySelectorAll('td.value').forEach(td=>{const l=td.previousElementSibling;if(l&&l.textContent.includes('Pixel '+px.idx)) td.textContent=px.state?'On':'Off';});
  fetch(url).then(r=>{if(r.ok){updateValues();const s=document.getElementById('status');if(s){s.textContent='✓';setTimeout(()=>s.textContent='',1500);}}}).catch(e=>console.error(e));
}
window.addEventListener('load',()=>{updateValues();setInterval(updateValues,3000);});
</script>
</script>



</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });



  // === JSON ENDPOINT ===
    server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getJSON());
  });


  // === OTA UPDATE PAGE ===
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html;
    html.reserve(2000);
    html = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>OTA</title>
<style>
body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}
.header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;}
.header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}
.container{display:flex;min-height:calc(100vh - 60px);}
.sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;flex-shrink:0;}
.sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}
.sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}
.main{flex:1;padding:30px;text-align:center;}
.btn{background:#369;color:#fff;padding:11px 22px;border:none;border-radius:7px;cursor:pointer;font-size:15px;margin:8px;}
.btn:hover{background:#036;}.btn-red{background:#c00;}.btn-red:hover{background:#900;}
</style></head><body>
<div class="header">
  <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
  <div class="header-right">)rawliteral" + String(uptime_sec) + " s" + R"rawliteral(</div>
</div>
<div class="container">
  <div class="sidebar">
    <a href="/">Status</a>
    <a href="/update" class="active">OTA</a>
    <a href="/json">JSON</a>
    <a href="/settings">Settings</a>
  </div>
  <div class="main">
    <h2 style="color:#369;">OTA Firmware Update</h2>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update" accept=".bin"><br><br>
      <button class="btn" type="submit">Upload</button>
    </form>
    <br>
    <button class="btn btn-red" onclick="location.href='/reboot'">Reboot</button>
    <br><br><a href="/">← Terug</a>
  </div>
</div>
</body></html>
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
    bed = !bed;
    preferences.putBool(NVS_BED_STATE, bed);  // direct opslaan
    request->send(200, "text/plain", "OK");
  });




// Dynamische toggles voor pixels:
// Pixel 0  → altijd MODE (MOV1)
// Pixel 1  → MODE alleen als mov2_enabled
// Pixel 2+ → altijd ON/OFF
for (int i = 0; i < pixels_num; i++) {

  bool is_mode_pixel =
    (i == 0) ||
    (i == 1 && mov2_enabled);

  String path = is_mode_pixel
    ? "/toggle_pixel_mode" + String(i)
    : "/toggle_pixel" + String(i);

  server.on(path.c_str(), HTTP_GET, [i, is_mode_pixel](AsyncWebServerRequest *request) {

    if (is_mode_pixel) {
      pixel_mode[i] = 1 - pixel_mode[i];
      const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
      preferences.putInt(key, pixel_mode[i]);
    } else {
      pixel_on[i] = !pixel_on[i];
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      preferences.putBool(key.c_str(), pixel_on[i]);
    }

    request->send(200, "text/plain", "OK");
  });
}





  // === NEOPIXEL KLEURKIEZER PAGE ===
    server.on("/neopixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html;
    html.reserve(5000);
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
      border-radius:8px;cursor:pointer;font-size:16px;margin-top:30px;
    }
    .button:hover {background:#003366;}
    #status {margin-top:20px;font-weight:bold;color:#336699;}
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
    <div class="header-right">
      )rawliteral" + String(uptime_sec) + " s &nbsp;&nbsp; " + getFormattedDateTime() + R"rawliteral(
    </div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status &<br>Control</a>
      <a href="/update">OTA<br>Update</a>
      <a href="/json">JSON<br>Data</a>
      <a href="/settings">Settings</a>
    </div>
    <div class="main">
      <h1 style="color:#336699;">NeoPixel Kleur Instellen</h1>
      <form id="colorForm">
        <p>R: <input type="range" name="r" min="0" max="255" value=")rawliteral" + String(neo_r) + R"rawliteral("><span id="r_val"> )rawliteral" + String(neo_r) + R"rawliteral(</span><br><br></p>
        <p>G: <input type="range" name="g" min="0" max="255" value=")rawliteral" + String(neo_g) + R"rawliteral("><span id="g_val"> )rawliteral" + String(neo_g) + R"rawliteral(</span><br><br></p>
        <p>B: <input type="range" name="b" min="0" max="255" value=")rawliteral" + String(neo_b) + R"rawliteral("><span id="b_val"> )rawliteral" + String(neo_b) + R"rawliteral(</span><br><br></p>
        <button type="submit" class="button">Pas kleur toe</button>
      </form>
      <div id="status"></div>
      <br><br><a href="/" style="color:#336699;text-decoration:underline;">← Terug naar Status</a>
    </div>
  </div>

<script>
  document.querySelectorAll('input[type=range]').forEach(slider => {
    const output = document.getElementById(slider.name + '_val');
    output.textContent = slider.value;
    slider.oninput = function() {
      output.textContent = this.value;
    }
  });

  document.getElementById('colorForm').onsubmit = function(e) {
    e.preventDefault();
    const params = new URLSearchParams();
    params.append('r', document.querySelector('input[name=r]').value);
    params.append('g', document.querySelector('input[name=g]').value);
    params.append('b', document.querySelector('input[name=b]').value);

    fetch('/setcolor?' + params.toString(), {method: 'GET'})
      .then(response => response.text())
      .then(text => {
        document.getElementById('status').textContent = 'Kleur toegepast!';
        setTimeout(() => { document.getElementById('status').textContent = ''; }, 2000);
      })
      .catch(err => {
        document.getElementById('status').textContent = 'Fout bij toepassen';
      });
  };
</script>

</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });


// === CAPTIVE PORTAL HANDLERS ===
server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->redirect("/settings");
});

server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->redirect("/settings");
});

server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->redirect("/settings");
});

// === SETTINGS PAGE - complete versie, werkende checkboxes ===
server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {

  // Pixel nicknames velden
  String pixelNamesHtml = "";
  for (int i = 0; i < pixels_num; i++) {
    pixelNamesHtml += "<label style=\"display:block;margin:4px 0;\">P" + String(i) + ": ";
    pixelNamesHtml += "<input type=\"text\" name=\"pixel_nick_" + String(i) + "\" value=\"" + pixel_nicknames[i] + "\" style=\"width:200px;\"></label>";
  }

  String html;
  html.reserve(10000);
  html = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>)rawliteral" + room_id + R"rawliteral( - Settings</title>
<style>
body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}
.header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;}
.header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}
.container{display:flex;min-height:calc(100vh - 60px);}
.sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}
.sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}
.sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}
.main{flex:1;padding:20px;overflow-y:auto;}
table{width:100%;border-collapse:collapse;margin:10px 0;}
td.lbl{width:38%;padding:9px 6px;font-weight:bold;color:#369;border-bottom:1px solid #eee;vertical-align:middle;}
td.inp{padding:9px 6px;border-bottom:1px solid #eee;vertical-align:middle;}
input[type=text],input[type=password],input[type=number],select{width:100%;padding:7px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}
.btn{background:#369;color:#fff;padding:11px 28px;border:none;border-radius:6px;font-size:15px;cursor:pointer;margin:15px 8px;}
.btn:hover{background:#036;}
.btn-red{background:#c00;}.btn-red:hover{background:#900;}
@media(max-width:800px){.container{flex-direction:column;}.sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;display:flex;justify-content:center;}.sidebar a{margin:0 3px;}}
</style></head><body>
<div class="header">
  <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
  <div class="header-right">Instellingen</div>
</div>
<div class="container">
  <div class="sidebar">
    <a href="/">Status</a>
    <a href="/update">OTA</a>
    <a href="/json">JSON</a>
    <a href="/settings" class="active">Settings</a>
  </div>
  <div class="main">
    <form action="/save_settings" method="get" id="sf">
    <table>
      <tr><td class="lbl">MAC adres</td><td class="inp"><code>)rawliteral" + mac_address + R"rawliteral(</code></td></tr>
      <tr><td class="lbl">Room naam</td><td class="inp"><input type="text" name="room_id" value=")rawliteral" + room_id + R"rawliteral(" required></td></tr>
      <tr><td class="lbl">WiFi SSID</td><td class="inp"><input type="text" name="wifi_ssid" value=")rawliteral" + wifi_ssid + R"rawliteral(" required></td></tr>
      <tr><td class="lbl">WiFi wachtwoord</td><td class="inp"><input type="password" name="wifi_pass" value=")rawliteral" + wifi_pass + R"rawliteral("></td></tr>
      <tr><td class="lbl">Static IP</td><td class="inp"><input type="text" name="static_ip" value=")rawliteral" + static_ip_str + R"rawliteral(" placeholder="leeg = DHCP"></td></tr>
      <tr><td class="lbl">Heating setpoint</td><td class="inp"><input type="number" name="heat_sp" min="10" max="30" value=")rawliteral" + String(heating_setpoint_default) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Vent default %</td><td class="inp"><input type="number" name="vent_req" min="0" max="100" value=")rawliteral" + String(vent_request_default) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Dew margin (°C)</td><td class="inp"><input type="number" step="0.1" name="dew_margin" min="0.5" max="5.0" value=")rawliteral" + String(dew_safety_margin, 1) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Home mode default</td><td class="inp">
        <select name="home_mode">
          <option value="0" )rawliteral" + (home_mode_default == 0 ? "selected" : "") + R"rawliteral(>Uit</option>
          <option value="1" )rawliteral" + (home_mode_default == 1 ? "selected" : "") + R"rawliteral(>Thuis</option>
        </select></td></tr>
      <tr><td class="lbl">LDR dark threshold</td><td class="inp"><input type="number" name="ldr_dark" min="10" max="100" value=")rawliteral" + String(ldr_dark_threshold) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Beam threshold</td><td class="inp"><input type="number" name="beam_thresh" min="0" max="100" value=")rawliteral" + String(beam_alert_threshold) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Aantal NeoPixels</td><td class="inp"><input type="number" name="pixels" min="1" max="30" value=")rawliteral" + String(pixels_num) + R"rawliteral("></td></tr>
      <tr><td class="lbl">Standaard RGB</td><td class="inp">
        R:<input type="number" name="neo_r" min="0" max="255" value=")rawliteral" + String(neo_r) + R"rawliteral(" style="width:70px;">
        G:<input type="number" name="neo_g" min="0" max="255" value=")rawliteral" + String(neo_g) + R"rawliteral(" style="width:70px;">
        B:<input type="number" name="neo_b" min="0" max="255" value=")rawliteral" + String(neo_b) + R"rawliteral(" style="width:70px;"></td></tr>
      <tr><td class="lbl">Pixel namen</td><td class="inp">)rawliteral" + pixelNamesHtml + R"rawliteral(</td></tr>
      <tr><td class="lbl">Optionele sensoren</td><td class="inp">
        <label><input type="checkbox" name="co2")rawliteral" + String(co2_enabled ? " checked" : "") + R"rawliteral(> CO₂</label>
        <label><input type="checkbox" name="dust")rawliteral" + String(dust_enabled ? " checked" : "") + R"rawliteral(> Stof</label>
        <label><input type="checkbox" name="sun")rawliteral" + String(sun_light_enabled ? " checked" : "") + R"rawliteral(> Zonlicht</label>
        <label><input type="checkbox" name="mov2")rawliteral" + String(mov2_enabled ? " checked" : "") + R"rawliteral(> MOV2</label>
        <label><input type="checkbox" name="tstat")rawliteral" + String(tstat_enabled ? " checked" : "") + R"rawliteral(> Thermostaat</label>
        <label><input type="checkbox" name="beam")rawliteral" + String(beam_enabled ? " checked" : "") + R"rawliteral(> Beam</label>
      </td></tr>
      <tr><td class="lbl">Serial logging</td><td class="inp">
        <label><input type="checkbox" name="serial_verbose")rawliteral" + String(serial_verbose ? " checked" : "") + R"rawliteral(> Aan</label>
        &nbsp;&nbsp;interval: <input type="number" name="serial_interval" min="5" max="30" value=")rawliteral" + String(serial_interval) + R"rawliteral(" style="width:50px;"> s
      </td></tr>
    </table>)rawliteral";

  // DS18B20 sectie
  html += "<p style=\"margin:12px 0 4px 0;\"><b>DS18B20</b> &mdash; " + String(ds_count) + " sensor(s) gevonden";
  if (ds_count == 0) html += " <span style=\"color:#c00;\">&mdash; controleer bedrading</span>";
  html += "</p>";
  for (int i = 0; i < ds_count; i++) {
    html += "<div style=\"margin:4px 0;padding:6px;background:#f5f5f5;border-radius:4px;\">";
    html += "<code style=\"font-size:12px;color:#666;\">";
    for (int j = 0; j < 8; j++) { char buf[4]; snprintf(buf, 4, "%02X", ds_addrs[i][j]); html += buf; if (j < 7) html += ":"; }
    html += "</code>";
    if (temp_ds_arr[i] != 0.0) html += " <b style=\"color:#369;\">" + String(temp_ds_arr[i], 1) + " &deg;C</b>";
    html += "<br><label style=\"font-size:13px;\">Nickname: <input type=\"text\" name=\"ds_nick_" + String(i) + "\" value=\"" + ds_nicknames[i] + "\" style=\"width:180px;margin-top:3px;\"></label></div>";
  }
  html += "<div style=\"margin:8px 0;\"><label><b>Primaire sensor: </b><select name=\"ds_primary\" style=\"padding:4px;\">";
  for (int i = 0; i < ds_count; i++) {
    html += "<option value='" + String(i) + "'" + (i == ds_primary ? " selected" : "") + ">" + ds_nicknames[i];
    if (temp_ds_arr[i] != 0.0) html += " (" + String(temp_ds_arr[i], 1) + " °C)";
    html += "</option>";
  }
  html += R"rawliteral(</select></label>
  &nbsp; <a href="/rescan_ds" onclick="return confirm('Rescan uitvoeren?');" style="background:#369;color:#fff;padding:6px 14px;border-radius:5px;text-decoration:none;font-size:13px;">Rescan 1-Wire</a>
</div>
    <div style="text-align:center;margin-top:16px;">
      <button type="submit" class="btn">Opslaan &amp; Reboot</button>
      <button type="button" class="btn btn-red" onclick="if(confirm('Alles wissen?')) location.href='/factory_reset';">Factory Reset</button>
    </div>
    </form>
    <script>
    document.getElementById('sf').onsubmit=function(e){
      const ip=this.static_ip.value.trim();
      if(ip&&!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)){alert('Ongeldig IP!');e.preventDefault();return false;}
      if(!this.room_id.value.trim()||!this.wifi_ssid.value.trim()){alert('Room naam en SSID verplicht!');e.preventDefault();return false;}
      return true;
    };
    </script>
  </div>
</div>
</body></html>
)rawliteral";

  request->send(200, "text/html; charset=utf-8", html);
});

// === SAVE SETTINGS ===
server.on("/save_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
  auto arg = [&](const char* n, const String& d="") {
    return request->hasArg(n) ? request->arg(n) : d;
  };

  // Basisinstellingen
  preferences.putString(NVS_ROOM_ID, arg("room_id", room_id));
  preferences.putString(NVS_WIFI_SSID, arg("wifi_ssid", wifi_ssid));
  preferences.putString(NVS_WIFI_PASS, arg("wifi_pass", wifi_pass));
  preferences.putString(NVS_STATIC_IP, arg("static_ip", ""));

  preferences.putInt(NVS_HEATING_SETPOINT, arg("heat_sp","20").toInt());
  preferences.putInt(NVS_VENT_REQUEST, arg("vent_req","0").toInt());
  preferences.putFloat(NVS_DEW_MARGIN, arg("dew_margin","2.0").toFloat());
  preferences.putInt(NVS_HOME_MODE, arg("home_mode","0").toInt());
  preferences.putInt(NVS_LDR_DARK, arg("ldr_dark","50").toInt());
  preferences.putInt(NVS_BEAM_THRESHOLD, arg("beam_thresh","50").toInt());

  // Checkboxes betrouwbaar
  preferences.putBool(NVS_CO2_ENABLED, request->hasArg("co2"));
  preferences.putBool(NVS_DUST_ENABLED, request->hasArg("dust"));
  preferences.putBool(NVS_SUN_ENABLED, request->hasArg("sun"));
  preferences.putBool(NVS_MOV2_ENABLED, request->hasArg("mov2"));
  preferences.putBool(NVS_TSTAT_ENABLED, request->hasArg("tstat"));
  preferences.putBool(NVS_BEAM_ENABLED, request->hasArg("beam"));
  preferences.putBool(NVS_SERIAL_VERBOSE, request->hasArg("serial_verbose"));
  preferences.putInt(NVS_SERIAL_INTERVAL, constrain(arg("serial_interval","15").toInt(), 5, 30));
  serial_interval = constrain(arg("serial_interval","15").toInt(), 5, 30); // direct actief, geen reboot nodig

  // NeoPixels

  // NeoPixels aantal wijzigen + nieuwe pixels resetten naar uit
  int new_pixels = arg("pixels","8").toInt();
  new_pixels = constrain(new_pixels, 1, 30);
  int old_pixels = pixels_num;  // huidige waarde (nog niet herladen, maar we weten het nog niet – wacht, we laden niet her, maar we rebooten toch)
  preferences.putInt(NVS_PIXELS_NUM, new_pixels);

  // Als aantal verhoogd: nieuwe pixels default uit zetten in NVS
  if (new_pixels > old_pixels) {
    for (int i = old_pixels; i < new_pixels; i++) {
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      preferences.putBool(key.c_str(), false);
    }
  }

  // NeoPixels kleur bewaren!
  preferences.putUChar(NVS_NEO_R, arg("neo_r","255").toInt());
  preferences.putUChar(NVS_NEO_G, arg("neo_g","255").toInt());
  preferences.putUChar(NVS_NEO_B, arg("neo_b","255").toInt());


  // Opslaan pixel nicknames
  for (int i = 0; i < 30; i++) {
    String argName = "pixel_nick_" + String(i);
    if (request->hasArg(argName.c_str())) {
      String nick = request->arg(argName.c_str());
      nick.trim();
      if (nick.isEmpty()) {
        nick = room_id + " Pixel " + String(i);
      }
      String key = String(NVS_PIXEL_NICK_BASE) + String(i);
      preferences.putString(key.c_str(), nick);
      if (i < pixels_num) {
        pixel_nicknames[i] = nick;  // Update runtime array
      }
    }
  }

  // DS18B20 nicknames en primaire sensor opslaan (v1.3)
  for (int i = 0; i < DS_MAX_SENSORS; i++) {
    String argName = "ds_nick_" + String(i);
    if (request->hasArg(argName.c_str())) {
      String nick = request->arg(argName.c_str());
      nick.trim();
      if (!nick.isEmpty()) {
        preferences.putString(argName.c_str(), nick);
      }
    }
  }
  if (request->hasArg("ds_primary")) {
    int prim = constrain(request->arg("ds_primary").toInt(), 0, DS_MAX_SENSORS - 1);
    preferences.putInt(NVS_DS_PRIMARY, prim);
  }

  request->send(200, "text/html",
    "<h2 style='text-align:center;padding:50px;color:#336699;'>Instellingen opgeslagen!<br>Rebooting...</h2>");
  delay(800);
  ESP.restart();
});  // einde save_settings





  // === FACTORY RESET VIA WEB ===
  server.on("/factory_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h2 style='color:#f00'>Factory reset uitgevoerd!<br>Rebooting...</h2>");
    preferences.clear();
    delay(1000);
    ESP.restart();
  });

  // === RESCAN DS18B20 BUS (v1.3) ===
  server.on("/rescan_ds", HTTP_GET, [](AsyncWebServerRequest *request) {
    scanDS18B20();
    // Lees meteen temperaturen zodat ze zichtbaar zijn in /settings
    readDS18B20temps();
    String html = "<h2 style='text-align:center;padding:30px;color:#336699;'>";
    html += "🔍 Rescan uitgevoerd!<br>";
    html += String(ds_count) + " sensor(s) gevonden.<br><br>";
    html += "<a href='/settings' style='color:#336699;'>← Terug naar Settings</a></h2>";
    request->send(200, "text/html; charset=utf-8", html);
  });



  // === SET COLOR HANDLER ===
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("r")) {
      neo_r = constrain(request->getParam("r")->value().toInt(), 0, 255);
      preferences.putUChar(NVS_NEO_R, neo_r);
    }
    if (request->hasParam("g")) {
      neo_g = constrain(request->getParam("g")->value().toInt(), 0, 255);
      preferences.putUChar(NVS_NEO_G, neo_g);
    }
    if (request->hasParam("b")) {
      neo_b = constrain(request->getParam("b")->value().toInt(), 0, 255);
      preferences.putUChar(NVS_NEO_B, neo_b);
    }
    request->send(200, "text/plain", "OK");
  });




  // Handler voor Instelbare fade duration (slider)
  server.on("/set_fade_duration", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("duration")) {
    fade_duration = request->getParam("duration")->value().toInt();
    fade_duration = constrain(fade_duration, 1, 10);
    preferences.putInt(NVS_FADE_DURATION, fade_duration);  // direct opslaan
    updateFadeInterval();
  }
  request->send(200, "text/plain", "OK");
  });


  // Slider Heating setpoint (wijzig alleen setpoint, geen mode-switch)
  server.on("/set_setpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (request->hasParam("setpoint")) {
    heating_setpoint = request->getParam("setpoint")->value().toInt();
    heating_setpoint = constrain(heating_setpoint, 10, 30);
    preferences.putInt(NVS_CURRENT_SETPOINT, heating_setpoint);  // direct opslaan
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

  // Toggle Thuis/Uit
  server.on("/toggle_home", HTTP_GET, [](AsyncWebServerRequest *request) {
    home_mode = 1 - home_mode; // Toggle tussen 0 en 1
    preferences.putInt(NVS_HOME_MODE_STATE, home_mode);  // direct opslaan
    request->send(200, "text/plain", "OK");
  });

  Serial.println("Commando: typ 'reset_nvs' in serial monitor voor factory reset");

  // Afsluiting webserver (alle handlers hiervoor!)





  server.begin();
  Serial.printf("HTTP server gestart – http://%s.local of http://%s\n", mdns_name.c_str(), WiFi.localIP().toString().c_str());
}

unsigned long lastSerial = 0;
unsigned long last_slow = 0;








void loop() {

  // WiFi status change detectie voor mDNS
  wl_status_t current_status = WiFi.status();

  if (current_status != last_wifi_status) {

    if (current_status == WL_CONNECTED &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {

      if (mdns_running) {
        MDNS.end();
        mdns_running = false;
        Serial.println("mDNS gestopt (herstart wegens WiFi connect)");
      }

      if (MDNS.begin(room_id.c_str())) {
        mdns_running = true;
        Serial.print("mDNS opnieuw gestart: http://");
        Serial.print(room_id);
        Serial.println(".local");
      } else {
        Serial.println("mDNS herstart mislukt");
      }
    }

    if (current_status != WL_CONNECTED && mdns_running) {
      MDNS.end();
      mdns_running = false;
      Serial.println("mDNS gestopt (WiFi disconnect)");
    }

    last_wifi_status = current_status;
  }



  if (ap_mode_active) {
    dnsServer.processNextRequest();
  }

  handleSerialCommands();

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





  // NeoPixel aansturing — gesaneerd & deterministisch

  for (int i = 0; i < pixels_num; i++) {

    // -------- PIXEL 0 : MOV1 --------
    if (i == 0) {

      if (bed == 1) {
        setTargetColor(0, 0, 0, 0);
        mov1_light = 0;
        pixel_on[0] = false;   // <<< SYNC UI
      }
      else if (pixel_mode[0] == 1) {
        // Manueel AAN
        setTargetColor(0, neo_r, neo_g, neo_b);
        mov1_light = 1;
        pixel_on[0] = true;    // <<< SYNC UI
      }
      else {
        // AUTO: PIR + LDR
        bool dark = (light_ldr > LDR_DARK_THRESHOLD);
        bool movement = (millis() < mov1_off_time);
        bool on = dark && movement;

        setTargetColor(0, 0, on ? 220 : 0, 0);
        mov1_light = on;
        pixel_on[0] = on;      // <<< SYNC UI
      }

      continue;
    }




    // -------- PIXEL 1 : MOV2 --------
    if (i == 1 && mov2_enabled) {
      if (bed == 1) {
        setTargetColor(1, 0, 0, 0);
        mov2_light = 0;
        pixel_on[1] = false;
      }
      else if (pixel_mode[1] == 1) {
        // Manueel AAN
        setTargetColor(1, neo_r, neo_g, neo_b);
        mov2_light = 1;
        pixel_on[1] = true;
      }
      else {
        // AUTO: PIR + LDR
        bool dark = (light_ldr > LDR_DARK_THRESHOLD);
        bool movement = (millis() < mov2_off_time);
        bool on = dark && movement;

        setTargetColor(1, 0, on ? 220 : 0, 0);
        mov2_light = on;
        pixel_on[1] = on;
      }
      continue;
    }




    // -------- NORMALE PIXELS --------
    if (pixel_on[i]) {
      setTargetColor(i, neo_r, neo_g, neo_b);
    } else {
      setTargetColor(i, 0, 0, 0);
    }
  }









  // Thermostaat pinlezing + verwarmingslogica — buiten 60s-gate voor snelle respons
  tstat_on = !digitalRead(TSTAT_PIN);
  float effective_setpoint = max((float)heating_setpoint, dew + dew_safety_margin);
  if (heating_mode == 1) {  // MANUEEL
    heating_on = 1;
  } else {  // AUTO
    if (home_mode == 1 && tstat_enabled) {  // Thuis + thermostaat aanwezig → volg hardware
      heating_on = tstat_on;
    } else {  // Thuis zonder thermostaat, of Weg → ESP regelt met anti-condens bescherming
      heating_on = (room_temp < (effective_setpoint - 0.5f)) ? 1 : 0;
    }
  }

  if (millis() - last_slow < 60000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;



  // Sensoren
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  readDS18B20temps();  // Lees alle DS18B20 sensoren → temp_ds = primaire sensor
  
  if (sun_light_enabled) {
    sensors_event_t e; tsl.getEvent(&e); sun_light = (int)e.light;
  }
  
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));
  if (dust_enabled) dust = readDust();  // v1.9: bewaakt — delayMicroseconds blokkeert WDT als sensor niet aanwezig
  if (co2_enabled) co2 = readCO2();  // v1.7: bewaakt — pulseIn() blokkeert WDT als sensor niet aanwezig
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


  // Ventilation logica (slider zet mode = 1)
  if (vent_mode == 0) {  // AUTO
    vent_percent = map(constrain(co2, 400, 800), 400, 800, 0, 100);
  }  // MANUEEL: vent_percent = slider-waarde (gezet in handler)



  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
  mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);




  // Serial rapport (elke 15s, alleen als serial_verbose aan)
    if (serial_verbose && !ap_mode_active && millis() - lastSerial > (unsigned long)(serial_interval * 1000)) {
    lastSerial = millis();

    String upper_room = room_id;           // Kopieer de room_id
    upper_room.toUpperCase();              // Maak alles hoofdletters (bijv. "woonkamer" → "WOONKAMER")
    Serial.println("\n" + upper_room + " – " + String(uptime_sec) + " s");
    String divider = "";
    for (int i = 0; i < upper_room.length() + String(uptime_sec).length() + 12; i++) {
      divider += "─";
    }
    Serial.println(divider);
    Serial.printf("DHT22 Temp2          : %.2f °C\n", temp_dht);
    Serial.printf("DHT22 Humidity       : %.1f %%\n", humi);
    Serial.printf("Dauwpunt             : %.1f °C\n", dew);
    Serial.printf("DewAlert (Temp2<Dew) : %s\n", dew_alert ? "JA" : "NEE");
    Serial.printf("DS18B20 Temp1        : %.2f °C\n", temp_ds);
    Serial.printf("Room temp            : %.1f °C %s\n", room_temp, temp_melding.c_str());
    Serial.printf("Heating setpoint     : %d °C\n", heating_setpoint);
    Serial.printf("Heating mode         : %s\n", heating_mode == 0 ? "AUTO" : "MANUEEL");
    Serial.printf("Thuis/Uit modus      : %s\n", home_mode ? "Thuis" : "Uit");
    if (tstat_enabled) {
      Serial.printf("Hardware thermostaat : %s\n", tstat_on ? "AAN" : "UIT");
    }
    Serial.printf("Effective setpoint   : %.1f °C\n", effective_setpoint);
    Serial.printf("Heating aan          : %s\n", heating_on ? "JA" : "NEE");
    if (dust_enabled) {
      Serial.printf("Stof                 : %d\n", dust);
    }
    if (co2_enabled) {
      Serial.printf("CO₂                  : %d ppm\n", co2);
    }
    Serial.printf("Ventilatie snelheid  %         : %d %%\n", vent_percent);
    Serial.printf("Ventilation mode     : %s\n", vent_mode == 0 ? "AUTO" : "MANUEEL");
    if (sun_light_enabled) {
      Serial.printf("Zonlicht             : %d lux\n", sun_light);
    }
    Serial.printf("LDR (donker=100)     : %d\n", light_ldr);
    Serial.printf("MOV1 PIR trig/min    : %d\n", mov1_triggers);
    if (mov2_enabled) {
      Serial.printf("MOV2 PIR trig/min    : %d\n", mov2_triggers);
    }
    Serial.printf("MOV1 PIR licht aan   : %s\n", mov1_light ? "JA" : "NEE");
    if (mov2_enabled) {
      Serial.printf("MOV2 PIR licht aan   : %s\n", mov2_light ? "JA" : "NEE");
    }
    Serial.printf("Night mode (donker)  : %s\n", night ? "JA" : "NEE");
    Serial.printf("Bed switch           : %s\n", bed ? "AAN" : "UIT");
    if (beam_enabled) {
      Serial.printf("Beam sensor waarde   : %d\n", beam_value);
      Serial.printf("Beam sensor alert    : %s\n", beam_alert_new ? "JA" : "NEE");
    }
    Serial.printf("NeoPixel RGB         : %d, %d, %d\n", neo_r, neo_g, neo_b);
    Serial.printf("Dim snelheid (s)     : %d s\n", fade_duration);
    

    Serial.print("Pixel modes (MOV1");
      if (mov2_enabled) Serial.print(",MOV2");
      Serial.print("): ");
      Serial.print(pixel_mode[0]);
      if (mov2_enabled) {
        Serial.print(", ");
        Serial.print(pixel_mode[1]);
      }
     Serial.println();


    Serial.print("Pixels on (0-" + String(pixels_num-1) + ")      : ");
      for (int i = 0; i < pixels_num; i++) {
        Serial.print(pixel_on[i] ? "1" : "0");
      }
    Serial.println();
    Serial.printf("WiFi RSSI            : %d dBm\n", WiFi.RSSI());
    Serial.printf("WiFi kwaliteit       : %d %%\n", constrain(2 * (WiFi.RSSI() + 100), 0, 100));
    Serial.printf("Free heap            : %d %%\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    Serial.println("─────────────────────────────────────\n");
  }
}

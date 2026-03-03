/* ESP32_C6_MATTER_ROOM.ino – Zarlar thuisautomatisering
// Filip Delannoy

OPGEPAST: HARDWARE: ESP32-C6 16MB, arduino-esp32 3.3.2
BOARD: Custom partition table → Compileer met "partitions.csv" in de sketchfolder:
# Name,   Type, SubType, Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x600000,
app1,     app,  ota_1,   0x610000, 0x600000,
spiffs,   data, spiffs,  0xC10000, 0x3F0000,

v2.3  03mar26  Matter integrated, nvs correcties
v2.2  01mrt26  Matter transport-rij in /settings aangepast.
De dropdown-opties "WiFi (actief)" en "Thread (placeholder)"
Uitleg: WiFi = werkt, Thread = ESP32-C6 heeft de hardware maar arduino-esp32 3.3.2 nog niet productierijp!

v2.0  28feb26  Matter integratie:
  - Matter endpoints: temp, humidity, occupancy, thermostat, color light, on/off lights
  - ESPmDNS verwijderd (Matter vervangt intern mDNS)
  - Custom partitietabel 16MB vereist (partitions_16mb.csv in schetsmap)
  - serial_verbose: NVS toggle, instelbaar via /settings (geen hercompileren)
  - matter_transport: WiFi / Thread keuze via /settings
  - reset-all / reset-matter serial commando's (was: reset_nvs)
  - /matter webpagina: pairing code + Matter reset knop
  - Webserver + alle sensor/pixel/verwarmingslogica: ongewijzigd

v2.1  01mrt26  Endpoint-types gecorrigeerd + ignore_callbacks:
  - MatterOnOffLight matter_bed   → MatterOnOffPlugin
      → bed-modus is een logische schakelaar, geen lamp
  - MatterOnOffLight matter_thuis → MatterOnOffPlugin
      → aanwezigheidsmodus is een logische schakelaar, geen lamp
  - matter_pir1_light / matter_pir2_light blijven MatterOnOffLight
      → sturen echte NeoPixels aan (pixel_mode[0/1]) → wél lampen
  - ignore_callbacks flag toegevoegd (HVAC-patroon)
      → voorkomt feedback-loop in matter_pixels.onChangeOnOff
      die programmatisch setOnOff(true) terugschrijft
*/

#include <WiFi.h>
// ESPmDNS VERWIJDERD — Matter neemt intern mDNS over
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <DHT.h>
#include <OneWireNg_CurrentPlatform.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>
#include <Preferences.h>
#include <string.h>
#include <nvs_flash.h>
#include <Matter.h>
#include <MatterEndPoints/MatterTemperatureSensor.h>
#include <MatterEndPoints/MatterHumiditySensor.h>
#include <MatterEndPoints/MatterOccupancySensor.h>
#include <MatterEndPoints/MatterThermostat.h>
#include <MatterEndPoints/MatterColorLight.h>
#include <MatterEndPoints/MatterOnOffLight.h>

Preferences preferences;

#define Serial Serial0  // Fix: ESP32-C6 gebruikt Serial0 als hardware serial

// ============== PIN DEFINITIONS (ESP32-C6 via Photon Shield) ==============
#define DHT_PIN        6   // IO6  - DHT22 data
#define ONE_WIRE_PIN   3   // IO3  - DS18B20 OneWire
#define PIR_MOV1       5   // IO5  - MOV1 PIR
#define PIR_MOV2      19   // IO19 - MOV2 PIR
#define SHARP_LED     12   // IO12 - Sharp dust LED out
#define SHARP_ANALOG   7   // IO7  - Sharp dust analog
#define LDR_ANALOG     1   // IO1  - LDR1 analog (10k pull-up naar 3V3!)
#define CO2_PWM       18   // IO18 - CO2 PWM input
#define TSTAT_PIN     10   // IO10 - TSTAT switch
#define OPTION_LDR     2   // IO2  - LDR2 analog
#define NEOPIXEL_PIN   4   // IO4  - Pixels data


// ============== NVS KEYS ==============
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
const char* NVS_BED_STATE           = "bed_state";
const char* NVS_CURRENT_SETPOINT    = "curr_setpoint";
const char* NVS_FADE_DURATION       = "fade_duration";
const char* NVS_HOME_MODE_STATE     = "home_mode_state";
const char* NVS_PIXEL_NICK_BASE     = "pixel_nick_";
const char* NVS_PIXEL_ON_BASE       = "pixel_on_";
const char* NVS_PIXEL_USER_ON_BASE  = "pixel_user_on_";
const char* NVS_PIXEL_MODE_0        = "pixel_mode_0";
const char* NVS_PIXEL_MODE_1        = "pixel_mode_1";
const char* NVS_DS_COUNT            = "ds_count";
const char* NVS_DS_PRIMARY          = "ds_primary";
// --- Nieuw in v2.0 ---
const char* NVS_SERIAL_VERBOSE      = "serial_verbose";  // bool: statusrapport aan/uit
const char* NVS_MATTER_TRANSPORT    = "matter_transport"; // int: 0=WiFi, 1=Thread


// ============== LIBRARY INITIALISATIE ==============
DHT dht(DHT_PIN, DHT22);
OneWireNg_CurrentPlatform ow(ONE_WIRE_PIN, false);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);


// ============== MATTER ENDPOINTS ==============
MatterTemperatureSensor  matter_temp;        // room_temp (°C)
MatterHumiditySensor     matter_humidity;    // humi (%)
MatterOccupancySensor    matter_motion1;     // MOV1 PIR
MatterOccupancySensor    matter_motion2;     // MOV2 PIR
MatterTemperatureSensor  matter_co2;         // CO2 FAKE (ppm÷100 als °C – hernoem naar "CO2 ÷100")
MatterTemperatureSensor  matter_lux;         // Lux FAKE (lux÷10 als °C – hernoem naar "Lux ÷10")
MatterThermostat         matter_thermostat;  // heating_setpoint + room_temp
MatterColorLight         matter_pixels;      // neo_r/g/b kleurpicker
MatterOnOffLight         matter_bed;         // bed (0/1)
MatterOnOffLight         matter_thuis;       // home_mode (0=Weg, 1=Thuis)
MatterOnOffLight         matter_pir1_light;  // pixel_mode[0] (0=AUTO, 1=MANUEEL)
MatterOnOffLight         matter_pir2_light;  // pixel_mode[1] (0=AUTO, 1=MANUEEL)


// ============== ROOM CONFIGURATIE (uit NVS) ==============
String room_id              = "Testroom";
String wifi_ssid            = "netwerknaam";
String wifi_pass            = "paswoord";
String static_ip_str        = "192.168.xx.xx";
String pixel_nicknames[30];
String mac_address          = "";


// ============== CONFIGUREERBARE DEFAULTS ==============
int heating_setpoint_default = 20;
int vent_request_default     = 0;
float dew_safety_margin      = 2.0;
int home_mode_default        = 0;
int light_dark_threshold     = 50;
unsigned long mov_window_ms  = 60000;
int ldr_dark_threshold       = 50;
int beam_alert_threshold     = 50;


// ============== OPTIONELE FEATURES ==============
bool co2_enabled       = true;
bool dust_enabled      = true;
bool sun_light_enabled = true;
bool mov2_enabled      = true;
bool tstat_enabled     = true;
bool beam_enabled      = true;
int  pixels_num        = 8;
int  num_mov_pixels    = 2;

// --- Nieuw in v2.0 ---
bool serial_verbose   = true;   // Statusrapport aan/uit (instelbaar via /settings)
int  matter_transport = 0;      // 0=WiFi, 1=Thread


// ============== AP MODE ==============
bool ap_mode_active = false;
DNSServer dnsServer;
const byte DNS_PORT = 53;

// mDNS: volledig verwijderd — Matter vervangt


// ============== HVAC VARIABELEN ==============
float room_temp = 0.0;
String temp_melding = "";
int heating_setpoint = 20;
int heating_on = 0;
int vent_percent = 0;
int heating_mode = 0;
int vent_mode = 0;
int home_mode = 1;

// Matter thermostat mode (0=UIT, 4=VERWARMING per spec)
uint8_t thermostat_mode = 0;


// ============== SENSOR VARIABELEN ==============
float temp_dht = 0, temp_ds = 0, humi = 0, dew = 0;
int light_ldr = 0, sun_light = 0, dust = 0, co2 = 0;
int tstat_on = 0, mov1_triggers = 0, mov2_triggers = 0;
int mov1_light = 0, mov2_light = 0;
unsigned long uptime_sec = 0;
int dew_alert = 0;
int night = 0;
int bed = 0;
int beam_value = 0;
int beam_alert_new = 0;
uint8_t neo_r = 255;
uint8_t neo_g = 255;
uint8_t neo_b = 255;

// DS18B20 multi-sensor (v1.3) - max 4 sensors
#define DS_MAX_SENSORS 4
int ds_count = 0;
OneWireNg::Id ds_addrs[DS_MAX_SENSORS];
float temp_ds_arr[DS_MAX_SENSORS];
String ds_nicknames[DS_MAX_SENSORS];
int ds_primary = 0;


// ============== PIXEL ARRAYS ==============
int pixel_mode[2] = {0, 0};
bool pixel_on[30] = {false};
bool pixel_user_on[30] = {false};

uint8_t currR[30], currG[30], currB[30];
uint8_t targetR[30], targetG[30], targetB[30];
uint8_t startR[30], startG[30], startB[30];
float fade_progress[30] = {0.0};


// ============== PIR TIMING ==============
unsigned long mov1_off_time = 0;
unsigned long mov2_off_time = 0;
const unsigned long LIGHT_ON_DURATION = 30000;
int LDR_DARK_THRESHOLD = 40;
unsigned long MOV_WINDOW_MS = 60000;

#define MOV_BUF_SIZE 50
unsigned long mov1Times[MOV_BUF_SIZE] = {0};
unsigned long mov2Times[MOV_BUF_SIZE] = {0};


// ============== FADE ENGINE ==============
unsigned long lastFadeStep = 0;
unsigned long fade_interval_ms = 15;
int fade_duration = 2;
const int FADE_NUM_STEPS = 20;

void initFadeEngine() {
  for (int i = 0; i < pixels_num; i++) {
    uint32_t c = pixels.getPixelColor(i);
    currR[i] = (c >> 16) & 0xFF;
    currG[i] = (c >> 8)  & 0xFF;
    currB[i] = c & 0xFF;
    targetR[i] = currR[i]; targetG[i] = currG[i]; targetB[i] = currB[i];
    startR[i] = currR[i]; startG[i] = currG[i]; startB[i] = currB[i];
  }
}

void setTargetColor(int idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < 0 || idx >= pixels_num) return;
  if (targetR[idx] != r || targetG[idx] != g || targetB[idx] != b) {
    targetR[idx] = r; targetG[idx] = g; targetB[idx] = b;
    startR[idx] = currR[idx]; startG[idx] = currG[idx]; startB[idx] = currB[idx];
    fade_progress[idx] = 0.0;
  }
}

void updateFades() {
  unsigned long now = millis();
  if (now - lastFadeStep < fade_interval_ms) return;
  lastFadeStep = now;
  bool changed = false;
  for (int i = 0; i < pixels_num; i++) {
    if (fade_progress[i] >= 1.0f &&
        currR[i] == targetR[i] && currG[i] == targetG[i] && currB[i] == targetB[i]) continue;
    fade_progress[i] += 1.0f / FADE_NUM_STEPS;
    if (fade_progress[i] > 1.0f) fade_progress[i] = 1.0f;
    float ease = sin(fade_progress[i] * PI / 2.0f);
    int newR = startR[i] + (int)round((targetR[i] - startR[i]) * ease);
    int newG = startG[i] + (int)round((targetG[i] - startG[i]) * ease);
    int newB = startB[i] + (int)round((targetB[i] - startB[i]) * ease);
    currR[i] = constrain(newR, 0, 255);
    currG[i] = constrain(newG, 0, 255);
    currB[i] = constrain(newB, 0, 255);
    if (fade_progress[i] >= 1.0f) {
      currR[i] = targetR[i]; currG[i] = targetG[i]; currB[i] = targetB[i];
    }
    pixels.setPixelColor(i, currR[i], currG[i], currB[i]);
    changed = true;
  }
  if (changed) pixels.show();
}

void updateFadeInterval() {
  fade_duration = constrain(fade_duration, 1, 10);
  fade_interval_ms = (fade_duration * 1000UL) / FADE_NUM_STEPS;
  if (fade_interval_ms < 10) fade_interval_ms = 10;
}


// ============== HELPERS ==============
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

float calculateDewPoint(float t, float h) {
  return isnan(t) || isnan(h) ? 0 : t - ((100 - h) / 5.0);
}

int scaleLDR(int r) { return map(constrain(r, 100, 3800), 100, 3800, 100, 0); }

int readDust() {
  digitalWrite(SHARP_LED, LOW); delayMicroseconds(280);
  int v = analogRead(SHARP_ANALOG);
  delayMicroseconds(40); digitalWrite(SHARP_LED, HIGH); delayMicroseconds(9680);
  return v;
}


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
  for (int i = 0; i < ds_count; i++) {
    ow.reset(); ow.writeByte(0x55);
    for (int j = 0; j < 8; j++) ow.writeByte(ds_addrs[i][j]);
    ow.writeByte(0x44); delay(750);
    ow.reset(); ow.writeByte(0x55);
    for (int j = 0; j < 8; j++) ow.writeByte(ds_addrs[i][j]);
    ow.writeByte(0xBE);
    uint8_t data[9];
    for (int j = 0; j < 9; j++) data[j] = ow.touchByte(0xFF);
    int16_t raw = (int16_t)((data[1] << 8) | data[0]);
    float t = raw / 16.0f;
    if (t >= -55.0f && t <= 125.0f) temp_ds_arr[i] = t;
  }
  temp_ds = (ds_count > 0) ? temp_ds_arr[ds_primary] : 0.0;
}
// ============== EINDE DS18B20 ==============


int readCO2() {
  unsigned long h = pulseIn(CO2_PWM, HIGH, 200000);
  unsigned long l = pulseIn(CO2_PWM, LOW, 200000);
  return (h < 100 || l < 100) ? 0 : (int)(5000.0 * (h - 2.0) / (h + l - 4.0));
}


// ============== MATTER: SENSOR UPDATES → HOMEKIT ==============
// Aangeroepen in de slow-loop (elke 2s), enkel als WiFi STA verbonden
void update_matter_sensors() {
  matter_temp.setTemperature(room_temp);
  matter_humidity.setHumidity(humi);
  matter_motion1.setOccupancy(mov1_light);
  matter_motion2.setOccupancy(mov2_light);
  matter_co2.setTemperature(co2 / 100.0f);        // FAKE: ppm÷100 als "°C"
  matter_lux.setTemperature(sun_light / 10.0f);   // FAKE: lux÷10 als "°C"
  matter_thermostat.setLocalTemperature(room_temp);
}


// ============== SERIAL COMMANDO'S (v2.0) ==============
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // reset-all: wis alles (TESTROOM config + Matter) → reboot
    if (cmd.equalsIgnoreCase("reset-all")) {
      Serial.println(F("\n=== RESET-ALL: wis TESTROOM config + Matter namespaces ==="));
      preferences.clear();                                          // wist "room-config"
      preferences.begin("chip-factory",  false); preferences.clear(); preferences.end();
      preferences.begin("chip-config",   false); preferences.clear(); preferences.end();
      preferences.begin("chip-counters", false); preferences.clear(); preferences.end();
      Serial.println(F("Alles gewist – reboot..."));
      delay(500); ESP.restart();
    }

    // reset-matter: wis alleen Matter namespaces → reboot
    if (cmd.equalsIgnoreCase("reset-matter")) {
      Serial.println(F("\n=== RESET-MATTER: wis alleen Matter namespaces ==="));
      preferences.begin("chip-factory",  false); preferences.clear(); preferences.end();
      preferences.begin("chip-config",   false); preferences.clear(); preferences.end();
      preferences.begin("chip-counters", false); preferences.clear(); preferences.end();
      Serial.println(F("Matter gewist – reboot..."));
      delay(500); ESP.restart();
    }

    // status: druk statusrapport af (ongeacht serial_verbose instelling)
    if (cmd.equalsIgnoreCase("status")) {
      Serial.println(F("[CMD] Status afgedrukt:"));
      // forceert één uitdraai van het rapport hieronder
      // (zet lastSerial ver in het verleden)
      lastSerial = 0;
    }
  }
}


// ============== COMPACT STATUSRAPPORT ==============
unsigned long lastSerial = 0;

void print_status_compact() {
  String upper_room = room_id;
  upper_room.toUpperCase();
  const char* th_str = (thermostat_mode == 4) ? "VERWARM" :
                       (thermostat_mode == 1) ? "AUTO"    :
                       (thermostat_mode == 3) ? "KOELING" : "UIT";
  const char* transport_str = (matter_transport == 1) ? "Thread" : "WiFi";

  Serial.println(F("\n╔══════════════════════════════════════════╗"));
  Serial.printf(   "║  %-20s  %8lu s       ║\n", upper_room.c_str(), uptime_sec);
  Serial.println(F("╠══════════════════════════════════════════╣"));
  Serial.println(F("║ SENSOREN                                  ║"));
  Serial.printf(   "║  room_temp : %5.1f°C    humi   : %4.0f %%     ║\n", room_temp, humi);
  Serial.printf(   "║  dew       : %5.1f°C    alert  : %-3s         ║\n", dew, dew_alert ? "JA" : "NEE");
  Serial.printf(   "║  DS18B20   : %5.1f°C    DHT22  : %5.1f°C    ║\n", temp_ds, temp_dht);
  if (co2_enabled)
    Serial.printf( "║  CO2       : %4d ppm   lux    : %4d        ║\n", co2, sun_light);
  Serial.printf(   "║  LDR       : %3d         stof   : %4d        ║\n", light_ldr, dust);
  Serial.printf(   "║  MOV1      : %-8s   MOV2   : %-8s   ║\n",
                   mov1_light ? "beweging" : "rust",
                   mov2_light ? "beweging" : "rust");
  if (tstat_enabled)
    Serial.printf( "║  tstat     : %-3s         beam   : %3d (%-3s)  ║\n",
                   tstat_on ? "AAN" : "UIT", beam_value, beam_alert_new ? "JA" : "NEE");
  Serial.println(F("║ BEDIENING                                 ║"));
  Serial.printf(   "║  setpoint  : %2d°C  [%s]  heating: %-3s       ║\n",
                   heating_setpoint, th_str, heating_on ? "AAN" : "UIT");
  Serial.printf(   "║  home_mode : %-5s       bed    : %-3s         ║\n",
                   home_mode ? "THUIS" : "WEG", bed ? "AAN" : "UIT");
  Serial.printf(   "║  vent      : %3d%%        mode   : %-7s    ║\n",
                   vent_percent, vent_mode ? "MANUEEL" : "AUTO");
  Serial.printf(   "║  neo RGB   : %3d,%3d,%3d                    ║\n", neo_r, neo_g, neo_b);
  Serial.printf(   "║  pix[0]MOV1: %-7s   pix[1]MOV2: %-7s ║\n",
                   pixel_mode[0] ? "MANUEEL" : "AUTO",
                   pixel_mode[1] ? "MANUEEL" : "AUTO");
  Serial.println(F("╠══════════════════════════════════════════╣"));
  Serial.printf(   "║  Matter    : %-9s  transport : %-6s  ║\n",
                   Matter.isDeviceCommissioned() ? "GEPAARD" : "ONGEPAARD", transport_str);
  Serial.printf(   "║  Heap      : %3d %%        RSSI      : %4d dBm ║\n",
                   (ESP.getFreeHeap() * 100) / ESP.getHeapSize(), WiFi.RSSI());
  Serial.println(F("╚══════════════════════════════════════════╝\n"));
}


// ============== JSON ==============
String getJSON() {
  String pixel_on_str = "";
  String pixel_mode_str = String(pixel_mode[0]) + String(pixel_mode[1]);
  for (int i = 0; i < pixels_num; i++) pixel_on_str += pixel_on[i] ? "1" : "0";

  String ds_json = "";
  for (int i = 0; i < ds_count; i++)
    ds_json += ",\"ds" + String(i) + "\":" + String(temp_ds_arr[i], 1);

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
         ",\"matter_commissioned\":" + String(Matter.isDeviceCommissioned() ? 1 : 0) +
         ",\"ds_count\":" + String(ds_count) +
         ",\"ds_primary\":" + String(ds_primary) +
         ds_json + "}";
}


String getFormattedDateTime() {
  time_t now; time(&now);
  if (now < 1700000000) return "tijd nog niet gesynchroniseerd";
  struct tm tm; localtime_r(&now, &tm);
  char buf[32]; strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tm);
  return String(buf);
}


// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);

  // === 1. NVS INITIALISATIE ===
  preferences.begin("room-config", false);

  // Boot: restore pixel states
  for (int i = 0; i < pixels_num; i++) {
    if (i < num_mov_pixels) {
      const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
      pixel_mode[i] = preferences.getInt(key, 0);
    } else {
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      pixel_on[i] = preferences.getBool(key.c_str(), false);
    }
  }

  // Eerste boot detectie
  bool first_boot = preferences.getString(NVS_ROOM_ID, "").isEmpty();

  if (first_boot) {
    Serial.println(F("\n*** EERSTE BOOT – DEFAULTS TOEPASSEN ***"));
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
    for (int i = 0; i < 30; i++) {
      String key = String(NVS_PIXEL_ON_BASE) + String(i);
      preferences.putBool(key.c_str(), false);
    }
    preferences.putInt(NVS_PIXEL_MODE_0, 0);
    preferences.putInt(NVS_PIXEL_MODE_1, 0);
    // v2.0 defaults
    preferences.putBool(NVS_SERIAL_VERBOSE, true);
    preferences.putInt(NVS_MATTER_TRANSPORT, 0);
    Serial.println(F("Defaults opgeslagen. Configureer via /settings"));
  }

  // === 2. LAAD ALLE NVS WAARDEN ===
  room_id     = preferences.getString(NVS_ROOM_ID, "Testroom");
  wifi_ssid   = preferences.getString(NVS_WIFI_SSID, "netwerknaam");
  wifi_pass   = preferences.getString(NVS_WIFI_PASS, "paswoord");
  static_ip_str = preferences.getString(NVS_STATIC_IP, "192.168.xx.xx");

  heating_setpoint_default = preferences.getInt(NVS_HEATING_SETPOINT, 20);
  vent_request_default     = preferences.getInt(NVS_VENT_REQUEST, 0);
  dew_safety_margin        = preferences.getFloat(NVS_DEW_MARGIN, 2.0);
  home_mode_default        = preferences.getInt(NVS_HOME_MODE, 0);
  light_dark_threshold     = preferences.getInt(NVS_LIGHT_THRESHOLD, 50);
  mov_window_ms            = preferences.getULong(NVS_MOV_WINDOW, 60000UL);
  ldr_dark_threshold       = preferences.getInt(NVS_LDR_DARK, 50);
  beam_alert_threshold     = preferences.getInt(NVS_BEAM_THRESHOLD, 50);

  co2_enabled       = preferences.getBool(NVS_CO2_ENABLED, true);
  dust_enabled      = preferences.getBool(NVS_DUST_ENABLED, true);
  sun_light_enabled = preferences.getBool(NVS_SUN_ENABLED, true);
  mov2_enabled      = preferences.getBool(NVS_MOV2_ENABLED, true);
  int num_mov_pixels = 1 + (mov2_enabled ? 1 : 0);
  tstat_enabled     = preferences.getBool(NVS_TSTAT_ENABLED, true);
  beam_enabled      = preferences.getBool(NVS_BEAM_ENABLED, true);
  neo_r = preferences.getUChar(NVS_NEO_R, 255);
  neo_g = preferences.getUChar(NVS_NEO_G, 255);
  neo_b = preferences.getUChar(NVS_NEO_B, 255);
  pixels_num = constrain(preferences.getInt(NVS_PIXELS_NUM, 8), 1, 30);

  // v2.0
  serial_verbose   = preferences.getBool(NVS_SERIAL_VERBOSE, true);
  matter_transport = preferences.getInt(NVS_MATTER_TRANSPORT, 0);

  // Pixel nicknames
  for (int i = 0; i < 30; i++) {
    String key = String(NVS_PIXEL_NICK_BASE) + String(i);
    pixel_nicknames[i] = preferences.getString(key.c_str(), "");
    if (pixel_nicknames[i].isEmpty()) {
      pixel_nicknames[i] = room_id + " Pixel " + String(i);
      preferences.putString(key.c_str(), pixel_nicknames[i]);
    }
  }

  // Pixel states
  for (int i = 0; i < pixels_num; i++) {
    String key = String(NVS_PIXEL_USER_ON_BASE) + String(i);
    pixel_user_on[i] = preferences.getBool(key.c_str(), false);
    pixel_on[i] = pixel_user_on[i];
  }

  pixel_mode[0] = preferences.getInt(NVS_PIXEL_MODE_0, 0);
  if (mov2_enabled) pixel_mode[1] = preferences.getInt(NVS_PIXEL_MODE_1, 0);
  else              pixel_mode[1] = 0;

  for (int i = pixels_num; i < 30; i++) pixel_nicknames[i] = "";

  // Runtime variabelen
  heating_setpoint = preferences.getInt(NVS_CURRENT_SETPOINT, heating_setpoint_default);
  vent_percent     = vent_request_default;
  home_mode        = preferences.getInt(NVS_HOME_MODE_STATE, home_mode_default);
  bed              = preferences.getBool(NVS_BED_STATE, false);
  LDR_DARK_THRESHOLD = ldr_dark_threshold;
  fade_duration    = constrain(preferences.getInt(NVS_FADE_DURATION, 2), 1, 10);
  updateFadeInterval();

  Serial.printf("Room ID: %s\n", room_id.c_str());
  Serial.printf("Serial verbose: %s | Matter transport: %s\n",
                serial_verbose ? "AAN" : "UIT",
                matter_transport == 1 ? "Thread" : "WiFi");

  // === 3. HARDWARE INIT ===
  pinMode(PIR_MOV1, INPUT_PULLUP);
  pinMode(PIR_MOV2, INPUT_PULLUP);
  pinMode(SHARP_LED, OUTPUT); digitalWrite(SHARP_LED, HIGH);
  pinMode(TSTAT_PIN, INPUT_PULLUP);
  pinMode(OPTION_LDR, INPUT);

  dht.begin();
  if (!tsl.begin()) Serial.println(F("TSL2561 niet gevonden"));
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);

  if (ds_count == 0 && preferences.getInt(NVS_DS_COUNT, 0) == 0) {
    Serial.println(F("Eerste boot → DS18B20 scan..."));
    scanDS18B20();
  } else {
    loadDS18B20fromNVS();
    Serial.printf("DS18B20: %d sensor(s) geladen uit NVS\n", ds_count);
  }

  pixels.begin();
  pixels.updateLength(pixels_num);
  pixels.clear(); pixels.show();
  initFadeEngine();
  updateFadeInterval();

  // Forceer pixel states direct na NVS laden
  for (int i = 0; i < pixels_num; i++) {
    uint8_t r = 0, g = 0, b = 0;
    bool is_on = false;
    if (i < num_mov_pixels) {
      if (pixel_mode[i] == 1) { r = neo_r; g = neo_g; b = neo_b; is_on = true; }
      if (i == 0) mov1_light = is_on ? 1 : 0;
      if (i == 1) mov2_light = is_on ? 1 : 0;
    } else {
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
  pixels.show();


  // === 4. WIFI ===
  WiFi.mode(WIFI_STA);

  IPAddress local_ip;
  if (local_ip.fromString(static_ip_str)) {
    IPAddress gateway = local_ip; gateway[3] = 1;
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(local_ip, gateway, subnet, gateway);
    Serial.printf("Static IP: %s (gateway %s)\n",
                  local_ip.toString().c_str(), gateway.toString().c_str());
  } else {
    Serial.println(F("Geen geldig static IP → DHCP"));
  }

  Serial.print(F("WiFi verbinden: ")); Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  mac_address = WiFi.macAddress();
  Serial.println("MAC: " + mac_address);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500); Serial.print("."); attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi verbonden: " + WiFi.localIP().toString());
    ap_mode_active = false;
  } else {
    Serial.println(F("\nWiFi mislukt → Access Point starten"));
    WiFi.mode(WIFI_AP_STA);
    String ap_ssid = "ROOM-" + room_id;
    WiFi.softAP(ap_ssid.c_str());
    IPAddress ap_ip(192, 168, 4, 1);
    WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
    delay(1000);
    ap_mode_active = true;
    dnsServer.start(DNS_PORT, "*", ap_ip);
    Serial.printf("AP: %s — http://192.168.4.1/settings\n", ap_ssid.c_str());
  }

  // === 5. TIJDINITIALISATIE ===
  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();
  configTzTime("CET-1CEST,M3.5.0/02,M10.5.0/03", "pool.ntp.org", "time.nist.gov");


  // === 5b. MATTER INITIALISATIE (alleen bij STA verbinding) ===
  // ESPmDNS: NIET meer starten — Matter neemt intern mDNS over
  if (!ap_mode_active) {

    if (matter_transport == 1) {
      Serial.println(F("Matter transport: Thread (vereist border router: Apple TV 4K of HomePod)"));
      // Thread-specifieke OpenThread initialisatie hier toevoegen indien nodig
      // arduino-esp32 3.3.2: Matter.begin() detecteert Thread automatisch
      // als de Thread radio geactiveerd is via de board config
    } else {
      Serial.println(F("Matter transport: WiFi"));
    }

    // --- Endpoints initialiseren ---
    matter_temp.begin();
    matter_humidity.begin();
    matter_motion1.begin();
    matter_motion2.begin();
    matter_co2.begin();
    matter_lux.begin();

    // Thermostat: heating-only (ControlSequenceOfOperation = 2)
    matter_thermostat.begin((MatterThermostat::ControlSequenceOfOperation_t)2);
    matter_thermostat.setLocalTemperature(room_temp);
    matter_thermostat.setHeatingSetpoint((float)heating_setpoint);

    matter_thermostat.onChangeHeatingSetpoint([](double new_sp) -> bool {
      heating_setpoint = constrain((int)round(new_sp), 15, 28);
      preferences.putInt(NVS_CURRENT_SETPOINT, heating_setpoint);  // NVS persistent
      Serial.printf("[HomeKit] heating_setpoint → %d°C\n", heating_setpoint);
      return true;
    });

    matter_thermostat.onChangeMode([](uint8_t mode) -> bool {
      thermostat_mode = mode;
      const char* s = (mode==0)?"UIT":(mode==1)?"AUTO":(mode==3)?"KOELING":(mode==4)?"VERWARMING":"?";
      Serial.printf("[HomeKit] thermostat_mode → %s (%d)\n", s, mode);
      return true;
    });

    // Sfeerverlichting kleurpicker
    // MEMO: matter_pixels is GEEN aan/uit schakelaar — uitsluitend kleurpicker
    //       on/off toggle wordt genegeerd en altijd op "aan" gehouden
    //       onChangeColorHSV → converteert HSV → RGB → neo_r/g/b voor alle pixels
    matter_pixels.begin();
    matter_pixels.setOnOff(true);
    matter_pixels.onChangeOnOff([](bool on_off) -> bool {
      matter_pixels.setOnOff(true);  // Altijd aan houden
      Serial.println(F("[HomeKit] matter_pixels on/off genegeerd (kleurpicker only)"));
      return true;
    });
    matter_pixels.onChangeColorHSV([](HsvColor_t hsv) -> bool {
      float h = (hsv.h / 254.0f) * 360.0f;
      float s = hsv.s / 254.0f;
      float v = hsv.v / 254.0f;
      float c = v*s, x = c*(1.0f - fabsf(fmodf(h/60.0f, 2.0f) - 1.0f)), m = v - c;
      float rr, gg, bb;
      if      (h < 60)  { rr=c; gg=x; bb=0; }
      else if (h < 120) { rr=x; gg=c; bb=0; }
      else if (h < 180) { rr=0; gg=c; bb=x; }
      else if (h < 240) { rr=0; gg=x; bb=c; }
      else if (h < 300) { rr=x; gg=0; bb=c; }
      else              { rr=c; gg=0; bb=x; }
      neo_r = (uint8_t)((rr+m)*255);
      neo_g = (uint8_t)((gg+m)*255);
      neo_b = (uint8_t)((bb+m)*255);
      preferences.putUChar(NVS_NEO_R, neo_r);  // NVS persistent
      preferences.putUChar(NVS_NEO_G, neo_g);
      preferences.putUChar(NVS_NEO_B, neo_b);
      Serial.printf("[HomeKit] neo_r/g/b → %d,%d,%d\n", neo_r, neo_g, neo_b);
      return true;
    });

    // Bed
    matter_bed.begin();
    matter_bed.setOnOff(bed);
    matter_bed.onChangeOnOff([](bool on_off) -> bool {
      bed = on_off ? 1 : 0;
      preferences.putBool(NVS_BED_STATE, bed);  // NVS persistent
      Serial.printf("[HomeKit] bed → %s\n", bed ? "AAN" : "UIT");
      return true;
    });

    // Thuis / Weg
    matter_thuis.begin();
    matter_thuis.setOnOff(home_mode);
    matter_thuis.onChangeOnOff([](bool on_off) -> bool {
      home_mode = on_off ? 1 : 0;
      preferences.putInt(NVS_HOME_MODE_STATE, home_mode);  // NVS persistent
      Serial.printf("[HomeKit] home_mode → %s\n", home_mode ? "THUIS" : "WEG");
      return true;
    });

    // PIR1 manueel override
    matter_pir1_light.begin();
    matter_pir1_light.setOnOff(pixel_mode[0]);
    matter_pir1_light.onChangeOnOff([](bool on_off) -> bool {
      pixel_mode[0] = on_off ? 1 : 0;
      preferences.putInt(NVS_PIXEL_MODE_0, pixel_mode[0]);  // NVS persistent
      Serial.printf("[HomeKit] pixel_mode[0] MOV1 → %s\n", pixel_mode[0] ? "MANUEEL" : "AUTO");
      return true;
    });

    // PIR2 manueel override
    matter_pir2_light.begin();
    matter_pir2_light.setOnOff(pixel_mode[1]);
    matter_pir2_light.onChangeOnOff([](bool on_off) -> bool {
      pixel_mode[1] = on_off ? 1 : 0;
      preferences.putInt(NVS_PIXEL_MODE_1, pixel_mode[1]);  // NVS persistent
      Serial.printf("[HomeKit] pixel_mode[1] MOV2 → %s\n", pixel_mode[1] ? "MANUEEL" : "AUTO");
      return true;
    });

    // --- Matter starten (non-blocking) ---
    Matter.begin();

    Serial.println(F("\n══════════════════════════════════════"));
    if (!Matter.isDeviceCommissioned()) {
      Serial.println(F("MATTER: Nog niet gepaard."));
      Serial.println(F("► Manuele pairingcode:"));
      Serial.println("    " + Matter.getManualPairingCode());
      Serial.println(F("► Home app → + → Accessoire → Meer opties → code invoeren"));
      Serial.println(F("► Of: ga naar http://<IP>/matter"));
    } else {
      Serial.println(F("MATTER: Al gepaard."));
      Serial.println(F("Typ 'reset-matter' om Matter pairing te wissen."));
    }
    Serial.println(F("══════════════════════════════════════\n"));

    update_matter_sensors();  // Eerste update direct na init

  } else {
    Serial.println(F("AP-mode actief → Matter overgeslagen (WiFi STA vereist)"));
  }


  // === 6. WEBSERVER ROUTES ===
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");


  // === HOME PAGE ===
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html;
    html.reserve(8000);
    html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)rawliteral" + room_id + R"rawliteral( Status & Control</title>
  <style>
  body {font-family:Arial,Helvetica,sans-serif;background:#ffffff;margin:0;padding:0;}
  .header {display:flex;background:#ffcc00;color:black;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;box-sizing:border-box;}
  .header-left {flex:1;text-align:left;}
  .header-right {flex:1;text-align:right;font-size:15px;}
  .container {display:flex;flex-direction:row;min-height:calc(100vh - 60px);}
  .sidebar {width:80px;padding:10px 5px;background:#ffffff;border-right:3px solid #cc0000;box-sizing:border-box;flex-shrink:0;}
  .sidebar a {display:block;background:#336699;color:white;padding:8px;margin:8px 0;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;line-height:1.3;width:60px;box-sizing:border-box;margin-left:auto;margin-right:auto;}
  .sidebar a:hover {background:#003366;}
  .sidebar a.active {background:#cc0000;}
  .main {flex:1;padding:15px;overflow-y:auto;box-sizing:border-box;}
  .group-title {font-size:17px;font-style:italic;font-weight:bold;color:#336699;margin:20px 0 8px 0;}
  table {width:100%;border-collapse:collapse;margin-bottom:15px;}
  td.label {color:#336699;font-size:13px;padding:8px 5px;width:30%;border-bottom:1px solid #ddd;text-align:left;vertical-align:middle;}
  td.value {background:#e6f0ff;font-size:13px;padding:8px 5px;width:100px;border-bottom:1px solid #ddd;text-align:center;vertical-align:middle;}
  td.control {font-size:13px;padding:8px 5px;width:auto;border-bottom:1px solid #ddd;text-align:right;vertical-align:middle;}
  .slider {width:150px;height:28px;}
  .switch {position:relative;display:inline-block;width:50px;height:28px;vertical-align:middle;}
  .switch input {opacity:0;width:0;height:0;}
  .slider-switch {position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.4s;border-radius:28px;}
  .slider-switch:before {position:absolute;content:"";height:20px;width:20px;left:4px;bottom:4px;background:white;transition:.4s;border-radius:50%;}
  input:checked + .slider-switch {background:#336699;}
  input:checked + .slider-switch:before {transform:translateX(22px);}
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
    <div class="header-right" id="hdr_time">)rawliteral" + getFormattedDateTime() + R"rawliteral(</div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/" class="active">Status</a>
      <a href="/neopixel">Pixels</a>
      <a href="/matter">Matter</a>
      <a href="/update">OTA</a>
      <a href="/json">JSON</a>
      <a href="/settings">Settings</a>
    </div>
    <div class="main" id="main_content">Laden...</div>
  </div>
<script>
function updateUI(d) {
  let html = '';
  html += '<div class="group-title">Klimaat</div><table>';
  html += '<tr><td class="label">Kamertemp (DS18B20)</td><td class="value">' + d.h + ' °C</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Kamertemp (DHT22)</td><td class="value">' + d.g + ' °C</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Vochtigheid</td><td class="value">' + d.d + ' %</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Dauwpunt</td><td class="value">' + d.c + ' °C</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Dew Alert</td><td class="value" style="color:' + (d.k ? '#cc0000' : '#336699') + '">' + (d.k ? 'JA' : 'NEE') + '</td><td class="control"></td></tr>';
  html += '</table>';

  html += '<div class="group-title">Verwarming</div><table>';
  html += '<tr><td class="label">Setpoint</td><td class="value">' + d.aa + ' °C</td>';
  html += '<td class="control"><input class="slider" type="range" min="15" max="28" value="' + d.aa + '" oninput="this.nextSibling.textContent=this.value" onchange="setSetpoint(this.value)"><span> ' + d.aa + '</span></td></tr>';
  html += '<tr><td class="label">Verwarming</td><td class="value" style="color:' + (d.y ? '#cc0000' : '#336699') + '">' + (d.y ? 'AAN' : 'UIT') + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Thuis modus</td><td class="value">' + (d.af ? 'Thuis' : 'Weg') + '</td>';
  html += '<td class="control"><label class="switch"><input type="checkbox"' + (d.af ? ' checked' : '') + ' onchange="fetch(\'/toggle_home\')"><span class="slider-switch"></span></label></td></tr>';
  html += '</table>';

  html += '<div class="group-title">Ventilatie</div><table>';
  html += '<tr><td class="label">Ventilatie %</td><td class="value">' + d.z + ' %</td>';
  html += '<td class="control"><input class="slider" type="range" min="0" max="100" value="' + d.z + '" oninput="this.nextSibling.textContent=this.value" onchange="setVent(this.value)"><span> ' + d.z + '</span></td></tr>';
  html += '</table>';

  html += '<div class="group-title">Sensoren</div><table>';
  html += '<tr><td class="label">CO₂</td><td class="value">' + d.a + ' ppm</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Stof</td><td class="value">' + d.b + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Lux (TSL2561)</td><td class="value">' + d.f + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">LDR (donker=100)</td><td class="value">' + d.e + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Beam waarde</td><td class="value">' + d.o + '</td><td class="control"></td></tr>';
  html += '</table>';

  html += '<div class="group-title">Aanwezigheid & Bed</div><table>';
  html += '<tr><td class="label">MOV1</td><td class="value">' + (d.m ? 'beweging' : 'rust') + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">MOV2</td><td class="value">' + (d.n ? 'beweging' : 'rust') + '</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Bed</td><td class="value">' + (d.r ? 'AAN' : 'UIT') + '</td>';
  html += '<td class="control"><label class="switch"><input type="checkbox"' + (d.r ? ' checked' : '') + ' onchange="fetch(\'/toggle_bed\')"><span class="slider-switch"></span></label></td></tr>';
  html += '</table>';

  html += '<div class="group-title">NeoPixels (RGB ' + d.s + ',' + d.t + ',' + d.u + ')</div><table>';
  html += '<tr><td class="label">Dim snelheid (s)</td><td class="value">' + d.ab + '</td>';
  html += '<td class="control"><input class="slider" type="range" min="1" max="10" value="' + d.ab + '" oninput="this.nextSibling.textContent=this.value" onchange="setFade(this.value)"><span> ' + d.ab + '</span></td></tr>';
  html += '</table>';

  html += '<div class="group-title">Systeem</div><table>';
  html += '<tr><td class="label">WiFi RSSI</td><td class="value">' + d.v + ' dBm (' + d.w + '%)</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Vrij geheugen</td><td class="value">' + d.x + ' %</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Uptime</td><td class="value">' + d.ac + ' s</td><td class="control"></td></tr>';
  html += '<tr><td class="label">Matter</td><td class="value">' + (d.matter_commissioned ? '✅ Gepaard' : '⏳ Ongepaard') + '</td><td class="control"><a href="/matter" style="color:#336699;font-size:12px;">→ Matter</a></td></tr>';
  html += '</table>';

  document.getElementById('main_content').innerHTML = html;
}
function setSetpoint(v) { fetch('/set_setpoint?setpoint=' + v); }
function setVent(v) { fetch('/set_vent?vent=' + v); }
function setFade(v) { fetch('/set_fade_duration?duration=' + v); }
function poll() {
  fetch('/json').then(r => r.json()).then(d => { updateUI(d); }).catch(()=>{});
}
poll();
setInterval(poll, 2000);
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


  // === MATTER PAGINA ===
  server.on("/matter", HTTP_GET, [](AsyncWebServerRequest *request) {
    String commissioned = Matter.isDeviceCommissioned() ? "true" : "false";
    String pairingCode  = Matter.isDeviceCommissioned() ? "" : Matter.getManualPairingCode();
    String transport_str = (matter_transport == 1) ? "Thread" : "WiFi";

    String html;
    html.reserve(3000);
    html = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Matter</title>
  <style>
    body {font-family:Arial,Helvetica,sans-serif;background:#fff;margin:0;padding:0;}
    .header {display:flex;background:#ffcc00;color:black;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}
    .header-left {flex:1;}
    .header-right {flex:1;text-align:right;font-size:15px;}
    .sidebar {width:80px;padding:10px 5px;background:#fff;border-right:3px solid #cc0000;box-sizing:border-box;flex-shrink:0;}
    .sidebar a {display:block;background:#336699;color:white;padding:8px;margin:8px 0;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;line-height:1.3;width:60px;margin-left:auto;margin-right:auto;}
    .sidebar a:hover {background:#003366;}
    .sidebar a.active {background:#cc0000;}
    .container {display:flex;min-height:calc(100vh - 60px);}
    .main {flex:1;padding:30px;}
    .card {background:#e6f0ff;border:2px solid #336699;border-radius:10px;padding:25px;max-width:500px;margin:20px 0;}
    .code {font-family:monospace;font-size:28px;font-weight:bold;color:#003366;background:#fff;padding:12px 20px;border-radius:6px;border:2px solid #336699;display:inline-block;letter-spacing:2px;margin:12px 0;}
    .ok {color:#006600;font-size:22px;font-weight:bold;}
    .btn-reset {background:#cc0000;color:white;padding:10px 24px;border:none;border-radius:6px;font-size:15px;cursor:pointer;margin-top:20px;}
    .btn-reset:hover {background:#990000;}
    .hint {font-size:13px;color:#666;margin-top:8px;}
    .transport {background:#fffacd;border:1px solid #ccc;border-radius:6px;padding:8px 14px;font-size:13px;display:inline-block;margin-top:10px;}
  </style>
</head>
<body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
    <div class="header-right">Matter / HomeKit</div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status</a>
      <a href="/neopixel">Pixels</a>
      <a href="/matter" class="active">Matter</a>
      <a href="/update">OTA</a>
      <a href="/json">JSON</a>
      <a href="/settings">Settings</a>
    </div>
    <div class="main">
      <div class="card">)rawliteral";

    if (Matter.isDeviceCommissioned()) {
      html += R"rawliteral(
        <div class="ok">✅ Matter gepaard</div>
        <p>Deze controller is verbonden met Apple Home.</p>)rawliteral";
    } else {
      html += R"rawliteral(
        <h2 style="color:#336699;margin-top:0;">Matter koppelen</h2>
        <p><b>1.</b> Open de <b>Apple Home</b> app</p>
        <p><b>2.</b> Tik op <b>+</b> → <b>Accessoire toevoegen</b> → <b>Meer opties</b></p>
        <p><b>3.</b> Voer de onderstaande code in:</p>
        <div class="code">)rawliteral" + pairingCode + R"rawliteral(</div>
        <p class="hint">Of gebruik de QR-code via de Apple Home app.</p>)rawliteral";
    }

    html += R"rawliteral(
        <div class="transport">Transport: <b>)rawliteral" + transport_str + R"rawliteral(</b>
)rawliteral";

    if (matter_transport == 1) {
      html += R"rawliteral( &nbsp;⚠️ <span style="color:#cc0000;">Border router: Apple TV 4K of HomePod nodig!</span>)rawliteral";
    }

    html += R"rawliteral(</div>
        <br>
        <button class="btn-reset" onclick="if(confirm('Matter pairing wissen?')) location.href='/matter_reset';">
          Matter reset (pairing wissen)
        </button>
        <p class="hint">Matter reset wist alleen de HomeKit koppeling. TESTROOM instellingen blijven intact.</p>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";

    request->send(200, "text/html; charset=utf-8", html);
  });


  // === MATTER RESET HANDLER ===
  server.on("/matter_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
      "<h2 style='text-align:center;padding:40px;color:#cc0000;'>Matter pairing gewist.<br>Rebooting...</h2>");
    preferences.begin("chip-factory",  false); preferences.clear(); preferences.end();
    preferences.begin("chip-config",   false); preferences.clear(); preferences.end();
    preferences.begin("chip-counters", false); preferences.clear(); preferences.end();
    delay(800);
    ESP.restart();
  });


  // === OTA UPDATE ===
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(<!DOCTYPE html><html><head><meta charset="utf-8">
    <title>OTA Update</title>
    <style>body{font-family:Arial;background:#fff;padding:30px;} h2{color:#336699;}</style>
    </head><body>
    <h2>OTA Firmware Update</h2>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update' accept='.bin' required><br><br>
      <input type='submit' value='Upload & Flash' style='background:#336699;color:white;padding:10px 20px;border:none;border-radius:6px;cursor:pointer;font-size:15px;'>
    </form>
    <br><a href="/" style="color:#336699;">← Terug</a>
    </body></html>)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });

  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = !Update.hasError();
      request->send(200, "text/html", success
        ? "<h2 style='color:#0f0'>Update succesvol!</h2><p>Rebooting...</p>"
        : "<h2 style='color:#f00'>Update mislukt!</h2><a href='/update'>Terug</a>");
      if (success) { delay(1000); ESP.restart(); }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) { Update.begin(UPDATE_SIZE_UNKNOWN); }
      Update.write(data, len);
      if (final) { Update.end(true); }
    }
  );


  // === REBOOT ===
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h2>Rebooting...</h2>");
    delay(500); ESP.restart();
  });


  // === TOGGLE BED ===
  server.on("/toggle_bed", HTTP_GET, [](AsyncWebServerRequest *request) {
    bed = !bed;
    preferences.putBool(NVS_BED_STATE, bed);
    if (!ap_mode_active) matter_bed.setOnOff(bed);  // Sync → HomeKit
    request->send(200, "text/plain", "OK");
  });


  // === TOGGLE HOME ===
  server.on("/toggle_home", HTTP_GET, [](AsyncWebServerRequest *request) {
    home_mode = 1 - home_mode;
    preferences.putInt(NVS_HOME_MODE_STATE, home_mode);
    if (!ap_mode_active) matter_thuis.setOnOff(home_mode);  // Sync → HomeKit
    request->send(200, "text/plain", "OK");
  });


  // === PIXEL TOGGLES ===
  for (int i = 0; i < pixels_num; i++) {
    bool is_mode_pixel = (i == 0) || (i == 1 && mov2_enabled);
    String path = is_mode_pixel ? "/toggle_pixel_mode" + String(i) : "/toggle_pixel" + String(i);
    server.on(path.c_str(), HTTP_GET, [i, is_mode_pixel](AsyncWebServerRequest *request) {
      if (is_mode_pixel) {
        pixel_mode[i] = 1 - pixel_mode[i];
        const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
        preferences.putInt(key, pixel_mode[i]);
        if (!ap_mode_active) {
          if (i == 0) matter_pir1_light.setOnOff(pixel_mode[0]);
          if (i == 1) matter_pir2_light.setOnOff(pixel_mode[1]);
        }
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
    html.reserve(3000);
    html = R"rawliteral(
<!DOCTYPE html><html lang="nl"><head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>NeoPixel Kleur</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;background:#fff;margin:0;padding:0;}
    .header{display:flex;background:#ffcc00;color:black;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}
    .header-left{flex:1;} .header-right{flex:1;text-align:right;font-size:15px;}
    .container{display:flex;min-height:calc(100vh - 60px);}
    .sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #cc0000;box-sizing:border-box;flex-shrink:0;}
    .sidebar a{display:block;background:#336699;color:white;padding:8px;margin:8px 0;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;line-height:1.3;width:60px;margin-left:auto;margin-right:auto;}
    .sidebar a:hover{background:#003366;} .sidebar a.active{background:#cc0000;}
    .main{flex:1;padding:30px;text-align:center;}
    input[type=range]{width:80%;height:30px;}
    .btn{background:#336699;color:white;padding:12px 24px;border:none;border-radius:8px;cursor:pointer;font-size:16px;margin-top:20px;}
    .btn:hover{background:#003366;}
  </style>
</head><body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
    <div class="header-right">NeoPixel Kleur</div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status</a>
      <a href="/neopixel" class="active">Pixels</a>
      <a href="/matter">Matter</a>
      <a href="/update">OTA</a>
      <a href="/json">JSON</a>
      <a href="/settings">Settings</a>
    </div>
    <div class="main">
      <h1 style="color:#336699;">NeoPixel Kleur</h1>
      <p>R: <input type="range" id="r" min="0" max="255" value=")rawliteral" + String(neo_r) + R"rawliteral("><span id="rv"> )rawliteral" + String(neo_r) + R"rawliteral(</span></p>
      <p>G: <input type="range" id="g" min="0" max="255" value=")rawliteral" + String(neo_g) + R"rawliteral("><span id="gv"> )rawliteral" + String(neo_g) + R"rawliteral(</span></p>
      <p>B: <input type="range" id="b" min="0" max="255" value=")rawliteral" + String(neo_b) + R"rawliteral("><span id="bv"> )rawliteral" + String(neo_b) + R"rawliteral(</span></p>
      <button class="btn" onclick="applyColor()">Pas toe</button>
      <div id="st" style="margin-top:15px;color:#336699;font-weight:bold;"></div>
    </div>
  </div>
<script>
  ['r','g','b'].forEach(c => {
    document.getElementById(c).oninput = function() { document.getElementById(c+'v').textContent = ' '+this.value; };
  });
  function applyColor() {
    let r=document.getElementById('r').value, g=document.getElementById('g').value, b=document.getElementById('b').value;
    fetch('/setcolor?r='+r+'&g='+g+'&b='+b).then(()=>{
      document.getElementById('st').textContent='Kleur toegepast!';
      setTimeout(()=>{document.getElementById('st').textContent='';},2000);
    });
  }
</script>
</body></html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });


  // === CAPTIVE PORTAL ===
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/settings"); });
  server.on("/generate_204",        HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/settings"); });
  server.on("/ncsi.txt",            HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/settings"); });


  // === SETTINGS PAGE ===
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {

    String pixelNamesHtml = "";
    for (int i = 0; i < pixels_num; i++) {
      pixelNamesHtml += "<label style='display:block;margin:6px 0;'>Pixel " + String(i) + ": ";
      pixelNamesHtml += "<input type='text' name='pixel_nick_" + String(i) + "' value='" + pixel_nicknames[i] + "' style='width:220px;'></label>";
    }

    String html;
    html.reserve(16000);

    html = R"rawliteral(
<!DOCTYPE html><html lang="nl"><head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>)rawliteral" + room_id + R"rawliteral( - Instellingen</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;background:#fff;margin:0;padding:0;}
    .header{display:flex;background:#ffcc00;color:black;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;box-sizing:border-box;}
    .header-left{flex:1;} .header-right{flex:1;text-align:right;font-size:15px;}
    .container{display:flex;flex-direction:row;min-height:calc(100vh - 60px);}
    .sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #cc0000;box-sizing:border-box;flex-shrink:0;}
    .sidebar a{display:block;background:#336699;color:white;padding:8px;margin:8px 0;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;line-height:1.3;width:60px;margin-left:auto;margin-right:auto;}
    .sidebar a:hover{background:#003366;} .sidebar a.active{background:#cc0000;}
    .main{flex:1;padding:20px;overflow-y:auto;box-sizing:border-box;}
    .form-table{width:100%;border-collapse:collapse;margin:20px 0;}
    .form-table td.label{width:35%;padding:12px 8px;vertical-align:middle;font-weight:bold;color:#336699;}
    .form-table td.input{width:40%;padding:12px 8px;vertical-align:middle;}
    .form-table td.hint{width:25%;padding:12px 8px;vertical-align:middle;font-size:12px;color:#666;font-style:italic;}
    .form-table input[type=text],.form-table input[type=password],.form-table input[type=number],.form-table select{width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}
    .form-table tr{border-bottom:1px solid #eee;}
    .submit-btn{background:#336699;color:white;padding:12px 30px;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin:20px 10px;}
    .submit-btn:hover{background:#003366;}
    .reset-btn{background:#cc0000;color:white;padding:12px 30px;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin:20px 10px;}
    .reset-btn:hover{background:#990000;}
    .section-title{color:#336699;font-size:16px;font-weight:bold;margin:24px 0 8px 0;border-bottom:2px solid #336699;padding-bottom:4px;}
    @media(max-width:800px){.container{flex-direction:column;}.sidebar{width:100%;border-right:none;border-bottom:3px solid #cc0000;padding:10px 0;display:flex;justify-content:center;}.sidebar a{width:80px;margin:0 5px;}.form-table td.hint{display:none;}.form-table td.label,.form-table td.input{width:50%;}}
  </style>
</head><body>
  <div class="header">
    <div class="header-left">)rawliteral" + room_id + R"rawliteral(</div>
    <div class="header-right">Instellingen</div>
  </div>
  <div class="container">
    <div class="sidebar">
      <a href="/">Status</a>
      <a href="/neopixel">Pixels</a>
      <a href="/matter">Matter</a>
      <a href="/update">OTA</a>
      <a href="/json">JSON</a>
      <a href="/settings" class="active">Settings</a>
    </div>
    <div class="main">

      <!-- MAC ADRES -->
      <div style="background:#e6f0ff;border:3px solid #336699;padding:20px;margin:0 0 20px 0;border-radius:8px;text-align:center;">
        <h3 style="margin:0 0 8px 0;color:#336699;">📡 Controller MAC Adres</h3>
        <div style="font-size:20px;font-weight:bold;color:#003366;font-family:monospace;background:#fff;padding:10px;border-radius:4px;display:inline-block;border:2px solid #336699;">)rawliteral" + mac_address + R"rawliteral(</div>
        <div style="font-size:13px;color:#666;margin-top:8px;">Voor DHCP-reservering in de router</div>
      </div>

      <div style="background:#fffacd;border:2px solid #cc0000;padding:12px 15px;margin:0 0 20px 0;border-radius:8px;font-size:14px;">
        ⚠️ <b>Wijzigt permanente instellingen.</b>
        Bij WiFi-fout start de controller automatisch AP <code>ROOM-naam</code> → <code>http://192.168.4.1/settings</code>
      </div>

      <form action="/save_settings" method="get" id="settingsForm">

        <div class="section-title">Netwerk & Identiteit</div>
        <table class="form-table">
          <tr>
            <td class="label">Room naam</td>
            <td class="input"><input type="text" name="room_id" value=")rawliteral" + room_id + R"rawliteral(" required></td>
            <td class="hint">Naam van de kamer</td>
          </tr>
          <tr>
            <td class="label">WiFi SSID</td>
            <td class="input"><input type="text" name="wifi_ssid" value=")rawliteral" + wifi_ssid + R"rawliteral(" required></td>
            <td class="hint">Naam van je WiFi netwerk</td>
          </tr>
          <tr>
            <td class="label">WiFi wachtwoord</td>
            <td class="input"><input type="password" name="wifi_pass" value=")rawliteral" + wifi_pass + R"rawliteral("></td>
            <td class="hint">Leeg = huidige behouden</td>
          </tr>
          <tr>
            <td class="label">Static IP</td>
            <td class="input"><input type="text" name="static_ip" value=")rawliteral" + static_ip_str + R"rawliteral(" placeholder="bijv. 192.168.1.50 (leeg=DHCP)"></td>
            <td class="hint">Leeg = DHCP. xxx.xxx.xxx.xxx</td>
          </tr>
        </table>

        <div class="section-title">Klimaat & Ventilatie</div>
        <table class="form-table">
          <tr>
            <td class="label">Heating setpoint (default)</td>
            <td class="input"><input type="number" name="heat_sp" min="10" max="30" value=")rawliteral" + String(heating_setpoint_default) + R"rawliteral("></td>
            <td class="hint">Standaard verwarmingstemperatuur</td>
          </tr>
          <tr>
            <td class="label">Vent request (default %)</td>
            <td class="input"><input type="number" name="vent_req" min="0" max="100" value=")rawliteral" + String(vent_request_default) + R"rawliteral("></td>
            <td class="hint">Standaard ventilatie manueel</td>
          </tr>
          <tr>
            <td class="label">Dew safety margin (°C)</td>
            <td class="input"><input type="number" step="0.1" name="dew_margin" min="0.5" max="5.0" value=")rawliteral" + String(dew_safety_margin, 1) + R"rawliteral("></td>
            <td class="hint">Veiligheidsmarge boven dauwpunt</td>
          </tr>
          <tr>
            <td class="label">Home mode default</td>
            <td class="input">
              <select name="home_mode">
                <option value="0")rawliteral" + String(home_mode_default == 0 ? " selected" : "") + R"rawliteral(>Uit</option>
                <option value="1")rawliteral" + String(home_mode_default == 1 ? " selected" : "") + R"rawliteral(>Thuis</option>
              </select>
            </td>
            <td class="hint">Standaard Thuis/Uit modus</td>
          </tr>
        </table>

        <div class="section-title">Sensoren & Pixels</div>
        <table class="form-table">
          <tr>
            <td class="label">LDR dark threshold</td>
            <td class="input"><input type="number" name="ldr_dark" min="10" max="100" value=")rawliteral" + String(ldr_dark_threshold) + R"rawliteral("></td>
            <td class="hint">Drempel "donker" (0-100)</td>
          </tr>
          <tr>
            <td class="label">Beam alert threshold</td>
            <td class="input"><input type="number" name="beam_thresh" min="0" max="100" value=")rawliteral" + String(beam_alert_threshold) + R"rawliteral("></td>
            <td class="hint">Drempel beam alarm (0-100)</td>
          </tr>
          <tr>
            <td class="label">Aantal NeoPixels</td>
            <td class="input"><input type="number" name="pixels" min="1" max="30" value=")rawliteral" + String(pixels_num) + R"rawliteral("></td>
            <td class="hint">1-30 (reboot na wijzigen)</td>
          </tr>
          <tr>
            <td class="label">Standaard RGB</td>
            <td class="input" colspan="2">
              R: <input type="number" name="neo_r" min="0" max="255" value=")rawliteral" + String(neo_r) + R"rawliteral(" style="width:70px;">
              G: <input type="number" name="neo_g" min="0" max="255" value=")rawliteral" + String(neo_g) + R"rawliteral(" style="width:70px;">
              B: <input type="number" name="neo_b" min="0" max="255" value=")rawliteral" + String(neo_b) + R"rawliteral(" style="width:70px;">
            </td>
          </tr>
          <tr>
            <td class="label">Pixel namen</td>
            <td class="input" colspan="2">)rawliteral" + pixelNamesHtml + R"rawliteral(</td>
          </tr>
          <tr>
            <td class="label">Optionele sensoren</td>
            <td class="input" colspan="2">
              <label><input type="checkbox" name="co2")rawliteral" + String(co2_enabled ? " checked" : "") + R"rawliteral(> CO₂</label><br>
              <label><input type="checkbox" name="dust")rawliteral" + String(dust_enabled ? " checked" : "") + R"rawliteral(> Stof</label><br>
              <label><input type="checkbox" name="sun")rawliteral" + String(sun_light_enabled ? " checked" : "") + R"rawliteral(> Zonlicht</label><br>
              <label><input type="checkbox" name="mov2")rawliteral" + String(mov2_enabled ? " checked" : "") + R"rawliteral(> MOV2 PIR</label><br>
              <label><input type="checkbox" name="tstat")rawliteral" + String(tstat_enabled ? " checked" : "") + R"rawliteral(> Hardware thermostaat</label><br>
              <label><input type="checkbox" name="beam")rawliteral" + String(beam_enabled ? " checked" : "") + R"rawliteral(> Beam sensor</label>
            </td>
          </tr>
        </table>

        <div class="section-title">Matter & Diagnostiek</div>
        <table class="form-table">
          <tr>
            <td class="label">Serial logging</td>
            <td class="input">
              <label><input type="checkbox" name="serial_verbose")rawliteral" + String(serial_verbose ? " checked" : "") + R"rawliteral(>
                Statusrapport aan (elke 15s via serial)
              </label>
            </td>
            <td class="hint">Uitvinken = stille productie</td>
          </tr>
          <tr>
            <td class="label">Matter transport</td>
            <td class="input">
              <select name="matter_transport">
                <option value="0")rawliteral" + String(matter_transport == 0 ? " selected" : "") + R"rawliteral(>WiFi (actief)</option>
                <option value="1")rawliteral" + String(matter_transport == 1 ? " selected" : "") + R"rawliteral(>Thread (placeholder)</option>
              </select>
              <div style="margin-top:8px;background:#f0f4ff;border:1px solid #336699;border-radius:6px;padding:8px 10px;font-size:12px;line-height:1.6;">
                <b style="color:#336699;">WiFi</b> — standaard en volledig werkend. Vereist enkel WiFi-router.<br>
                <b style="color:#336699;">Thread</b> — laag-energie mesh-protocol, ideaal voor batterij-nodes.
                Vereist <b>border router</b> (Apple TV 4K gen3+ of HomePod 2e gen)
                én OpenThread-initialisatie in de sketch.<br>
                <span style="color:#cc0000;">⚠️ Thread is momenteel een <b>placeholder</b> —
                de ESP32-C6 heeft de hardware (802.15.4 radio), maar
                arduino-esp32 3.3.2 Thread-support is nog niet productierijp.
                Deze instelling heeft <b>geen functioneel effect</b>: Matter start altijd via WiFi.
                Toekomstig werk vereist: OpenThread init + border router aanwezig.</span>
              </div>
              <div style="margin-top:4px;font-size:11px;color:#666;">Wijziging van kracht na herstart.</div>
            </td>
            <td class="hint">Altijd WiFi laten staan</td>
          </tr>
        </table>)rawliteral";

    // DS18B20 sectie (dynamisch)
    html += R"rawliteral(
        <div class="section-title">🌡️ DS18B20 Temperatuursensoren</div>
        <div style="background:#e6f0ff;border:2px solid #336699;padding:15px;border-radius:8px;margin-bottom:20px;">
          <p style="margin:0 0 10px 0;font-size:14px;">)rawliteral";
    html += String(ds_count) + " sensor(s) gevonden op de 1-Wire bus.</p>";

    for (int i = 0; i < ds_count; i++) {
      html += "<div style='margin:8px 0;padding:8px;background:#fff;border-radius:6px;border:1px solid #ccc;'>";
      html += "<b>Sensor " + String(i+1) + "</b> <span style='font-family:monospace;font-size:12px;color:#666;'>";
      for (int j = 0; j < 8; j++) { char buf[4]; snprintf(buf,4,"%02X",ds_addrs[i][j]); html += buf; if(j<7) html+=":"; }
      html += "</span>";
      if (temp_ds_arr[i] != 0.0) html += " <b style='color:#336699;'>" + String(temp_ds_arr[i],1) + " °C</b>";
      else html += " <span style='color:#999;'>-- °C</span>";
      html += "<br><label style='font-size:13px;'>Nickname: <input type='text' name='ds_nick_" + String(i) + "' value='" + ds_nicknames[i] + "' style='width:200px;margin-top:4px;'></label></div>";
    }
    if (ds_count == 0) html += "<p style='color:#cc0000;'>Geen sensoren gevonden. Controleer bedrading en gebruik Rescan.</p>";

    html += R"rawliteral(
          <div style="margin-top:12px;">
            <label style="font-weight:bold;color:#336699;">Primaire sensor (room_temp):
              <select name="ds_primary" style="margin-left:8px;padding:4px;">)rawliteral";
    for (int i = 0; i < ds_count; i++) {
      html += "<option value='" + String(i) + "'" + (i == ds_primary ? " selected" : "") + ">";
      html += ds_nicknames[i];
      if (temp_ds_arr[i] != 0.0) html += " (" + String(temp_ds_arr[i],1) + " °C)";
      html += "</option>";
    }
    html += R"rawliteral(</select></label>
          </div>
          <div style="margin-top:12px;">
            <a href="/rescan_ds" onclick="return confirm('Rescan uitvoeren? Duurt ~5 seconden.');"
               style="background:#336699;color:#fff;padding:8px 18px;border-radius:6px;text-decoration:none;font-size:14px;">
              🔍 Rescan 1-Wire bus
            </a>
          </div>
        </div>

        <div style="text-align:center;">
          <button type="submit" class="submit-btn">Opslaan &amp; Reboot</button>
          <button type="button" class="reset-btn" onclick="if(confirm('Weet je zeker? Alle instellingen worden gewist!')) location.href='/factory_reset';">Factory Reset</button>
        </div>
      </form>

      <script>
        document.getElementById('settingsForm').onsubmit = function(e) {
          const ip = this.static_ip.value.trim();
          if (ip && !/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) {
            alert('Ongeldig IP-adres formaat!'); e.preventDefault(); return false;
          }
          if (!this.room_id.value.trim() || !this.wifi_ssid.value.trim()) {
            alert('Room naam en WiFi SSID zijn verplicht!'); e.preventDefault(); return false;
          }
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

    preferences.putString(NVS_ROOM_ID,   arg("room_id", room_id));
    preferences.putString(NVS_WIFI_SSID, arg("wifi_ssid", wifi_ssid));
    preferences.putString(NVS_WIFI_PASS, arg("wifi_pass", wifi_pass));
    preferences.putString(NVS_STATIC_IP, arg("static_ip", ""));

    preferences.putInt(NVS_HEATING_SETPOINT, arg("heat_sp","20").toInt());
    preferences.putInt(NVS_VENT_REQUEST,     arg("vent_req","0").toInt());
    preferences.putFloat(NVS_DEW_MARGIN,     arg("dew_margin","2.0").toFloat());
    preferences.putInt(NVS_HOME_MODE,        arg("home_mode","0").toInt());
    preferences.putInt(NVS_LDR_DARK,         arg("ldr_dark","50").toInt());
    preferences.putInt(NVS_BEAM_THRESHOLD,   arg("beam_thresh","50").toInt());

    preferences.putBool(NVS_CO2_ENABLED,   request->hasArg("co2"));
    preferences.putBool(NVS_DUST_ENABLED,  request->hasArg("dust"));
    preferences.putBool(NVS_SUN_ENABLED,   request->hasArg("sun"));
    preferences.putBool(NVS_MOV2_ENABLED,  request->hasArg("mov2"));
    preferences.putBool(NVS_TSTAT_ENABLED, request->hasArg("tstat"));
    preferences.putBool(NVS_BEAM_ENABLED,  request->hasArg("beam"));

    // v2.0: serial verbose + matter transport
    preferences.putBool(NVS_SERIAL_VERBOSE,  request->hasArg("serial_verbose"));
    preferences.putInt(NVS_MATTER_TRANSPORT, arg("matter_transport","0").toInt());

    int new_pixels = constrain(arg("pixels","8").toInt(), 1, 30);
    int old_pixels = pixels_num;
    preferences.putInt(NVS_PIXELS_NUM, new_pixels);
    if (new_pixels > old_pixels) {
      for (int i = old_pixels; i < new_pixels; i++) {
        String key = String(NVS_PIXEL_ON_BASE) + String(i);
        preferences.putBool(key.c_str(), false);
      }
    }

    preferences.putUChar(NVS_NEO_R, arg("neo_r","255").toInt());
    preferences.putUChar(NVS_NEO_G, arg("neo_g","255").toInt());
    preferences.putUChar(NVS_NEO_B, arg("neo_b","255").toInt());

    for (int i = 0; i < 30; i++) {
      String argName = "pixel_nick_" + String(i);
      if (request->hasArg(argName.c_str())) {
        String nick = request->arg(argName.c_str()); nick.trim();
        if (nick.isEmpty()) nick = room_id + " Pixel " + String(i);
        preferences.putString((String(NVS_PIXEL_NICK_BASE) + String(i)).c_str(), nick);
        if (i < pixels_num) pixel_nicknames[i] = nick;
      }
    }

    for (int i = 0; i < DS_MAX_SENSORS; i++) {
      String argName = "ds_nick_" + String(i);
      if (request->hasArg(argName.c_str())) {
        String nick = request->arg(argName.c_str()); nick.trim();
        if (!nick.isEmpty()) preferences.putString(argName.c_str(), nick);
      }
    }
    if (request->hasArg("ds_primary"))
      preferences.putInt(NVS_DS_PRIMARY, constrain(request->arg("ds_primary").toInt(), 0, DS_MAX_SENSORS-1));

    request->send(200, "text/html",
      "<h2 style='text-align:center;padding:50px;color:#336699;'>Instellingen opgeslagen!<br>Rebooting...</h2>");
    delay(800); ESP.restart();
  });


  // === FACTORY RESET ===
  server.on("/factory_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<h2 style='color:#f00'>Factory reset uitgevoerd!<br>Rebooting...</h2>");
    preferences.clear();
    delay(1000); ESP.restart();
  });


  // === RESCAN DS18B20 ===
  server.on("/rescan_ds", HTTP_GET, [](AsyncWebServerRequest *request) {
    scanDS18B20(); readDS18B20temps();
    String html = "<h2 style='text-align:center;padding:30px;color:#336699;'>🔍 Rescan: ";
    html += String(ds_count) + " sensor(s).<br><br><a href='/settings' style='color:#336699;'>← Settings</a></h2>";
    request->send(200, "text/html; charset=utf-8", html);
  });


  // === SET COLOR ===
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("r")) { neo_r = constrain(request->getParam("r")->value().toInt(),0,255); preferences.putUChar(NVS_NEO_R, neo_r); }
    if (request->hasParam("g")) { neo_g = constrain(request->getParam("g")->value().toInt(),0,255); preferences.putUChar(NVS_NEO_G, neo_g); }
    if (request->hasParam("b")) { neo_b = constrain(request->getParam("b")->value().toInt(),0,255); preferences.putUChar(NVS_NEO_B, neo_b); }
    request->send(200, "text/plain", "OK");
  });


  // === FADE DURATION ===
  server.on("/set_fade_duration", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("duration")) {
      fade_duration = constrain(request->getParam("duration")->value().toInt(), 1, 10);
      preferences.putInt(NVS_FADE_DURATION, fade_duration);
      updateFadeInterval();
    }
    request->send(200, "text/plain", "OK");
  });


  // === SETPOINT SLIDER ===
  server.on("/set_setpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("setpoint")) {
      heating_setpoint = constrain(request->getParam("setpoint")->value().toInt(), 10, 30);
      preferences.putInt(NVS_CURRENT_SETPOINT, heating_setpoint);
      if (!ap_mode_active) matter_thermostat.setHeatingSetpoint((float)heating_setpoint);  // Sync → HomeKit
    }
    request->send(200, "text/plain", "OK");
  });


  // === VENTILATIE SLIDER ===
  server.on("/set_vent", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("vent")) {
      vent_percent = constrain(request->getParam("vent")->value().toInt(), 0, 100);
      vent_mode = 1;
    }
    request->send(200, "text/plain", "OK");
  });


  // === HEATING / VENT MODE TOGGLES ===
  server.on("/toggle_heating_auto", HTTP_GET, [](AsyncWebServerRequest *request) {
    heating_mode = 1 - heating_mode;
    request->send(200, "text/plain", "OK");
  });
  server.on("/toggle_vent_auto", HTTP_GET, [](AsyncWebServerRequest *request) {
    vent_mode = 1 - vent_mode;
    request->send(200, "text/plain", "OK");
  });


  Serial.println(F("Serial commando's: 'reset-all', 'reset-matter', 'status'"));
  server.begin();
  Serial.printf("HTTP server gestart – http://%s\n", WiFi.localIP().toString().c_str());
}


// =============================================================================
// LOOP
// =============================================================================
unsigned long last_slow = 0;

void loop() {

  if (ap_mode_active) dnsServer.processNextRequest();

  handleSerialCommands();
  updateFades();

  // PIR detectie (op 3.3V: beweging = LOW)
  static bool last1 = HIGH, last2 = HIGH;
  bool p1 = digitalRead(PIR_MOV1);
  bool p2 = digitalRead(PIR_MOV2);
  if (!p1 && last1) { mov1_off_time = millis() + LIGHT_ON_DURATION; pushEvent(mov1Times, MOV_BUF_SIZE); }
  if (!p2 && last2) { mov2_off_time = millis() + LIGHT_ON_DURATION; pushEvent(mov2Times, MOV_BUF_SIZE); }
  last1 = p1; last2 = p2;


  // NeoPixel aansturing (ongewijzigd)
  for (int i = 0; i < pixels_num; i++) {

    // PIXEL 0: MOV1
    if (i == 0) {
      if (bed == 1) {
        setTargetColor(0, 0, 0, 0); mov1_light = 0; pixel_on[0] = false;
      } else if (pixel_mode[0] == 1) {
        setTargetColor(0, neo_r, neo_g, neo_b); mov1_light = 1; pixel_on[0] = true;
      } else {
        bool dark = (light_ldr > LDR_DARK_THRESHOLD);
        bool movement = (millis() < mov1_off_time);
        bool on = dark && movement;
        setTargetColor(0, 0, on ? 220 : 0, 0); mov1_light = on; pixel_on[0] = on;
      }
      continue;
    }

    // PIXEL 1: MOV2
    if (i == 1 && mov2_enabled) {
      if (bed == 1) {
        setTargetColor(1, 0, 0, 0); mov2_light = 0; pixel_on[1] = false;
      } else if (pixel_mode[1] == 1) {
        setTargetColor(1, neo_r, neo_g, neo_b); mov2_light = 1; pixel_on[1] = true;
      } else {
        bool dark = (light_ldr > LDR_DARK_THRESHOLD);
        bool movement = (millis() < mov2_off_time);
        bool on = dark && movement;
        setTargetColor(1, 0, on ? 220 : 0, 0); mov2_light = on; pixel_on[1] = on;
      }
      continue;
    }

    // NORMALE PIXELS
    if (pixel_on[i]) setTargetColor(i, neo_r, neo_g, neo_b);
    else             setTargetColor(i, 0, 0, 0);
  }


  // SLOW LOOP (elke 2s)
  if (millis() - last_slow < 2000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;

  // Sensoren lezen
  humi     = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew      = calculateDewPoint(temp_dht, humi);
  readDS18B20temps();
  sensors_event_t e; tsl.getEvent(&e); sun_light = (int)e.light;
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));
  dust  = readDust();
  co2   = readCO2();
  tstat_on = !digitalRead(TSTAT_PIN);

  dew_alert      = (temp_ds < dew) ? 1 : 0;
  night          = (light_ldr > 50) ? 1 : 0;
  beam_value     = map(analogRead(OPTION_LDR), 0, 4095, 0, 100);
  beam_alert_new = (beam_value > beam_alert_threshold) ? 1 : 0;

  // Room temp: primair DS18B20, backup DHT22
  room_temp = temp_ds;
  temp_melding = "";
  if (isnan(temp_ds) || temp_ds < 5.0 || temp_ds > 40.0) {
    room_temp = temp_dht;
    temp_melding = "DS18B20 defect – DHT22 gebruikt";
    if (isnan(temp_dht) || temp_dht < 5.0 || temp_dht > 40.0) {
      room_temp = 0.0;
      temp_melding = "Beide temp sensoren defect!";
    }
  }

  // Verwarmingslogica (ongewijzigd)
  float effective_setpoint = max((float)heating_setpoint, dew + dew_safety_margin);
  if (heating_mode == 1) {
    heating_on = 1;
  } else {
    if (home_mode == 1) heating_on = tstat_on;
    else                heating_on = (room_temp < (effective_setpoint - 0.5f)) ? 1 : 0;
  }

  // Ventilatie logica (ongewijzigd)
  if (vent_mode == 0) vent_percent = map(constrain(co2, 400, 800), 400, 800, 0, 100);

  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
  mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);

  // Matter sensor updates (enkel bij STA verbinding)
  if (!ap_mode_active) {
    update_matter_sensors();
  }

  // Serial statusrapport (elke 15s, alleen als serial_verbose aan)
  if (serial_verbose && !ap_mode_active && millis() - lastSerial > 15000) {
    lastSerial = millis();
    print_status_compact();
  }
}

// ESP32-C6_MATTER_ROOM_15mar_2200.ino = Photon based distributed Home automation system, converted to ESP32C6 controllers.
// Developed by Filip Delannoy in december '25.
// Bereikbaar op (bijvb) http://eetplaats.local of http://192.168.0.80 => Andere controller: Naam (sectie DNS/MDNS) + static IP aanpassen!
//
// PARTITIETABEL: Compileer met "partitions.csv" in de schetsmap (Custom partition table in Arduino IDE board settings):
//   # Name,   Type, SubType, Offset,   Size,    Flags
//   nvs,      data, nvs,     0x9000,   0x5000,
//   otadata,  data, ota,     0xe000,   0x2000,
//   app0,     app,  ota_0,   0x10000,  0x600000,
//   app1,     app,  ota_1,   0x610000, 0x600000,
//   spiffs,   data, spiffs,  0xC10000, 0x3F0000,
//
// 15mar26 v2.10 Matter fixes:
//               MatterEnhancedColorLight → MatterColorLight (kleurpicker only, altijd aan)
//               MatterOnOffPlugin → MatterOnOffLight voor SW1/SW2/SW3 (lamp-type → aparte tegels)
//               HSV callback: espHsvColor_t → HsvColor_t (MatterColorLight API, zoals oude sketch)
//               Pixel 6-8 fix: pixels.updateLength(30)+clear+show bij boot → alle fysieke LEDs uit
//               pixels_num grens strict in alle SW3 callbacks (nooit > geconfigureerd aantal)
//               MatterOnOffLight → MatterOnOffPlugin voor SW1/SW2/SW3 (aparte tegels in Apple Home)
//               Thermostat onChangeMode callback: mode OFF → manueel stop, HEAT → auto hervat
//               onChangeBrightness callback op color light: dim alle actieve pixels via fade engine
//               pixels_num grens strict gerespecteerd in alle callbacks (nooit > geconfigureerd aantal)
//               SW2 fix: werkt ook als mov2 uitgeschakeld (pixel 1 als gewone pixel)
//               8 endpoints: Thermostat, HumiditySensor, 2× OccupancySensor (MOV1 altijd, MOV2 optioneel),
//               EnhancedColorLight (globale RGB powerpixels), 3× OnOffLight (pixel 0 / pixel 1 / pixels 2+)
//               HSV→RGB conversie voor Apple Home kleurkiezer → neo_r/g/b
//               update_matter_sensors() elke 5s, matterNuclearReset() bewaart room-config
//               /matter pagina: "klaar voor integratie + code" of "gepaard" + rode resetknop
//               Sidebar "Matter" op alle pagina's
//               #define Serial Serial0 verwijderd: niet nodig bij CDCOnBoot=default (breekt compile)
// 15mar26 v2.8  Sensor health indicators in UI: sensorWarn() C++ helper + sw() JS helper
//               Abnormale sensorwaarden tonen ⚠ symbool (rood=kritiek, oranje=verdacht)
//               Optionele sensors (co2/dust/sun/mov2/beam/tstat): geen indicator als uitgeschakeld
//               #defines voor drempelwaarden — aanpasbaar per kamer
// 15mar26 v2.7b getJSON(): NaN-guards voor temp_dht(e), temp_ds(f), humi(h), dew(i)
//               zonder guard: NaN float→"nan" in JSON, NaN→int = INT_MAX (2147483647)
// 15mar26 v2.7  Resterende String-allocaties in recurring paden weggewerkt (Focus 1 — licht & stabiel):
//               getJSON(): return type String → const char* (static buf, nul heap-alloc bij elke JS-poll)
//               temp_melding: global String → char[48] (assign in 60s-gate)
//               Serial rapport (15s): String upper_room + String divider + String concatenaties → snprintf/printf
//               Homepage pixel-lus: String label + String action → char[48]/char[32] + snprintf
//               Setup pixel-handler registratie: String path → char[32] + snprintf
// 15mar26 v2.6  JSON schema definitief conform overnamedocument §4.2 + definitieve tabel:
//               ds_primary verwijderd uit JSON (was ah) — niet nuttig voor dashboard
//               DS extra sensoren: ah=Tds2, ai=Tds3 (sensor 0 = primair = zit al in f)
//               json buffer vergroot naar char[680]
// 14mar26 v2.5  TSL2561: tsl_available vlag, I2C-scanner bij boot, getEvent()-check → 65536 lux + I2C-errors opgelost
//               TSL2561: Zonlicht-rij altijd zichtbaar in UI (toont "I2C fout" als sensor niet gevonden)
//               JSON hernummerd naar standaard schema a..ah+ (overnamedocument §4.2), heap ae/af in KB
//               JSON key "t" (pixel_on_str): "P=" prefix toegevoegd → voorkomt getal-conversie in Google Sheets
//               Homepage JS: data.t.replace('P=','') vóór charAt() pixel-lookup
//               getJSON() pure snprintf → char[640], nul heap-alloc
//               AsyncResponseStream voor / en /settings (html.reserve(12000/10000) weg)
//               NVS-keys in loops: String → snprintf char-buf (heap-alloc weg)
//               Crashlog: String reason → snprintf
// 11mar26 v2.4  8 fixes: getJSON reserve(800), kleurkiezer oninput→onchange, first_boot co2/dust default false,
//               num_mov_pixels shadowing fix, rescan_ds async-safe (vlag), DS18B20 CRC-validatie,
//               crash-logging NVS (heap-bewaking + weergave in /settings + wis-knop), WiFi reconnect, printf typo.
// 05mar26 22:00 v. 2.3 btStop() teruggedraaid — veroorzaakte fragmentatie op C6. Heap-rapport behouden aan einde setup().
// 05mar26 21:30 v. 2.2 RGB kleurkiezer inline op statuspagina (vervangt aparte /neopixel pagina). /neopixel redirect naar /.
// 05mar26 20:30 v. 2.1 DS18B20: CONVERT_ALL broadcast (minder interrupt-blocking), leesfrequentie 2s→60s (zelfde als ECO/HVAC). Verwarmingslogica: tstat_enabled gerespecteerd, logica buiten 60s-gate voor snelle respons.
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



// v2.9 FIX: Verplicht voor ESP32-C6 (RISC-V) in Arduino IDE — zonder dit werkt Serial niet correct
#define Serial Serial0

#include <WiFi.h>
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
#include <nvs.h>
#include <nvs_flash.h>
#include <Matter.h>
#include <MatterEndPoints/MatterThermostat.h>
#include <MatterEndPoints/MatterHumiditySensor.h>
#include <MatterEndPoints/MatterOccupancySensor.h>
#include <MatterEndPoints/MatterColorLight.h>     // Zoals oude sketch: kleurpicker only, geen brightness
#include <MatterEndPoints/MatterOnOffLight.h>     // Voor pixel-switches (echte lampen → aparte tegels)
#include <MatterEndPoints/MatterOnOffPlugin.h>    // Behouden voor eventuele logische switches
Preferences preferences;     // Globale Preferences instantie

// ============== MATTER ENDPOINTS (v2.10) ==============
// Patroon uit oude sketch (v2.1 3mar26) — werkt wél als aparte tegels in Apple Home:
// MatterColorLight voor kleurpicker (altijd aan, on/off negeren)
// MatterOnOffLight voor echte pixel-switches (zijn lichten → aparte tegels)
MatterThermostat         matter_thermostat;   // EP1: temp + setpoint + heating
MatterHumiditySensor     matter_humidity;     // EP2: DHT22 vochtigheid
MatterOccupancySensor    matter_mov1;         // EP3: MOV1 PIR aanwezigheid
MatterOccupancySensor    matter_mov2_ep;      // EP4: MOV2 PIR (alleen als mov2_enabled)
MatterColorLight         matter_color;        // EP5: RGB kleurpicker voor alle powerpixels
MatterOnOffLight         matter_sw1;          // EP6: pixel 0 override (MOV1) — lamp → aparte tegel
MatterOnOffLight         matter_sw2;          // EP7: pixel 1 override (MOV2) — lamp → aparte tegel
MatterOnOffLight         matter_sw3;          // EP8: pixels 2+ samen — lamp → aparte tegel

// Matter runtime flags
bool matter_ignore_cb = false;               // Voorkomt callback-loops bij programmatisch updaten
bool matter_nuclear_reset_requested = false; // Vlag: loop() voert reset uit (async-safe)
unsigned long last_matter_update = 0;        // Throttle: Matter update elke 5s

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

// ============== SENSOR HEALTH THRESHOLDS (v2.8) ==============
// Aanpasbaar per kamer — gebruikt in sensorWarn() C++ + sw() JavaScript
#define SENSOR_TEMP_MIN    5.0f  // °C  — onder = sensor defect (rood)
#define SENSOR_TEMP_MAX   40.0f  // °C  — boven = sensor defect (rood)
#define SENSOR_HUMI_MIN   10     // %   — onder = sensor defect (rood)
#define SENSOR_HUMI_MAX   99     // %   — boven = sensor defect (rood)
#define SENSOR_RSSI_WARN  (-75)  // dBm — zwak signaal (oranje)
#define SENSOR_RSSI_CRIT  (-85)  // dBm — kritiek signaal (rood)
#define SENSOR_LUX_MAX    65000  // lux — >= 65535 = I2C garbage (TSL2561)


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
bool tsl_available = false;  // v2.5: vlag — voorkomt herhaalde I2C-errors als sensor niet gevonden bij init
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

wl_status_t last_wifi_status = WL_IDLE_STATUS;
bool rescan_ds_requested = false;  // v2.4 FIX 5: vlag voor async-safe rescan vanuit loop()



// HVAC Variabelen
float room_temp = 0.0;      // Berekende kamertemp: primair temp_ds, backup temp_dht
char temp_melding[48] = ""; // v2.7: char[] i.p.v. String — geen heap-alloc in 60s-gate
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
    char akey[16]; snprintf(akey, sizeof(akey), "ds_addr_%d", i);
    preferences.putBytes(akey, ds_addrs[i], 8);
    char nkey[16]; snprintf(nkey, sizeof(nkey), "ds_nick_%d", i);
    if (preferences.getString(nkey, "").isEmpty()) {
      char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id.c_str(), i+1);
      preferences.putString(nkey, defnick);
      ds_nicknames[i] = defnick;
    } else {
      char fallback[16]; snprintf(fallback, sizeof(fallback), "DS %d", i+1);
      ds_nicknames[i] = preferences.getString(nkey, fallback);
    }
  }
  ds_primary = constrain(preferences.getInt(NVS_DS_PRIMARY, 0), 0, max(ds_count - 1, 0));
  preferences.putInt(NVS_DS_PRIMARY, ds_primary);
}

void loadDS18B20fromNVS() {
  ds_count = preferences.getInt(NVS_DS_COUNT, 0);
  ds_primary = constrain(preferences.getInt(NVS_DS_PRIMARY, 0), 0, max(ds_count - 1, 0));
  for (int i = 0; i < ds_count; i++) {
    char akey[16]; snprintf(akey, sizeof(akey), "ds_addr_%d", i);
    preferences.getBytes(akey, ds_addrs[i], 8);
    char nkey[16]; snprintf(nkey, sizeof(nkey), "ds_nick_%d", i);
    char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id.c_str(), i+1);
    ds_nicknames[i] = preferences.getString(nkey, defnick);
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

    // v2.4 FIX 6: CRC-validatie — byte 8 is CRC over bytes 0–7.
    // Zonder CRC levert een 1-Wire glitch temperaturen van -127°C of 850°C op
    // die downstream (verwarming, dauwpunt, fallback) fout aansturen.
    uint8_t crc = OneWireNg::crc8(data, 8);
    if (crc != data[8]) {
      Serial.printf("[DS18B20] CRC fout sensor %d — gemeten waarde genegeerd\n", i);
      continue;  // Bewaar vorige waarde — beter dan fout getal gebruiken
    }

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



// ============== SENSOR HEALTH INDICATOR (v2.8) ==============
// Returnt pointer naar string-literal in flash — nul heap, nul RAM
// fault=true + critical=true  → rood ⚠  (sensor defect)
// fault=true + critical=false → oranje ⚠ (verdachte waarde / functioneel alarm)
// Optionele sensors: enkel aanroepen binnen bestaande if(sensor_enabled) blokken
const char* sensorWarn(bool fault, bool critical = true) {
  if (!fault) return "";
  return critical
    ? "<span style='color:#c00;font-size:13px;margin-left:4px' title='Abnormale waarde'>&#9888;</span>"
    : "<span style='color:#f80;font-size:13px;margin-left:4px' title='Verdachte waarde'>&#9888;</span>";
}

const char* getJSON() {
  // v2.7: return type const char* (static buf) — geen heap-alloc bij elke JSON-poll
  // v2.6: schema volledig conform overnamedocument §4.2 + definitieve tabel
  // a=uptime .. ag=ds_count, ah=Tds2, ai=Tds3 (sensor 0 = primair = zit al in f)
  // ds_primary index weggelaten uit JSON — niet nuttig voor dashboard/GAS

  char pixel_on_str[32] = {0};
  for (int i = 0; i < pixels_num && i < 30; i++) {
    pixel_on_str[i] = pixel_on[i] ? '1' : '0';
  }
  char pixel_mode_str[4] = {0};
  pixel_mode_str[0] = '0' + pixel_mode[0];
  pixel_mode_str[1] = mov2_enabled ? ('0' + pixel_mode[1]) : '0';

  static char json[680];
  snprintf(json, sizeof(json),
    "{"
    "\"a\":%lu,"      // uptime (s)
    "\"b\":%d,"       // heating_on (0/1)
    "\"c\":%d,"       // heating_setpoint (°C)
    "\"d\":%d,"       // tstat_on (0/1)
    "\"e\":%.1f,"     // temp_dht °C (Temp1 DHT22)  — NaN→0.0 via guard hieronder
    "\"f\":%.1f,"     // temp_ds °C  (Temp2 DS18B20 primair)
    "\"g\":%d,"       // vent_percent (%)
    "\"h\":%d,"       // humi (%)  — NaN→0 via guard hieronder; zonder guard geeft (int)round(NaN) = INT_MAX
    "\"i\":%.1f,"     // dew °C
    "\"j\":%d,"       // dew_alert (0/1)
    "\"k\":%d,"       // co2 (ppm)
    "\"l\":%d,"       // dust (raw)
    "\"m\":%d,"       // light_ldr (0-100)
    "\"n\":%d,"       // sun_light (lux)
    "\"o\":%d,"       // night (0/1)
    "\"p\":%d,"       // bed (0/1)
    "\"q\":%d,"       // neo_r (0-255)
    "\"r\":%d,"       // neo_g (0-255)
    "\"s\":%d,"       // neo_b (0-255)
    "\"t\":\"P=%s\"," // pixel_on_str — P= prefix voorkomt getal-conversie in Sheets
    "\"u\":\"%s\","   // pixel_mode_str
    "\"v\":%d,"       // home_mode (0/1)
    "\"w\":%d,"       // mov1_triggers
    "\"x\":%d,"       // mov2_triggers
    "\"y\":%d,"       // mov1_light (0/1)
    "\"z\":%d,"       // mov2_light (0/1)
    "\"aa\":%d,"      // beam_value (0-100)
    "\"ab\":%d,"      // beam_alert (0/1)
    "\"ac\":%d,"      // wifi_rssi (dBm)
    "\"ad\":%d,"      // free_heap (%)
    "\"ae\":%u,"      // largest_block (KB)
    "\"af\":%u,"      // min_free_heap (KB)
    "\"ag\":%d",      // ds_count — geen trailing comma, DS-temps of } volgt
    (unsigned long)uptime_sec,
    heating_on, heating_setpoint, tstat_on,
    isnan(temp_dht) ? 0.0f : temp_dht,   // v2.7: NaN-guard — anders "nan" in JSON
    isnan(temp_ds)  ? 0.0f : temp_ds,
    vent_percent,
    isnan(humi) ? 0 : (int)round(humi),  // v2.7: NaN-guard — anders INT_MAX (2147483647) in JSON
    isnan(dew)  ? 0.0f : dew,
    dew_alert,
    co2, dust, light_ldr, sun_light, night, bed,
    (int)neo_r, (int)neo_g, (int)neo_b,
    pixel_on_str, pixel_mode_str,
    home_mode, mov1_triggers, mov2_triggers,
    mov1_light, mov2_light, beam_value, beam_alert_new,
    (int)WiFi.RSSI(),
    (int)((ESP.getFreeHeap() * 100) / ESP.getHeapSize()),
    (unsigned)(ESP.getMaxAllocHeap() / 1024),
    (unsigned)(ESP.getMinFreeHeap() / 1024),
    ds_count
  );

  // Extra DS18B20 sensoren vanaf index 1 (index 0 = primair, zit al in "f")
  // ah = Tds2 (sensor 1), ai = Tds3 (sensor 2), aj = Tds4 (sensor 3)
  for (int i = 1; i < ds_count && i < DS_MAX_SENSORS; i++) {
    char ds_entry[24];
    snprintf(ds_entry, sizeof(ds_entry), ",\"a%c\":%.1f", (char)('g' + i), temp_ds_arr[i]);
    strncat(json, ds_entry, sizeof(json) - strlen(json) - 1);
  }
  strncat(json, "}", sizeof(json) - strlen(json) - 1);

  return json;  // v2.7: const char* — geen String-kopie op heap
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





// ============== MATTER HELPER FUNCTIES (v2.9) ==============

// HSV → RGB conversie (Matter stuurt kleur als Hue 0-255, Sat 0-255, Val 0-255)
// Nodig voor MatterEnhancedColorLight → neo_r/g/b
void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (s == 0) { r = g = b = v; return; }
  uint16_t hue = (uint16_t)h * 360 / 255;
  uint8_t  reg = hue / 60;
  uint8_t  rem = (hue - reg * 60) * 255 / 60;
  uint8_t  p   = (uint32_t)v * (255 - s)                            / 255;
  uint8_t  q   = (uint32_t)v * (255 - ((uint32_t)s * rem)     / 255) / 255;
  uint8_t  t   = (uint32_t)v * (255 - ((uint32_t)s * (255-rem))/ 255) / 255;
  switch (reg) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default:r=v; g=p; b=q; break;
  }
}

// Push actuele sensorwaarden + schakelaarstaten naar Matter/HomeKit (elke 5s)
// v2.10: MatterColorLight altijd aan — on/off alleen via SW1/SW2/SW3
void update_matter_sensors() {
  matter_thermostat.setLocalTemperature((float)room_temp);
  matter_ignore_cb = true;
  matter_thermostat.setHeatingSetpoint((double)heating_setpoint);
  matter_ignore_cb = false;

  if (!isnan(humi)) matter_humidity.setHumidity((float)humi);

  matter_mov1.setOccupancy(mov1_light == 1);
  if (mov2_enabled) matter_mov2_ep.setOccupancy(mov2_light == 1);

  // MatterColorLight: altijd aan houden (is kleurpicker, geen aan/uit schakelaar)
  matter_ignore_cb = true;
  matter_color.setOnOff(true);
  matter_ignore_cb = false;

  // SW1/SW2/SW3 sync naar HomeKit — pixel_on[] is de authoritative state
  matter_ignore_cb = true;
  matter_sw1.setOnOff(pixel_on[0]);
  matter_sw2.setOnOff(pixels_num > 1 && pixel_on[1]);
  bool sw3_on = false;
  for (int i = 2; i < pixels_num; i++) if (pixel_on[i]) { sw3_on = true; break; }
  matter_sw3.setOnOff(sw3_on);
  matter_ignore_cb = false;
}

// Nuclear Matter reset — wist Matter NVS (pairing) maar bewaart room-config settings
// Patroon uit HVAC v1.10: backup → nvs_flash_erase() → nvs_flash_init() → restore → reboot
void matterNuclearReset() {
  Serial.println(F("\n=== MATTER NUCLEAR RESET ==="));
  Serial.println(F("Stap 1: Settings laden naar RAM..."));

  // Laad alle room-config sleutels die we willen bewaren
  preferences.begin("room-config", true);
  String bk_room_id      = preferences.getString(NVS_ROOM_ID,          room_id);
  String bk_ssid         = preferences.getString(NVS_WIFI_SSID,        wifi_ssid);
  String bk_pass         = preferences.getString(NVS_WIFI_PASS,        wifi_pass);
  String bk_ip           = preferences.getString(NVS_STATIC_IP,        "");
  int    bk_heat_sp      = preferences.getInt   (NVS_HEATING_SETPOINT, 20);
  float  bk_dew_margin   = preferences.getFloat (NVS_DEW_MARGIN,       2.0);
  int    bk_home_mode    = preferences.getInt   (NVS_HOME_MODE,        0);
  int    bk_home_state   = preferences.getInt   (NVS_HOME_MODE_STATE,  0);
  int    bk_ldr_dark     = preferences.getInt   (NVS_LDR_DARK,        50);
  int    bk_beam_thresh  = preferences.getInt   (NVS_BEAM_THRESHOLD,  50);
  bool   bk_co2          = preferences.getBool  (NVS_CO2_ENABLED,     false);
  bool   bk_dust         = preferences.getBool  (NVS_DUST_ENABLED,    false);
  bool   bk_sun          = preferences.getBool  (NVS_SUN_ENABLED,     true);
  bool   bk_mov2         = preferences.getBool  (NVS_MOV2_ENABLED,    true);
  bool   bk_tstat        = preferences.getBool  (NVS_TSTAT_ENABLED,   true);
  bool   bk_beam         = preferences.getBool  (NVS_BEAM_ENABLED,    true);
  uint8_t bk_r           = preferences.getUChar (NVS_NEO_R,           255);
  uint8_t bk_g           = preferences.getUChar (NVS_NEO_G,           255);
  uint8_t bk_b           = preferences.getUChar (NVS_NEO_B,           255);
  int    bk_pixels_num   = preferences.getInt   (NVS_PIXELS_NUM,      8);
  int    bk_serial_intv  = preferences.getInt   (NVS_SERIAL_INTERVAL, 15);
  bool   bk_serial_verb  = preferences.getBool  (NVS_SERIAL_VERBOSE,  true);
  int    bk_setpoint     = preferences.getInt   (NVS_CURRENT_SETPOINT,20);
  int    bk_fade         = preferences.getInt   (NVS_FADE_DURATION,   2);
  // Pixel nicknames (0..29)
  String bk_nick[30];
  for (int i = 0; i < 30; i++) {
    char k[24]; snprintf(k, sizeof(k), "%s%d", NVS_PIXEL_NICK_BASE, i);
    bk_nick[i] = preferences.getString(k, "");
  }
  // DS18B20 nicknames
  String bk_ds_nick[DS_MAX_SENSORS];
  for (int i = 0; i < DS_MAX_SENSORS; i++) {
    char k[16]; snprintf(k, sizeof(k), "ds_nick_%d", i);
    bk_ds_nick[i] = preferences.getString(k, "");
  }
  preferences.end();
  Serial.println(F("  Settings in RAM."));

  Serial.println(F("Stap 2: nvs_flash_erase()..."));
  esp_err_t err = nvs_flash_erase();
  Serial.printf("  %s\n", esp_err_to_name(err));

  Serial.println(F("Stap 3: nvs_flash_init()..."));
  err = nvs_flash_init();
  Serial.printf("  %s\n", esp_err_to_name(err));

  Serial.println(F("Stap 4: Settings terugschrijven..."));
  preferences.begin("room-config", false);
  preferences.putString(NVS_ROOM_ID,          bk_room_id);
  preferences.putString(NVS_WIFI_SSID,        bk_ssid);
  preferences.putString(NVS_WIFI_PASS,        bk_pass);
  preferences.putString(NVS_STATIC_IP,        bk_ip);
  preferences.putInt   (NVS_HEATING_SETPOINT, bk_heat_sp);
  preferences.putFloat (NVS_DEW_MARGIN,       bk_dew_margin);
  preferences.putInt   (NVS_HOME_MODE,        bk_home_mode);
  preferences.putInt   (NVS_HOME_MODE_STATE,  bk_home_state);
  preferences.putInt   (NVS_LDR_DARK,        bk_ldr_dark);
  preferences.putInt   (NVS_BEAM_THRESHOLD,  bk_beam_thresh);
  preferences.putBool  (NVS_CO2_ENABLED,     bk_co2);
  preferences.putBool  (NVS_DUST_ENABLED,    bk_dust);
  preferences.putBool  (NVS_SUN_ENABLED,     bk_sun);
  preferences.putBool  (NVS_MOV2_ENABLED,    bk_mov2);
  preferences.putBool  (NVS_TSTAT_ENABLED,   bk_tstat);
  preferences.putBool  (NVS_BEAM_ENABLED,    bk_beam);
  preferences.putUChar (NVS_NEO_R,           bk_r);
  preferences.putUChar (NVS_NEO_G,           bk_g);
  preferences.putUChar (NVS_NEO_B,           bk_b);
  preferences.putInt   (NVS_PIXELS_NUM,      bk_pixels_num);
  preferences.putInt   (NVS_SERIAL_INTERVAL, bk_serial_intv);
  preferences.putBool  (NVS_SERIAL_VERBOSE,  bk_serial_verb);
  preferences.putInt   (NVS_CURRENT_SETPOINT,bk_setpoint);
  preferences.putInt   (NVS_FADE_DURATION,   bk_fade);
  for (int i = 0; i < 30; i++) {
    if (!bk_nick[i].isEmpty()) {
      char k[24]; snprintf(k, sizeof(k), "%s%d", NVS_PIXEL_NICK_BASE, i);
      preferences.putString(k, bk_nick[i]);
    }
  }
  for (int i = 0; i < DS_MAX_SENSORS; i++) {
    if (!bk_ds_nick[i].isEmpty()) {
      char k[16]; snprintf(k, sizeof(k), "ds_nick_%d", i);
      preferences.putString(k, bk_ds_nick[i]);
    }
  }
  preferences.end();
  Serial.println(F("  Settings teruggeschreven."));
  Serial.println(F("Stap 5: Reboot — Matter start ongepaard op."));
  delay(500);
  ESP.restart();
}



void setup() {
  Serial.begin(115200);
  delay(1500);
  while (Serial.available()) Serial.read();  // flush
  Serial.println("\n\n=== ROOM Controller — ESP32-C6_MATTER_ROOM_15mar_2200 ===");

  // v2.4 FIX 7: Crash-logging — lees vorige crash uit NVS bij elke boot
  {
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", true);  // read-only
    String lastCrash  = crashPrefs.getString("reason", "geen");
    uint32_t crashCnt = crashPrefs.getUInt("count", 0);
    crashPrefs.end();
    if (crashCnt > 0) {
      Serial.printf("[BOOT] ⚠️  Vorige crash (#%u): %s\n", crashCnt, lastCrash.c_str());
    } else {
      Serial.println("[BOOT] Geen crashes geregistreerd.");
    }
  }
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
      const char* mkey = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
      pixel_mode[i] = preferences.getInt(mkey, 0);   // default = AUTO
    } else {
      // Pixels 2+
      char pkey[24]; snprintf(pkey, sizeof(pkey), "%s%d", NVS_PIXEL_ON_BASE, i);
      pixel_on[i] = preferences.getBool(pkey, false);
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
    // v2.4 FIX 3: co2 en dust default FALSE — veilige default (globale variabelen staan ook op false)
    // Risico: bij eerste boot met niet-aangesloten sensoren veroorzaakten true-defaults WDT crashes
    preferences.putBool(NVS_CO2_ENABLED, false);
    preferences.putBool(NVS_DUST_ENABLED, false);
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
      char pbkey[24]; snprintf(pbkey, sizeof(pbkey), "%s%d", NVS_PIXEL_ON_BASE, i);
      preferences.putBool(pbkey, false);
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
  
  co2_enabled      = preferences.getBool(NVS_CO2_ENABLED, false);   // v2.5 fix: default false — anders WDT crash bij afwezige sensor
  dust_enabled     = preferences.getBool(NVS_DUST_ENABLED, false);  // v2.5 fix: default false
  sun_light_enabled= preferences.getBool(NVS_SUN_ENABLED, true);
  mov2_enabled     = preferences.getBool(NVS_MOV2_ENABLED, true);
  num_mov_pixels = 1 + (mov2_enabled ? 1 : 0);  // v2.4 FIX 4: 'int' verwijderd — was lokale shadowing van globale variabele → globale bleef altijd 2
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
    char nickkey[24]; snprintf(nickkey, sizeof(nickkey), "%s%d", NVS_PIXEL_NICK_BASE, i);
    pixel_nicknames[i] = preferences.getString(nickkey, "");
    
    // Als leeg (eerste boot of na factory reset) → genereer default
    if (pixel_nicknames[i].isEmpty()) {
      pixel_nicknames[i] = room_id + " Pixel " + String(i);
      preferences.putString(nickkey, pixel_nicknames[i].c_str());
    }
  }
  


  // === PIXEL STATES LADEN UIT NVS ===
  for (int i = 0; i < pixels_num; i++) {
    char pukey[24]; snprintf(pukey, sizeof(pukey), "%s%d", NVS_PIXEL_USER_ON_BASE, i);
    pixel_user_on[i] = preferences.getBool(pukey, false);
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
    char pokey[24]; snprintf(pokey, sizeof(pokey), "%s%d", NVS_PIXEL_ON_BASE, i);
    pixel_on[i] = preferences.getBool(pokey, false);
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

  // TSL2561 + I2C debug:
  dht.begin();
  if (sun_light_enabled) {
    Serial.println("\n[TSL2561] I2C init op SDA=IO13, SCL=IO11...");
    Wire.begin(13, 11);
    delay(100);  // Geef I2C bus tijd om te stabiliseren

    // I2C scanner: enkel via Wire — GEEN extra TSL-objecten aanmaken (beschadigt Wire-staat)
    Serial.println("[I2C SCAN] Zoeken naar apparaten op de bus...");
    int i2c_found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      uint8_t err = Wire.endTransmission();
      if (err == 0) {
        Serial.printf("[I2C SCAN] ✓ Apparaat op 0x%02X", addr);
        if (addr == 0x29) Serial.print("  ← TSL2561 ADDR_LOW  (ADDR→GND)");
        if (addr == 0x39) Serial.print("  ← TSL2561 ADDR_FLOAT (ADDR zwevend)");
        if (addr == 0x49) Serial.print("  ← TSL2561 ADDR_HIGH (ADDR→VCC)");
        Serial.println();
        i2c_found++;
      }
    }
    if (i2c_found == 0) {
      Serial.println("[I2C SCAN] ⚠️  Geen enkel I2C-apparaat gevonden!");
      Serial.println("[I2C SCAN]    Controleer: SDA=IO13 SCL=IO11, voeding 3.3V, pull-ups 4k7");
    } else {
      Serial.printf("[I2C SCAN] %d apparaat(en) gevonden\n", i2c_found);
    }

    // Initialiseer TSL2561 op geconfigureerd adres (FLOAT = 0x39)
    tsl_available = tsl.begin();
    if (!tsl_available) {
      Serial.println("[TSL2561] ⚠️  tsl.begin() MISLUKT op 0x39");
      Serial.println("[TSL2561]    Zonlicht uitgeschakeld — waarde blijft 0");
    } else {
      tsl.enableAutoRange(true);
      tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
      delay(20);  // Wacht op eerste conversie (13ms integratie)
      sensors_event_t test_e;
      memset(&test_e, 0, sizeof(test_e));
      bool ok = tsl.getEvent(&test_e);
      if (ok) {
        sun_light = (int)test_e.light;
        Serial.printf("[TSL2561] ✓ OK — eerste meting: %d lux\n", sun_light);
        if (sun_light == 0) Serial.println("[TSL2561]   ⚠️  0 lux — sensor afgedekt of overbelicht?");
      } else {
        Serial.println("[TSL2561] ✓ Init OK, maar getEvent() mislukt bij eerste meting");
      }
    }
    Serial.println();
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
    pixels.updateLength(30);   // Tijdelijk max → stuurt zwart naar ALLE fysieke LEDs
    pixels.clear();
    pixels.show();             // Zet LEDs 6+ ook uit (anders blijven ze hangen)
    pixels.updateLength(pixels_num);  // Terugzetten naar geconfigureerd aantal
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

  // mDNS verwijderd (v2.9) — veroorzaakt conflict met Matter's interne mDNS-stack
  // Gebruik static IP of hostnaam in router-DHCP voor toegang (bijv. http://192.168.0.80)

  // ── Matter initialisatie (alleen als WiFi verbonden — niet in AP mode) ─────
  if (!ap_mode_active) {
    Serial.println(F("\n── Matter initialisatie (v2.10) ─────────────────────────"));

    // EP1: Thermostat — setpoint + temp + mode (HEAT/OFF)
    matter_thermostat.begin(MatterThermostat::THERMOSTAT_SEQ_OP_HEATING);
    matter_thermostat.setLocalTemperature((float)room_temp);
    matter_thermostat.setHeatingSetpoint((double)heating_setpoint);

    matter_thermostat.onChangeHeatingSetpoint([](double sp) -> bool {
      if (matter_ignore_cb) return true;
      heating_setpoint = constrain((int)round(sp), 10, 30);
      preferences.putInt(NVS_CURRENT_SETPOINT, heating_setpoint);
      Serial.printf(F("[HomeKit] Thermostat setpoint → %d °C\n"), heating_setpoint);
      return true;
    });
    // Mode UIT → manueel stop; mode HEAT → auto hervat
    matter_thermostat.onChangeMode([](uint8_t mode) -> bool {
      if (matter_ignore_cb) return true;
      if (mode == MatterThermostat::THERMOSTAT_MODE_OFF) {
        heating_mode = 1; heating_on = 0;
        Serial.println(F("[HomeKit] Thermostat → UIT (manueel stop)"));
      } else {
        heating_mode = 0;
        Serial.println(F("[HomeKit] Thermostat → HEAT (auto hervat)"));
      }
      return true;
    });

    // EP2: Humidity
    matter_humidity.begin();

    // EP3+4: Occupancy
    matter_mov1.begin();
    if (mov2_enabled) matter_mov2_ep.begin();


    // EP5: ColorLight — kleurpicker only (patroon uit oude sketch v2.1)
    // On/off wordt genegeerd en altijd op "aan" gehouden — kleur is het enige doel
    matter_color.begin();
    matter_color.setOnOff(true);  // Altijd aan
    matter_color.onChangeOnOff([](bool on) -> bool {
      // Negeer on/off op kleurpicker — gebruik SW1/SW2/SW3 voor aan/uit controle
      matter_ignore_cb = true;
      matter_color.setOnOff(true);
      matter_ignore_cb = false;
      Serial.println(F("[HomeKit] Color on/off genegeerd (kleurpicker only)"));
      return true;
    });
    // HSV → RGB: gebruik HsvColor_t (MatterColorLight API)
    matter_color.onChangeColorHSV([](HsvColor_t hsv) -> bool {
      // Zelfde conversie als oude sketch
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
      preferences.putUChar(NVS_NEO_R, neo_r);
      preferences.putUChar(NVS_NEO_G, neo_g);
      preferences.putUChar(NVS_NEO_B, neo_b);
      Serial.printf(F("[HomeKit] Kleur → R=%d G=%d B=%d\n"), neo_r, neo_g, neo_b);
      return true;
    });

    // EP6: OnOffLight SW1 → pixel 0 (MOV1 manueel override)
    // OnOffLight = lamp-type → Apple Home toont als aparte licht-tegel
    matter_sw1.begin(pixel_mode[0] == 1);
    matter_sw1.onChangeOnOff([](bool on) -> bool {
      if (matter_ignore_cb) return true;
      pixel_mode[0] = on ? 1 : 0;
      preferences.putInt(NVS_PIXEL_MODE_0, pixel_mode[0]);
      Serial.printf(F("[HomeKit] SW1 pixel 0 (MOV1) → %s\n"), on ? "MANUEEL AAN" : "AUTO");
      return true;
    });

    // EP7: OnOffLight SW2 → pixel 1 (MOV2 of gewone pixel)
    matter_sw2.begin(pixels_num > 1 && pixel_on[1]);
    matter_sw2.onChangeOnOff([](bool on) -> bool {
      if (matter_ignore_cb) return true;
      if (pixels_num < 2) return true;
      if (mov2_enabled) {
        pixel_mode[1] = on ? 1 : 0;
        preferences.putInt(NVS_PIXEL_MODE_1, pixel_mode[1]);
      } else {
        pixel_on[1] = on; pixel_user_on[1] = on;
        char k[24]; snprintf(k, sizeof(k), "%s1", NVS_PIXEL_ON_BASE);
        preferences.putBool(k, on);
      }
      Serial.printf(F("[HomeKit] SW2 pixel 1 → %s\n"), on ? "AAN" : "auto");
      return true;
    });

    // EP8: OnOffLight SW3 → pixels 2..pixels_num-1 (STRICT grens!)
    {
      bool sw3_init = false;
      for (int i = 2; i < pixels_num; i++) if (pixel_on[i]) { sw3_init = true; break; }
      matter_sw3.begin(sw3_init);
      matter_sw3.onChangeOnOff([](bool on) -> bool {
        if (matter_ignore_cb) return true;
        for (int i = 2; i < pixels_num; i++) {  // Nooit voorbij pixels_num!
          pixel_on[i] = on; pixel_user_on[i] = on;
          char k[24]; snprintf(k, sizeof(k), "%s%d", NVS_PIXEL_ON_BASE, i);
          preferences.putBool(k, on);
        }
        Serial.printf(F("[HomeKit] SW3 pixels 2..%d → %s\n"), pixels_num-1, on ? "AAN" : "UIT");
        return true;
      });
    }

    // Heap meten vóór/na Matter.begin()
    uint32_t heap_pre = ESP.getFreeHeap();
    Serial.printf("[HEAP pre-Matter]  free=%uKB  largest=%uKB\n", heap_pre/1024, ESP.getMaxAllocHeap()/1024);
    Matter.begin();
    delay(200);
    Serial.printf("[HEAP post-Matter] free=%uKB  largest=%uKB  kost:-%uKB\n",
      ESP.getFreeHeap()/1024, ESP.getMaxAllocHeap()/1024, (heap_pre-ESP.getFreeHeap())/1024);

    if (!Matter.isDeviceCommissioned() && Matter.getManualPairingCode().length() < 5) {
      Serial.println(F("[MATTER] NVS corrupt → nuclear reset..."));
      matterNuclearReset();
    }

    Serial.println(F("\n══════════════════════════════════════════"));
    if (!Matter.isDeviceCommissioned()) {
      Serial.println(F("MATTER: Nog niet gepaard."));
      Serial.print(F("► Code: ")); Serial.println(Matter.getManualPairingCode());
      Serial.print(F("► http://")); Serial.print(WiFi.localIP().toString()); Serial.println(F("/matter"));
    } else {
      Serial.println(F("MATTER: Al gepaard. Ga naar /matter voor reset."));
    }
    Serial.println(F("══════════════════════════════════════════\n"));
  }
  // ── Einde Matter initialisatie ─────────────────────────────────────────────


  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");



  // === HOME PAGE ===
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // v2.5: AsyncResponseStream — geen html.reserve(12000) meer op de heap
    AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");

    p->print(F("<!DOCTYPE html><html><head>"
      "<meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<title>"));
    p->print(room_id);
    p->print(F(" Status</title>"
      "<style>"
      "body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}"
      ".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}"
      ".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}"
      ".container{display:flex;min-height:calc(100vh - 60px);}"
      ".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}"
      ".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}"
      ".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}"
      ".main{flex:1;padding:15px;overflow-y:auto;}"
      ".group-title{font-size:17px;font-style:italic;font-weight:bold;color:#369;margin:20px 0 8px 0;}"
      "table{width:100%;border-collapse:collapse;margin-bottom:15px;}"
      "td.label{color:#369;font-size:13px;padding:8px 5px;width:30%;border-bottom:1px solid #ddd;vertical-align:middle;}"
      "td.value{background:#e6f0ff;font-size:13px;padding:8px 5px;width:100px;border-bottom:1px solid #ddd;text-align:center;vertical-align:middle;}"
      "td.control{font-size:13px;padding:8px 5px;border-bottom:1px solid #ddd;text-align:right;vertical-align:middle;}"
      ".slider{width:150px;height:28px;}"
      ".switch{position:relative;display:inline-block;width:50px;height:28px;vertical-align:middle;}"
      ".switch input{opacity:0;width:0;height:0;}"
      ".slider-switch{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.4s;border-radius:28px;}"
      ".slider-switch:before{position:absolute;content:\"\";height:20px;width:20px;left:4px;bottom:4px;background:#fff;transition:.4s;border-radius:50%;}"
      "input:checked + .slider-switch{background:#369;}"
      "input:checked + .slider-switch:before{transform:translateX(22px);}"
      "@media(max-width:600px){"
      ".container{flex-direction:column;}"
      ".sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;padding:10px 0;display:flex;justify-content:center;}"
      ".sidebar a{width:80px;margin:0 5px;}"
      ".main{padding:10px;}"
      ".group-title{font-size:16px;margin:15px 0 6px 0;}"
      "td.label{font-size:12px;padding:6px 4px;width:40%;}"
      "td.value{font-size:12px;padding:6px 4px;width:auto;}"
      "td.control{padding:6px 4px;}"
      ".slider{width:100%;max-width:200px;}"
      "}"
      "</style></head><body>"
      "<div class=\"header\">"
      "<div class=\"header-left\">"));
    p->print(room_id);
    p->print(F("</div><div class=\"header-right\">"));
    p->print(uptime_sec);
    p->print(F(" s &nbsp;&nbsp; "));
    p->print(getFormattedDateTime());
    p->print(F("</div></div>"
      "<div class=\"container\">"
      "<div class=\"sidebar\">"
      "<a href=\"/\" class=\"active\">Status</a>"
      "<a href=\"/matter\">Matter</a>"
      "<a href=\"/update\">OTA</a>"
      "<a href=\"/json\">JSON</a>"
      "<a href=\"/settings\">Settings</a>"
      "</div>"
      "<div class=\"main\">"
      "<div class=\"group-title\">HVAC</div>"
      "<table>"
      "<tr><td class=\"label\">Room temp</td><td class=\"value\">"));
    p->printf("%.1f &deg;C<br><small>(%.1f, %.1f)</small>", room_temp, temp_dht, temp_ds);
    // v2.8: warn als DS18B20 (primair) of DHT22 buiten bereik
    p->print(sensorWarn(temp_ds  == 0.0f || temp_ds  < SENSOR_TEMP_MIN || temp_ds  > SENSOR_TEMP_MAX));
    p->print(sensorWarn(temp_dht == 0.0f || temp_dht < SENSOR_TEMP_MIN || temp_dht > SENSOR_TEMP_MAX));
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">Humidity</td><td class=\"value\">"));
    p->printf("%.1f %%", humi);
    // v2.8: warn als DHT22 vocht buiten bereik (ook 0 = was NaN)
    p->print(sensorWarn(humi == 0 || (int)humi < SENSOR_HUMI_MIN || (int)humi > SENSOR_HUMI_MAX));
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">Dauwpunt</td><td class=\"value\">"));
    p->printf("%.1f &deg;C", dew);
    // v2.8: oranje warn als dauwpunt-alarm actief (functioneel alarm, geen sensordefect)
    p->print(sensorWarn(dew_alert == 1, false));
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">DewAlert</td><td class=\"value\">"));
    // v2.8: "JA" in rood voor extra zichtbaarheid
    if (dew_alert) p->print(F("<b style='color:#c00'>JA</b>"));
    else           p->print(F("NEE"));
    p->print(F("</td><td class=\"control\"></td></tr>"));
    if (co2_enabled) {
      p->print(F("<tr><td class=\"label\">CO&#8322;</td><td class=\"value\">"));
      p->printf("%d ppm", co2);
      p->print(sensorWarn(co2 == 0));  // v2.8: 0 ppm = sensor leest niet
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    if (dust_enabled) {
      p->print(F("<tr><td class=\"label\">Stof</td><td class=\"value\">"));
      p->print(dust);
      // v2.8: warn als dust == 0 (sensor leest niet)
      p->print(sensorWarn(dust == 0));
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    p->print(F("<tr><td class=\"label\">Heating setpoint</td><td class=\"value\">"));
    p->printf("%d &deg;C", heating_setpoint);
    p->printf("</td><td class=\"control\"><form action=\"/set_setpoint\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><input type=\"range\" class=\"slider\" name=\"setpoint\" min=\"10\" max=\"30\" value=\"%d\" onchange=\"submitAjax(this.form);\"></form></td></tr>", heating_setpoint);
    p->print(F("<tr><td class=\"label\">Heating Auto</td><td class=\"value\">"));
    p->print(heating_mode == 0 ? F("AUTO") : F("MANUEEL"));
    p->printf("</td><td class=\"control\"><form action=\"/toggle_heating_auto\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      heating_mode == 0 ? "checked" : "");
    p->print(F("<tr><td class=\"label\">Ventilatie snelheid %</td><td class=\"value\">"));
    p->printf("%d %%", vent_percent);
    p->printf("</td><td class=\"control\"><form action=\"/set_vent\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><input type=\"range\" class=\"slider\" name=\"vent\" min=\"0\" max=\"100\" value=\"%d\" onchange=\"submitAjax(this.form);\"></form></td></tr>", vent_percent);
    p->print(F("<tr><td class=\"label\">Vent Auto</td><td class=\"value\">"));
    p->print(vent_mode == 0 ? F("AUTO") : F("MANUEEL"));
    p->printf("</td><td class=\"control\"><form action=\"/toggle_vent_auto\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      vent_mode == 0 ? "checked" : "");
    p->print(F("<tr><td class=\"label\">Thuis/Uit</td><td class=\"value\">"));
    p->print(home_mode ? F("Thuis") : F("Uit"));
    p->printf("</td><td class=\"control\"><form action=\"/toggle_home\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      home_mode ? "checked" : "");
    if (tstat_enabled) {
      p->print(F("<tr><td class=\"label\">Hardware thermostaat</td><td class=\"value\">"));
      p->print(tstat_on ? F("AAN") : F("UIT"));
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    p->print(F("<tr><td class=\"label\">Heating aan</td><td class=\"value\">"));
    p->print(heating_on ? F("JA") : F("NEE"));
    p->print(F("</td><td class=\"control\"></td></tr>"
      "</table>"
      "<div class=\"group-title\">VERLICHTING</div>"
      "<table>"));
    if (sun_light_enabled) {
      p->print(F("<tr><td class=\"label\">Zonlicht</td><td class=\"value\">"));
      if (tsl_available) {
        p->printf("%d lux", sun_light);
        // v2.8: warn als lux >= 65000 (I2C garbage waarde)
        p->print(sensorWarn(sun_light >= SENSOR_LUX_MAX));
      } else {
        p->print(F("<span style='color:#c00;font-size:11px;'>I2C fout</span>"));
      }
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    p->print(F("<tr><td class=\"label\">LDR (donker=100)</td><td class=\"value\">"));
    p->print(light_ldr);
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">MOV1 PIR licht aan</td><td class=\"value\">"));
    p->print(mov1_light ? F("JA") : F("NEE"));
    p->print(F("</td><td class=\"control\"></td></tr>"));
    if (mov2_enabled) {
      p->print(F("<tr><td class=\"label\">MOV2 PIR licht aan</td><td class=\"value\">"));
      p->print(mov2_light ? F("JA") : F("NEE"));
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    // NeoPixel kleurkiezer — hex string via printf
    p->print(F("<tr><td class=\"label\">NeoPixel Kleur</td><td class=\"value\" id=\"rgb_val\">"));
    p->printf("%d, %d, %d", (int)neo_r, (int)neo_g, (int)neo_b);
    p->printf("</td><td class=\"control\"><input type=\"color\" id=\"colorPicker\" value=\"#%02x%02x%02x\""
      " onchange=\"setNeoColor(this.value)\" style=\"width:48px;height:34px;border:none;cursor:pointer;padding:2px;\"></td></tr>",
      neo_r, neo_g, neo_b);
    p->print(F("<tr><td class=\"label\">Bed switch</td><td class=\"value\">"));
    p->print(bed ? F("AAN") : F("UIT"));
    p->printf("</td><td class=\"control\"><form action=\"/toggle_bed\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      bed ? "checked" : "");
    p->print(F("<tr><td class=\"label\">Dim snelheid (s)</td><td class=\"value\">"));
    p->print(fade_duration);
    p->printf("</td><td class=\"control\"><form action=\"/set_fade_duration\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><input type=\"range\" class=\"slider\" name=\"duration\" min=\"1\" max=\"10\" value=\"%d\" onchange=\"submitAjax(this.form);\"></form></td></tr>",
      fade_duration);

    // Dynamische pixels loop — v2.7: char[] i.p.v. String label/action (geen heap-alloc per pixel)
    for (int i = 0; i < pixels_num; i++) {
      char label[48];
      if (pixel_nicknames[i].isEmpty()) {
        if      (i == 0)               snprintf(label, sizeof(label), "Pixel %d (MOV1)", i);
        else if (i == 1 && mov2_enabled) snprintf(label, sizeof(label), "Pixel %d (MOV2)", i);
        else                           snprintf(label, sizeof(label), "Pixel %d", i);
      } else {
        snprintf(label, sizeof(label), "%s", pixel_nicknames[i].c_str());
      }
      const char* val  = pixel_on[i] ? "On" : "Off";
      const char* chkd = pixel_on[i] ? "checked" : "";
      char action[32];
      if (i == 0 || (i == 1 && mov2_enabled))
        snprintf(action, sizeof(action), "/toggle_pixel_mode%d", i);
      else
        snprintf(action, sizeof(action), "/toggle_pixel%d", i);
      p->printf("<tr><td class=\"label\">%s</td><td class=\"value\">%s</td>"
        "<td class=\"control\"><form action=\"%s\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\">"
        "<input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
        label, val, action, chkd);
    }

    p->print(F("</table>"
      "<div class=\"group-title\">BEWEGING</div>"
      "<table>"
      "<tr><td class=\"label\">MOV1 PIR trig/min</td><td class=\"value\">"));
    p->print(mov1_triggers);
    p->print(F("</td><td class=\"control\"></td></tr>"));
    if (mov2_enabled) {
      p->print(F("<tr><td class=\"label\">MOV2 PIR trig/min</td><td class=\"value\">"));
      p->print(mov2_triggers);
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    p->print(F("</table>"));
    if (beam_enabled) {
      p->print(F("<div class=\"group-title\">BEWAKING</div>"
        "<table>"
        "<tr><td class=\"label\">Beam sensor waarde</td><td class=\"value\">"));
      p->print(beam_value);
      p->print(F("</td><td class=\"control\"></td></tr>"
        "<tr><td class=\"label\">Beam sensor alert</td><td class=\"value\">"));
      p->print(beam_alert_new ? F("JA") : F("NEE"));
      p->print(F("</td><td class=\"control\"></td></tr></table>"));
    }
    p->print(F("<div class=\"group-title\">CONTROLLER</div>"
      "<table>"
      "<tr><td class=\"label\">WiFi RSSI</td><td class=\"value\">"));
    {
      int rssi = (int)WiFi.RSSI();
      p->printf("%d dBm", rssi);
      // v2.8: oranje < -75 dBm, rood < -85 dBm
      p->print(sensorWarn(rssi < SENSOR_RSSI_WARN, rssi < SENSOR_RSSI_CRIT));
    }
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">WiFi kwaliteit</td><td class=\"value\">"));
    p->printf("%d %%", constrain(2 * (WiFi.RSSI() + 100), 0, 100));
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">Free heap</td><td class=\"value\" id=\"heap-pct\">"));
    p->printf("%d %%", (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    {
      uint32_t lb = ESP.getMaxAllocHeap();
      const char* col = lb > 35000 ? "#0a0" : lb > 25000 ? "#f80" : "#c00";
      p->printf("</td><td class=\"control\" id=\"heap-lb\" style=\"font-size:12px;\">largest: <b style='color:%s'>%u KB</b></td></tr>",
        col, lb / 1024);
    }
    p->print(F("</table>"
      "<div style=\"text-align:center;margin:10px 0;\">"
      "<button class=\"button\" onclick=\"updateValues()\">Refresh</button>"
      "</div>"
      "<div id=\"status\" style=\"text-align:center;margin:8px 0;font-weight:bold;color:#369;\"></div>"
      "</div></div>"));

    // v2.5: JavaScript met bijgewerkte JSON-keys (schema §4.2)
    // Key-mapping: a=uptime, b=heating_on, c=setpoint, d=tstat, e=temp_dht, f=temp_ds,
    //   g=vent%, h=humi, i=dew, j=dew_alert, k=co2, l=dust, m=ldr, n=sun_light,
    //   o=night, p=bed, q=neo_r, r=neo_g, s=neo_b, t=pixel_on_str, u=pixel_mode_str,
    //   v=home_mode, w=mov1_trig, x=mov2_trig, y=mov1_light, z=mov2_light,
    //   aa=beam_value, ab=beam_alert, ac=rssi, ad=free_heap%, ae=largest_block_KB, af=min_free_KB
    // v2.8: sw() helper voor sensor health indicators (loopt in browser — nul ESP32-impact)
    p->print(F("<script>"
      // v2.8: sw(fault, critical) — geeft ⚠ span terug, '' als geen fout
      // Optionele sensors: sw() enkel aangeroepen als de tabelrij bestaat (= sensor ingeschakeld)
      "function sw(f,c){"
        "return f?\"<span style='color:\"+(c?'#c00':'#f80')+\";font-size:13px;margin-left:4px' "
        "title='\"+(c?'Abnormale':'Verdachte')+\" waarde'>&#9888;</span>\":'';}"
      "function updateValues(){"
      "fetch('/json?'+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(data=>{"
      "document.querySelectorAll('td.value').forEach(td=>{"
      "const l=td.previousElementSibling;if(!l)return;"
      "const lbl=l.textContent.trim();"
      // Room temp: warn als DS18B20 (f) of DHT22 (e) buiten bereik
      "if(lbl.includes('Room temp')) td.innerHTML=data.f.toFixed(1)+' \u00b0C<br><small>('+data.e.toFixed(1)+', '+data.f.toFixed(1)+')</small>'"
        "+sw(data.f==0||data.f<5||data.f>40)"
        "+sw(data.e==0||data.e<5||data.e>40);"
      // Humidity: warn als DHT22 vocht buiten bereik
      "else if(lbl.includes('Humidity')) td.innerHTML=Math.round(data.h)+' %'"
        "+sw(data.h==0||data.h<10||data.h>99);"
      // Dauwpunt: oranje warn als dew_alert actief
      "else if(lbl.includes('Dauwpunt')) td.innerHTML=data.i.toFixed(1)+' \u00b0C'"
        "+sw(data.j==1,false);"
      // DewAlert: rood JA voor extra zichtbaarheid
      "else if(lbl.includes('DewAlert')) td.innerHTML=data.j?\"<b style='color:#c00'>JA</b>\":'NEE';"
      // CO2: enkel zichtbaar als co2_enabled — warn als 0 ppm (sensor leest niet)
      "else if(lbl.includes('CO')) td.innerHTML=data.k+' ppm'+sw(data.k==0);"
      // Stof: enkel zichtbaar als dust_enabled — warn als 0 (sensor leest niet)
      "else if(lbl.includes('Stof')) td.innerHTML=data.l+sw(data.l==0);"
      "else if(lbl.includes('Heating setpoint')) td.textContent=data.c+' \u00b0C';"
      "else if(lbl.includes('Ventilatie snelheid')) td.textContent=data.g+' %';"
      "else if(lbl.includes('Hardware thermostaat')) td.textContent=data.d?'AAN':'UIT';"
      "else if(lbl.includes('Heating aan')) td.textContent=data.b?'JA':'NEE';"
      // Zonlicht: enkel zichtbaar als sun_light_enabled — warn als lux >= 65000 (I2C garbage)
      "else if(lbl.includes('Zonlicht')) td.innerHTML=data.n+' lux'+sw(data.n>=65000);"
      "else if(lbl.includes('LDR')) td.textContent=data.m;"
      "else if(lbl.includes('Night mode')) td.textContent=data.o?'JA':'NEE';"
      "else if(lbl.includes('MOV1 PIR licht')) td.textContent=data.y?'JA':'NEE';"
      "else if(lbl.includes('MOV2 PIR licht')) td.textContent=data.z?'JA':'NEE';"
      "else if(lbl.includes('NeoPixel Kleur')){"
        "td.textContent=data.q+', '+data.r+', '+data.s;"
        "var cp=document.getElementById('colorPicker');"
        "if(cp){var toHex=v=>('0'+Math.round(v).toString(16)).slice(-2);"
        "cp.value='#'+toHex(data.q)+toHex(data.r)+toHex(data.s);}}"
      "else if(lbl.includes('Bed switch')) td.textContent=data.p?'AAN':'UIT';"
      "else if(lbl.includes('MOV1 PIR trig')) td.textContent=data.w;"
      "else if(lbl.includes('MOV2 PIR trig')) td.textContent=data.x;"
      // Beam alert: enkel zichtbaar als beam_enabled — rood JA
      "else if(lbl.includes('Beam waarde')) td.textContent=data.aa;"
      "else if(lbl.includes('Beam alert')) td.innerHTML=data.ab?\"<b style='color:#c00'>JA</b>\":'NEE';"
      // WiFi RSSI: oranje < -75 dBm, rood < -85 dBm
      "else if(lbl.includes('WiFi RSSI')) td.innerHTML=data.ac+' dBm'"
        "+sw(data.ac<-75,data.ac<-85);"
      "else if(lbl.includes('WiFi kwaliteit')) td.innerHTML=Math.min(100,Math.max(0,2*(data.ac+100)))+' %'"
        "+sw(data.ac<-75,data.ac<-85);"
      "else if(lbl.includes('Free heap')){"
        "td.textContent=data.ad+' %';"
        "var lbKB=data.ae||0;"
        "var col=lbKB>50?'#0a0':lbKB>35?'#f80':'#c00';"
        "var detail=document.getElementById('heap-lb');"
        "if(detail) detail.innerHTML='largest: <b style=\"color:'+col+'\">'+lbKB+' KB</b>';}"
      "else if(lbl.includes('Pixel')){"
        "const idx=parseInt(lbl.match(/\\d+/)[0]);"
        "const pstr=(data.t||'').replace('P=','');"
        "if(idx===0) td.textContent=data.y?'On':'Off';"
        "else if(idx===1) td.textContent=data.z?'On':'Off';"
        "else td.textContent=(pstr.charAt(idx)==='1')?'On':'Off';}"
      "});"
      "const homeT=document.querySelector('td.control form[action=\"/toggle_home\"] input');"
      "if(homeT) homeT.checked=(data.v==1);"
      "document.querySelectorAll('td.control form[action^=\"/toggle_pixel_mode\"] input').forEach((cb,i)=>{"
        "cb.checked=(data.u&&data.u.charAt(i)==='1');});"
      "const hdr=document.querySelector('.header-right');"
      "if(hdr){const now=new Date();hdr.innerHTML=data.a+' s &nbsp;&nbsp; '+now.toLocaleDateString('nl-BE')+' '+now.toLocaleTimeString('nl-BE');}"
      "}).catch(e=>console.error(e));}"
      "function submitAjax(form){"
      "const p=new URLSearchParams();let px=null;"
      "for(const el of form.elements){"
        "if(el.name){p.append(el.name,el.value);"
        "if(el.name==='pixel'&&el.type==='hidden') px=parseInt(el.value);"
        "if(el.name==='state') px={idx:px,state:el.value==='1'};}"
      "}"
      "const url=p.toString()?form.action+'?'+p.toString():form.action;"
      "if(px&&px.idx!==null) document.querySelectorAll('td.value').forEach(td=>{const l=td.previousElementSibling;if(l&&l.textContent.includes('Pixel '+px.idx)) td.textContent=px.state?'On':'Off';});"
      "fetch(url).then(r=>{if(r.ok){updateValues();const s=document.getElementById('status');if(s){s.textContent='\u2713';setTimeout(()=>s.textContent='',1500);}}}).catch(e=>console.error(e));}"
      "window.addEventListener('load',()=>{updateValues();setInterval(updateValues,3000);});"
      "function setNeoColor(hex){"
        "var r=parseInt(hex.slice(1,3),16);"
        "var g=parseInt(hex.slice(3,5),16);"
        "var b=parseInt(hex.slice(5,7),16);"
        "document.getElementById('rgb_val').textContent=r+', '+g+', '+b;"
        "fetch('/setcolor?r='+r+'&g='+g+'&b='+b);}"
      "</script></body></html>"));

    request->send(p);
  });



  // === JSON ENDPOINT ===
    server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getJSON());
  });


  // === OTA UPDATE PAGE ===
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    // v2.5: AsyncResponseStream — geen html.reserve(2000) meer
    AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
    p->print(F("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<title>OTA</title>"
      "<style>"
      "body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}"
      ".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;}"
      ".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}"
      ".container{display:flex;min-height:calc(100vh - 60px);}"
      ".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;flex-shrink:0;}"
      ".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}"
      ".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}"
      ".main{flex:1;padding:30px;text-align:center;}"
      ".btn{background:#369;color:#fff;padding:11px 22px;border:none;border-radius:7px;cursor:pointer;font-size:15px;margin:8px;}"
      ".btn:hover{background:#036;}.btn-red{background:#c00;}.btn-red:hover{background:#900;}"
      "</style></head><body>"
      "<div class=\"header\"><div class=\"header-left\">"));
    p->print(room_id);
    p->print(F("</div><div class=\"header-right\">"));
    p->print(uptime_sec);
    p->print(F(" s</div></div>"
      "<div class=\"container\">"
      "<div class=\"sidebar\">"
      "<a href=\"/\">Status</a>"
      "<a href=\"/matter\">Matter</a>"
      "<a href=\"/update\" class=\"active\">OTA</a>"
      "<a href=\"/json\">JSON</a>"
      "<a href=\"/settings\">Settings</a>"
      "</div>"
      "<div class=\"main\">"
      "<h2 style=\"color:#369;\">OTA Firmware Update</h2>"
      "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\">"
      "<input type=\"file\" name=\"update\" accept=\".bin\"><br><br>"
      "<button class=\"btn\" type=\"submit\">Upload</button>"
      "</form><br>"
      "<button class=\"btn btn-red\" onclick=\"location.href='/reboot'\">Reboot</button>"
      "<br><br><a href=\"/\">\u2190 Terug</a>"
      "</div></div></body></html>"));
    request->send(p);
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

  // v2.7: char[] i.p.v. String path — geen heap-alloc bij setup
  char path[32];
  if (is_mode_pixel)
    snprintf(path, sizeof(path), "/toggle_pixel_mode%d", i);
  else
    snprintf(path, sizeof(path), "/toggle_pixel%d", i);

  server.on(path, HTTP_GET, [i, is_mode_pixel](AsyncWebServerRequest *request) {

    if (is_mode_pixel) {
      pixel_mode[i] = 1 - pixel_mode[i];
      const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
      preferences.putInt(key, pixel_mode[i]);
    } else {
      pixel_on[i] = !pixel_on[i];
      char pokey[24]; snprintf(pokey, sizeof(pokey), "%s%d", NVS_PIXEL_ON_BASE, i);
      preferences.putBool(pokey, pixel_on[i]);
    }

    request->send(200, "text/plain", "OK");
  });
}





  // === NEOPIXEL KLEURKIEZER PAGE ===
  server.on("/neopixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");  // Kleurkiezer zit nu inline op statuspagina
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

// === SETTINGS PAGE - v2.5 AsyncResponseStream ===
server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
  // v2.5: AsyncResponseStream — geen html.reserve(10000) meer op de heap
  AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");

  p->print(F("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>"));
  p->print(room_id);
  p->print(F(" - Settings</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}"
    ".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;}"
    ".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}"
    ".container{display:flex;min-height:calc(100vh - 60px);}"
    ".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}"
    ".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}"
    ".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}"
    ".main{flex:1;padding:20px;overflow-y:auto;}"
    "table{width:100%;border-collapse:collapse;margin:10px 0;}"
    "td.lbl{width:38%;padding:9px 6px;font-weight:bold;color:#369;border-bottom:1px solid #eee;vertical-align:middle;}"
    "td.inp{padding:9px 6px;border-bottom:1px solid #eee;vertical-align:middle;}"
    "input[type=text],input[type=password],input[type=number],select{width:100%;padding:7px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}"
    ".btn{background:#369;color:#fff;padding:11px 28px;border:none;border-radius:6px;font-size:15px;cursor:pointer;margin:15px 8px;}"
    ".btn:hover{background:#036;}"
    ".btn-red{background:#c00;}.btn-red:hover{background:#900;}"
    "@media(max-width:800px){.container{flex-direction:column;}.sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;display:flex;justify-content:center;}.sidebar a{margin:0 3px;}}"
    "</style></head><body>"
    "<div class=\"header\">"
    "<div class=\"header-left\">"));
  p->print(room_id);
  p->print(F("</div><div class=\"header-right\">Instellingen</div></div>"
    "<div class=\"container\">"
    "<div class=\"sidebar\">"
    "<a href=\"/\">Status</a>"
    "<a href=\"/matter\">Matter</a>"
    "<a href=\"/update\">OTA</a>"
    "<a href=\"/json\">JSON</a>"
    "<a href=\"/settings\" class=\"active\">Settings</a>"
    "</div>"
    "<div class=\"main\">"
    "<form action=\"/save_settings\" method=\"get\" id=\"sf\">"
    "<table>"
    "<tr><td class=\"lbl\">MAC adres</td><td class=\"inp\"><code>"));
  p->print(mac_address);
  p->print(F("</code></td></tr>"));

  // v2.4 Crash-log weergave
  {
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", true);
    uint32_t crashCnt = crashPrefs.getUInt("count", 0);
    String crashReason = crashPrefs.getString("reason", "geen");
    crashPrefs.end();
    const char* crashColor = crashCnt > 0 ? "#c00" : "#0a0";
    p->printf("<tr><td class=\"lbl\">Crashteller</td><td class=\"inp\"><b style=\"color:%s\">%u</b>", crashColor, crashCnt);
    if (crashCnt > 0) {
      p->print(F(" &nbsp; <a href=\"/clear_crash_log\" style=\"font-size:12px;color:#369;\" onclick=\"return confirm('Crash-log wissen?');\">Wissen</a>"));
    }
    p->print(F("</td></tr><tr><td class=\"lbl\">Laatste crash</td><td class=\"inp\"><code style=\"font-size:12px;\">"));
    p->print(crashReason);
    p->print(F("</code></td></tr>"));
  }

  p->print(F("<tr><td class=\"lbl\">Room naam</td><td class=\"inp\"><input type=\"text\" name=\"room_id\" value=\""));
  p->print(room_id);
  p->print(F("\" required></td></tr>"
    "<tr><td class=\"lbl\">WiFi SSID</td><td class=\"inp\"><input type=\"text\" name=\"wifi_ssid\" value=\""));
  p->print(wifi_ssid);
  p->print(F("\" required></td></tr>"
    "<tr><td class=\"lbl\">WiFi wachtwoord</td><td class=\"inp\"><input type=\"password\" name=\"wifi_pass\" value=\""));
  p->print(wifi_pass);
  p->print(F("\"></td></tr>"
    "<tr><td class=\"lbl\">Static IP</td><td class=\"inp\"><input type=\"text\" name=\"static_ip\" value=\""));
  p->print(static_ip_str);
  p->printf("\" placeholder=\"leeg = DHCP\"></td></tr>"
    "<tr><td class=\"lbl\">Heating setpoint</td><td class=\"inp\"><input type=\"number\" name=\"heat_sp\" min=\"10\" max=\"30\" value=\"%d\"></td></tr>"
    "<tr><td class=\"lbl\">Vent default %%</td><td class=\"inp\"><input type=\"number\" name=\"vent_req\" min=\"0\" max=\"100\" value=\"%d\"></td></tr>"
    "<tr><td class=\"lbl\">Dew margin (&deg;C)</td><td class=\"inp\"><input type=\"number\" step=\"0.1\" name=\"dew_margin\" min=\"0.5\" max=\"5.0\" value=\"%.1f\"></td></tr>",
    heating_setpoint_default, vent_request_default, dew_safety_margin);
  p->printf("<tr><td class=\"lbl\">Home mode default</td><td class=\"inp\">"
    "<select name=\"home_mode\">"
    "<option value=\"0\" %s>Uit</option>"
    "<option value=\"1\" %s>Thuis</option>"
    "</select></td></tr>",
    home_mode_default == 0 ? "selected" : "",
    home_mode_default == 1 ? "selected" : "");
  p->printf("<tr><td class=\"lbl\">LDR dark threshold</td><td class=\"inp\"><input type=\"number\" name=\"ldr_dark\" min=\"10\" max=\"100\" value=\"%d\"></td></tr>"
    "<tr><td class=\"lbl\">Beam threshold</td><td class=\"inp\"><input type=\"number\" name=\"beam_thresh\" min=\"0\" max=\"100\" value=\"%d\"></td></tr>"
    "<tr><td class=\"lbl\">Aantal NeoPixels</td><td class=\"inp\"><input type=\"number\" name=\"pixels\" min=\"1\" max=\"30\" value=\"%d\"></td></tr>",
    ldr_dark_threshold, beam_alert_threshold, pixels_num);
  p->printf("<tr><td class=\"lbl\">Standaard RGB</td><td class=\"inp\">"
    "R:<input type=\"number\" name=\"neo_r\" min=\"0\" max=\"255\" value=\"%d\" style=\"width:70px;\">"
    "G:<input type=\"number\" name=\"neo_g\" min=\"0\" max=\"255\" value=\"%d\" style=\"width:70px;\">"
    "B:<input type=\"number\" name=\"neo_b\" min=\"0\" max=\"255\" value=\"%d\" style=\"width:70px;\"></td></tr>",
    (int)neo_r, (int)neo_g, (int)neo_b);

  // Pixel namen
  p->print(F("<tr><td class=\"lbl\">Pixel namen</td><td class=\"inp\">"));
  for (int i = 0; i < pixels_num; i++) {
    p->printf("<label style=\"display:block;margin:4px 0;\">P%d: <input type=\"text\" name=\"pixel_nick_%d\" value=\"%s\" style=\"width:200px;\"></label>",
      i, i, pixel_nicknames[i].c_str());
  }
  p->print(F("</td></tr>"));

  p->printf("<tr><td class=\"lbl\">Optionele sensoren</td><td class=\"inp\">"
    "<label><input type=\"checkbox\" name=\"co2\"%s> CO&#8322;</label> "
    "<label><input type=\"checkbox\" name=\"dust\"%s> Stof</label> "
    "<label><input type=\"checkbox\" name=\"sun\"%s> Zonlicht</label> "
    "<label><input type=\"checkbox\" name=\"mov2\"%s> MOV2</label> "
    "<label><input type=\"checkbox\" name=\"tstat\"%s> Thermostaat</label> "
    "<label><input type=\"checkbox\" name=\"beam\"%s> Beam</label>"
    "</td></tr>",
    co2_enabled ? " checked" : "",
    dust_enabled ? " checked" : "",
    sun_light_enabled ? " checked" : "",
    mov2_enabled ? " checked" : "",
    tstat_enabled ? " checked" : "",
    beam_enabled ? " checked" : "");
  p->printf("<tr><td class=\"lbl\">Serial logging</td><td class=\"inp\">"
    "<label><input type=\"checkbox\" name=\"serial_verbose\"%s> Aan</label>"
    "&nbsp;&nbsp;interval: <input type=\"number\" name=\"serial_interval\" min=\"5\" max=\"30\" value=\"%d\" style=\"width:50px;\"> s"
    "</td></tr></table>",
    serial_verbose ? " checked" : "",
    serial_interval);

  // DS18B20 sectie
  p->printf("<p style=\"margin:12px 0 4px 0;\"><b>DS18B20</b> &mdash; %d sensor(s) gevonden", ds_count);
  if (ds_count == 0) p->print(F(" <span style=\"color:#c00;\">&mdash; controleer bedrading</span>"));
  p->print(F("</p>"));
  for (int i = 0; i < ds_count; i++) {
    p->print(F("<div style=\"margin:4px 0;padding:6px;background:#f5f5f5;border-radius:4px;\">"
      "<code style=\"font-size:12px;color:#666;\">"));
    for (int j = 0; j < 8; j++) {
      p->printf("%02X%s", ds_addrs[i][j], j < 7 ? ":" : "");
    }
    p->print(F("</code>"));
    if (temp_ds_arr[i] != 0.0) {
      p->printf(" <b style=\"color:#369;\">%.1f &deg;C</b>", temp_ds_arr[i]);
    }
    p->printf("<br><label style=\"font-size:13px;\">Nickname: "
      "<input type=\"text\" name=\"ds_nick_%d\" value=\"%s\" style=\"width:180px;margin-top:3px;\"></label></div>",
      i, ds_nicknames[i].c_str());
  }

  p->print(F("<div style=\"margin:8px 0;\"><label><b>Primaire sensor: </b><select name=\"ds_primary\" style=\"padding:4px;\">"));
  for (int i = 0; i < ds_count; i++) {
    p->printf("<option value='%d'%s>%s", i, i == ds_primary ? " selected" : "", ds_nicknames[i].c_str());
    if (temp_ds_arr[i] != 0.0) p->printf(" (%.1f \xC2\xB0""C)", temp_ds_arr[i]);
    p->print(F("</option>"));
  }
  p->print(F("</select></label>"
    " &nbsp; <a href=\"/rescan_ds\" onclick=\"return confirm('Rescan uitvoeren?');\" "
    "style=\"background:#369;color:#fff;padding:6px 14px;border-radius:5px;text-decoration:none;font-size:13px;\">Rescan 1-Wire</a>"
    "</div>"
    "<div style=\"text-align:center;margin-top:16px;\">"
    "<button type=\"submit\" class=\"btn\">Opslaan &amp; Reboot</button>"
    "<button type=\"button\" class=\"btn btn-red\" onclick=\"if(confirm('Alles wissen?')) location.href='/factory_reset';\">Factory Reset</button>"
    "</div>"
    "</form>"
    "<script>"
    "document.getElementById('sf').onsubmit=function(e){"
    "const ip=this.static_ip.value.trim();"
    "if(ip&&!/^(\\d{1,3}\\.){3}\\d{1,3}$/.test(ip)){alert('Ongeldig IP!');e.preventDefault();return false;}"
    "if(!this.room_id.value.trim()||!this.wifi_ssid.value.trim()){alert('Room naam en SSID verplicht!');e.preventDefault();return false;}"
    "return true;};"
    "</script>"
    "</div></div></body></html>"));

  request->send(p);
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
      char pbkey[24]; snprintf(pbkey, sizeof(pbkey), "%s%d", NVS_PIXEL_ON_BASE, i);
      preferences.putBool(pbkey, false);
    }
  }

  // NeoPixels kleur bewaren!
  preferences.putUChar(NVS_NEO_R, arg("neo_r","255").toInt());
  preferences.putUChar(NVS_NEO_G, arg("neo_g","255").toInt());
  preferences.putUChar(NVS_NEO_B, arg("neo_b","255").toInt());


  // Opslaan pixel nicknames
  for (int i = 0; i < 30; i++) {
    char argName[20]; snprintf(argName, sizeof(argName), "pixel_nick_%d", i);
    if (request->hasArg(argName)) {
      String nick = request->arg(argName);
      nick.trim();
      if (nick.isEmpty()) {
        char defnick[48]; snprintf(defnick, sizeof(defnick), "%s Pixel %d", room_id.c_str(), i);
        nick = defnick;
      }
      char nickkey[24]; snprintf(nickkey, sizeof(nickkey), "%s%d", NVS_PIXEL_NICK_BASE, i);
      preferences.putString(nickkey, nick.c_str());
      if (i < pixels_num) {
        pixel_nicknames[i] = nick;  // Update runtime array
      }
    }
  }

  // DS18B20 nicknames en primaire sensor opslaan (v1.3)
  for (int i = 0; i < DS_MAX_SENSORS; i++) {
    char argName[16]; snprintf(argName, sizeof(argName), "ds_nick_%d", i);
    if (request->hasArg(argName)) {
      String nick = request->arg(argName);
      nick.trim();
      if (!nick.isEmpty()) {
        preferences.putString(argName, nick.c_str());
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

  // v2.4 FIX 7: Crash-log wissen via webUI
  server.on("/clear_crash_log", HTTP_GET, [](AsyncWebServerRequest *request) {
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", false);
    crashPrefs.putUInt("count", 0);
    crashPrefs.putString("reason", "geen");
    crashPrefs.end();
    Serial.println("[CRASH-LOG] Gewist via webUI");
    request->redirect("/settings");
  });

  // === RESCAN DS18B20 BUS (v1.3) ===
  // v2.4 FIX 5: scanDS18B20() + readDS18B20temps() bevatten delay(750) — NOOIT aanroepen
  // vanuit een AsyncWebServer handler (blokkeert AsyncTCP-taak 750ms → crashes bij gelijktijdige requests).
  // Oplossing: vlag zetten, loop() voert de scan uit, response geeft een wacht-pagina die daarna redirect.
  server.on("/rescan_ds", HTTP_GET, [](AsyncWebServerRequest *request) {
    rescan_ds_requested = true;
    request->send(200, "text/html; charset=utf-8",
      "<h2 style='text-align:center;padding:30px;color:#336699;'>"
      "🔍 Rescan gestart...<br><br>"
      "<a href='/settings' style='color:#336699;'>← Terug naar Settings</a>"
      "</h2>"
      "<script>setTimeout(()=>location.href='/settings',2500);</script>");
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





  // === MATTER PAGINA (v2.9) ===
  server.on("/matter", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
    p->print(F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Matter</title><style>"
      "body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}"
      ".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}"
      ".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}"
      ".container{display:flex;min-height:calc(100vh - 60px);}"
      ".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}"
      ".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}"
      ".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}"
      ".main{flex:1;padding:30px;}"
      ".card{background:#e6f0ff;border:2px solid #369;border-radius:10px;padding:25px;max-width:520px;margin:20px 0;}"
      ".code{font-family:monospace;font-size:30px;font-weight:bold;color:#003366;background:#fff;padding:14px 22px;border-radius:6px;border:2px solid #369;display:inline-block;letter-spacing:4px;margin:14px 0;}"
      ".ok{color:#060;font-size:22px;font-weight:bold;margin-bottom:10px;}"
      ".btn-reset{background:#c00;color:#fff;padding:11px 26px;border:none;border-radius:6px;font-size:15px;cursor:pointer;margin-top:18px;}"
      ".btn-reset:hover{background:#900;}"
      ".hint{font-size:13px;color:#666;margin-top:8px;}"
      "@media(max-width:600px){.container{flex-direction:column;}"
      ".sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;display:flex;justify-content:center;}"
      ".sidebar a{margin:0 3px;}}"
      "</style></head><body>"
      "<div class='header'><div class='header-left'>"));
    p->print(room_id);
    p->print(F("</div><div class='header-right'>Matter / HomeKit</div></div>"
      "<div class='container'><div class='sidebar'>"
      "<a href='/'>Status</a>"
      "<a href='/matter' class='active'>Matter</a>"
      "<a href='/update'>OTA</a>"
      "<a href='/json'>JSON</a>"
      "<a href='/settings'>Settings</a>"
      "</div><div class='main'><div class='card'>"));
    if (Matter.isDeviceCommissioned()) {
      p->print(F("<div class='ok'>&#x2705; Matter gepaard</div>"
        "<p>Deze controller is verbonden met Apple Home (of ander Matter-platform).</p>"));
    } else {
      p->print(F("<h2 style='color:#369;margin-top:0;'>Matter klaar voor integratie</h2>"
        "<p><b>1.</b> Open de <b>Apple Home</b> app</p>"
        "<p><b>2.</b> Tik op <b>+</b> &rarr; <b>Accessoire toevoegen</b> &rarr; <b>Meer opties</b></p>"
        "<p><b>3.</b> Voer de onderstaande code in:</p>"
        "<div class='code'>"));
      p->print(Matter.getManualPairingCode());
      p->print(F("</div>"
        "<p class='hint'>Of scan de QR-code via Apple Home &rarr; <b>Voeg toe via code</b>.</p>"));
    }
    p->print(F("<br><button class='btn-reset' "
      "onclick=\"if(confirm('Matter pairing wissen? ROOM-instellingen blijven intact.')) location.href='/matter_reset';\">"
      "Matter reset (pairing wissen)</button>"
      "<p class='hint'>Reset wist enkel de HomeKit/Matter koppeling. Alle ROOM-instellingen, pixels en sensor-config blijven bewaard.</p>"
      "</div></div></div></body></html>"));
    request->send(p);
  });

  // Nuclear reset via vlag — handler zet vlag, main loop() voert uit (async-safe: geen NVS-race)
  server.on("/matter_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    matter_nuclear_reset_requested = true;
    request->send(200, "text/html",
      "<h2 style='text-align:center;padding:40px;color:#c00;'>"
      "Matter nuclear reset gestart...<br>"
      "<small style='font-size:16px;color:#666;'>ROOM-instellingen worden bewaard. Rebooting...</small>"
      "</h2>");
    Serial.println(F("\n[WEB] Matter nuclear reset aangevraagd via /matter_reset"));
  });

  server.begin();
  Serial.printf("HTTP server gestart op http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("\n=== Setup klaar ===\n");
  Serial.printf("Free heap     : %d%% (%d bytes)\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("Largest block : %d KB\n", ESP.getMaxAllocHeap() / 1024);
}

unsigned long lastSerial = 0;
unsigned long last_slow = 0;








void loop() {

  // WiFi status bewaken — herverbinden bij verlies (v2.9: mDNS verwijderd, conflict met Matter)
  wl_status_t current_status = WiFi.status();
  if (current_status != last_wifi_status) {
    if (current_status != WL_CONNECTED && !ap_mode_active) {
      Serial.println("[WiFi] Verbinding verloren — herverbinden...");
      WiFi.reconnect();
    }
    last_wifi_status = current_status;
  }

  // v2.9: Matter nuclear reset — vlag gezet door /matter_reset handler, uitvoering hier (async-safe)
  if (matter_nuclear_reset_requested) {
    matter_nuclear_reset_requested = false;
    delay(200);  // Geef async response tijd om te verzenden
    matterNuclearReset();
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

  // v2.4 FIX 5: Rescan DS18B20 vanuit loop() (async-safe — delay(750) is hier WDT-safe)
  if (rescan_ds_requested) {
    rescan_ds_requested = false;
    scanDS18B20();
    readDS18B20temps();
    Serial.printf("[RESCAN] DS18B20: %d sensor(s) gevonden\n", ds_count);
  }

  if (millis() - last_slow < 60000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;

  // v2.4 FIX 7: Heap-bewaking — schrijf naar NVS als largest block < 25 KB
  // Zo is er na een crash bewijs van wat er voorafging
  {
    uint32_t lb = ESP.getMaxAllocHeap();
    if (lb < 25000) {
      Preferences crashPrefs;
      crashPrefs.begin("crash-log", false);
      uint32_t cnt = crashPrefs.getUInt("count", 0) + 1;
      crashPrefs.putUInt("count", cnt);
      char reason[48];
      snprintf(reason, sizeof(reason), "heap %uKB @ %lus", lb/1024, (unsigned long)uptime_sec);
      crashPrefs.putString("reason", reason);
      crashPrefs.end();
      Serial.printf("[HEAP] ⚠️  Largest block %u KB — crash-log geschreven (#%u)\n", lb / 1024, cnt);
    }
  }



  // Sensoren
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  readDS18B20temps();  // Lees alle DS18B20 sensoren → temp_ds = primaire sensor
  
  if (sun_light_enabled && tsl_available) {
    sensors_event_t e;
    memset(&e, 0, sizeof(e));
    bool ok = tsl.getEvent(&e);
    if (ok) {
      sun_light = (int)e.light;
      // Debug elke 5 minuten (300s) of eerste keer
      static uint32_t last_tsl_debug = 0;
      if (uptime_sec - last_tsl_debug >= 300 || last_tsl_debug == 0) {
        last_tsl_debug = uptime_sec;
        Serial.printf("[TSL2561] %.2f lux  (int=%d)\n", e.light, sun_light);
      }
    } else {
      Serial.printf("[TSL2561] getEvent() MISLUKT @ %lus — vorige waarde %d lux bewaard\n",
        (unsigned long)uptime_sec, sun_light);
    }
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
  temp_melding[0] = '\0';  // v2.7: reset char[] zonder heap-alloc
  if (isnan(temp_ds) || temp_ds < 5.0 || temp_ds > 40.0) {  // Falen detectie
    room_temp = temp_dht;
    strncpy(temp_melding, "DS18B20 defect - DHT22 gebruikt", sizeof(temp_melding) - 1);
    if (isnan(temp_dht) || temp_dht < 5.0 || temp_dht > 40.0) {
      room_temp = 0.0;
      strncpy(temp_melding, "Beide temp sensoren defect!", sizeof(temp_melding) - 1);
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

    // v2.7: Serial rapport zonder String-allocaties (upper_room, divider, concatenaties → char[]/printf)
    char upper_room[32];
    snprintf(upper_room, sizeof(upper_room), "%s", room_id.c_str());
    for (int k = 0; upper_room[k]; k++) upper_room[k] = toupper((unsigned char)upper_room[k]);
    char uptime_buf[12];
    snprintf(uptime_buf, sizeof(uptime_buf), "%lu", (unsigned long)uptime_sec);
    Serial.printf("\n%s \xe2\x80\x93 %s s\n", upper_room, uptime_buf);  // "–" UTF-8
    int div_len = strlen(upper_room) + strlen(uptime_buf) + 5;
    for (int k = 0; k < div_len; k++) Serial.print("\xe2\x94\x80");  // "─" UTF-8
    Serial.println();
    Serial.printf("DHT22 Temp2          : %.2f °C\n", temp_dht);
    Serial.printf("DHT22 Humidity       : %.1f %%\n", humi);
    Serial.printf("Dauwpunt             : %.1f °C\n", dew);
    Serial.printf("DewAlert (Temp2<Dew) : %s\n", dew_alert ? "JA" : "NEE");
    Serial.printf("DS18B20 Temp1        : %.2f °C\n", temp_ds);
    Serial.printf("Room temp            : %.1f °C %s\n", room_temp, temp_melding);
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
    Serial.printf("Ventilatie snelheid %%         : %d %%\n", vent_percent);
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


    Serial.printf("Pixels on (0-%d)      : ", pixels_num - 1);
      for (int i = 0; i < pixels_num; i++) {
        Serial.print(pixel_on[i] ? "1" : "0");
      }
    Serial.println();
    Serial.printf("WiFi RSSI            : %d dBm\n", WiFi.RSSI());
    Serial.printf("WiFi kwaliteit       : %d %%\n", constrain(2 * (WiFi.RSSI() + 100), 0, 100));
    Serial.printf("Free heap            : %d %%\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
    Serial.printf("Matter gepaard       : %s\n", Matter.isDeviceCommissioned() ? "JA" : "NEE");
    Serial.println("─────────────────────────────────────\n");
  }

  // v2.9: Matter sensor-update elke 5s (buiten AP-mode)
  if (!ap_mode_active && millis() - last_matter_update > 5000) {
    last_matter_update = millis();
    update_matter_sensors();
  }
}

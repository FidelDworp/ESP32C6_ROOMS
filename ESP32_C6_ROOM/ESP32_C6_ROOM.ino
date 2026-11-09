// ESP32-C6 ROOM v3.4 — 9 nov 2026 — zonder Matter
// Zarlar thuisautomatisering — Room controller (Apple Home via de RPi-bridge, niet meer via Matter op de ESP32)
// Filip Delannoy — Zarlardinge (BE)
//
// PARTITIETABEL: Compileer met "partitions.csv" in de schetsmap (Custom partition scheme):
//   nvs (20 KB), otadata (8 KB), app0 (6 MB), app1 (6 MB), spiffs (~4 MB)
//
// v3.4 (9nov26): SCD41 (CO2 + temperatuur + vocht via I2C), naar voren gehaald in het verbeterplan.
//                 (1) Bibliotheek "Sensirion I2C SCD4x" (v1.1.0, + "Sensirion Core"). I2C op IO13 (SDA) /
//                 IO11 (SCL), 100 kHz, adres 0x62 — alleen gestart als de SCD41 aan staat in /settings.
//                 (2) Niet-blokkerend: de sensor meet zelf elke 5 s (periodieke meting); de loop kijkt om de
//                 5 s of er een nieuwe waarde klaarstaat en leest die in enkele ms. 3 fouten op rij of 60 s
//                 zonder geldige meting → "ontbreekt" (waarden 0); elke 60 s een nieuwe poging.
//                 (3) CO2 via PWM weg: pulseIn(), readCO2() en CO2_PWM (IO18) verdwenen; IO18 is vrij (GP4, v3.9).
//                 (4) /json: k = CO2 (ppm, stuurt de ventilatie zoals voorheen, 400–800 ppm → 0–100 %).
//                 NIEUW achteraan: al = SCD41-temperatuur (°C, 1 decimaal), am = SCD41-vocht (%). 0 = geen meting.
//                 Alle bestaande keys ongewijzigd. De DHT22 blijft de bron voor vocht en dauwpunt.
//                 (5) /advanced: extra rijen "SCD41 temp" en "SCD41 vocht". /settings: "SCD41 (CO2)" vervangt
//                 "CO2" (zelfde NVS-sleutel co2_en), ASC aan/uit (scd_asc, standaard aan), temperatuuroffset
//                 (scd_toff, standaard 4,0 °C), knop "Kalibreren in buitenlucht" (FRC 425 ppm, pas na 3 min
//                 meten; uitgevoerd in de loop, niet in de webtaak). Diagnose: SCD41-toestand, serienummer,
//                 leesfouten, laatste kalibratie. (6) Geen sensormetingen in AP-mode (en dan ook geen verwarmingsvraag).
// v3.3 (7nov26): STABILITEIT. (1) TSL2561 volledig weg: bibliotheek, I2C-start (Wire), instelling
//                 "Zonlicht" (NVS sun_en), UI-rij en loop-uitlezing. De sensor hoort niet meer op een room
//                 (ROOM-standaard v1.1); /json-key n blijft bestaan met waarde 0. Aanleiding: 4 herstarts
//                 door de interrupt-watchdog in de 48 u-test van v3.2, met een defecte TSL2561 op het testbord.
//                 (2) "LAATSTE STAP": de sketch noteert wat hij aan het doen is (loop: WiFi, pixels, DHT22,
//                 DS18B20, NVS…; web: laatste pagina/opdracht) in RTC-geheugen dat een herstart overleeft
//                 (niet een stroomonderbreking). Bij de volgende opstart wordt dat met de reden en de uptime
//                 vóór de herstart bewaard in NVS ("crash-log": h1–h3) en getoond in /settings → Diagnose
//                 ("Vorige herstarts"). Zo is de oorzaak van een watchdog of crash via de browser te zien,
//                 zonder seriële kabel. "Wissen" wist nu ook die lijst. /json en bediening ongewijzigd.
// v3.2 (5nov26): VERSIE EN DIAGNOSE ZICHTBAAR. Eén #define ROOM_VERSION, getoond onderaan de
//                 landingspagina, in de kop van /advanced, /update en /settings en bij het opstarten.
//                 /settings: blok "Diagnose" met versie, uptime, reden laatste herstart
//                 (esp_reset_reason: stroom, software/OTA, crash, watchdog, spanningsdip),
//                 aantal opstarts (NVS "crash-log"/"boots"), geheugen (vrij %, grootste blok,
//                 laagste ooit) en WiFi-signaal. De vroegere "Crashteller" heet nu "Lage-heap meldingen"
//                 (dat is wat hij telt: grootste blok < 25 KB). Geen nieuwe /json-keys, geen andere bediening.
// v3.1 (3nov26): LANDINGSPAGINA (zoals de Sjalay). "/" = eenvoudige pagina voor iedereen
//                 (landing.h): temperatuur + setpoint-schuifbalk, een tegel per lamp (pixelnamen;
//                 MOV-pixels wisselen tussen automatisch en altijd aan), kleur, bed-modus,
//                 thuis/weg, ⚙ naar de technische pagina. De vroegere statuspagina staat nu op
//                 /advanced. Menu op elke pagina: 🏠 (landingspagina) + Status (/advanced).
//                 BESTANDEN: ESP32_C6_ROOM.ino + landing.h, samen in de map ESP32_C6_ROOM.
//                 /json, endpoints en NVS ongewijzigd.
// v3.0 (3nov26): ZONDER MATTER (Master §7.9 / §16, Portal Plan F6). Weg: Matter-includes, de 8
//                 endpoints (thermostaat, vocht, MOV1, MOV2, kleurlicht, SW1-SW3) + callbacks,
//                 update_matter_sensors() + 30 s-update in loop(), matterNuclearReset() + vlag,
//                 /matter en /matter_reset + menulinks, pairing-meldingen in setup(), regel
//                 "Matter gepaard" in serial-status. Apple Home: toestel "Eetplaats" via de
//                 RPi-bridge (server.js v2.9 + matterbridge-zarlar). Verder niets gewijzigd
//                 (JSON-keys, endpoints, NVS identiek). Header: partitions.csv.
// v2.24 (8mei26): Heap-optimalisatie alternatief — shared CSS helper functie (~3-5 KB winst)
//                 Serial version string verwijderd (verwarrend bij troubleshoot)
// v2.23 (8mei26): PROGMEM experiment — werkte niet (FPSTR kopieert alsnog naar heap)
// v2.22 (1mei26): /capabilities endpoint voor Portal verlichting UI (pixel nicknames JSON)
// v2.21 (13apr26): KISS — heating_mode/vent_mode verwijderd, Matter thermostat mode=home_mode
// v2.20 (13apr26): Crash-stabiliteit — NVS crashlog fix, Matter interval 30s, CO2 timeout 50ms
//
// Hardware: ESP32-C6 32-pin clone — 192.168.0.80 (Eetplaats referentie)
// Arduino IDE: Board=ESP32C6 Dev Module, Flash=16MB, USB CDC On Boot=Enabled


// v2.9 FIX: Verplicht voor ESP32-C6 (RISC-V) in Arduino IDE — zonder dit werkt Serial niet correct
#define Serial Serial0

#include <WiFi.h>
#include <DNSServer.h>        // Toegevoegd om captive portal toe te voegen
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>           // Voor OTA update
#include <DHT.h>
#include <OneWireNg_CurrentPlatform.h>  // C6 compatibel (vervangt OneWire + DallasTemperature)

#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>            // Voor sin() in dimmer engine
#include <Preferences.h>     // NVS library voor Preferences
#include <string.h>          // Voor memset()
#include <nvs.h>
#include <nvs_flash.h>
#include "landing.h"     // v3.1: landingspagina "/"
#include <esp_system.h>      // v3.2: esp_reset_reason()
#include <esp_attr.h>        // v3.3: RTC_NOINIT_ATTR voor "laatste stap"
#include <Wire.h>             // v3.4: I2C voor de SCD41
#include <SensirionI2cScd4x.h> // v3.4: bibliotheek "Sensirion I2C SCD4x" (+ "Sensirion Core")

#define ROOM_VERSION "v3.4"  // v3.2: enige plaats met het versienummer

// v3.2: reden van de laatste herstart, leesbaar
esp_reset_reason_t boot_reason = ESP_RST_UNKNOWN;
uint32_t boot_count = 0;
const char* resetReasonText(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "Stroom aangezet";
    case ESP_RST_EXT:       return "Reset-knop / extern";
    case ESP_RST_SW:        return "Software (OTA, instellingen of serieel R)";
    case ESP_RST_PANIC:     return "CRASH (fout in de software)";
    case ESP_RST_INT_WDT:   return "Watchdog (interrupt)";
    case ESP_RST_TASK_WDT:  return "Watchdog (taak hing vast)";
    case ESP_RST_WDT:       return "Watchdog (andere)";
    case ESP_RST_DEEPSLEEP: return "Uit diepe slaap";
    case ESP_RST_BROWNOUT:  return "Spanningsdip (brown-out)";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "Onbekend";
  }
}

// v3.3: "LAATSTE STAP" — overleeft watchdog, crash en software-herstart (niet stroom uit).
// Twee sporen: de loop-taak en de webserver-taak (AsyncTCP) lopen apart.
enum CrumbLoop : uint8_t {
  CL_NONE = 0, CL_SETUP, CL_WIFI, CL_SERIAL, CL_FADE, CL_PIR, CL_PIXLOGIC, CL_HEAT,
  CL_DSSCAN, CL_IDLE, CL_NVS, CL_DHT, CL_DS, CL_LDR, CL_DUST, CL_CO2, CL_REST, CL_COUNT
};
const char* const CRUMB_LOOP_TXT[CL_COUNT] = {
  "onbekend", "opstarten (setup)", "WiFi-controle", "seriële commando's", "pixels dimmen/tonen",
  "PIR lezen", "pixel-logica", "verwarmingslogica", "DS18B20 opnieuw zoeken", "wachten (rust)",
  "NVS schrijven (heap-log)", "DHT22 lezen", "DS18B20 lezen", "LDR lezen", "stofsensor lezen",
  "SCD41 lezen (I2C)", "berekeningen"
};
enum CrumbWeb : uint8_t {
  CW_NONE = 0, CW_LANDING, CW_ADVANCED, CW_JSON, CW_CAPS, CW_SETTINGS, CW_SAVE, CW_OTA,
  CW_PIXEL, CW_COLOR, CW_HOME, CW_SETPOINT, CW_OTHER, CW_COUNT
};
const char* const CRUMB_WEB_TXT[CW_COUNT] = {
  "geen", "/ (landingspagina)", "/advanced", "/json", "/capabilities", "/settings", "/save_settings",
  "/update (OTA)", "pixel aan/uit", "kleur", "thuis/weg", "setpoint", "andere opdracht"
};
#define CRUMB_MAGIC 0x5A524D33UL   // "ZRM3"
RTC_NOINIT_ATTR uint32_t crumb_magic;
RTC_NOINIT_ATTR uint8_t  crumb_loop;     // huidige stap in loop()
RTC_NOINIT_ATTR uint8_t  crumb_web;      // laatste webopdracht
RTC_NOINIT_ATTR uint32_t crumb_web_s;    // uptime (s) van die webopdracht
RTC_NOINIT_ATTR uint32_t crumb_up_s;     // laatst gekende uptime (s)
static inline void crumbL(uint8_t c) { crumb_loop = c; }
static inline void crumbW(uint8_t c) { crumb_web = c; crumb_web_s = millis() / 1000; }
char prev_step[96] = "";   // v3.3: laatste stap vóór deze herstart (voor /settings)
Preferences preferences;     // Globale Preferences instantie


// ============== PIN DEFINITIONS (ESP32-C6 via Photon Shield) ==============
#define DHT_PIN        6   // IO6  - DHT22 data         (was GPIO18)
#define ONE_WIRE_PIN   3   // IO3  - DS18B20 OneWire     (was GPIO4)
#define PIR_MOV1       5   // IO5  - MOV1 PIR            (was GPIO17)
#define PIR_MOV2      19   // IO19 - MOV2 PIR            (was GPIO26)
#define SHARP_LED     12   // IO12 - Sharp dust LED out  (was GPIO19)
#define SHARP_ANALOG   7   // IO7  - Sharp dust analog   (was GPIO32)
#define LDR_ANALOG     1   // IO1  - LDR1 analog         (was GPIO33, 10k pull-up naar 3V3!)
// IO18 vrij sinds v3.4 (was CO2 PWM) — gepland voor GP4 (v3.9)
#define I2C_SDA       13   // v3.4: IO13 - I2C SDA (SCD41)
#define I2C_SCL       11   // v3.4: IO11 - I2C SCL (SCD41)
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
const char* NVS_CO2_ENABLED         = "co2_en";          // v3.4: = SCD41 aan/uit
const char* NVS_SCD_ASC             = "scd_asc";         // v3.4: bool, automatische kalibratie (standaard aan)
const char* NVS_SCD_TOFF            = "scd_toff";        // v3.4: float, temperatuuroffset °C (standaard 4,0)
const char* NVS_DUST_ENABLED        = "dust_en";
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
const char* NVS_LIGHT_ON_MIN        = "light_on_min";    // int: licht-aan tijd in minuten (0-30, default 0=5s)
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
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);  // Tijdelijk 1, lengte wordt in setup() gezet
AsyncWebServer server(80);




// Room-specifieke instellingen (worden uit NVS geladen)
// v2.10: String → char[] (BSS i.p.v. heap) — patroon uit HVAC v1.11
char room_id[32]           = "Testroom";      // Default bij eerste flash
// mdns_name verwijderd (v2.10) — ongebruikt na MDNS.begin() verwijdering in v2.9
char wifi_ssid[64]         = "netwerknaam";
char wifi_pass[64]         = "paswoord";
char static_ip_str[20]     = "192.168.xx.xx"; // Wordt omgezet naar IPAddress
char pixel_nicknames[30][32];                  // v2.10: char[30][32] i.p.v. String[30] — ~1.5KB heap bespaard
char mac_address[20]       = "";              // Voor display in settings



// Configureerbare defaults
int heating_setpoint_default = 20;
int vent_request_default     = 25;  // v2.21: default 0→25%
float dew_safety_margin      = 2.0;
int home_mode_default        = 0;              // 0 = Weg
int light_dark_threshold     = 50;
unsigned long mov_window_ms  = 60000;
int ldr_dark_threshold       = 50;
int beam_alert_threshold     = 50;



// Optionele feature enables (default 1 = aan)
bool co2_enabled   = false;  // v1.7: default false. v3.4: = SCD41 aan/uit
// v3.4: SCD41
SensirionI2cScd4x scd;
bool  scd_asc        = true;    // automatische zelfkalibratie
float scd_toff       = 4.0f;    // temperatuuroffset (°C), Sensirion-standaard
bool  scd_ok         = false;   // sensor antwoordt en meet
uint64_t scd_serial  = 0;
uint32_t scd_errors  = 0;       // leesfouten sinds de opstart
uint8_t  scd_err_run = 0;       // fouten op rij
unsigned long scd_started_ms = 0;  // start van de periodieke meting
unsigned long scd_last_ok_ms = 0;  // laatste geldige meting
unsigned long scd_last_poll  = 0;
unsigned long scd_last_try   = 0;  // laatste init-poging
volatile bool scd_frc_requested = false;  // kalibratie gevraagd via /scd_frc, uitgevoerd in loop()
char  scd_frc_result[64] = "";
float scd_temp = 0.0f;          // al
int   scd_humi = 0;             // am
bool dust_enabled  = false;  // v1.9: default false — delayMicroseconds(9680) kan WDT triggeren als sensor niet aanwezig
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
int heating_on = 0;         // b: Verwarming aan (0/1)
int vent_percent = 0;       // g: Ventilatie % (0-100)
// v2.21: heating_mode + vent_mode verwijderd — verwarming altijd AUTO, ventilatie altijd via slider of CO2
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
char ds_nicknames[DS_MAX_SENSORS][48];     // v2.10: char[4][48] i.p.v. String[4]
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
// v2.16: licht-aan tijd instelbaar (0-30 min + 5s overtime). Default 0 = 5s.
int light_on_min = 0;
inline unsigned long lightOnDuration() {
  return (unsigned long)light_on_min * 60000UL + 5000UL;
}
int LDR_DARK_THRESHOLD = 40;  // Wordt overschreven door NVS waarde
unsigned long MOV_WINDOW_MS = 60000;  // Wordt overschreven door NVS (in ms)


#define MOV_BUF_SIZE 20  // v2.10: 50→20 — 20 triggers/min is ruim voldoende, spaart 240 bytes BSS
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
    String existing = preferences.getString(nkey, "");
    if (existing.isEmpty()) {
      char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id, i+1);
      preferences.putString(nkey, defnick);
      strlcpy(ds_nicknames[i], defnick, sizeof(ds_nicknames[i]));
    } else {
      strlcpy(ds_nicknames[i], existing.c_str(), sizeof(ds_nicknames[i]));
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
    char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id, i+1);
    String tmp = preferences.getString(nkey, defnick);
    strlcpy(ds_nicknames[i], tmp.c_str(), sizeof(ds_nicknames[i]));
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

// ============== SCD41 (v3.4) ==============
// Alleen aangeroepen vanuit setup() en loop() — nooit vanuit de webtaak (I2C niet gedeeld tussen taken).

void scdClearValues() {
  co2 = 0; scd_temp = 0.0f; scd_humi = 0;
}

// Start of herstart de sensor: periodieke meting stoppen, ASC en offset (alleen RAM, geen EEPROM-slijtage),
// serienummer lezen, periodieke meting starten. Duurt ±0,6 s (stopPeriodicMeasurement wacht 500 ms).
bool scdInit() {
  scd_last_try = millis();
  scd.wakeUp();                      // geen fout als de sensor al wakker is
  scd.stopPeriodicMeasurement();     // nodig voor configuratie
  int16_t err = scd.reinit();
  if (!err) err = scd.getSerialNumber(scd_serial);
  if (!err) err = scd.setAutomaticSelfCalibrationEnabled(scd_asc ? 1 : 0);
  if (!err) err = scd.setTemperatureOffset(scd_toff);
  if (!err) err = scd.startPeriodicMeasurement();
  scd_ok = (err == 0);
  scd_err_run = 0;
  if (scd_ok) {
    scd_started_ms = millis();
    scd_last_ok_ms = millis();       // geef de eerste meting 60 s de tijd
    Serial.printf("SCD41: gestart (serienr %llX, ASC %s, offset %.1f C)\n",
                  (unsigned long long)scd_serial, scd_asc ? "aan" : "uit", scd_toff);
  } else {
    scdClearValues();
    Serial.printf("SCD41: geen antwoord op I2C (fout %d)\n", err);
  }
  return scd_ok;
}

// Om de 5 s: nieuwe meting klaar? Dan lezen (enkele ms). Fouten tellen; na 3 op rij of 60 s zonder
// geldige meting → "ontbreekt" (waarden 0) en elke 60 s opnieuw initialiseren.
void scdPoll() {
  if (!co2_enabled || ap_mode_active) return;
  if (!scd_ok) {
    if (millis() - scd_last_try >= 60000UL) scdInit();
    return;
  }
  if (millis() - scd_last_poll < 5000UL) return;
  scd_last_poll = millis();

  bool ready = false;
  int16_t err = scd.getDataReadyStatus(ready);
  if (!err && ready) {
    uint16_t c = 0; float t = 0.0f, h = 0.0f;
    err = scd.readMeasurement(c, t, h);
    if (!err && c > 0) {             // c = 0: ongeldige meting (kan na de start)
      co2 = c;
      scd_temp = t;
      scd_humi = (int)roundf(h);
      scd_last_ok_ms = millis();
      scd_err_run = 0;
    }
  }
  if (err) { scd_errors++; scd_err_run++; }
  if (scd_err_run >= 3 || millis() - scd_last_ok_ms > 60000UL) {
    scd_ok = false;
    scdClearValues();
    Serial.println("SCD41: ontbreekt (3 fouten op rij of 60 s zonder meting)");
  }
}

// Geforceerde kalibratie (FRC) op 425 ppm, gevraagd via /scd_frc. Sensor moet ≥3 min in buitenlucht
// gemeten hebben (gecontroleerd in de webhandler). Duurt ±1 s; daarna periodieke meting opnieuw.
void scdForcedRecalibration() {
  uint16_t corr = 0xFFFF;
  scd.stopPeriodicMeasurement();
  int16_t err = scd.performForcedRecalibration(425, corr);
  if (!err && corr != 0xFFFF) {
    snprintf(scd_frc_result, sizeof(scd_frc_result), "OK, correctie %+d ppm (na %lu min uptime)",
             (int)corr - 0x8000, (unsigned long)(millis() / 60000UL));
  } else {
    snprintf(scd_frc_result, sizeof(scd_frc_result), "mislukt (fout %d)", err);
  }
  Serial.printf("SCD41 kalibratie: %s\n", scd_frc_result);
  scdInit();                         // periodieke meting opnieuw starten
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

// ======================== HELPER: Shared CSS (v2.24) ========================
// Schrijft gedeelde CSS direct naar stream — vermijdt F() overhead
void writeSharedCSS(AsyncResponseStream *p) {
  p->print("body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}");
  p->print(".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}");
  p->print(".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}");
  p->print(".container{display:flex;min-height:calc(100vh - 60px);}");
  p->print(".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}");
  p->print(".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}");
  p->print(".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}");
  p->print("@media(max-width:600px){");
  p->print(".container{flex-direction:column;}");
  p->print(".sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;display:flex;justify-content:center;}");
  p->print(".sidebar a{margin:0 3px;}}");
}

// ==============================================================================

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
    "\"w\":%d,"       // mov1_triggers (/min)
    "\"x\":%d,"       // mov2_triggers (/min)
    "\"y\":%d,"       // mov1_light (lamp aan 0/1)
    "\"z\":%d,"       // mov2_light (lamp aan 0/1)
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
  // v3.4: SCD41 — al = temperatuur (°C), am = vocht (%); 0 = geen meting. Altijd achteraan.
  {
    char scd_entry[32];
    snprintf(scd_entry, sizeof(scd_entry), ",\"al\":%.1f,\"am\":%d", scd_temp, scd_humi);
    strncat(json, scd_entry, sizeof(json) - strlen(json) - 1);
  }
  strncat(json, "}", sizeof(json) - strlen(json) - 1);

  return json;  // v2.7: const char* — geen String-kopie op heap
}



// v2.10: const char* met static buf — geen heap-alloc bij elke aanroep
const char* getFormattedDateTime() {
  static char buf[32];
  time_t now;
  time(&now);
  if (now < 1700000000) {
    return "tijd nog niet gesynchroniseerd";
  }
  struct tm tm;
  localtime_r(&now, &tm);
  strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tm);
  return buf;
}








void setup() {
  Serial.begin(115200);
  delay(1500);
  while (Serial.available()) Serial.read();  // flush
  // v3.2: versie, reden herstart en opstartteller
  boot_reason = esp_reset_reason();
  {
    Preferences bootPrefs;
    bootPrefs.begin("crash-log", false);
    boot_count = bootPrefs.getUInt("boots", 0) + 1;
    bootPrefs.putUInt("boots", boot_count);
    bootPrefs.end();
  }
  Serial.printf("\n=== ESP32-C6 ROOM %s ===\n", ROOM_VERSION);
  Serial.printf("[BOOT] Opstart #%u — reden: %s\n", boot_count, resetReasonText(boot_reason));

  // v3.3: "laatste stap" van vóór de herstart ophalen en in de geschiedenis (NVS h1–h3) zetten
  {
    bool crumbs_ok = (crumb_magic == CRUMB_MAGIC) && (boot_reason != ESP_RST_POWERON);
    char entry[160];
    if (crumbs_ok) {
      uint8_t cl = crumb_loop < CL_COUNT ? crumb_loop : CL_NONE;
      uint8_t cw = crumb_web  < CW_COUNT ? crumb_web  : CW_NONE;
      unsigned long ago = (crumb_up_s >= crumb_web_s) ? (crumb_up_s - crumb_web_s) : 0;
      if (cw == CW_NONE) {
        snprintf(prev_step, sizeof(prev_step), "loop: %s", CRUMB_LOOP_TXT[cl]);
      } else {
        snprintf(prev_step, sizeof(prev_step), "loop: %s &middot; web: %s (%lu s ervoor)",
                 CRUMB_LOOP_TXT[cl], CRUMB_WEB_TXT[cw], ago);
      }
      snprintf(entry, sizeof(entry), "#%u %s &middot; na %.1f u &middot; %s",
               boot_count, resetReasonText(boot_reason), crumb_up_s / 3600.0f, prev_step);
    } else {
      snprintf(entry, sizeof(entry), "#%u %s", boot_count, resetReasonText(boot_reason));
    }
    Preferences histPrefs;
    histPrefs.begin("crash-log", false);
    String h1 = histPrefs.getString("h1", ""), h2 = histPrefs.getString("h2", "");
    histPrefs.putString("h3", h2);
    histPrefs.putString("h2", h1);
    histPrefs.putString("h1", entry);
    histPrefs.end();
    Serial.printf("[BOOT] Laatste stap vóór de herstart: %s\n", crumbs_ok ? prev_step : "(niet gekend: stroom aan of eerste start)");
    crumb_magic = CRUMB_MAGIC;
    crumb_loop = CL_SETUP; crumb_web = CW_NONE; crumb_web_s = 0; crumb_up_s = 0;
  }

  // v2.4 FIX 7: Crash-logging — lees vorige crash uit NVS bij elke boot
  {
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", true);  // read-only
    char lastCrash[48];
    String tmp = crashPrefs.getString("reason", "geen");
    strlcpy(lastCrash, tmp.c_str(), sizeof(lastCrash));
    uint32_t crashCnt = crashPrefs.getUInt("count", 0);
    crashPrefs.end();
    if (crashCnt > 0) {
      Serial.printf("[BOOT] ⚠️  Lage-heap meldingen: %u (laatste: %s)\n", crashCnt, lastCrash);
    } else {
      Serial.println("[BOOT] Geen lage-heap meldingen.");
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
    preferences.putInt(NVS_VENT_REQUEST, 25);  // v2.21: default 25%
    preferences.putFloat(NVS_DEW_MARGIN, 2.0);
    preferences.putInt(NVS_HOME_MODE, 0);
    preferences.putInt(NVS_LIGHT_THRESHOLD, 50);
    preferences.putULong(NVS_MOV_WINDOW, 60000UL);
    preferences.putInt(NVS_LDR_DARK, 50);
    preferences.putInt(NVS_BEAM_THRESHOLD, 50);
    
    preferences.putBool(NVS_CO2_ENABLED, false);
    preferences.putBool(NVS_SCD_ASC, true);      // v3.4
    preferences.putFloat(NVS_SCD_TOFF, 4.0f);    // v3.4
    preferences.putBool(NVS_DUST_ENABLED, false);
    preferences.putBool(NVS_MOV2_ENABLED, true);
    preferences.putBool(NVS_TSTAT_ENABLED, true);
    preferences.putBool(NVS_BEAM_ENABLED, true);
    preferences.putUChar(NVS_NEO_R, 255);
    preferences.putUChar(NVS_NEO_G, 255);
    preferences.putUChar(NVS_NEO_B, 255);
    preferences.putInt(NVS_PIXELS_NUM, 8);
    for (int i = 0; i < 30; i++) {
      char pbkey[24]; snprintf(pbkey, sizeof(pbkey), "%s%d", NVS_PIXEL_ON_BASE, i);
      preferences.putBool(pbkey, false);
    }
    preferences.putInt(NVS_PIXEL_MODE_0, 0);
    preferences.putInt(NVS_PIXEL_MODE_1, 0);
    Serial.println("Defaults opgeslagen in NVS. Configureer via webinterface /settings");
  }
  
  // Laad alles uit NVS — v2.10: strlcpy naar char[] i.p.v. String=
  { String tmp = preferences.getString(NVS_ROOM_ID, "Testroom");
    strlcpy(room_id, tmp.c_str(), sizeof(room_id)); }
  // mdns_name verwijderd (v2.10) — ongebruikt na MDNS.begin() verwijdering
  { String tmp = preferences.getString(NVS_WIFI_SSID, "netwerknaam");
    strlcpy(wifi_ssid, tmp.c_str(), sizeof(wifi_ssid)); }
  { String tmp = preferences.getString(NVS_WIFI_PASS, "paswoord");
    strlcpy(wifi_pass, tmp.c_str(), sizeof(wifi_pass)); }
  { String tmp = preferences.getString(NVS_STATIC_IP, "192.168.xx.xx");
    strlcpy(static_ip_str, tmp.c_str(), sizeof(static_ip_str)); }
  
  heating_setpoint_default = preferences.getInt(NVS_HEATING_SETPOINT, 20);
  vent_request_default     = preferences.getInt(NVS_VENT_REQUEST, 25);  // v2.21: default 25%
  dew_safety_margin        = preferences.getFloat(NVS_DEW_MARGIN, 2.0);
  home_mode_default        = preferences.getInt(NVS_HOME_MODE, 0);
  light_dark_threshold     = preferences.getInt(NVS_LIGHT_THRESHOLD, 50);
  mov_window_ms            = preferences.getULong(NVS_MOV_WINDOW, 60000UL);
  ldr_dark_threshold       = preferences.getInt(NVS_LDR_DARK, 50);
  beam_alert_threshold     = preferences.getInt(NVS_BEAM_THRESHOLD, 50);
  
  co2_enabled      = preferences.getBool(NVS_CO2_ENABLED, false);   // v2.5 fix: default false. v3.4: = SCD41 aan/uit
  scd_asc          = preferences.getBool(NVS_SCD_ASC, true);        // v3.4
  scd_toff         = constrain(preferences.getFloat(NVS_SCD_TOFF, 4.0f), 0.0f, 20.0f);  // v3.4
  dust_enabled     = preferences.getBool(NVS_DUST_ENABLED, false);  // v2.5 fix: default false
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
  // v2.10: char[30][32] — geen heap-alloc per nickname
  for (int i = 0; i < 30; i++) {
    char nickkey[24]; snprintf(nickkey, sizeof(nickkey), "%s%d", NVS_PIXEL_NICK_BASE, i);
    String tmp = preferences.getString(nickkey, "");
    if (tmp.isEmpty()) {
      // Genereer default nickname
      snprintf(pixel_nicknames[i], sizeof(pixel_nicknames[i]), "%s Pixel %d", room_id, i);
      preferences.putString(nickkey, pixel_nicknames[i]);
    } else {
      strlcpy(pixel_nicknames[i], tmp.c_str(), sizeof(pixel_nicknames[i]));
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
    pixel_nicknames[i][0] = '\0';  // Niet gebruiken
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
  light_on_min  = preferences.getInt(NVS_LIGHT_ON_MIN, 0);
  light_on_min  = constrain(light_on_min, 0, 30);
  updateFadeInterval();


  
  Serial.printf("Room ID: %s\n", room_id);
  // mDNS naam niet meer getoond (MDNS verwijderd v2.9)
  
  if (first_boot) {
    Serial.println("Typ 'reset_nvs' in serial monitor voor factory reset");
  }


  pinMode(PIR_MOV1, INPUT_PULLUP);  // Voor 3.3V HC-SR501: beweging = LOW
  pinMode(PIR_MOV2, INPUT_PULLUP);
  pinMode(SHARP_LED, OUTPUT); digitalWrite(SHARP_LED, HIGH);
  pinMode(TSTAT_PIN, INPUT_PULLUP);
  pinMode(OPTION_LDR, INPUT);

  dht.begin();
  // v3.4: I2C + SCD41, alleen als de sensor aan staat in /settings
  if (co2_enabled) {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    scd.begin(Wire, SCD41_I2C_ADDR_62);
    scdInit();
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
  WiFi.begin(wifi_ssid, wifi_pass);

  // Haal MAC op NA WiFi.begin voor betere compatibiliteit
  { String tmp = WiFi.macAddress(); strlcpy(mac_address, tmp.c_str(), sizeof(mac_address)); }
  Serial.printf("MAC adres: %s\n", mac_address);

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
    
    char ap_ssid[48];
    snprintf(ap_ssid, sizeof(ap_ssid), "ROOM-%s", room_id);
    WiFi.softAP(ap_ssid);

    IPAddress ap_ip(192, 168, 4, 1);
    WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
    
    Serial.println("\n=== ACCESS POINT GESTART ===");
    Serial.printf("SSID: %s\n", ap_ssid);
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

  // Geen mDNS (verwijderd in v2.9 wegens Matter; terug in v3.3). Toegang via static IP (bv. http://192.168.0.80)



  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");



  // === HOME PAGE ===
    // v3.1: LANDINGSPAGINA — eenvoudige bediening voor iedereen (zie landing.h)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_LANDING);  // v3.3
      AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
      p->print(FPSTR(LANDING_KOP));
      p->print(room_id);
      p->print(FPSTR(LANDING_MIDDEN));
      p->print(F("<script>var CFG={ver:\"ROOM " ROOM_VERSION "\",mov2:"));
      p->print(mov2_enabled ? 1 : 0);
      p->print(F(",px:["));
      for (int i = 0; i < pixels_num; i++) {
        if (i) p->print(',');
        p->print('"');
        for (int j = 0; j < 32 && pixel_nicknames[i][j] != '\0'; j++) {   // veilig voor JS én HTML
          char c = pixel_nicknames[i][j];
          if      (c == '\"') p->print(F("\\\""));
          else if (c == '\\') p->print(F("\\\\"));
          else if (c == '<')  p->print(F("&lt;"));
          else if (c == '&')  p->print(F("&amp;"));
          else                p->print(c);
        }
        p->print('"');
      }
      p->print(F("]};</script>"));
      p->print(FPSTR(LANDING_SCRIPT));
      request->send(p);
    });

    // v3.1: de vroegere statuspagina (technisch) staat nu op /advanced
    server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_ADVANCED);  // v3.3
    // v2.5: AsyncResponseStream — geen html.reserve(12000) meer op de heap
    AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");

    // v2.12: actuele neo-kleur als hex string voor dot-cirkels
    char nh[8];
    snprintf(nh, sizeof(nh), "#%02x%02x%02x", neo_r, neo_g, neo_b);

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
      ".dot{display:inline-block;width:14px;height:14px;border-radius:50%;vertical-align:middle;}"
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
    p->print(F("<span style=\"font-size:12px;font-weight:normal;opacity:.75\"> &middot; ROOM " ROOM_VERSION "</span></div><div class=\"header-right\">"));
    p->print(uptime_sec);
    p->print(F(" s &nbsp;&nbsp; "));
    p->print(getFormattedDateTime());
    p->print(F("</div></div>"
      "<div class=\"container\">"
      "<div class=\"sidebar\" style=\"flex-wrap:wrap\">"
      "<a href=\"/\" title=\"Start\" style=\"width:36px\">&#127968;</a>"
      "<a href=\"/advanced\" class=\"active\">Status</a>"
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
      "<tr><td class=\"label\">Dauwpunt alarm</td><td class=\"value\">"));
    // v2.12: dot — rood als alarm actief, grijs als ok
    p->printf("<span class='dot' style='background:%s'></span>", dew_alert ? "#c00" : "#bbb");
    p->print(F("</td><td class=\"control\"></td></tr>"));
    if (co2_enabled) {
      p->print(F("<tr><td class=\"label\">CO&#8322;</td><td class=\"value\">"));
      p->printf("%d ppm", co2);
      p->print(sensorWarn(co2 == 0));  // v2.8: 0 ppm = sensor leest niet
      p->print(F("</td><td class=\"control\"></td></tr>"));
      // v3.4: SCD41 temperatuur en vocht (extra; de DHT22 blijft de bron voor vocht en dauwpunt)
      p->printf("<tr><td class=\"label\">SCD41 temp</td><td class=\"value\">%.1f &deg;C</td><td class=\"control\"></td></tr>", scd_temp);
      p->printf("<tr><td class=\"label\">SCD41 vocht</td><td class=\"value\">%d %%</td><td class=\"control\"></td></tr>", scd_humi);
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
    p->print(F("<tr><td class=\"label\">Ventilatie snelheid %</td><td class=\"value\">"));
    p->printf("%d %%", vent_percent);
    // v2.21: CO2 dot — lichtblauw als CO2 stuurt, grijs als slider bepaalt
    p->printf("<span class='dot' style='background:%s;margin-left:5px' title='CO2'>",
      (co2_enabled && co2 > 0) ? "#3aafe0" : "#bbb");
    p->print(F("</span>"));
    p->printf("</td><td class=\"control\"><form action=\"/set_vent\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><input type=\"range\" class=\"slider\" name=\"vent\" min=\"0\" max=\"100\" value=\"%d\" onchange=\"submitAjax(this.form);\"></form></td></tr>", vent_percent);
    p->print(F("<tr><td class=\"label\">Thuis</td><td class=\"value\">"));
    // v2.12: dot — groen als thuis
    p->printf("<span class='dot' style='background:%s'></span>", home_mode ? "#2a9d2a" : "#bbb");
    p->printf("</td><td class=\"control\"><form action=\"/toggle_home\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      home_mode ? "checked" : "");
    if (tstat_enabled) {
      p->print(F("<tr><td class=\"label\">Hardware thermostaat</td><td class=\"value\">"));
      // v2.12: dot — groen als AAN
      p->printf("<span class='dot' style='background:%s'></span>", tstat_on ? "#2a9d2a" : "#bbb");
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    p->print(F("<tr><td class=\"label\">Verwarming aan</td><td class=\"value\">"));
    // v2.12: dot — oranje als verwarming actief
    p->printf("<span class='dot' style='background:%s'></span>", heating_on ? "#e05c00" : "#bbb");
    p->print(F("</td><td class=\"control\"></td></tr>"
      "</table>"
      "<div class=\"group-title\">VERLICHTING</div>"
      "<table>"));
    p->print(F("<tr><td class=\"label\">LDR (donker=100)</td><td class=\"value\">"));
    p->print(light_ldr);
    p->print(F("</td><td class=\"control\"></td></tr>"
      "<tr><td class=\"label\">MOV1</td><td class=\"value\">"));
    // v2.16: dot — rood als recent beweging (triggers>0), ongeacht licht
    p->printf("<span class='dot' style='background:%s'></span>", mov1_triggers > 0 ? "#c00" : "#bbb");
    p->print(F("</td><td class=\"control\"></td></tr>"));
    if (mov2_enabled) {
      p->print(F("<tr><td class=\"label\">MOV2</td><td class=\"value\">"));
      p->printf("<span class='dot' style='background:%s'></span>", mov2_triggers > 0 ? "#c00" : "#bbb");
      p->print(F("</td><td class=\"control\"></td></tr>"));
    }
    // v2.16: Licht-aan tijd slider (0-30 min + 5s overtime)
    p->print(F("<tr><td class=\"label\">Licht tijd (min)</td><td class=\"value\">"));
    p->printf("%d min", light_on_min);
    p->printf("</td><td class=\"control\"><form action=\"/set_light_on_min\" method=\"get\" "
      "onsubmit=\"event.preventDefault();submitAjax(this);\">"
      "<input type=\"range\" class=\"slider\" name=\"mins\" min=\"0\" max=\"30\" value=\"%d\" "
      "onchange=\"submitAjax(this.form);\"></form></td></tr>", light_on_min);
    // NeoPixel kleurkiezer — hex string via printf
    p->print(F("<tr><td class=\"label\">NeoPixel Kleur</td><td class=\"value\" id=\"rgb_val\">"));
    p->printf("%d, %d, %d", (int)neo_r, (int)neo_g, (int)neo_b);
    p->printf("</td><td class=\"control\"><input type=\"color\" id=\"colorPicker\" value=\"#%02x%02x%02x\""
      " onchange=\"setNeoColor(this.value)\" style=\"width:48px;height:34px;border:none;cursor:pointer;padding:2px;\"></td></tr>",
      neo_r, neo_g, neo_b);
    p->print(F("<tr><td class=\"label\">Bed modus</td><td class=\"value\">"));
    // v2.12: dot — paars als bed-modus actief
    p->printf("<span class='dot' style='background:%s'></span>", bed ? "#7b2fbe" : "#bbb");
    p->printf("</td><td class=\"control\"><form action=\"/toggle_bed\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\"><input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
      bed ? "checked" : "");
    p->print(F("<tr><td class=\"label\">Dim snelheid (s)</td><td class=\"value\">"));
    p->print(fade_duration);
    p->printf("</td><td class=\"control\"><form action=\"/set_fade_duration\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><input type=\"range\" class=\"slider\" name=\"duration\" min=\"1\" max=\"10\" value=\"%d\" onchange=\"submitAjax(this.form);\"></form></td></tr>",
      fade_duration);

    // Dynamische pixels loop — v2.7: char[] i.p.v. String label/action (geen heap-alloc per pixel)
    for (int i = 0; i < pixels_num; i++) {
      char label[48];
      if (pixel_nicknames[i][0] == '\0') {
        if      (i == 0)               snprintf(label, sizeof(label), "Pixel %d (MOV1)", i);
        else if (i == 1 && mov2_enabled) snprintf(label, sizeof(label), "Pixel %d (MOV2)", i);
        else                           snprintf(label, sizeof(label), "Pixel %d", i);
      } else {
        snprintf(label, sizeof(label), "%s", pixel_nicknames[i]);
      }
      const char* val  = pixel_on[i] ? "On" : "Off";
      const char* chkd = pixel_on[i] ? "checked" : "";
      char action[40];
      if (i == 0 || (i == 1 && mov2_enabled))
        snprintf(action, sizeof(action), "/toggle_pixel_mode?idx=%d", i);
      else
        snprintf(action, sizeof(action), "/toggle_pixel?idx=%d", i);
      // v2.12: dot met neo-kleur als pixel aan, grijs als uit
      char dotSpan[64];
      snprintf(dotSpan, sizeof(dotSpan), "<span class='dot' style='background:%s'></span>",
               pixel_on[i] ? nh : "#bbb");
      p->printf("<tr><td class=\"label\">%s</td><td class=\"value\">%s</td>"
        "<td class=\"control\"><form action=\"%s\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\"><label class=\"switch\">"
        "<input type=\"checkbox\" %s onchange=\"submitAjax(this.form);\"><span class=\"slider-switch\"></span></label></form></td></tr>",
        label, dotSpan, action, chkd);
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
        "<tr><td class=\"label\">Beam alert</td><td class=\"value\">"));
      // v2.12: dot — rood als alert actief
      p->printf("<span class='dot' style='background:%s'></span>", beam_alert_new ? "#c00" : "#bbb");
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
      // v2.16: timer fix — onafhankelijke klok, losgekoppeld van JSON-fetch
      "var lastUptime=0;"
      "var lastNh='#bbb';"
      "function updateClock(){"
        "var h=document.querySelector('.header-right');"
        "if(!h)return;"
        "var n=new Date();"
        "h.innerHTML=lastUptime+' s &nbsp;&nbsp;'+n.toLocaleDateString('nl-BE')+' '+n.toLocaleTimeString('nl-BE');}"
      "function dot(v,col){return '<span class=\"dot\" style=\"background:'+(v?(col||'#2a9d2a'):'#bbb')+'\"></span>';}"
      // v2.8: sw(fault, critical) — geeft ⚠ span terug, '' als geen fout
      "function sw(f,c){"
        "return f?\"<span style='color:\"+(c?'#c00':'#f80')+\";font-size:13px;margin-left:4px' "
        "title='\"+(c?'Abnormale':'Verdachte')+\" waarde'>&#9888;</span>\":'';}"
      "function updateValues(){"
      "fetch('/json?'+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(data=>{"
      // v2.12: neo-hex berekenen voor dot-cirkels van pixels en MOV-lampen
      "var toHex=v=>('0'+Math.round(v).toString(16)).slice(-2);"
      "lastNh='#'+toHex(data.q)+toHex(data.r)+toHex(data.s);"
      "var nh=lastNh;"
      "document.querySelectorAll('td.value').forEach(td=>{"
      "const l=td.previousElementSibling;if(!l)return;"
      "const lbl=l.textContent.trim();"
      "if(lbl.includes('Room temp')) td.innerHTML=data.f.toFixed(1)+' \u00b0C<br><small>('+data.e.toFixed(1)+', '+data.f.toFixed(1)+')</small>'"
        "+sw(data.f==0||data.f<5||data.f>40)"
        "+sw(data.e==0||data.e<5||data.e>40);"
      "else if(lbl.includes('Humidity')) td.innerHTML=Math.round(data.h)+' %'"
        "+sw(data.h==0||data.h<10||data.h>99);"
      "else if(lbl.includes('Dauwpunt')&&!lbl.includes('alarm')) td.innerHTML=data.i.toFixed(1)+' \u00b0C'"
        "+sw(data.j==1,false);"
      "else if(lbl.includes('alarm')) td.innerHTML=dot(data.j,'#c00');"
      "else if(lbl.includes('CO')) td.innerHTML=data.k+' ppm'+sw(data.k==0);"
      // v3.4: SCD41-rijen
      "else if(lbl==='SCD41 temp') td.innerHTML=(data.al||0).toFixed(1)+' \u00b0C';"
      "else if(lbl==='SCD41 vocht') td.innerHTML=(data.am||0)+' %';"
      "else if(lbl.includes('Stof')) td.innerHTML=data.l+sw(data.l==0);"
      "else if(lbl.includes('Heating setpoint')) td.textContent=data.c+' \u00b0C';"
      "else if(lbl.includes('Ventilatie snelheid')){"
        "td.innerHTML=data.g+' %'+(data.k>0?\"<span class='dot' style='background:#3aafe0;margin-left:5px'></span>\":\"<span class='dot' style='background:#bbb;margin-left:5px'></span>\");"
        "var sl=document.querySelector('input[name=vent]');"
        "if(sl&&sl!==document.activeElement) sl.value=data.g;}"
      "else if(lbl.includes('Hardware thermostaat')) td.innerHTML=dot(data.d,'#2a9d2a');"
      "else if(lbl.includes('Verwarming aan')) td.innerHTML=dot(data.b,'#e05c00');"
      "else if(lbl.includes('LDR')) td.textContent=data.m;"
      "else if(lbl.includes('NeoPixel Kleur')){"
        "td.textContent=data.q+', '+data.r+', '+data.s;"
        "var cp=document.getElementById('colorPicker');"
        "if(cp) cp.value='#'+toHex(data.q)+toHex(data.r)+toHex(data.s);}"
      "else if(lbl==='Bed modus') td.innerHTML=dot(data.p,'#7b2fbe');"
      "else if(lbl==='Thuis') td.innerHTML=dot(data.v,'#2a9d2a');"
      // v2.16: nieuwe dot-handlers — sp.style.background (geen innerHTML, geen quote issues)
      "else if(lbl==='MOV1'){var sp=td.querySelector('.dot');if(sp)sp.style.background=data.w>0?'#c00':'#bbb';}"
      "else if(lbl==='MOV2'){var sp=td.querySelector('.dot');if(sp)sp.style.background=data.x>0?'#c00':'#bbb';}"
      "else if(lbl.includes('MOV1 PIR trig')) td.textContent=data.w;"
      "else if(lbl.includes('MOV2 PIR trig')) td.textContent=data.x;"
      // v2.19: Dim snelheid + Licht tijd — slider DOM-waarde lezen (geen JSON key)
      "else if(lbl.includes('Dim snelheid')){var sl=document.querySelector('input[name=duration]');if(sl)td.textContent=sl.value+' s';}"
      "else if(lbl.includes('Licht tijd')){var sl=document.querySelector('input[name=mins]');if(sl)td.textContent=sl.value+' min';}"
      "else if(lbl.includes('Beam sensor waarde')) td.textContent=data.aa;"
      "else if(lbl.includes('Beam alert')) td.innerHTML=dot(data.ab,'#c00');"
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
      // v2.12: pixel-cirkels met neo-kleur
      "else if(lbl.includes('Pixel')){"
        "const idx=parseInt(lbl.match(/\\d+/)[0]);"
        "const pstr=(data.t||'').replace('P=','');"
        "if(idx===0) td.innerHTML=dot(data.y,nh);"
        "else if(idx===1) td.innerHTML=dot(data.z,nh);"
        "else td.innerHTML=dot(pstr.charAt(idx)==='1',nh);}"
      "});"
      "const homeT=document.querySelector('td.control form[action=\"/toggle_home\"] input');"
      "if(homeT) homeT.checked=(data.v==1);"
      "document.querySelectorAll('td.control form[action^=\"/toggle_pixel_mode\"] input').forEach((cb,i)=>{"
        "cb.checked=(data.u&&data.u.charAt(i)==='1');});"
      // v2.16: uptime opslaan voor updateClock(), header update zit in updateClock()
      "lastUptime=data.a;"
      "}).catch(e=>console.error(e));}"
      "function submitAjax(form){"
      "const p=new URLSearchParams();let px=null;"
      "for(const el of form.elements){"
        "if(el.name){p.append(el.name,el.value);"
        "if(el.name==='pixel'&&el.type==='hidden') px=parseInt(el.value);"
        "if(el.name==='state') px={idx:px,state:el.value==='1'};}"
      "}"
      "const url=p.toString()?form.action+'?'+p.toString():form.action;"
      // v2.12: pixel preview via dot met lastNh (meest recente neo-kleur)
      "if(px&&px.idx!==null) document.querySelectorAll('td.value').forEach(td=>{const l=td.previousElementSibling;if(l&&l.textContent.includes('Pixel '+px.idx)) td.innerHTML=dot(px.state,lastNh);});"
      "fetch(url).then(r=>{if(r.ok){updateValues();const s=document.getElementById('status');if(s){s.textContent='\u2713';setTimeout(()=>s.textContent='',1500);}}}).catch(e=>console.error(e));}"
      // v2.16: DOMContentLoaded betrouwbaarder dan load voor inline scripts
      // updateClock elke 1s onafhankelijk van JSON-fetch
      "document.addEventListener('DOMContentLoaded',function(){"
        "updateValues();"
        "setInterval(updateValues,3000);"
        "setInterval(updateClock,1000);});"
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
    crumbW(CW_JSON);  // v3.3
    request->send(200, "application/json", getJSON());
  });

  // === CAPABILITIES ENDPOINT (v2.22) ===
  // Geeft pixel nicknames terug voor portal UI
  // Heap impact: ~100 bytes tijdens request (streaming), geen persistent memory
  server.on("/capabilities", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_CAPS);  // v3.3
    AsyncResponseStream *p = request->beginResponseStream("application/json");
    p->print(F("{\"pixels\":["));
    for (int i = 0; i < pixels_num; i++) {
      if (i > 0) p->print(',');
      p->print('"');
      // Escape quotes in nickname (JSON safety)
      for (int j = 0; pixel_nicknames[i][j] != '\0' && j < 32; j++) {
        if (pixel_nicknames[i][j] == '"') p->print("\\\"");
        else p->print(pixel_nicknames[i][j]);
      }
      p->print('"');
    }
    p->print(F("]}"));
    request->send(p);
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
    p->print(F("<span style=\"font-size:12px;font-weight:normal;opacity:.75\"> &middot; ROOM " ROOM_VERSION "</span></div><div class=\"header-right\">"));
    p->print(uptime_sec);
    p->print(F(" s</div></div>"
      "<div class=\"container\">"
      "<div class=\"sidebar\" style=\"flex-wrap:wrap\">"
      "<a href=\"/\" title=\"Start\" style=\"width:36px\">&#127968;</a>"
      "<a href=\"/advanced\">Status</a>"
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
    crumbW(CW_OTA);  // v3.3
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
    crumbW(CW_OTHER);  // v3.3
    request->send(200, "text/html", "<h2>Rebooting ESP32...</h2>");
    delay(500);
    ESP.restart();
  });



  // Toggle voor Bed (r)
  server.on("/toggle_bed", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);  // v3.3
    bed = !bed;
    preferences.putBool(NVS_BED_STATE, bed);  // direct opslaan
    request->send(200, "text/plain", "OK");
  });




// v2.10: N pixel-handlers → 2 universele handlers met ?idx= parameter
// Spaart (pixels_num - 2) × ~200 bytes handler-heap (bijv. 5 pixels = 3 handlers minder = ~600 bytes)
// Pad: /toggle_pixel_mode?idx=N  en  /toggle_pixel?idx=N
server.on("/toggle_pixel_mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_PIXEL);  // v3.3
  if (!request->hasParam("idx")) { request->send(400, "text/plain", "idx?"); return; }
  int i = constrain(request->getParam("idx")->value().toInt(), 0, 1);
  pixel_mode[i] = 1 - pixel_mode[i];
  const char* key = (i == 0) ? NVS_PIXEL_MODE_0 : NVS_PIXEL_MODE_1;
  preferences.putInt(key, pixel_mode[i]);
  request->send(200, "text/plain", "OK");
});

server.on("/toggle_pixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_PIXEL);  // v3.3
  if (!request->hasParam("idx")) { request->send(400, "text/plain", "idx?"); return; }
  int i = constrain(request->getParam("idx")->value().toInt(), 0, pixels_num - 1);
  pixel_on[i] = !pixel_on[i];
  char pokey[24]; snprintf(pokey, sizeof(pokey), "%s%d", NVS_PIXEL_ON_BASE, i);
  preferences.putBool(pokey, pixel_on[i]);
  request->send(200, "text/plain", "OK");
});





  // === NEOPIXEL KLEURKIEZER PAGE ===
  server.on("/neopixel", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_COLOR);  // v3.3
    request->redirect("/");  // Kleurkiezer zit nu inline op statuspagina
  });

// === CAPTIVE PORTAL HANDLERS — alleen registreren in AP mode (v2.10: heap besparing) ===
if (ap_mode_active) {
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/settings");
  });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/settings");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/settings");
  });
}

// === SETTINGS PAGE - v2.5 AsyncResponseStream ===
server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_SETTINGS);  // v3.3
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
  p->print(F("<span style=\"font-size:12px;font-weight:normal;opacity:.75\"> &middot; ROOM " ROOM_VERSION "</span></div><div class=\"header-right\">Instellingen</div></div>"
    "<div class=\"container\">"
    "<div class=\"sidebar\" style=\"flex-wrap:wrap\">"
    "<a href=\"/\" title=\"Start\" style=\"width:36px\">&#127968;</a>"
    "<a href=\"/advanced\">Status</a>"
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

  // v3.2: Diagnose (versie, uptime, herstart, geheugen, WiFi) + lage-heap meldingen (vroeger "Crashteller")
  {
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", true);
    uint32_t crashCnt = crashPrefs.getUInt("count", 0);
    String crashReason = crashPrefs.getString("reason", "geen");
    crashPrefs.end();
    unsigned long up = millis() / 1000;
    bool slecht = (boot_reason == ESP_RST_PANIC || boot_reason == ESP_RST_INT_WDT || boot_reason == ESP_RST_TASK_WDT ||
                   boot_reason == ESP_RST_WDT || boot_reason == ESP_RST_BROWNOUT);
    p->print(F("<tr><td colspan=\"2\" style=\"background:#336699;color:#fff;font-weight:bold;padding:6px 8px;\">Diagnose</td></tr>"));
    p->print(F("<tr><td class=\"lbl\">Versie</td><td class=\"inp\"><b>ROOM " ROOM_VERSION "</b></td></tr>"));
    p->printf("<tr><td class=\"lbl\">Uptime</td><td class=\"inp\">%lu d %lu u %lu min</td></tr>",
      up / 86400, (up % 86400) / 3600, (up % 3600) / 60);
    p->printf("<tr><td class=\"lbl\">Laatste herstart</td><td class=\"inp\"><b style=\"color:%s\">%s</b></td></tr>",
      slecht ? "#c00" : "#0a0", resetReasonText(boot_reason));
    if (prev_step[0]) {   // v3.3
      p->printf("<tr><td class=\"lbl\">Laatste stap ervoor</td><td class=\"inp\">%s</td></tr>", prev_step);
    }
    p->printf("<tr><td class=\"lbl\">Aantal opstarts</td><td class=\"inp\">%u</td></tr>", boot_count);
    p->printf("<tr><td class=\"lbl\">Geheugen</td><td class=\"inp\">vrij %d %% &middot; grootste blok %u KB &middot; laagste ooit %u KB</td></tr>",
      (int)((ESP.getFreeHeap() * 100) / ESP.getHeapSize()),
      (unsigned)(ESP.getMaxAllocHeap() / 1024), (unsigned)(ESP.getMinFreeHeap() / 1024));
    p->printf("<tr><td class=\"lbl\">WiFi-signaal</td><td class=\"inp\">%d dBm</td></tr>", (int)WiFi.RSSI());
    // v3.4: SCD41-toestand
    p->print(F("<tr><td class=\"lbl\">SCD41</td><td class=\"inp\">"));
    if (!co2_enabled) {
      p->print(F("uit"));
    } else if (scd_ok) {
      p->printf("<b style=\"color:#0a0\">ok</b> &middot; serienr %llX &middot; leesfouten %u &middot; ASC %s &middot; offset %.1f &deg;C",
        (unsigned long long)scd_serial, scd_errors, scd_asc ? "aan" : "uit", scd_toff);
    } else {
      p->printf("<b style=\"color:#c00\">ontbreekt</b> (geen antwoord op I2C IO13/IO11) &middot; leesfouten %u", scd_errors);
    }
    if (scd_frc_result[0]) p->printf("<br>Laatste kalibratie: %s", scd_frc_result);
    p->print(F("</td></tr>"));
    const char* crashColor = crashCnt > 0 ? "#c00" : "#0a0";
    p->printf("<tr><td class=\"lbl\">Lage-heap meldingen</td><td class=\"inp\"><b style=\"color:%s\">%u</b>", crashColor, crashCnt);
    if (crashCnt > 0) {
      p->print(F(" &nbsp; <code style=\"font-size:12px;\">"));
      p->print(crashReason);
      p->print(F("</code> &nbsp; <a href=\"/clear_crash_log\" style=\"font-size:12px;color:#369;\" onclick=\"return confirm('Meldingen wissen?');\">Wissen</a>"));
    }
    p->print(F("</td></tr>"));
    // v3.3: laatste drie opstarts met reden, uptime ervoor en laatste stap
    {
      Preferences hp;
      hp.begin("crash-log", true);
      String hh[3] = { hp.getString("h1", ""), hp.getString("h2", ""), hp.getString("h3", "") };
      hp.end();
      p->print(F("<tr><td class=\"lbl\">Vorige herstarts</td><td class=\"inp\" style=\"font-size:12px;\">"));
      bool any = false;
      for (int i = 0; i < 3; i++) {
        if (hh[i].length() == 0) continue;
        if (any) p->print(F("<br>"));
        p->print(hh[i]);
        any = true;
      }
      if (!any) p->print(F("(nog geen)"));
      else p->print(F(" &nbsp; <a href=\"/clear_crash_log\" style=\"color:#369;\" onclick=\"return confirm('Lijst en meldingen wissen?');\">Wissen</a>"));
      p->print(F("</td></tr>"));
    }
    p->print(F("<tr><td colspan=\"2\" style=\"background:#336699;color:#fff;font-weight:bold;padding:6px 8px;\">Instellingen</td></tr>"));
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
      i, i, pixel_nicknames[i]);
  }
  p->print(F("</td></tr>"));

  p->printf("<tr><td class=\"lbl\">Optionele sensoren</td><td class=\"inp\">"
    "<label><input type=\"checkbox\" name=\"co2\"%s> SCD41 (CO&#8322;)</label> "
    "<label><input type=\"checkbox\" name=\"dust\"%s> Stof</label> "
    "<label><input type=\"checkbox\" name=\"mov2\"%s> MOV2</label> "
    "<label><input type=\"checkbox\" name=\"tstat\"%s> Thermostaat</label> "
    "<label><input type=\"checkbox\" name=\"beam\"%s> Beam</label>"
    "</td></tr>",
    co2_enabled ? " checked" : "",
    dust_enabled ? " checked" : "",
    mov2_enabled ? " checked" : "",
    tstat_enabled ? " checked" : "",
    beam_enabled ? " checked" : "");
  // v3.4: SCD41-instellingen + kalibratie in buitenlucht
  p->printf("<tr><td class=\"lbl\">SCD41</td><td class=\"inp\">"
    "<label><input type=\"checkbox\" name=\"scd_asc\"%s> Automatische kalibratie (ASC)</label>"
    "<span style=\"font-size:12px;color:#666;\"> &mdash; aan in kamers die gelucht worden, uit in afgesloten ruimtes</span><br>"
    "Temperatuuroffset: <input type=\"number\" name=\"scd_toff\" step=\"0.1\" min=\"0\" max=\"20\" value=\"%.1f\" style=\"width:70px;\"> &deg;C"
    "<span style=\"font-size:12px;color:#666;\"> (standaard 4,0)</span><br>"
    "<a href=\"/scd_frc\" onclick=\"return confirm('Staat de sensor al minstens 3 minuten in buitenlucht? Hij wordt dan op 425 ppm gezet.');\" "
    "style=\"display:inline-block;margin-top:6px;background:#369;color:#fff;padding:6px 14px;border-radius:5px;text-decoration:none;font-size:13px;\">Kalibreren in buitenlucht (425 ppm)</a>"
    "</td></tr>",
    scd_asc ? " checked" : "", scd_toff);
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
      i, ds_nicknames[i]);
  }

  p->print(F("<div style=\"margin:8px 0;\"><label><b>Primaire sensor: </b><select name=\"ds_primary\" style=\"padding:4px;\">"));
  for (int i = 0; i < ds_count; i++) {
    p->printf("<option value='%d'%s>%s", i, i == ds_primary ? " selected" : "", ds_nicknames[i]);
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
    crumbW(CW_SAVE);  // v3.3
  // v2.10: geen String= toewijzing aan globals meer — char[] direct

  // Basisinstellingen
  preferences.putString(NVS_ROOM_ID,   request->hasArg("room_id")   ? request->arg("room_id").c_str()   : room_id);
  preferences.putString(NVS_WIFI_SSID, request->hasArg("wifi_ssid") ? request->arg("wifi_ssid").c_str() : wifi_ssid);
  preferences.putString(NVS_WIFI_PASS, request->hasArg("wifi_pass") ? request->arg("wifi_pass").c_str() : wifi_pass);
  preferences.putString(NVS_STATIC_IP, request->hasArg("static_ip") ? request->arg("static_ip").c_str() : "");

  preferences.putInt(NVS_HEATING_SETPOINT, request->hasArg("heat_sp")        ? request->arg("heat_sp").toInt()        : 20);
  preferences.putInt(NVS_VENT_REQUEST,     request->hasArg("vent_req")        ? request->arg("vent_req").toInt()       : 25);  // v2.21: default 25%
  preferences.putFloat(NVS_DEW_MARGIN,     request->hasArg("dew_margin")      ? request->arg("dew_margin").toFloat()   : 2.0);
  preferences.putInt(NVS_HOME_MODE,        request->hasArg("home_mode")       ? request->arg("home_mode").toInt()      : 0);
  preferences.putInt(NVS_LDR_DARK,        request->hasArg("ldr_dark")        ? request->arg("ldr_dark").toInt()       : 50);
  preferences.putInt(NVS_BEAM_THRESHOLD,  request->hasArg("beam_thresh")      ? request->arg("beam_thresh").toInt()    : 50);

  // Checkboxes
  preferences.putBool(NVS_CO2_ENABLED,    request->hasArg("co2"));
  preferences.putBool(NVS_SCD_ASC,        request->hasArg("scd_asc"));   // v3.4
  preferences.putFloat(NVS_SCD_TOFF,      request->hasArg("scd_toff") ? constrain(request->arg("scd_toff").toFloat(), 0.0f, 20.0f) : 4.0f);  // v3.4
  preferences.putBool(NVS_DUST_ENABLED,   request->hasArg("dust"));
  preferences.putBool(NVS_MOV2_ENABLED,   request->hasArg("mov2"));
  preferences.putBool(NVS_TSTAT_ENABLED,  request->hasArg("tstat"));
  preferences.putBool(NVS_BEAM_ENABLED,   request->hasArg("beam"));
  preferences.putBool(NVS_SERIAL_VERBOSE, request->hasArg("serial_verbose"));
  int new_interval = request->hasArg("serial_interval") ? constrain(request->arg("serial_interval").toInt(), 5, 30) : 15;
  preferences.putInt(NVS_SERIAL_INTERVAL, new_interval);
  serial_interval = new_interval;

  // NeoPixels
  int new_pixels = constrain(request->hasArg("pixels") ? request->arg("pixels").toInt() : 8, 1, 30);
  int old_pixels = pixels_num;
  preferences.putInt(NVS_PIXELS_NUM, new_pixels);
  if (new_pixels > old_pixels) {
    for (int i = old_pixels; i < new_pixels; i++) {
      char pbkey[24]; snprintf(pbkey, sizeof(pbkey), "%s%d", NVS_PIXEL_ON_BASE, i);
      preferences.putBool(pbkey, false);
    }
  }
  preferences.putUChar(NVS_NEO_R, request->hasArg("neo_r") ? request->arg("neo_r").toInt() : 255);
  preferences.putUChar(NVS_NEO_G, request->hasArg("neo_g") ? request->arg("neo_g").toInt() : 255);
  preferences.putUChar(NVS_NEO_B, request->hasArg("neo_b") ? request->arg("neo_b").toInt() : 255);

  // Pixel nicknames — tijdelijke String voor trim(), dan strlcpy naar char[]
  for (int i = 0; i < 30; i++) {
    char argName[20]; snprintf(argName, sizeof(argName), "pixel_nick_%d", i);
    if (request->hasArg(argName)) {
      String nick = request->arg(argName);
      nick.trim();
      char nickkey[24]; snprintf(nickkey, sizeof(nickkey), "%s%d", NVS_PIXEL_NICK_BASE, i);
      if (nick.isEmpty()) {
        char defnick[48]; snprintf(defnick, sizeof(defnick), "%s Pixel %d", room_id, i);
        preferences.putString(nickkey, defnick);
        if (i < pixels_num) strlcpy(pixel_nicknames[i], defnick, sizeof(pixel_nicknames[i]));
      } else {
        preferences.putString(nickkey, nick.c_str());
        if (i < pixels_num) strlcpy(pixel_nicknames[i], nick.c_str(), sizeof(pixel_nicknames[i]));
      }
    }
  }

  // DS18B20 nicknames
  for (int i = 0; i < DS_MAX_SENSORS; i++) {
    char argName[16]; snprintf(argName, sizeof(argName), "ds_nick_%d", i);
    if (request->hasArg(argName)) {
      String nick = request->arg(argName);
      nick.trim();
      if (!nick.isEmpty()) {
        preferences.putString(argName, nick.c_str());
        strlcpy(ds_nicknames[i], nick.c_str(), sizeof(ds_nicknames[i]));
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
    crumbW(CW_OTHER);  // v3.3
    request->send(200, "text/html", "<h2 style='color:#f00'>Factory reset uitgevoerd!<br>Rebooting...</h2>");
    preferences.clear();
    delay(1000);
    ESP.restart();
  });

  // v2.4 FIX 7: Crash-log wissen via webUI
  server.on("/clear_crash_log", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);  // v3.3
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", false);
    crashPrefs.putUInt("count", 0);
    crashPrefs.putString("reason", "geen");
    crashPrefs.remove("h1"); crashPrefs.remove("h2"); crashPrefs.remove("h3");  // v3.3
    crashPrefs.end();
    Serial.println("[CRASH-LOG] Gewist via webUI");
    request->redirect("/settings");
  });

  // === RESCAN DS18B20 BUS (v1.3) ===
  // v2.4 FIX 5: scanDS18B20() + readDS18B20temps() bevatten delay(750) — NOOIT aanroepen
  // vanuit een AsyncWebServer handler (blokkeert AsyncTCP-taak 750ms → crashes bij gelijktijdige requests).
  // Oplossing: vlag zetten, loop() voert de scan uit, response geeft een wacht-pagina die daarna redirect.
  server.on("/rescan_ds", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);  // v3.3
    rescan_ds_requested = true;
    request->send(200, "text/html; charset=utf-8",
      "<h2 style='text-align:center;padding:30px;color:#336699;'>"
      "🔍 Rescan gestart...<br><br>"
      "<a href='/settings' style='color:#336699;'>← Terug naar Settings</a>"
      "</h2>"
      "<script>setTimeout(()=>location.href='/settings',2500);</script>");
  });

  // v3.4: SCD41 kalibreren in buitenlucht (FRC 425 ppm) — uitgevoerd in loop(), niet hier
  server.on("/scd_frc", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);
    const char* msg;
    if (!co2_enabled)        msg = "SCD41 staat uit in de instellingen.";
    else if (!scd_ok)        msg = "SCD41 antwoordt niet: kalibratie niet mogelijk.";
    else if (millis() - scd_started_ms < 180000UL) msg = "Nog geen 3 minuten aan het meten: wacht even en probeer opnieuw.";
    else { scd_frc_requested = true; msg = "Kalibratie gestart. Resultaat over enkele seconden in Diagnose."; }
    char page[400];
    snprintf(page, sizeof(page),
      "<h2 style='text-align:center;padding:30px;color:#336699;'>%s<br><br>"
      "<a href='/settings' style='color:#336699;'>&larr; Terug naar Settings</a></h2>"
      "<script>setTimeout(()=>location.href='/settings',4000);</script>", msg);
    request->send(200, "text/html; charset=utf-8", page);
  });



  // === SET COLOR HANDLER ===
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_COLOR);  // v3.3
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
    crumbW(CW_OTHER);  // v3.3
  if (request->hasParam("duration")) {
    fade_duration = request->getParam("duration")->value().toInt();
    fade_duration = constrain(fade_duration, 1, 10);
    preferences.putInt(NVS_FADE_DURATION, fade_duration);  // direct opslaan
    updateFadeInterval();
  }
  request->send(200, "text/plain", "OK");
  });

  // v2.16: Licht-aan tijd slider (0-30 min + 5s overtime)
  server.on("/set_light_on_min", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);  // v3.3
    if (request->hasParam("mins")) {
      light_on_min = request->getParam("mins")->value().toInt();
      light_on_min = constrain(light_on_min, 0, 30);
      preferences.putInt(NVS_LIGHT_ON_MIN, light_on_min);
    }
    request->send(200, "text/plain", "OK");
  });


  // Slider Heating setpoint (wijzig alleen setpoint, geen mode-switch)
  server.on("/set_setpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_SETPOINT);  // v3.3
  if (request->hasParam("setpoint")) {
    heating_setpoint = request->getParam("setpoint")->value().toInt();
    heating_setpoint = constrain(heating_setpoint, 10, 30);
    preferences.putInt(NVS_CURRENT_SETPOINT, heating_setpoint);  // direct opslaan
  }
  request->send(200, "text/plain", "OK");
  });


  // Slider Ventilation %
  server.on("/set_vent", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_OTHER);  // v3.3
    if (request->hasParam("vent")) {
      vent_percent = request->getParam("vent")->value().toInt();
      vent_percent = constrain(vent_percent, 0, 100);
    }
    request->send(200, "text/plain", "OK");
  });


  // Toggle Thuis/Uit
  server.on("/toggle_home", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_HOME);  // v3.3
    home_mode = 1 - home_mode; // Toggle tussen 0 en 1
    preferences.putInt(NVS_HOME_MODE_STATE, home_mode);  // direct opslaan
    request->send(200, "text/plain", "OK");
  });

  // v2.11: Direct set Thuis/Uit via dashboard broadcast (/set_home?v=0 of ?v=1)
  // Zelfde NVS-key als toggle_home — beide endpoints zijn volledig compatibel
  server.on("/set_home", HTTP_GET, [](AsyncWebServerRequest *request) {
    crumbW(CW_HOME);  // v3.3
    if (request->hasParam("v")) {
      home_mode = constrain(request->getParam("v")->value().toInt(), 0, 1);
      preferences.putInt(NVS_HOME_MODE_STATE, home_mode);
    }
    request->send(200, "text/plain", "OK");
  });

  Serial.println("Commando: typ 'reset_nvs' in serial monitor voor factory reset");

  // Afsluiting webserver (alle handlers hiervoor!)






  server.begin();
  Serial.printf("HTTP server gestart op http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("\n=== Setup klaar ===\n");
  Serial.printf("Free heap     : %d%% (%d bytes)\n", (ESP.getFreeHeap() * 100) / ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("Largest block : %d KB\n", ESP.getMaxAllocHeap() / 1024);
}

unsigned long lastSerial = 0;
unsigned long last_slow = 0;








void loop() {
  crumb_up_s = millis() / 1000;   // v3.3

  // WiFi status bewaken — herverbinden bij verlies
  crumbL(CL_WIFI);
  wl_status_t current_status = WiFi.status();
  if (current_status != last_wifi_status) {
    if (current_status != WL_CONNECTED && !ap_mode_active) {
      Serial.println("[WiFi] Verbinding verloren — herverbinden...");
      WiFi.reconnect();
    }
    last_wifi_status = current_status;
  }




  if (ap_mode_active) {
    dnsServer.processNextRequest();
  }

  crumbL(CL_SERIAL);
  handleSerialCommands();

  crumbL(CL_FADE);
  updateFades();

  crumbL(CL_PIR);

  // PIR op 3.3V: beweging = LOW → rising edge van HIGH naar LOW
  static bool last1 = HIGH, last2 = HIGH;
  bool p1 = digitalRead(PIR_MOV1);
  bool p2 = digitalRead(PIR_MOV2);

  if (!p1 && last1) { 
    mov1_off_time = millis() + lightOnDuration();
    pushEvent(mov1Times, MOV_BUF_SIZE);
    mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);  // v2.18: direct update, niet wachten op 60s
  }
  if (!p2 && last2) { 
    mov2_off_time = millis() + lightOnDuration();
    pushEvent(mov2Times, MOV_BUF_SIZE);
    mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);  // v2.18: direct update
  }

  last1 = p1; last2 = p2;





  // NeoPixel aansturing — gesaneerd & deterministisch
  crumbL(CL_PIXLOGIC);

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

        setTargetColor(0, on ? neo_r : 0, on ? neo_g : 0, on ? neo_b : 0);  // v2.12: was hardcoded groen
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

        setTargetColor(1, on ? neo_r : 0, on ? neo_g : 0, on ? neo_b : 0);  // v2.12: was hardcoded groen
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
  // v2.21: altijd AUTO — setpoint + dauwpuntbeveiliging (heating_mode verwijderd)
  crumbL(CL_HEAT);
  tstat_on = !digitalRead(TSTAT_PIN);
  float effective_setpoint = max((float)heating_setpoint, dew + dew_safety_margin);
  if (home_mode == 1 && tstat_enabled) {
    heating_on = tstat_on;              // Thuis + hardware TSTAT → volg hardware
  } else {
    heating_on = (room_temp < (effective_setpoint - 0.5f)) ? 1 : 0;  // Anti-condens altijd actief
  }
  if (ap_mode_active) heating_on = 0;   // v3.4: in AP-mode geen metingen → ook geen verwarmingsvraag

  // v2.4 FIX 5: Rescan DS18B20 vanuit loop() (async-safe — delay(750) is hier WDT-safe)
  if (rescan_ds_requested) {
    crumbL(CL_DSSCAN);
    rescan_ds_requested = false;
    scanDS18B20();
    readDS18B20temps();
    Serial.printf("[RESCAN] DS18B20: %d sensor(s) gevonden\n", ds_count);
  }

  // v3.4: SCD41 — elke 5 s kijken of er een nieuwe meting is; kalibratie op vraag
  if (co2_enabled) {
    crumbL(CL_CO2);
    if (scd_frc_requested) {
      scd_frc_requested = false;
      scdForcedRecalibration();
    }
    scdPoll();
  }

  crumbL(CL_IDLE);
  if (millis() - last_slow < 60000) return;
  last_slow = millis();
  uptime_sec = millis() / 1000;

  // v2.20 FIX 1: Heap-bewaking — schrijf naar NVS als largest block < 25 KB
  // Max 1x per low-heap episode — voorkomt feedback loop (elke begin()/end() alloceert NVS-buffer)
  {
    uint32_t lb = ESP.getMaxAllocHeap();
    static bool crash_logged_this_episode = false;
    if (lb < 25000 && !crash_logged_this_episode) {
      crumbL(CL_NVS);
      crash_logged_this_episode = true;
      Preferences crashPrefs;
      crashPrefs.begin("crash-log", false);
      uint32_t cnt = crashPrefs.getUInt("count", 0) + 1;
      crashPrefs.putUInt("count", cnt);
      char reason[48];
      snprintf(reason, sizeof(reason), "heap %uKB @ %lus", lb/1024, (unsigned long)uptime_sec);
      crashPrefs.putString("reason", reason);
      crashPrefs.end();
      Serial.printf("[HEAP] ⚠️  Largest block %u KB — crash-log geschreven (#%u)\n", lb / 1024, cnt);
    } else if (lb >= 25000) {
      crash_logged_this_episode = false;  // reset voor volgende episode
    }
  }



  // v3.4: geen sensormetingen in AP-mode (eerste installatie: /settings moet vlot reageren)
  if (ap_mode_active) return;

  // Sensoren
  crumbL(CL_DHT);
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  crumbL(CL_DS);
  readDS18B20temps();  // Lees alle DS18B20 sensoren → temp_ds = primaire sensor
  
  crumbL(CL_LDR);
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));
  crumbL(CL_DUST);
  if (dust_enabled) dust = readDust();  // v1.9: bewaakt — delayMicroseconds blokkeert WDT als sensor niet aanwezig
  // v3.4: CO2 komt van de SCD41 (scdPoll hierboven, elke 5 s)
  dew_alert = (temp_ds < dew) ? 1 : 0;  
  night = (light_ldr > 50) ? 1 : 0;  
  // bed blijft voorlopig hardcoded 0, tot toggle in webserver  
  beam_value = map(analogRead(OPTION_LDR), 0, 4095, 0, 100);  // Schaal ruwe ADC naar 0-100 voor o  
  beam_alert_new = (beam_value > 50) ? 1 : 0;  // Voor p, vervangt oude beam_alert


  crumbL(CL_REST);
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


  // Ventilation logica — v2.21: CO2-gestuurd als sensor leest, anders slider-waarde bewaren
  if (co2_enabled && co2 > 0) {
    vent_percent = map(constrain(co2, 400, 800), 400, 800, 0, 100);
  }
  // co2 = 0 of uitgeschakeld → vent_percent blijft op slider-waarde



  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
  mov2_triggers = countRecent(mov2Times, MOV_BUF_SIZE);




  // Serial rapport (elke 15s, alleen als serial_verbose aan)
    if (serial_verbose && !ap_mode_active && millis() - lastSerial > (unsigned long)(serial_interval * 1000)) {
    lastSerial = millis();

    // v2.7: Serial rapport zonder String-allocaties (upper_room, divider, concatenaties → char[]/printf)
    char upper_room[32];
    snprintf(upper_room, sizeof(upper_room), "%s", room_id);
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
      Serial.printf("CO₂ (SCD41)          : %d ppm%s\n", co2, scd_ok ? "" : "  (sensor ontbreekt)");
      Serial.printf("SCD41 temp / vocht   : %.1f °C / %d %%\n", scd_temp, scd_humi);
    }
    Serial.printf("Ventilatie snelheid %%         : %d %%\n", vent_percent);
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
    Serial.println("─────────────────────────────────────\n");
  }

}

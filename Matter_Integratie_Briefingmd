Lees dit eerst: Dit is een document dat in verschillende stappen is gegroeid: 

Deel 1: Achtergrond voor de eerste matter integratie.

Dit bevat instructies en info voor we de eerste matter integratie deden voor de drie sketches. Het resultaat was dat de matter versie van mijn eenvoudigste sketch (ECO-boiler) sketch vrij goed draait en stabiel genoeg is, zowel in de UI als op apple home.
De andere twee daarentegen (TESTROOM en HVAC) werken beide vrij goed zowel in de UI als in apple home, maar de heap is te klein. Dat resulteert in weinig betrouwbare werking van de UI. Ook werden door problemen met de Claude tools enkele essentiele stukken functionaliteit beschadigd of weggelaten. Dit is een ramp! 

Deel 2:  Instructies voor de tweede grondigere poging tot matter integratie met maximale heap. (= vanaf "HANDOVER DOCUMENT — Matter Integratie to do!"

Omwille van bovenstaande problemen werd besloten om een stap terug te nemen en opnieuw te beginnen vanaf de basisversies zonder matter, en te proberen van die sketches eerst opnieuw te testen en te verbeteren, vooral op gebied van free heap.

Ik geef voor een nieuw gesprek aan Claude dus telkens 3 documenten mee:

1) Dit complete briefing document (met de 2 delen)
2) De huidige versie van sketch zonder matter, als vertrekpunt om stap voor stap matter toe te voegen en de heap evolutie te evalueren.
3) De vorige matter sketch om te leren hoe we dit deden, niet klakkeloos over te nemen.

----------------

# Matter Integratie — Briefing voor nieuw gesprek - 05mar26

## Context
Filip Delannoy, thuisautomatisering "Zarlar" in Zarlardinge (BE).
Drie ESP32-C6 controllers (16MB flash), één webserver per controller (AsyncWebServer),
vaste IP adressen op 192.168.0.x subnet.
Ervaring: Arduino IDE, HomeSpan (eerder gebruikt), Homebridge (vervangen).
Doel: Matter integratie voor Apple HomeKit + Google Home + Amazon Alexa.

---

## Hardware
| Controller | IP | MAC | Functie |
|---|---|---|---|
| ESP32_ECO-boiler | 192.168.0.71 | 58:8C:81:32:2B:D4 | Zonneboiler |
| ESP32C6_HVAC | 192.168.0.70 | 58:8C:81:32:29:54 | Verwarming/ventilatie |
| ESP32_ROOM | 192.168.0.80 | 58:8C:81:5D:B0:88 | Kamer multi-sensor |

Alle drie: ESP32-C6, 16MB flash, Arduino IDE, huge_app partition scheme.

opm: Op dit ogenblik is ESP32_ROOM nog een oudere ESP32-WROOM-32 processor, maar we maakten net een nieuwe sketch voor een C6 processor. Deze moet in de eerstkomende dagen nog getest worden.

---

## Gekozen architectuur: Thread voor Matter + WiFi voor webUI

- **Matter** loopt via **Thread radio** (ingebouwd in C6)
- **Webserver** blijft op **WiFi** (fixed IP, AsyncWebServer poort 80)
- Geen resource conflict tussen beide
- Vereist een **Thread border router**: HomePod Mini of Apple TV 4K (2021+)
- ESP32-C6 ondersteunt WiFi + Thread simultaan

---

## Wat in HomeKit per controller (subset — details blijven in webUI)

### ECO Boiler
**Zien:**
- Boiler temperatuur (gemiddelde van 6 sensoren)
- Collector temperatuur (Tsun)
- Pomp status (aan/uit + %)
- Energie vandaag (yield_today kWh)

**Bedienen:**
- Pomp override aan/uit

**Matter accessory type:** Temperature Sensor + Switch

---

### HVAC
**Zien:**
- Aanvoer temperatuur
- Retour temperatuur
- Verwarmingsstatus per circuit (3 circuits)
- Buitentemperatuur

**Bedienen:**
- Verwarming aan/uit per circuit

**Matter accessory type:** Thermostat + Switch (per circuit)

---

### ROOM (meest complex)
**Zien:**
- Kamertemperatuur (primaire DS18B20 sensor)
- Alle DS18B20 sensoren (1-4, dynamisch ontdekt)
- Luchtvochtigheid (DHT22)
- CO₂ (indien ingeschakeld)
- Beweging MOV1 / MOV2
- Daglicht (LDR)

**Bedienen:**
- NeoPixels aan/uit per pixel (4-8 stuks, elk als lamp)
- Verwarming setpoint
- Bed modus aan/uit

**Matter accessory types:**
- Temperature Sensor (per DS18B20)
- Humidity Sensor
- Air Quality Sensor (CO₂)
- Occupancy Sensor (PIR)
- Light Sensor (LDR)
- Dimmable Light (per NeoPixel)
- Thermostat
- Switch (Bed)

---

## Aanpak: stap-voor-stap

### Fase 1 — Experiment (blank sketch)
- Nieuwe lege sketch op spare C6 (4MB is voldoende voor test)
- Matter library installeren
- Één temperatuursensor + Thread
- Pairen met Apple Home, Google Home testen
- Resource monitoring (heap, flash)

### Fase 2 — Evaluatie
- 24u stabiliteitstest
- Heap meten: moet >30% vrij blijven
- Pairing proces documenteren

### Fase 3 — Integratie per sketch
- Beginnen met ROOM (meest gebruik, meest sensoren)
- Dan HVAC
- Dan ECO

---

## Technische keuzes

- **Library:** Espressif Arduino Matter library (Board Manager esp32 3.0+)
- **Partition:** huge_app (3MB) — voldoende voor Matter + webserver op 16MB
- **Thread border router:** HomePod Mini of Apple TV 4K aanwezig?
  *(Filip: bevestig dit voor start Fase 1)*
- **#define Serial Serial0** — vaste fix voor alle C6 sketches

---

## Bestaande sketch features die BEWAARD blijven
- Volledige webUI (/, /settings, /json, /update)
- AsyncWebServer op poort 80
- Alle sensorlogica
- NVS opslag
- OTA updates
- mDNS (.local namen)
- AP fallback bij WiFi fout


-----------------------------------------------------------------

# Integratieplan: TESTROOM → Matter-enabled
## ESP32_C6_ROOM_MATTER.ino
**Filip Delannoy – Zarlar thuisautomatisering**
**28 feb 2026**

---

## 0. HARDWARE VEREISTE — KRITISCH PUNT

**Gebruik uitsluitend de 16MB ESP32-C6 voor dit project.**

De 4MB versie is niet geschikt voor TESTROOM + Matter samen.

### Waarom 16MB de sleuteloplossing is

Met 16MB flash kunnen we een **custom partitietabel** maken die twee problemen tegelijk oplost:

| Probleem | 4MB Huge App | 16MB custom |
|---|---|---|
| Voldoende ruimte voor Matter + TESTROOM | ❌ krap/onmogelijk | ✅ ruim |
| OTA firmware update | ❌ niet mogelijk* | ✅ hersteld |

*Huge App heeft slechts één app-partitie → OTA heeft twee nodig. **OTA is momenteel al kapot in TESTROOM** zodra je Huge App gebruikt — dit is waarschijnlijk nog niet opgemerkt.

### Custom partitietabel voor 16MB (bestand: `partitions_16mb.csv`)
```
# Name,   Type, SubType, Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x600000,
app1,     app,  ota_1,   0x610000, 0x600000,
spiffs,   data, spiffs,  0xC10000, 0x3F0000,
```
- app0 + app1: elk **6MB** → ruim voldoende voor Matter + volledige TESTROOM
- OTA volledig hersteld
- SPIFFS: ~4MB voor eventuele toekomstige webpagina's/bestanden

Dit CSV-bestand wordt geplaatst in de schetsmap, Arduino IDE pikt het automatisch op.

---

## 1. CONFLICT: mDNS

**Probleem:** TESTROOM start `ESPmDNS` (→ `eetplaats.local`). Matter start zijn eigen interne mDNS-stack. Twee mDNS-instanties op één chip conflicteren — dit is de oorzaak van de `mdns_service_remove_for_host` errors die al zichtbaar waren.

**Oplossing:** TESTROOM's `MDNS.begin()` en `MDNS.end()` calls volledig verwijderen. Matter's interne mDNS neemt het over. De `.local` hostname zal niet meer werken via deze route — toegang verloopt via het **statisch IP-adres** (dat is al ingesteld) of via de Matter-app.

**Alternatief** als `.local` absoluut nodig blijft: Matter en ESPmDNS kunnen naast elkaar bestaan als ESPmDNS pas gestart wordt *nadat* Matter volledig geïnitialiseerd is. Dit is fragiel en wordt **niet aanbevolen**.

---

## 2. CONFLICT: NVS / Factory Reset

**Probleem:**
- TESTROOM gebruikt `preferences.begin("room-config")` → namespace `room-config`
- Matter gebruikt intern zijn eigen namespaces: `chip-factory`, `chip-config`, `chip-counters`
- `preferences.clear()` in TESTROOM wist **alleen** `room-config` → Matter data blijft intact ✅
- `nvs_flash_erase()` wist **alles** inclusief `room-config` → TESTROOM verliest alle instellingen ❌

**Oplossing — twee aparte reset commando's:**

```
Serial commando 'reset-matter'  → wist alleen Matter namespaces + herstart
Serial commando 'reset-all'     → wist alles (Matter + TESTROOM config) + herstart
Web UI /factory_reset           → behoudt huidige gedrag (wist TESTROOM config, laat Matter intact)
```

Matter-namespaces selectief wissen (zonder `nvs_flash_erase()`):
```cpp
preferences.begin("chip-factory", false); preferences.clear(); preferences.end();
preferences.begin("chip-config",  false); preferences.clear(); preferences.end();
preferences.begin("chip-counters",false); preferences.clear(); preferences.end();
```

---

## 3. COMMISSIONING — NON-BLOCKING

**Probleem:** De simulatiesketch heeft een `while (!Matter.isDeviceCommissioned())` blokkeerlus. In TESTROOM moet de webserver onmiddellijk draaien — ook als Matter nog niet gepaard is.

**Oplossing:** Geen blokkeerlus. Na `Matter.begin()` gewoon verder. Matter draait in zijn eigen FreeRTOS-taak en pairt op de achtergrond. De endpoints zijn beschikbaar zodra gepaard.

**Pairing code zichtbaar maken zonder serial monitor:**
Nieuwe webpagina `/matter` toevoegen aan de bestaande webserver:
- Toont pairingcode als nog niet gepaard
- Toont "Gepaard ✅" als al gepaard
- Knop "Matter reset" (wist alleen Matter namespaces)

---

## 4. AP MODE + MATTER

**Situatie:** Als WiFi mislukt gaat TESTROOM naar AP-modus (192.168.4.1 captive portal). Matter heeft een actieve WiFi STA-verbinding nodig en kan in AP-modus niet werken.

**Oplossing:** Matter endpoints en `Matter.begin()` worden **enkel gestart als WiFi STA verbonden is**. In AP-modus: Matter volledig overgeslagen, webserver werkt normaal voor configuratie.

```cpp
if (!ap_mode_active) {
  // Matter initialiseren
}
```

---

## 5. SETUP VOLGORDE — KRITISCH

De volgorde in `setup()` moet exact zo zijn:

```
1. Serial.begin()
2. preferences.begin("room-config") + laden van alle NVS-waarden
3. WiFi.begin() met geladen credentials
4. Wacht op WiFi (met AP-mode fallback zoals nu)
5. ALS WiFi verbonden (niet AP-mode):
   a. Matter endpoints .begin() + callbacks instellen
   b. Matter.begin()
   c. Print pairingcode naar Serial (niet-blokkerend)

BELANGRIJK!
Voeg optie toe in /settings: "Matter transport: WiFi / Thread" => Opgeslagen in NVS
-Bij Thread: WiFi-verbinding blijft voor webserver, Matter gebruikt Thread radio
-Bij WiFi: huidige implementatie

6. GEEN MDNS.begin() meer
7. Webserver routes instellen (inclusief nieuwe /matter pagina)
8. Sensor initialisatie (DHT, DS18B20, TSL2561, NeoPixel)
```

---

## 6. CALLBACKS → BESTAANDE LOGICA BEWAREN

Alle callbacks zijn al ontworpen in de simulatiesketch met de juiste variabelenamen. Bij integratie uitbreiden met NVS-persistentie:

| Callback | Variabele | NVS sleutel |
|---|---|---|
| onChangeHeatingSetpoint | heating_setpoint | NVS_HEATING_SETPOINT |
| onChangeMode | (geen persistentie nodig) | — |
| onChangeColorHSV | neo_r, neo_g, neo_b | NVS_NEO_R/G/B |
| onChangeOnOff (bed) | bed | NVS_BED_STATE |
| onChangeOnOff (thuis) | home_mode | NVS_HOME_MODE_STATE |
| onChangeOnOff (pir1/2) | pixel_mode[0/1] | NVS_PIXEL_MODE_0/1 |

**Belangrijk:** de thermostat callback overschrijft `heating_setpoint` — maar de echte verwarmingslogica in TESTROOM gebruikt ook `tstat_on`, `heating_mode` en `dew + dew_safety_margin`. Die logica blijft ongewijzigd in de slow-loop. Matter stuurt enkel het setpoint bij.

---

## 7. SENSOR UPDATES INTEGREREN

TESTROOM heeft al een `last_slow` timer (elke 2s). `update_matter_sensors()` wordt daar aan toegevoegd — geen aparte timer nodig:

```cpp
// In de bestaande slow-loop, na het lezen van sensoren:
if (!ap_mode_active) {
  update_matter_sensors();
}
```

---

## 8. THREAD SAFETY

Matter callbacks draaien in een eigen FreeRTOS-taak, niet in de Arduino loop-taak. Gedeelde variabelen zoals `neo_r`, `heating_setpoint`, `home_mode` worden vanuit de callback geschreven en vanuit de loop gelezen.

Voor enkelvoudige integer/bool types op ESP32 is dit in de praktijk veilig (atomaire 32-bit lees/schrijfoperaties). Complexe bewerkingen of string-operaties vanuit callbacks worden vermeden. Geen `mutex` nodig voor dit gebruik.

---

## 9. SERIAL OUTPUT — ADVIES

**Jouw vraag:** is de uitgebreide serial tabel overtollig als we alles in de UI kunnen monitoren?

**Antwoord:** De flash-kostprijs van de serial strings is minimaal (~5-10KB). RAM-impact via `F()` macro: nul. Het is **geen significante geheugenbesparing** om de tabel te verwijderen.

**Aanbeveling:** Vervang de huidige TESTROOM serial output (elke 3s, ~30 printf regels zonder consistente `F()`) door de compacte tabel uit de simulatiesketch (elke 15s, consistent `F()`, overzichtelijker). Dit is hoofdzakelijk een **codekwaliteitsverbetering**, niet een geheugenbesparing.

Voeg toe bovenaan de sketch:
```cpp
#define SERIAL_VERBOSE   // Uitcommentariëren voor productie → geen statustabel
```

De echte geheugenbesparing zit in de **custom partitietabel** (zie punt 0) — dat geeft 6MB app-ruimte, wat alle andere optimalisaties overbodig maakt.

---

## 10. PIXEL KLEUR — MEMO INTEGRATIE

`matter_pixels` is een kleurpicker voor `neo_r/g/b`, GEEN aan/uit schakelaar.
- `onChangeOnOff` wordt genegeerd, switch wordt altijd terug op "aan" gezet
- `onChangeColorHSV` → converteert HSV naar RGB → schrijft naar `neo_r`, `neo_g`, `neo_b`
- De bestaande pixelloop in TESTROOM gebruikt `neo_r/g/b` automatisch voor alle `pixel_on[i]==true` pixels
- Geen wijziging nodig aan de pixelloop zelf

---

## 11. OVERZICHT: WAT WIJZIGT, WAT BLIJFT

| Component | Actie |
|---|---|
| Partitietabel | Nieuw: `partitions_16mb.csv` in schetsmap |
| Arduino IDE board settings | Huge App vervangen door Custom, CSV selecteren |
| WiFi | Ongewijzigd |
| ESPmDNS | **Verwijderd** (Matter vervangt) |
| Webserver + alle routes | Ongewijzigd + nieuwe `/matter` pagina |
| OTA via webserver | Hersteld dankzij custom partitietabel |
| Sensor logica slow-loop | Ongewijzigd + `update_matter_sensors()` call |
| Verwarmingslogica | Ongewijzigd, Matter stuurt enkel setpoint bij |
| Pixelloop | Ongewijzigd |
| Factory reset `/factory_reset` | Ongewijzigd (wist TESTROOM config) |
| Serial commando `reset_nvs` | Hernoemd naar `reset-all` + nieuw `reset-matter` |
| NVS namespace "room-config" | Ongewijzigd |
| Serial output | Vervangen door compacte tabel + `#define SERIAL_VERBOSE` |

---

## 12. TESTSTRATEGIE

1. Maak kopie van TESTROOM → `ESP32_C6_ROOM_MATTER.ino`
2. Maak `partitions_16mb.csv` aan in schetsmap
3. Integreer Matter stap voor stap (niet alles tegelijk)
4. Test eerst alleen de sensor-endpoints (read-only, geen callbacks)
5. Dan de bedieningselementen (callbacks + NVS persistentie)
6. Valideer dat webserver + Matter gelijktijdig werken
7. Test factory reset scenario's
8. Pas toe op één productie-room, dan uitrollen

------

## Vraag voor start Fase 1
Heb je een Thread border router (HomePod Mini / Apple TV 4K 2021+)?
Antwoord: Jazeker: Beneden een apple tv 4k en een homepod mini, boven idem.

---------

# Resultaat = 2 bestanden: ESP32_C6_ROOM_MATTER.ino & partitions_16mb.csv

Wat gewijzigd is t.o.v. TESTROOM v1.3:

#include <ESPmDNS.h> en alle MDNS.begin()/end() code volledig verwijderd
Matter includes en 12 endpoints toegevoegd
Matter init bloc in setup() — volledig non-blocking, wrapped in if (!ap_mode_active)
Alle 6 callbacks met NVS persistentie (setpoint, kleur, bed, home_mode, pixel_mode[0/1])
Web UI handlers (/toggle_home, /toggle_bed, /set_setpoint, pixel toggles) synken nu ook terug naar HomeKit
reset_nvs → reset-all + reset-matter serial commando's
update_matter_sensors() toegevoegd aan de slow loop
serial_verbose en matter_transport als NVS variabelen, beide instelbaar in /settings onder nieuw kopje "Matter & Diagnostiek" — inclusief de border router opmerking bij Thread
Compacte statusrapport print_status_compact() vervangt de lange tabel, elke 15s, bewaakt door serial_verbose
Nieuwe /matter pagina met pairingcode of "gepaard" status + Matter reset knop
"Matter" link toegevoegd aan de sidebar op alle pagina's

Wat ongewijzigd is: pixel loop, verwarmingslogica, sensor logica, alle andere webserver routes, NVS namespace room-config, JSON endpoint, OTA.

------

# TO DO: 

Praktisch stappenplan:

**1. Bestanden plaatsen**
Maak een nieuwe schetsmap aan (bijv. `ESP32_C6_ROOM_MATTER`) en plaats beide bestanden daarin — de `.ino` én de `.csv`. Arduino IDE verwacht dat ze samen in dezelfde map zitten.

**2. Arduino IDE instellen**
- Board: `ESP32C6 Dev Module` (of jouw C6 variant)
- Partition Scheme: **Custom** → selecteer de `partitions_16mb.csv` uit de schetsmap
- Flash Size: **16MB**
- De rest van je board settings (baud, CPU freq) zoals je die al had voor TESTROOM

**3. Eerste keer flashen**
Doe dit via USB, niet via OTA — het is een nieuwe partitietabel, die kan niet over-the-air gezet worden. Na deze eerste keer is OTA wél hersteld voor toekomstige updates.

**4. Na het flashen**
- Open de serial monitor (115200 baud)
- De pairingcode verschijnt in de serial output, of ga naar `http://<IP>/matter`
- Koppel via Apple Home: **+** → **Accessoire** → **Meer opties** → code invoeren

**5. Aanbevolen testvolgorde** (zoals in het integratieplan punt 12)
Eerst enkel de read-only sensoren valideren in HomeKit, daarna de callbacks testen, dan factory reset scenario's doorlopen — vóór je het op een productie-room zet.

Eén aandachtspunt: als je de huidige TESTROOM op dit apparaat al gepaard had met een eerder Matter experiment, typ dan eerst `reset-matter` in de serial monitor vóór je koppelt.

-------------

## HANDOVER DOCUMENT — Matter Integratie to do! (5mar26)

**Bijgewerkt: 5 maart 2026 | Versie 3 — na heap monitoring implementatie**

---

## 1. Controller Status

| Bestand | Status | Versie | Opmerking |
|---|---|---|---|
| ESP32-C6_TESTROOM_v3.1 / v1.6 | ✅ Klaar voor SPIFFS | v1.6 (5mar_1600) | Heap monitoring actief, 232 KB largest block |
| ESP32_HVAC.ino | ⚠️ HTML compressie nodig | v1.6+ | Ventilatie fixes toegepast, white screen slider open |
| ESP32-C6_MATTER_ROOM.ino | 📁 Referentie | 3mar_2200 | Oude Matter poging — voor herbruik callbacks |

---

## 2. Gemeten Heap Baseline — ROOM v1.6

Gemeten op 5 maart 2026, na WiFi stabilisatie, alle sensoren actief:

| Metric | Waarde | Status |
|---|---|---|
| Vrij heap (totaal) | 67% (~339 KB) | ✅ Goed |
| Largest free block | 232 KB | ✅ 🟢 Uitstekend |
| Min ever (na boot) | te meten | |
| Core Debug Level | Default (nog niet None) | ⚠️ Kan 20-30 KB besparen |

---

## 3. Heap Evolutietabel — Integratielog

Na elke stap invullen. **Largest free block is het echte meetcriterium.**

| Stap | Actie | Largest block | Delta | Status |
|---|---|---|---|---|
| Baseline | ROOM v1.6 — alle sensoren, WiFi | 232 KB | — | ✅ 5 mrt 2026 |
| 2a | Core Debug Level = None (IDE) | — KB | — | ⏳ |
| 2b | SPIFFS migratie — alle pagina's | — KB | — | ⏳ |
| 2c | RGB kleurkiezer inline (vervangt /neopixel) | — KB | — | ⏳ |
| 2d | Serial interval instelbaar in /settings | — KB | — | ⏳ |
| 3.1 | Matter.begin() — geen endpoints | — KB | — | ⏳ |
| 3.2 | OnOffPlugin (1x) | — KB | — | ⏳ |
| 3.3 | Thermostat endpoint | — KB | — | ⏳ |
| 3.4 | ColorLight (RGB) | — KB | — | ⏳ |
| 3.5 | Temp + Humidity sensoren | — KB | — | ⏳ |
| 3.6 | CO2 + Lux (fake Temp) | — KB | — | ⏳ |
| 3.7 | Occupancy MOV1 + MOV2 | — KB | — | ⏳ |
| 3.8 | pir1_light + pir2_light OnOffLight | — KB | — | ⏳ |
| 3.9 | Plugins: bed, thuis, pixels_on | — KB | — | ⏳ |
| EINDE | Alle 13 endpoints actief | — KB | — | ⏳ |

> 🔴 **Stop als largest free block < 25 KB** — evalueer SPIFFS of endpoint-schrapping.

---

## 4. To-Do Basisversie (vóór Github + Matter)

Scope: alleen wat géén Matter is. Na afwerking → commit naar Github als veilige basis.

### 4.1 Core Debug Level = None

Instelling in Arduino IDE: **Tools → Core Debug Level → None**  
Besparing: 20-30 KB — grootste gratis winst, kost nul code.

**⚠️ Herinnering in sketch header — voeg toe bovenaan .ino:**

```cpp
// ============================================================
// COMPILATIE-INSTELLINGEN (Arduino IDE → Tools)
// Board:            ESP32-C6 Dev Module
// Partition:        Huge App (3MB no OTA / 1MB SPIFFS)  ← VEREIST
// Core Debug Level: None                                ← HEAP BESPARING
// Na firmware flash: Tools → ESP32 Sketch Data Upload   ← SPIFFS DATA
// ============================================================
```

### 4.2 SPIFFS Migratie — alle pagina's

Bestanden in `/data/` map naast `.ino` plaatsen. Voordeel: **~27 KB heap vrijgemaakt.**

> ⚠️ Na elke firmware flash ook **Tools → ESP32 Sketch Data Upload** uitvoeren!

| Pagina | Route | Huidige heap | Na SPIFFS | Aanpak |
|---|---|---|---|---|
| Hoofdpagina | `/` | ~12 KB | 0 KB | Volledig statisch — data via `/json` |
| NeoPixel picker | `/neopixel` | ~5 KB | 0 KB | Vervalt — kleurkiezer inline op `/` |
| OTA Update | `/update` | ~2 KB | 0 KB | Volledig statisch |
| /settings | `/settings` | ~10 KB | ~2 KB | Statisch skelet in SPIFFS, dynamisch deel via `/json_settings` |
| **Totaal** | | **~29 KB** | **~2 KB** | **~27 KB vrijgemaakt** |

Settings pagina dynamisch deel via `fetch('/json_settings')` bij page load.  
Velden: `room_id`, pixel nicknames, DS18B20 sensor namen, `serial_interval`.

### 4.3 RGB Kleurkiezer inline op statuspagina

Aparte `/neopixel` pagina verdwijnt. Kleurkiezer komt onder de 'Pixel RGB' rij in de tabel.

- Native `<input type="color">` — compact, werkt op iOS/Android
- `/neopixel` route blijft als redirect naar `/` voor bestaande bookmarks
- Winst: ~5 KB heap + één handler minder

### 4.4 Serial Logging Interval instelbaar via /settings

Huidig: hardcoded 15s. Nieuw: instelbaar in /settings UI, opgeslagen in NVS/Preferences.

- Variabele: `serial_interval` (int, seconden)
- Bereik: 5s – 300s, default 15s
- Veld toevoegen aan `/json_settings` response
- Settings pagina: numeriek inputveld met label

---

## 5. Reeds Gedaan in Basisversie

| Item | Versie | Status | Opmerking |
|---|---|---|---|
| HTML compressie (reserve + gzip-like) | v3.1 | ✅ | Heap van ~50% naar 67% |
| TSL2561 init conditioneel op `sun_light_enabled` | v3.1 | ✅ | Geen crash bij uitgeschakelde sensor |
| Serial logging interval 15s | v3.1 | ✅ | Was 3s, gecorrigeerd |
| Heap monitoring op statuspagina | v1.6 | ✅ | Largest block + kleurcode, `/json` uitgebreid |
| I2C pin fix TSL2561 | v1.6 | ✅ | SDA=GPIO6, SCL=GPIO7 correct |
| CO2 sensor 5V power geïdentificeerd | — | 📋 | Hardware fix nodig, niet code |

---

## 6. Matter Integratie — Volgorde (ROOM)

> Pas starten **nadat** basisversie naar Github gepusht is en hernoemd naar `ESP32-C6_ROOM_MATTER_5mar_hhhh.ino`

| Stap | Actie | Type | Heap verwacht |
|---|---|---|---|
| 3.1 | Matter.begin() — nul endpoints, meten | Matter baseline | −130 KB verwacht |
| 3.2 | OnOffPlugin (1x) — meten | Licht | −3-5 KB |
| 3.3 | Thermostat endpoint — meten | Zwaarste! | −25 KB verwacht |
| 3.4 | ColorLight (RGB) endpoint — meten | Zwaar | −15 KB verwacht |
| 3.5 | Temp + Humidity sensoren — meten | Licht | −4-6 KB |
| 3.6 | CO2 + Lux (fake TemperatureSensor) — meten | Conditioneel | −2-3 KB elk |
| 3.7 | Occupancy MOV1 + MOV2 — meten | Licht | −3-4 KB elk |
| 3.8 | pir1_light + pir2_light OnOffLight — meten | Bidirectioneel | −5 KB elk |
| 3.9 | Plugins: bed, thuis, pixels_on — meten | Licht | −3-5 KB elk |

Na elke stap: largest free block invullen in tabel §3. Stop bij < 25 KB.

---

## 7. Matter Endpoints — ROOM (definitief)

| Naam | Type | Conditioneel | Gedrag |
|---|---|---|---|
| matter_temp | TemperatureSensor | Nee | room_temp (DS18B20 primair) |
| matter_humidity | HumiditySensor | Nee | humi (DHT22) |
| matter_thermostat | Thermostat | Nee | setpoint lezen/schrijven |
| matter_co2 | TemperatureSensor (fake) | co2_enabled | ppm ÷ 100 als °C |
| matter_lux | TemperatureSensor (fake) | sun_light_enabled | lux ÷ 10 als °C |
| matter_motion1 | OccupancySensor | Nee | MOV1 PIR |
| matter_motion2 | OccupancySensor | mov2_enabled && pixels_num > 1 | MOV2 PIR |
| matter_pir1_light | OnOffLight | Nee | AAN = pixel_mode[0]=1 (manueel), UIT = auto/PIR |
| matter_pir2_light | OnOffLight | pixels_num > 1 | AAN = pixel_mode[1]=1, UIT = auto/PIR |
| matter_pixels_rgb | ColorLight | Nee | RGB kleur alle pixels |
| matter_pixels_on | OnOffPlugin | Nee | Pixels 2+ aan/uit (groep) |
| matter_bed | OnOffPlugin | Nee | Bed schakelaar |
| matter_thuis | OnOffPlugin | Nee | Thuis/Uit modus |

Totaal: 10 vaste + max 3 conditionele = **max 13 endpoints**

---

## 8. Technische Referentie

### 8.1 Kritische lessen

- `Matter.begin()` returns void — geen `if(Matter.begin())` gebruiken
- `F("text")` + String concatenatie is illegaal — plain string literals gebruiken
- Blocking `while(!Matter.isDeviceCommissioned())` — altijd timeout toevoegen
- `html.reserve()` vóór alle `+=` — voorkomt realloc fragmentatie
- 12 Matter endpoints = praktische limiet op ESP32-C6 met volledige web UI
- Scenes cluster: **AAN** laten (~10-15 KB, nodig voor Apple Home scenes)
- Groups cluster: **UIT** — Apple Home gebruikt controller-side grouping

### 8.2 Matter auto-recovery (defer tot Matter-versie)

Patroon uit HVAC v1.6 — overnemen bij stap 3.1:

```cpp
Matter.begin(); delay(200);
if (!Matter.isDeviceCommissioned() && Matter.getManualPairingCode().length() < 5) {
  // Auto-erase corrupt NVS + restart
}
```

### 8.3 Reporting aanpak

- Altijd push vanuit `loop()` — geen automatische Matter reporting timers
- Centrale `update_matter()` functie, interval 5-10 sec via `millis()`
- Geen min/max interval reporting activeren

### 8.4 Heap drempelwaarden

| Largest block | Status | Actie |
|---|---|---|
| > 35 KB | 🟢 Comfortabel | Geen actie |
| 25-35 KB | 🟡 Werkbaar | Nauwlettend opvolgen |
| < 25 KB | 🔴 Instabiel | **STOP** — evalueer SPIFFS of endpoint-schrapping |

---

## 9. HVAC Controller — Openstaande Issues

| Prio | Issue | Actie |
|---|---|---|
| P1 | White screen op iPhone bij ventilatieslider | HTML compressie — zelfde aanpak als ROOM v3.1 |
| P1 | SPIFFS migratie hoofdpagina | Vóór nieuwe Matter tests |
| P2 | Heap monitoring toevoegen | Zelfde implementatie als ROOM v1.6 |

HVAC Matter endpoints zijn reeds werkend in v1.6 — zie vorig document voor volledige lijst.

---

## 10. Workflow — Nieuw Gesprek Starten

### Upload bestanden

| Bestand | Rol |
|---|---|
| `ESP32-C6_TESTROOM_5mar_1600.ino` (v1.6) | Huidige basisversie — startpunt optimalisatie |
| `ESP32-C6_MATTER_ROOM_3mar_2200.ino` | Matter referentie — callbacks hergebruiken |
| Dit handover document (v3) | Volledige context |

### Eerste opdracht voor nieuw gesprek

1. Header comment toevoegen met compilatie-instellingen
2. SPIFFS migratie — alle pagina's (`/`, `/neopixel`, `/update`, `/settings` skelet)
3. RGB kleurkiezer inline op statuspagina
4. Serial interval instelbaar in `/settings`
5. Heap meten → invullen in tabel §3 → commit naar Github
6. Kopie hernoemen → Matter integratie beginnen


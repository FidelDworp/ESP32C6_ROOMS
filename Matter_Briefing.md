# Zarlar Matter Integratie — Briefing voor nieuw gesprek - 27feb26

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

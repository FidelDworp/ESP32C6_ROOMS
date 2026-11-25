# ESP32C6_ROOMS — Zarlar ROOM-controllers

Room-controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings` (bewaard in NVS).

- **Wat er in een room zit (en wat niet):** [`docs/ROOM_STANDAARD.md`](docs/ROOM_STANDAARD.md). Eerst lezen.
- **Ombouw per room van Photon naar ESP32:** [`docs/aansluitingen.md`](docs/aansluitingen.md).
- **Nieuw gesprek met Claude:** [`docs/ROOMS_OVERDRACHT.md`](docs/ROOMS_OVERDRACHT.md).
- **Systeembreed:** [Master-overnamedocument](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/Zarlar_Master_Overnamedocument.md) §7 (ROOM).

## Status (22 november 2026)

| | |
|---|---|
| **Sketch** | **v3.7.1** — map `ESP32_C6_ROOM/`, klaar om te uploaden: `bin/ESP32_C6_ROOM_v3.7.1.ino.bin` |
| **Draait op** | room80: `192.168.0.80` · `http://eetplaats.local/`. Testbord met alle sensoren, rij R-EETPL op de Dashboard-matrix |
| **Board** | ESP32-C6-DevKitM-1, 16 MB, op het Zarlar shield |
| **Apple Home** | Via de RPi-bridge (`server.js` + `matterbridge-zarlar`), toestel "Eetplaats". Geen Matter op de ESP32 |
| **Logging** | Het Dashboard (`.60`) haalt `/json` op en post **elke 5 min** naar de Sheet "R-ROOMS Log" (GAS v1.6). De room post zelf niets. Een room verschijnt pas in de Sheet als hij op het Dashboard → `/settings` op **actief** staat |
| **Portal (RPi)** | `192.168.0.50:3000` gebruikt `/json`, `/capabilities`, `/toggle_pixel_mode`, `/toggle_pixel`, `/setcolor`, `/set_home` |

## Pagina's

| Adres | Voor wie | Inhoud |
|---|---|---|
| `/` | iedereen | **Landingspagina:** kamertemperatuur met setpoint en schuifbalk, 🔥 = verwarming vraagt warmte, een tegel per lamp (bewegingslampen: tik = *automatisch* ↔ *altijd aan*), kleur, 🛏️ bed-modus, 🏠 thuis/weg, Generic ADC met bijnaam (als hij aan staat), ⚙️ naar de technische pagina |
| `/advanced` | FiDel | Alle metingen |
| `/settings` | FiDel | Instellingen + **Diagnose** (versie, herstarts met laatste stap, geheugen, WiFi, mDNS, SCD41, DHT22, OTA, NVS) |
| `/update` | FiDel | OTA met voortgangsbalk; meldt "Terug online" en gaat dan naar `/advanced` (sinds v3.6.1) |
| `/ver` | OTA-pagina | `versie\|uptime_s` in platte tekst (sinds v3.6.1) |
| `/json` | Dashboard, Portal, RPi-bridge | Live-gegevens (zie hieronder) |

## Bestanden

| Pad | Inhoud |
|---|---|
| `ESP32_C6_ROOM/` | De sketch (`.ino`, `landing.h`, `partitions.csv`). Open deze map in de Arduino IDE |
| `bin/` | Laatste gecompileerde `.bin` (alleen de laatste versie) |
| `GoogleScript/ROOM_GoogleScript.gs` | Google Apps Script v1.6 voor de Sheet |
| `docs/` | Standaard, overdracht, aansluitingen, Photon-inventaris |
| `old/` | v2.8, laatste versie vóór Matter (referentie) |

## Compileren en uploaden

**Arduino IDE:** ESP32C6 Dev Module · Flash Size **16 MB** · Partition: `partitions.csv` in de schetsmap (automatisch) · **USB CDC On Boot = Disabled** (anders foutmelding) · Core Debug Level None. Bibliotheken: ESPAsyncWebServer, AsyncTCP, OneWireNg, Adafruit NeoPixel, Sensirion I2C SCD4x (+ Sensirion Core). Geen DHT-bibliotheek (eigen uitlezing).

**USB:** voeding, eerste flash en seriële monitor (115200) via de poort **"UART"** van de devkit. De poort "USB" niet gebruiken (IO12/IO13 zijn shield-pinnen).

**Uploaden (OTA):** upload enkel `ESP32_C6_ROOM.ino.bin` (niet `.bootloader`, `.partitions` of `.merged`) op `http://<ip>/update`. Of zonder compileren de `.bin` uit `bin/`.

**Van op afstand** (Mac via mobiele data + Tailscale, **niet** via de wifi in Nieuwpoort: zelfde 192.168.0.x-bereik):
```
curl -L -o /tmp/room.bin https://raw.githubusercontent.com/FidelDworp/ESP32C6_ROOMS/main/bin/ESP32_C6_ROOM_v3.7.1.ino.bin
ls -l /tmp/room.bin
curl -F "update=@/tmp/room.bin" http://192.168.0.80/update
```
Sinds v3.5.1 is dat veilig: het antwoord is "Update succesvol!" of "Update mislukt!" met de reden; een afgebroken upload blokkeert de volgende niet.

**Na de overstap van een oude sketch met Matter:** `/settings` → Diagnose → eenmaal "Ongebruikte NVS-gegevens wissen".

## `/json` (contract)

Keys en bedieningsadressen wijzigen alleen met uitdrukkelijk akkoord: Dashboard, Portal, RPi-bridge en Sheet hangen ervan af. Nieuwe keys komen achteraan.

| Key | Inhoud | Key | Inhoud |
|---|---|---|---|
| `a` | uptime (s) | `t` | pixels aan (`P=0101…`) |
| `b` | verwarming vraagt (0/1) | `u` | pixelmodus MOV1/MOV2 |
| `c` | setpoint (°C) | `v` | thuis (0/1) |
| `d` | thermostaatcontact (0/1) | `w`, `x` | MOV1/MOV2 triggers per min |
| `e` | DHT22-temp (°C of `null`) | `y`, `z` | MOV1/MOV2-lamp aan |
| `f` | DS18B20 primair (°C of `null`) | `ac` | WiFi RSSI (dBm) |
| `g` | ventilatie (%) | `ad` | vrij geheugen (%) |
| `h` | vocht (%, DHT22, anders SCD41) | `ae`, `af` | grootste blok, laagste heap (KB) |
| `i` | dauwpunt (°C) | `ag` | aantal DS18B20 |
| `j` | dauwpuntalarm | `ah`–`aj`, `an`, `ao` | DS18B20 2–6 (°C of `null`; altijd aanwezig sinds v3.7) |
| `k` | CO2 (ppm, SCD41) | `al`, `am` | SCD41-temp (°C), -vocht (%) |
| `m` | LDR: **100 = licht, 0 = donker** (sinds v3.5.3) | `ar` | Generic ADC GP3 (V of `null`) |
| `o` | nacht (0/1) | | |
| `p` | bed (0/1) | **gepland** | `ak` effectief setpoint (v3.8) · `ap`, `aq`, `as` GP1, GP2, GP4 (v3.9) |
| `q`, `r`, `s` | kamerkleur RGB | **vervallen** | `l`, `n`, `aa`, `ab` (v3.5.2) |

## Google Sheet (GAS v1.6)

Sheet "R-ROOMS Log", tabblad **"Data"**: 43 kolommen (A–AQ), één rij per room per 5 min, kamerfilter in **C1** (ALL of één room). Ontbrekende meting of `null` = lege cel. Oude rijen (v1.5) staan in tabblad "Archief v1.5".

- **Kolom N "Licht LDR":** vóór 22/11 21:07 = 100 donker, erna = 100 licht.
- **Script wijzigen:** Apps Script → Implementeren → Implementaties beheren → potlood → **Nieuwe versie** (nooit "Nieuwe implementatie": dan verandert de URL). Na een kolomwijziging `archiveerEnNieuweKoppen()` of `setupHeaders()` uitvoeren.

## Plan

**Spelregels:** één versie per stap · plan, dan code na GO · na elke flash testen (Sheet: geheugen en herstarts) · `/json` is een contract · README en versiekop bij elke stap bijwerken.

| Versie | Doel | Voor |
|---|---|---|
| v3.6.1 | **OTA-voortgangspagina** zoals Vakantiehuizen v1.13: upload via JavaScript met voortgangsbalk (% en KB), daarna wachten tot de controller terug is ("✅ Terug online: ROOM vX") en automatisch naar `/advanced`; nieuwe route `/ver` (`versie\|uptime_s`). POST-handler en curl-upload ongewijzigd — ✅ 25/11 geflasht en getest | alle rooms |
| — | **RPi:** ontbrekende temperatuur (`null`) als "–" i.p.v. 0,0 °C (`room.html`, `index.html`) — ✅ 25/11 in de overkoepelende repo | RPi |
| v3.6.3 | **CO2-referentie SCD41** instelbaar ("CO2 buitenlucht", standaard 420 ppm): ASC-streefwaarde én waarde voor "Kalibreren in buitenlucht" — ✅ geflasht 25/11 | alle rooms |
| v3.7 | **Temperatuurbewaking** zoals de Sjalay, max. **6 DS18B20's** (`an`, `ao`), vocht valt terug op de SCD41 — ✅ 25/11 geflasht en getest met de RoomSense-kabel los (v3.7.1: SCD41-terugval ook op landingspagina, `/advanced`, RPi en Apple Home) | alle rooms |
| v3.8 | **Verwarmingslogica** zoals de Sjalay (`ak`) | alle rooms |
| *open* | **Pixelmodi:** per pixel manueel / beweging / nachtlampje, kleur per pixel (vraag van 18/11, nog te beslissen) | B&B (vloermat) |
| v3.8b | **Pixel-effecten** achter de Powerpixels | B&B, BADK, SLPK |
| → | **B&B gaat live** | B&B |
| v3.9 | **Generieke I/O** GP1, GP2, GP4 (`ap`, `aq`, `as`, `/set_gp`) | alle rooms |
| v3.10–v3.12 | Pagina's naar eigen `.h`-bestanden: `/update` (voortgang al in v3.6.1), `/advanced`, `/settings` | alle rooms |
| — | **Uitrol:** elke Photon-room krijgt een ESP32 met deze sketch (B&B eerst, dan BADK, INKOM, KEUKEN, WASPL, ZITPL) | |

### v3.8 Verwarmingslogica

- **Effectief setpoint** = dauwpuntbeveiliging aan ? max(setpoint, dauwpunt + marge) : setpoint → nieuwe key **`ak`**.
- **Echte hysterese:** AAN onder effectief − hysterese/2, UIT boven effectief + hysterese/2. Altijd UIT boven 30 °C; zonder geldige temperatuur de vorstbeveiliging (v3.5).
- **Thuis + thermostaatcontact:** `b` volgt de thermostaat **of** de dauwpuntbeveiliging (beslist 5/11).
- **`/settings`:** dauwpuntbeveiliging aan/uit (aan) · marge −10…+10 °C (2,0) · hysterese 0,2…5,0 °C (1,0) · minimum setpoint 2…25 °C (10, onderkant van de schuifbalk).
- **Landingspagina:** "21,3° (→ 18°)" met het echte doel uit `ak`; korte uitleg als de dauwpuntcorrectie het doel optrekt.

### v3.8b Pixel-effecten (idee 18/11)

Effectstrook "pixel X tot Y" achter de Powerpixels: volg kamerkleur, kleurgolf, ademen, regenboog, nachtlampje bij beweging 's nachts. Eén update-lus (±40 beelden/s), max. ±500 pixels, optioneel eigen datalijn op GP4 (IO18). Detailplan later (striptype en aantallen per room; B&B: treden 3 + 9).

### v3.9 Generieke I/O

GP1 = IO12, GP2 = IO7, GP4 = IO18 (GP3 = IO2 is sinds v3.5.2 de Generic ADC, `ar`). Per pin in `/settings`: modus (uit · digitale ingang · digitale uitgang · PWM), bijnaam, pull-up; standaard uit (ingang zonder pull-up). Op `/advanced` met knop of schuifbalk; in `/json` als `ap`, `aq`, `as` (0/1 of 0–100 %, leeg als uit). Bediening: `/set_gp?n=1..4&v=0..100` (beslist 7/11).

### v3.10–v3.12 Pagina's apart

Elke pagina in een eigen `.h`-bestand (zoals `landing.h`): de `.ino` bevat enkel logica en de Arduino IDE kan het JavaScript niet beschadigen. Meteen live en vaste gegevens scheiden (vaste `id` per veld), statuscirkels zoals de landingspagina, korte Nederlandse labels. Volgorde: `/update` (klein, eerste test; de voortgangsbalk komt al in v3.6.1), `/advanced`, `/settings`.

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.7.1** | 25/11 | Terugval op de SCD41 ook op de landingspagina en in de melding op `/advanced` (+ RPi `index.html`, bridge v0.8.1) |
| v3.7 | 25/11 | **Temperatuurbewaking:** max. 6 DS18B20's (`an`, `ao`; `ah`–`ao` altijd aanwezig), fouten per sensor (3 op rij = ontbreekt), 85,0 °C genegeerd, DHT22 5 min geldig, vocht/dauwpunt terugval op de SCD41, overzicht "Alle temperatuursensoren" op `/advanced` |
| v3.6.3 | 25/11 | CO2-referentie SCD41 instelbaar (ASC-doel + FRC, standaard 420 ppm; was 400/425) |
| v3.6.2 | 25/11 | Paginatitels met de kamernaam: landingspagina "Eetplaats" (was "Zarlar"), "Eetplaats - OTA", "Eetplaats - Status" |
| v3.6.1 | 25/11 | **OTA-voortgangspagina** (uit Vakantiehuizen v1.13): voortgangsbalk, "Terug online", naar `/advanced`; route `/ver` |
| v3.6 | 22/11 | **WiFi robuuster + mDNS:** geen modem-slaap; elke 30 s herverbinden; herstart na 15 min zonder WiFi; AP-setupmodus enkel zonder ingestelde SSID, anders **nood-AP** met normale werking dat sluit zodra de router terug is (vroeger na een stroompanne voor altijd AP zonder verwarming); `<kamernaam>.local`; Diagnose WiFi en mDNS |
| v3.5.3 | 22/11 | **LDR omgekeerd** (`m` 100 = licht), drempel eenmalig omgerekend, nacht volgt de drempel. Samen met Dashboard v6.5 en RPi `matrix.html` |
| v3.5.2 | 22/11 | **Opruimen:** stof, zon, lichtstraal weg (`l`, `n`, `aa`, `ab`); IO2 = **Generic ADC** in volt (`ar`) met bijnaam en factor; echte oorzaak van een geplande herstart in de geschiedenis |
| v3.5.1 | 22/11 | **Veilige OTA:** afgebroken upload opgeruimd, succes alleen na een volledige upload, reden bij mislukking, herstart vanuit `loop()` |
| v3.5 | 20/11 | **Temperatuurveiligheid:** −10…40 °C, terugval DS18B20 → DHT22 → SCD41, `null` i.p.v. 0,0, vorstbeveiliging 15 min/uur |
| v3.4.5 | 11/11 | NVS opruimen (Matter-resten), knop in Diagnose |
| v3.4.4 | 11/11 | Eigen DHT22-uitlezing: einde van de interrupt-watchdog-herstarts |
| v3.4.3 | 9/11 | Ventilatie op CO2 instelbaar (600/1200 ppm) |
| v3.4.2 | 9/11 | I2C op IO13 werkt: USB CDC uit, USB-functie van IO12/IO13 uitgeschakeld |
| v3.4–v3.4.1 | 9/11 | SCD41 via I2C (`al`, `am`), CO2-PWM weg, I2C-diagnose |
| v3.3 | 7/11 | TSL2561 weg, "laatste stap vóór herstart" in Diagnose |
| v3.2 | 5/11 | Versie en Diagnose zichtbaar |
| v3.1 | 3/11 | Landingspagina `/`, technische pagina naar `/advanced` |
| v3.0 | 3/11 | Matter weg (grootste vrije blok van ±30 KB naar ±245 KB; geen dagelijkse herstarts meer) |
| v2.8 | mrt 2026 | Laatste versie vóór Matter (`old/`) |

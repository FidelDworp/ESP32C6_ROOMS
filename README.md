# ESP32C6_ROOMS — Zarlar ROOM controllers

Room controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings`; ze worden bewaard in NVS.

**Systeembrede referentie:** [`Zarlar_Master_Overnamedocument.md`](https://github.com/FidelDworp/ZarlarApp/blob/main/ESP32_Zarlar/Zarlar_Master_Overnamedocument.md), §7 (ROOM) en §3 (softwareregels). Hardware, pinout en de `/json`-keys staan daar, niet in deze repo.

## Status (5 november 2026)

| | |
|---|---|
| **Actieve sketch** | **v3.2** (5 nov 2026), zonder Matter, met landingspagina en diagnose — `TESTROOM/ESP32_C6_ROOM.ino` + `TESTROOM/landing.h` |
| **Draait op** | room80 (`192.168.0.80`): het TESTROOM-testbord met alle sensoren. Rij R-EETPL op de matrix van het Dashboard |
| **Board** | ESP32-C6-DevKitM-1 (clone), 32-pin, 16 MB, op het Zarlar shield |
| **Apple Home** | Via de RPi-bridge (`server.js` + plugin `matterbridge-zarlar`): toestel "Eetplaats" in de woning Zarlardinge. De ESP32 zelf doet geen Matter meer |
| **Logging** | Het Zarlar Dashboard (`192.168.0.60`) haalt `/json` op en stuurt het **elke 5 min** naar Google Sheets (`ROOM_GoogleScript.gs`, Sheet "R-ROOMS Log"). De controller post zelf niets. Het ritme stel je in op het Dashboard (`/settings` → Poll interval × 5), niet in deze sketch. Ontbreekt er één rij rond een flash of herstart, dan was de controller op dat moment niet bereikbaar: normaal |
| **Portal (RPi)** | `192.168.0.50:3000` gebruikt `/json`, `/capabilities`, `/toggle_pixel_mode`, `/toggle_pixel`, `/setcolor` en `/set_home` |

## Waarom v3.0 zonder Matter

Met Matter (v2.24) zat room80 voortdurend tegen de geheugengrens. Uit de Sheet, 1 mei – 3 nov 2026 (5982 rijen):

| v2.24 (met Matter) | Waarde |
|---|---|
| Vrij geheugen | mediaan **19 %** |
| Grootste vrije blok | mediaan **31–35 KB**, laagste 20 KB; 58× onder 25 KB |
| Laagste heap ooit | mediaan 7 KB, soms 0 KB |
| Herstarts | **100 in 6 maanden**; sinds 18/10 ongeveer 1 per dag |

Bij de HVAC (v2.0) en het Dashboard (v6.0) leverde het weghalen van Matter een grootste blok van ±200–250 KB op en geen herstarts meer.

**Eerste meting v3.0** (OTA op 3 nov 2026 om 18:07; de Sheet-rijen vlak voor en na de flash):

| | v2.24 (18:02, uptime ±58 u) | v3.0 (18:12, uptime 300 s) |
|---|---|---|
| Vrij geheugen | 18 % | **68 %** |
| Grootste blok | 28 KB | **247 KB** |
| Heap min (laagste ooit) | 2 KB | **233 KB** |

Dezelfde winst als bij de HVAC en het Dashboard. De sensorwaarden lopen gewoon door.

**Test v3.1** (flash 3 nov 2026 ±19:55, Sheet tot 5 nov 14:39, 511 rijen):

| Meting v3.1 | Waarde |
|---|---|
| Vrij geheugen | mediaan **68 %**, laagste 66 % |
| Grootste blok | mediaan **247 KB**, laagste 239 KB |
| Heap min (laagste ooit) | 230–264 KB, daalt niet: geen lek |
| Herstarts | 3 (4/11 23:28, 5/11 12:27 en 14:33), alle drie door werk aan het testbord (verhuis naar de eetplaats, opties gewijzigd, CM1106 los, MOV2 naar 3V3). Vlak ervoor telkens 247–251 KB vrij: **geen herstart door de sketch** |
| Logging | elke 5 min; 3 losse gemiste rijen (één gemiste poll van het Dashboard, uptime liep door) |

**Besluit:** v3.1 is stabiel. Geen enkele herstart meer zoals met Matter (±1 per dag).

## Pagina's

| Adres | Voor wie | Inhoud |
|---|---|---|
| `/` | iedereen | **Landingspagina:** grote kamertemperatuur met het gewenste setpoint (→) en een schuifbalk (10-30 °C); 🔥-ring rood = verwarming vraagt warmte · een tegel per lamp met de pixelnamen uit `/settings`; aan = tegel in de gekozen kleur. De bewegingslampen (pixel 0, en pixel 1 als MOV2 aan staat) wisselen bij een tik tussen *automatisch* (aan bij beweging in het donker) en *altijd aan*; de andere lampen gewoon aan/uit · kleurkeuze · 🛏️ bed-modus (donkerblauw = aan: bewegingslampen blijven uit) · 🏠 thuis/weg (groen = thuis) · ⚙️ naar de technische pagina |
| `/advanced` | Filip | Technische statuspagina (alle metingen, diagnose) — tot v3.0 was dit `/` |
| `/settings` | Filip | Instellingen (kamernaam, WiFi, sensoren aan/uit, pixels en hun namen …) |
| `/update` | Filip | OTA |
| `/json` | Dashboard, Portal, RPi-bridge | Live-gegevens (ongewijzigd) |

Het menu links (of bovenaan op een gsm) heeft op elke technische pagina een **🏠** naar de landingspagina.

## Bestanden

| Pad | Inhoud |
|---|---|
| `TESTROOM/ESP32_C6_ROOM.ino` | Actieve ROOM-sketch (versie in de kop van het bestand) |
| `TESTROOM/landing.h` | Landingspagina `/` (sinds v3.1). Apart bestand zodat de Arduino IDE het JavaScript niet aanpast. **Moet in dezelfde map als het `.ino`-bestand staan** |
| `TESTROOM/ROOM_GoogleScript.gs` | Google Apps Script: 37 kolommen A–AK |
| `TESTROOM/partitions.csv` | Partitietabel 16 MB (2 × 6 MB app, voor OTA). Moet naast het `.ino`-bestand staan |
| `TESTROOM/old/` | Archief: v2.8 (maart 2026). Alleen ter referentie |

## Compileren en uploaden

**Arduino IDE:** board **ESP32C6 Dev Module** · Flash Size **16 MB** · Partition Scheme: `partitions.csv` in de schetsmap (wordt automatisch gebruikt) · USB CDC On Boot **Enabled** · Core Debug Level **None**.

**Uploaden:**
1. De eerste keer via de USB-kabel.
2. Daarna via OTA: *Sketch → Export Compiled Binary*. In `build/esp32.esp32.esp32c6/` staan meerdere `.bin`-bestanden: kies **enkel `ESP32_C6_ROOM.ino.bin`**, niet `.bootloader.bin`, `.partitions.bin` of `.merged.bin`. Upload dat op `http://<ip>/update`; de controller herstart vanzelf.
3. Staat Flash Size per ongeluk op 4 MB (zichtbaar als `FlashSize=4M` in de compile-uitvoer), dan kan de OTA mislukken: eerst op 16 MB zetten en opnieuw exporteren.

**Na een update naar v3.0:** de oude Eetplaats-tegels in de testwoning "Matter Home" verdwijnen. De tegels in Zarlardinge (via de RPi) blijven werken. De testwoning "Matter Home" kan pas weg (Portal Plan AP26) als ook de ECO-boiler geen Matter meer heeft: dat is de laatste ESP32 met Matter.

## Verbeterplan

Dit is de sketch voor **alle** rooms: ze vervangt stap voor stap de Photons. **Beslist 5 nov 2026: eerst de functies** die nodig zijn om de rooms over te zetten (de B&B eerst), daarna pas de pagina's herwerken. Hardware per room: zie `README_ROOM_ACCESS_aansluitingen.md`.

**Spelregels:** één versie per stap · code pas na GO · na elke flash 48 u testen (Sheet: geheugen en herstarts) · `/json`-keys en bedieningsadressen blijven gelijk, tenzij uitdrukkelijk afgesproken (Dashboard, Portal, RPi-bridge en Sheet hangen ervan af) · README en versiekop bij elke stap bijwerken.

| Fase | Versie | Wat | Voor | Status |
|---|---|---|---|---|
| — | v3.0 | Matter verwijderd | | ✅ 3/11 |
| — | v3.1 | Landingspagina `/` (`landing.h`), technische pagina naar `/advanced` | | ✅ 3/11 |
| **A. Basis** | v3.2 | Versie en diagnose zichtbaar | alle rooms | ⏳ klaar, flashen + 48 u |
| | v3.3 | WiFi robuuster + mDNS terug | alle rooms | |
| **B. Functies** | v3.4 | **SCD41** (CO2 + temp + vocht via I2C) · CO2-PWM weg · geen metingen in AP-mode · **GAS v1.6** | B&B, BADK, ZITPL | |
| | → | **B&B gaat live** | B&B | |
| | v3.5 | **Opruimen:** stofsensor en lichtstraal volledig weg, ook hun `/json`-keys | alle rooms | |
| | v3.6 | **Buitenlicht** op IO2 (PWM-E, oostkant) | B&B | |
| | v3.7 | **Pixelklok** na de Powerpixels | ZITPL | |
| **C. Pagina's** | v3.8 | `/update` → `ota.h`, met voortgangsbalk | alle rooms | |
| | v3.9 | `/advanced` → `advanced.h` | alle rooms | |
| | v3.10 | `/settings` → `settings.h` | alle rooms | |
| **D. Later** | v3.11 | LDR-schaal gelijk met de Sjalay (samen met Dashboard en Sheet) | alle rooms | |
| | — | Zonlicht (TSL2561) delen tussen rooms: **optioneel, lage prioriteit** | | |
| | — | Uitrol: elke Photon-room krijgt een ESP32 met deze sketch | | |

### Fase A — Basis

| Versie | Wat | Waarom |
|---|---|---|
| v3.2 | **Versie en diagnose zichtbaar:** één `#define ROOM_VERSION`, getoond op elke pagina, in `/settings` en bij het opstarten. In `/settings` ook uptime, reden van de laatste herstart (stroom, software/OTA, crash, watchdog, spanningsdip), aantal opstarts, geheugen en WiFi | Nu is nergens te zien welke versie op een room draait en waarom hij herstartte; bij 8 rooms onmisbaar |
| v3.3 | **WiFi robuuster:** geen modem-slaap (`WiFi.setSleep(false)`) en periodiek opnieuw verbinden. **mDNS terug:** `<kamer>.local`, enkel na een geldige WiFi-verbinding | Les uit de PWM-test (onbereikbaar in de kelder); rooms vinden op naam |

### Fase B — Functies

**v3.4 SCD41** (Sensirion SCD41 op de I2C-bus bij de controller, samen met de TSL2561: SDA IO13, SCL IO11, adres 0x62)
- **Niet-blokkerend:** periodieke meting in de sensor zelf; de sketch leest om de 5 s als er een nieuwe waarde klaarstaat. `pulseIn()`, `readCO2()` en `CO2_PWM` (IO18) verdwijnen. IO18 wordt vrij.
- **CO2** blijft in `/json` key `k` en stuurt de ventilatie zoals nu (400–800 ppm → 0–100 %).
- **Temperatuur en vocht van de SCD41** komen **apart** van die van de DHT22, in twee **nieuwe keys achteraan**: `ak` = SCD-temp (°C), `al` = SCD-vocht (%). (`ae`/`af` zijn al bezet door het geheugen, `ah`–`aj` door de extra DS18B20's.) De DHT22 blijft de kamertemperatuur en het vocht leveren (`e`, `h`); de SCD-waarden zijn alleen extra in de UI en de Sheet.
- **`/settings`:** SCD41 aan/uit (vervangt "CO2 aan/uit"), **ASC aan/uit** (aan in kamers die gelucht worden, uit in afgesloten ruimtes), **temperatuuroffset**, knop **"kalibreren in buitenlucht"** (FRC op ±425 ppm).
- **Geen sensormetingen in AP-mode:** `/settings` reageert vlot bij de eerste installatie.
- **GAS v1.6** (`ROOM_GoogleScript.gs`) samen met v3.4 in gebruik nemen: kolommen voor `ak` en `al` erbij, Stof (`l`) en Beam (`aa`, `ab`) eruit. Het script leest per key, dus oudere sketches loggen gewoon verder. Daarna `setupHeaders()` één keer uitvoeren en "Implementaties beheren → potlood → nieuwe versie" (nooit een nieuwe implementatie). Oude rijen in de Sheet hebben de oude kolomindeling: archiveren in een apart tabblad.

**v3.5 Opruimen:** de stofsensor (Sharp, IO12/IO7) en de lichtstraal (LDR2 op IO2) volledig weg: code, instellingen (`dust_en`, `beam_en`, `beam_thresh`), UI en de `/json`-keys `l`, `aa` en `ab`. Het Dashboard en de RPi gebruiken die keys niet (nagekeken 5/11). IO2 komt vrij voor v3.6.

**v3.6 Buitenlicht** (B&B: LED-modules oost E1/E2 via de buffer 3,3 → 5 V)
- Optionele uitgang op IO2, aan/uit in `/settings`. PWM, dus dimbaar.
- Aan bij beweging op MOV2 (buiten, trap) en/of MOV1 (binnen), instelbaar; alleen 's nachts.
- Instelbaar: duur na beweging, helderheid, omkeren (0 V = aan).
- Veilige opstart: uitgang op "uit" vóór WiFi en de rest starten.

**v3.7 Pixelklok** (ZITPL): een reeks pixels **na** de lichtgroepen (Powerpixels) als klok: uur, minuut en seconde, helderheid instelbaar, tijd via NTP met zomer- en wintertijd. Aantal klokpixels instelbaar (12).

### Fase C — Pagina's apart en opgefrist

**Waarom apart.** De landingspagina staat sinds v3.1 in een eigen bestand (`landing.h`). De andere pagina's (`/advanced`, `/settings`, `/update`) worden nog stukje per stukje in de `.ino` opgebouwd. Elke pagina in een eigen `.h`-bestand geeft overzicht (de `.ino` bevat enkel nog logica) en voorkomt dat de Arduino IDE het JavaScript in een `.ino` beschadigt. Geheugen win je er niet mee.

**Hoe.** Vaste HTML + JavaScript; live-waarden uit `/json`, vaste waarden (versie, IP, instellingen) uit een klein JSON-adres dat alleen bij het laden gelezen wordt. Bij elke verhuis meteen drie opfrissingen:
1. **Live en vaste gegevens scheiden**, live-waarden via een vaste `id` per veld (nu zoekt het script de rij via de labeltekst).
2. **Statuscirkels** zoals de Sjalay en de landingspagina (wit = rust, lichtrood met rode rand = actief).
3. **Korte, eenduidige labels**, consequent in het Nederlands.

| Versie | Pagina | Opmerking |
|---|---|---|
| v3.8 | `/update` → `ota.h` | Klein: eerste test van de aanpak. Met voortgangsbalk en uitleg over het juiste `.bin`-bestand |
| v3.9 | `/advanced` → `advanced.h` | Grootste winst in onderhoudbaarheid |
| v3.10 | `/settings` → `settings.h` | Grootste en meest vervlochten pagina, daarom als laatste |

### Fase D — Later

| Versie | Wat |
|---|---|
| v3.11 | **LDR-schaal gelijk met de Sjalay:** 0 = donker, 100 = fel; één instelbare nachtdrempel. Raakt ook het Dashboard en de Sheet: samen aanpassen |
| — | **Zonlicht delen** (TSL2561 in één room, de andere lezen die lux): optioneel, lage prioriteit. Dag/nacht blijft voorlopig uit de kamer-LDR komen |
| — | **Uitrol:** per kamer een ESP32 met deze sketch in plaats van de Photon; instellingen via `/settings`; RPi-bridge en Dashboard krijgen de nieuwe room erbij |

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.2** | 5 nov 2026 | Versie en diagnose zichtbaar: `ROOM_VERSION` op elke pagina en bij het opstarten; in `/settings` een blok Diagnose (uptime, reden laatste herstart, aantal opstarts, geheugen, WiFi). "Crashteller" heet nu "Lage-heap meldingen". Geen nieuwe `/json`-keys |
| v3.1 | 3 nov 2026 | Landingspagina `/` (landing.h); technische statuspagina naar `/advanced`; 🏠 in het menu |
| v3.0 | 3 nov 2026 | Matter verwijderd; Apple Home via de RPi-bridge. Verder ongewijzigd (JSON-keys, endpoints, NVS) |
| v2.24 | 8 mei 2026 | Gedeelde CSS-helper (heap), versieregel op serial verwijderd |
| v2.22 | 1 mei 2026 | `/capabilities` voor de verlichting in de Portal |
| v2.21 | 13 apr 2026 | Thuis/Weg vereenvoudigd (heating_mode/vent_mode weg) |
| v2.20 | 13 apr 2026 | Stabiliteit: crash-log in NVS, CO2-timeout 50 ms |
| v2.8 | mrt 2026 | Laatste versie vóór Matter (in `old/`) |

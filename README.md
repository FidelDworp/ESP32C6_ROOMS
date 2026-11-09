# ESP32C6_ROOMS — Zarlar ROOM controllers

Room controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings`; ze worden bewaard in NVS.

**Systeembrede referentie:** [`Zarlar_Master_Overnamedocument.md`](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/Zarlar_Master_Overnamedocument.md), §7 (ROOM) en §3 (softwareregels). Hardware, pinout en de `/json`-keys staan daar, niet in deze repo.

## Status (5 november 2026)

| | |
|---|---|
| **Actieve sketch** | **v3.4.3** (9 nov 2026), zonder Matter, met landingspagina, diagnose, "laatste stap vóór herstart" en **SCD41** (CO2 + temp + vocht via I2C) — map `ESP32_C6_ROOM/` |
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
| `ESP32_C6_ROOM/` | **De sketch.** Open deze map in de Arduino IDE |
| `ESP32_C6_ROOM/ESP32_C6_ROOM.ino` | ROOM-sketch (versie in de kop en in `ROOM_VERSION`) |
| `ESP32_C6_ROOM/landing.h` | Landingspagina `/` (sinds v3.1). Apart bestand zodat de Arduino IDE het JavaScript niet aanpast |
| `ESP32_C6_ROOM/partitions.csv` | Partitietabel 16 MB (2 × 6 MB app, voor OTA). Wordt automatisch gebruikt omdat hij naast het `.ino`-bestand staat |
| `bin/ESP32_C6_ROOM_v3.4.3.ino.bin` | **Klaar om te uploaden** via `/update` (gecompileerd door Claude, zie hieronder). Alleen de laatste versie |
| `GoogleScript/ROOM_GoogleScript.gs` | Google Apps Script voor de Sheet "R-ROOMS Log" (37 kolommen A–AK) |
| `docs/ROOM_STANDAARD.md` | **De ROOM-standaard:** wat er in elke room zit en wat niet. Eerst lezen |
| `docs/ROOMS_OVERDRACHT.md` | Overdracht voor een nieuw gesprek met Claude: werkwijze, stand, beslissingen, open punten |
| `docs/aansluitingen.md` | Ombouw per room van Photon naar ESP32: aansluitingen, meten, buffer voor ACCESS |
| `docs/Zarlar_Photon_inventaris_v3.xlsx` | Inventaris van alle Photon-signalen per room en ACCESS |
| `old/` | Archief: v2.8 (maart 2026), de laatste versie vóór Matter. Alleen ter referentie |

## Compileren en uploaden

**Arduino IDE:** board **ESP32C6 Dev Module** · Flash Size **16 MB** · Partition Scheme: `partitions.csv` in de schetsmap (wordt automatisch gebruikt) · USB CDC On Boot **Disabled** (sinds v3.4.2; met Enabled geeft de sketch een foutmelding) · Core Debug Level **None**.

**USB-poorten van de devkit (sinds v3.4.2):** voeding, flashen en seriële monitor (115200 baud) via de USB-C-poort **"UART"**. De poort **"USB"** niet gebruiken: die zit op IO12/IO13, en die pinnen gebruikt het shield (IO13 = I2C SDA, IO12 = stof-LED, later GP1).

**Uploaden:**
1. De eerste keer via de USB-kabel.
2. Daarna via OTA: *Sketch → Export Compiled Binary*. In `build/esp32.esp32.esp32c6/` staan meerdere `.bin`-bestanden: kies **enkel `ESP32_C6_ROOM.ino.bin`**, niet `.bootloader.bin`, `.partitions.bin` of `.merged.bin`. Upload dat op `http://<ip>/update`; de controller herstart vanzelf.
3. **Of zonder compileren:** download `bin/ESP32_C6_ROOM_v3.<x>.ino.bin` uit deze repo (knop *Download raw file*) en upload die op `http://<ip>/update`. Gecompileerd met ESP32-core 3.3.12 en de nieuwste versies van de bibliotheken (o.a. ESPAsyncWebServer 3.12.1, AsyncTCP 4.0.0, OneWireNg 0.14.1, Adafruit NeoPixel 1.15.5, DHT 1.4.7, Sensirion I2C SCD4x 1.1.0 + Sensirion Core 0.7.3).

**Bibliotheken voor de Arduino IDE** (sinds v3.4 ook): *Sensirion I2C SCD4x* (installeert *Sensirion Core* mee).
4. Staat Flash Size per ongeluk op 4 MB (zichtbaar als `FlashSize=4M` in de compile-uitvoer), dan kan de OTA mislukken: eerst op 16 MB zetten en opnieuw exporteren.

**Na een update naar v3.0:** de oude Eetplaats-tegels in de testwoning "Matter Home" verdwijnen. De tegels in Zarlardinge (via de RPi) blijven werken. De testwoning "Matter Home" kan pas weg (Portal Plan AP26) als ook de ECO-boiler geen Matter meer heeft: dat is de laatste ESP32 met Matter.

## Verbeterplan

Dit is de sketch voor **alle** rooms: ze vervangt stap voor stap de Photons. **Beslist 5 nov 2026: eerst de functies** die nodig zijn om de rooms over te zetten (de B&B eerst), daarna pas de pagina's herwerken. **Wat er in een room zit (en wat niet): [`docs/ROOM_STANDAARD.md`](docs/ROOM_STANDAARD.md)** (vastgelegd 6 nov 2026). Ombouw per room: [`docs/aansluitingen.md`](docs/aansluitingen.md).

Bijgekomen op 5 nov 2026: de **"goodies" van de Sjalay-controller** (v1.8, repo `SJALAY-YSERMONDE`): temperatuurbewaking, de volledige verwarmings- en dauwpuntlogica, instelbare hysterese en een overzicht van alle DS18B20's (v3.7 en v3.8).

**Planning aangepast op 9 nov 2026 (2×):** de SCD41 zit al op room80, dus hij komt naar voren als **v3.4** (was v3.7). WiFi + mDNS schuift naar v3.5, temperatuurbewaking naar v3.6, verwarmingslogica naar v3.7. Na v3.4 volstaat een **24 u-test** (beslist 9/11). **Tweede aanpassing (9/11 avond):** het opruimen (stofsensor en lichtstraal weg) komt als **v3.5**, samen met **GAS v1.6** (die haalt Stof en Beam ook uit de Sheet), direct na de 24 u-test van v3.4.3. Daardoor: WiFi + mDNS v3.6, temperatuurbewaking v3.7, verwarmingslogica v3.8 (daarna B&B live).

**Spelregels:** één versie per stap · code pas na GO · na elke flash 48 u testen (Sheet: geheugen en herstarts) · `/json`-keys en bedieningsadressen blijven gelijk, tenzij uitdrukkelijk afgesproken (Dashboard, Portal, RPi-bridge en Sheet hangen ervan af) · README en versiekop bij elke stap bijwerken.

| Fase | Versie | Wat | Voor | Status |
|---|---|---|---|---|
| — | v3.0 | Matter verwijderd | | ✅ 3/11 |
| — | v3.1 | Landingspagina `/` (`landing.h`), technische pagina naar `/advanced` | | ✅ 3/11 |
| **A. Basis** | v3.2 | Versie en diagnose zichtbaar | alle rooms | ⚠️ 48 u-test niet gehaald: 4 herstarts door de interrupt-watchdog (geheugen in orde) |
| | v3.3 | **Stabiliteit:** TSL2561 weg + "laatste stap vóór herstart" bewaren (diagnose via OTA) | alle rooms | ✅ room80: 7/11 geflasht, 48 u-test geslaagd (50 u 47 min zonder herstart, geheugen stabiel) |
| | v3.4 | **SCD41** (CO2 + temp + vocht via I2C) · CO2-PWM weg · geen metingen in AP-mode — naar voren gehaald 9/11, daarna 24 u-test. **GAS v1.6** als aparte stap ná de test (beslist 9/11) | B&B, BADK, ZITPL (test op room80) | ⏳ v3.4.3 in de repo 9/11 (SCD41 ok sinds v3.4.2); te flashen, dan 24 u-test |
| | v3.5 | **Opruimen:** stofsensor en lichtstraal volledig weg, ook hun `/json`-keys | alle rooms | samen met **GAS v1.6**, na de 24 u-test van v3.4.3 |
| | v3.6 | WiFi robuuster + mDNS terug | alle rooms | |
| **B. Functies** | v3.7 | **Temperatuurbewaking** (zoals de Sjalay) + overzicht alle DS18B20's, **maximum 6** (nieuwe keys `an`, `ao`) | alle rooms | |
| | v3.8 | **Verwarmingslogica zoals de Sjalay:** effectief setpoint, dauwpunt aan/uit, marge −10…+10, instelbare hysterese, minimum setpoint, landingspagina "IST (→ SOLL)" | alle rooms | |
| | → | **B&B gaat live** | B&B | |
| | v3.9 | **Generieke I/O** GP1–GP4 op de vrije pinnen van RoomSense en Option | alle rooms | |
| **C. Pagina's** | v3.10 | `/update` → `ota.h`, met voortgangsbalk | alle rooms | |
| | v3.11 | `/advanced` → `advanced.h` | alle rooms | |
| | v3.12 | `/settings` → `settings.h` | alle rooms | |
| **D. Later** | v3.13 | LDR-schaal gelijk met de Sjalay (samen met Dashboard en Sheet) | alle rooms | |

Niet meer in dit plan (6 nov 2026, zie `docs/ROOM_STANDAARD.md` §1.5): het **buitenlicht** van de B&B (eigen controllers ACCESS2, ACCESS3…), de **pixelklok** (eigen ESP32 in de ZITPL). De **TSL2561** gaat al in v3.3 weg: die sensor komt voortaan op een ACCESS- of OUTSIDE-controller, niet meer op een room.
| | — | Uitrol: elke Photon-room krijgt een ESP32 met deze sketch | | |

### Nieuwe `/json`-keys (beslist 5 nov 2026)

Alle nieuwe keys komen **achteraan**; bestaande keys blijven ongewijzigd, op `l`, `aa` en `ab` na (vervallen in v3.5). `ae`/`af` zijn bezet door het geheugen, `ah`–`aj` door de extra DS18B20's 2–4. Het Dashboard en de Portal gebruiken `ag`–`aj` niet (nagekeken 5/11); alleen de Sheet.

| Key | Inhoud | Sinds | In de Sheet |
|---|---|---|---|
| `an` | DS18B20 nr. 5 (°C) | v3.7 | ja (vanaf GAS v1.6) |
| `ao` | DS18B20 nr. 6 (°C) | v3.7 | ja (vanaf GAS v1.6) |
| `ak` | Effectief setpoint (°C, 1 decimaal) = het echte doel na dauwpuntcorrectie | v3.8 | ja (vanaf GAS v1.6) |
| `al` | SCD41-temperatuur (°C) | v3.4 | ja |
| `am` | SCD41-vochtigheid (%) | v3.4 | ja |
| `ap`–`as` | Generieke I/O GP1–GP4: 0/1 (digitaal), 0–100 % (analoog of PWM), leeg als de pin uit staat | v3.9 | ja (kolommen al in GAS v1.6) — beslist 7/11 |

### Fase A — Basis

| Versie | Wat | Waarom |
|---|---|---|
| v3.2 | **Versie en diagnose zichtbaar:** één `#define ROOM_VERSION`, getoond op elke pagina, in `/settings` en bij het opstarten. In `/settings` ook uptime, reden van de laatste herstart (stroom, software/OTA, crash, watchdog, spanningsdip), aantal opstarts, geheugen en WiFi | Nu is nergens te zien welke versie op een room draait en waarom hij herstartte; bij 8 rooms onmisbaar |
| v3.3 | **Stabiliteit.** De TSL2561 en zijn code verdwijnen uit de room (de sensor hoort op een ACCESS- of OUTSIDE-controller, zie `docs/ROOM_STANDAARD.md`); key `n` blijft, waarde 0. Na een herstart door watchdog of crash toont `/settings` → Diagnose de **laatste stap** die de sketch uitvoerde (bv. "DHT22 lezen", "pixels tonen", "webpagina /settings"), bewaard in geheugen dat een herstart overleeft. Geen seriële kabel nodig | v3.2 herstartte 4× door de interrupt-watchdog (5–7/11); de defecte TSL2561 op het testbord is de eerste verdachte |
| v3.4 | **SCD41** — details hieronder | De sensor zit al op room80 (9/11) |
| v3.6 | **WiFi robuuster:** geen modem-slaap (`WiFi.setSleep(false)`) en periodiek opnieuw verbinden. **mDNS terug:** `<kamer>.local`, enkel na een geldige WiFi-verbinding | Les uit de PWM-test (onbereikbaar in de kelder); rooms vinden op naam |

#### v3.4 SCD41 (naar voren gehaald 9/11)

Sensirion SCD41 op de I2C-bus bij de controller (SDA IO13, SCL IO11, adres 0x62). Hardware en kalibratie: `docs/aansluitingen.md` §6.1.
- **Niet-blokkerend:** periodieke meting in de sensor zelf; de sketch leest om de 5 s als er een nieuwe waarde klaarstaat. `pulseIn()`, `readCO2()` en `CO2_PWM` (IO18) verdwijnen. IO18 wordt vrij.
- **CO2** blijft in `/json` key `k` en stuurt de ventilatie zoals nu (400–800 ppm → 0–100 %).
- **Temperatuur en vocht van de SCD41** komen **apart** van die van de DHT22, in de nieuwe keys **`al`** (SCD-temp) en **`am`** (SCD-vocht). De DHT22 blijft vocht en dauwpunt leveren (`e`, `h`); de SCD-waarden zijn extra in de UI en de Sheet.
- **`/settings`:** SCD41 aan/uit (vervangt "CO2 aan/uit"), **ASC aan/uit** (aan in kamers die gelucht worden, uit in afgesloten ruimtes), **temperatuuroffset**, knop **"kalibreren in buitenlucht"** (FRC op ±425 ppm).
- **Geen sensormetingen in AP-mode:** `/settings` reageert vlot bij de eerste installatie.
- **GAS v1.6** (`ROOM_GoogleScript.gs`) samen met **v3.5 (opruimen)**, direct na de 24 u-test van v3.4.3 (beslist 9/11), met een eigen GO: kolommen voor `aj` (DS4, ontbreekt in v1.5), `ak`, `al`, `am`, `an`, `ao` en `ap`–`as` (generieke I/O, gevuld vanaf v3.9) erbij, Stof (`l`) en Beam (`aa`, `ab`) eruit. Het script leest per key, dus oudere sketches loggen gewoon verder. Daarna `setupHeaders()` één keer uitvoeren en "Implementaties beheren → potlood → nieuwe versie" (nooit een nieuwe implementatie). Oude rijen in de Sheet hebben de oude kolomindeling: archiveren in een apart tabblad.

#### v3.5 Opruimen

De stofsensor (Sharp, IO12/IO7) en de lichtstraal (LDR2 op IO2) volledig weg: code, instellingen (`dust_en`, `beam_en`, `beam_thresh`), UI en de `/json`-keys `l`, `aa` en `ab`. Het Dashboard en de RPi gebruiken die keys niet (nagekeken 5/11). (De TSL2561-code is al weg sinds v3.3.) IO2, IO7 en IO12 komen vrij voor de generieke I/O (v3.9).


### Fase B — Functies

#### v3.7 Temperatuurbewaking (zoals de Sjalay v1.4–v1.6)

**Waarom.** Nu valt de ROOM bij een DS18B20-fout zonder meer terug op de DHT22, zonder foutentellers en zonder te kijken hoe oud de laatste DHT22-meting is. Valt de DHT22 weg, dan worden vocht en dauwpunt 0. De verwarmingsvraag kan zo op een verkeerde temperatuur gebaseerd zijn.

**DS18B20 (T-bus)**
- Per sensor een **foutenreeks**: CRC-fout of waarde buiten −55…125 °C telt als fout. Na een aantal fouten op rij (Sjalay: 3) geldt de sensor als **"ontbreekt"**; bij de volgende goede lezing is hij terug.
- De **opstartwaarde 85,0 °C** van een DS18B20 die net stroom krijgt, wordt genegeerd (geen fout).
- **Totaal fouten** per sensor bijhouden voor de diagnose.

**DHT22**
- Bij een mislukte lezing blijven temperatuur en vocht op de **laatste geldige waarde** staan (vocht en dauwpunt lopen dus niet naar 0).
- **Foutenreeks en totaal fouten**; na **5 fouten op rij** wordt de DHT22 opnieuw gestart (`dht.begin()`).
- **Leeftijd** van de laatste geldige meting wordt bijgehouden.
- Geen power cycle met een relais (de rooms hebben geen derde relais, anders dan de Sjalay).

**Kamertemperatuur: bron en melding**

| Toestand | Kamertemperatuur | Melding | Kleur |
|---|---|---|---|
| Primaire DS18B20 in orde | DS18B20 (bijnaam) | "DS18B20 (kamer) actief" | groen |
| DS18B20 ontbreekt of defect, DHT22 jonger dan 5 min | DHT22 | "DS18B20 defect – DHT22 actief (terugval)" | oranje |
| Ook de DHT22 te oud of defect | geen | "Beide sensoren defect – verwarming geblokkeerd" | rood |

- Bij "geen betrouwbare sensor" vraagt de room **geen verwarming** (`b` = 0), ook niet via de thermostaat-logica van v3.8.
- `/advanced` toont **bron**, **melding** (in kleur), DHT22-status (fouten op rij, totaal, herstarts, leeftijd laatste meting).

**Overzicht alle DS18B20's** (zoals de Sjalay)
- Op `/advanced` een tabel **"Alle DS18B20-sensoren"**: aantal gevonden, per sensor bijnaam, rol (kamer = primair), temperatuur, en **"ontbreekt"** bij een foutenreeks.
- Bestaat al en blijft: bijnamen, keuze primaire sensor en "opnieuw scannen" in `/settings`, CRC-controle.
- **Maximum van 4 naar 6 sensoren** (beslist 5/11): JSON `f` (primair), `ah`–`aj` (2–4) en nieuw **`an`, `ao`** (5–6). Bijnamen en primaire keuze ook voor sensor 5 en 6.

**`/json`:** nieuwe keys **`an`, `ao`** (DS18B20 nr. 5 en 6). `e` (DHT22-temp) en `h` (vocht) tonen voortaan de laatste geldige waarde in plaats van 0.

#### v3.8 Verwarmingslogica zoals de Sjalay (v1.4, v1.5, v1.8)

**Waarom.** Nu: effectief setpoint = max(setpoint, dauwpunt + marge) met een marge van 0,5–5 °C, geen aan/uit, en de verwarming schakelt op een vaste −0,5 °C zonder echte band (kan pendelen rond het setpoint). De landingspagina toont alleen het ingestelde setpoint; de schuifbalk begint vast op 10 °C.

**Regeling** (software-thermostaat)
- **Effectief setpoint** = dauwpuntbeveiliging aan ? max(setpoint, dauwpunt + marge) : setpoint.
- **Echte hysterese**: verwarming AAN onder effectief setpoint − hysterese/2, UIT boven effectief setpoint + hysterese/2; ertussen blijft de stand behouden.
- **Veiligheid, altijd actief:** UIT zodra de kamertemperatuur het maximum (30 °C) bereikt; UIT als er geen betrouwbare kamertemperatuur is (v3.7).

**ROOM-uitzondering: Thuis + thermostaatcontact.** Staat de room op Thuis en is het thermostaatcontact ingeschakeld, dan volgt `b` de thermostaat aan de muur (zoals nu), **met dauwpuntbeveiliging** (beslist 5/11): `b` = thermostaat vraagt **of** kamer onder dauwpunt + marge (met hysterese), ook als de thermostaat uit staat. Alleen als de dauwpuntbeveiliging aan staat. De veiligheden (30 °C, geen betrouwbare sensor) gaan altijd voor.

**`/settings`** (nieuw of gewijzigd)

| Instelling | Bereik | Standaard |
|---|---|---|
| Dauwpuntbeveiliging | aan / uit | aan |
| Dauwpuntmarge | −10 … +10 °C (negatief = vochtrisico voor eigen rekening) | 2,0 |
| Hysterese | 0,2 … 5,0 °C (zoals de Sjalay) | 1,0 |
| Minimum setpoint (onderkant van de schuifbalk) | 2 … 25 °C (zoals de Sjalay) | 10 (= de huidige onderkant; de Sjalay gebruikt 5) |

Wordt het minimum boven het opgeslagen setpoint gezet, dan wordt het setpoint mee opgetrokken (zoals de Sjalay).

**Landingspagina**
- Groot de gemeten kamertemperatuur, daarachter kleiner het **echte doel**: bv. "21,3° (→ 18°)". Het doel komt live uit `/json` `ak`; tijdens het verschuiven toont de schuifbalk meteen de nieuwe waarde.
- De schuifbalk begint bij het **minimum setpoint**.
- Als de dauwpuntcorrectie het doel optrekt, een korte uitleg ("verhoogd tegen condensatie").

**`/advanced`:** dauwpunt, effectief setpoint, hysterese (± waarde), bron van de kamertemperatuur.

**`/json`:** nieuwe key **`ak` = effectief setpoint**. `c` blijft het ingestelde setpoint (de schuifbalk), `j` (dauwpunt-alert) blijft zoals hij is (het Dashboard gebruikt hem).

#### v3.9 Generieke I/O (GP1–GP4)

De vrije pinnen van RoomSense en Option worden algemeen bruikbaar, zonder de sketch aan te passen. Volledige beschrijving: `docs/ROOM_STANDAARD.md` §1.4.

| Naam | Pin | Mogelijk |
|---|---|---|
| GP1 | RoomSense 4 = IO12 | digitaal in / uit / PWM |
| GP2 | RoomSense 5 = IO7 | digitaal in / uit / PWM |
| GP3 | Option 3 = IO2 | digitaal in / uit / PWM, analoog in |
| GP4 | Option 4 = IO18 | digitaal in / uit / PWM |

- **`/settings`:** per pin modus (uit · digitale ingang · analoge ingang (alleen GP3) · digitale uitgang · PWM-uitgang), bijnaam, pull-up. Standaard: uit.
- **`/advanced`:** per pin bijnaam, modus en waarde; uitgangen met een knop of schuifbalk.
- **`/json`:** keys `ap`–`as` (beslist 7 nov 2026).
- **Bediening van uitgangen:** nieuw adres `/set_gp?n=1..4&v=0..100` (beslist 7 nov 2026): zo kunnen ook de RPi, Apple Home of het Dashboard een vrije pin schakelen of dimmen. `v` = 0 uit, 100 volledig aan, daartussen PWM. Een pin die niet als uitgang ingesteld is, negeert de opdracht.
- **Ingangen:** digitaal (0/1, met of zonder pull-up) op alle vier; **analoog alleen op GP3** (IO2, ADC), weergegeven als 0–100 %. Voor een LDR of spanningsdeler op GP3 een vaste weerstand (bv. 10 kΩ naar 3V3) voorzien; maximum 3,3 V op de pin.
- **Veilige opstart:** elke GP-pin start als ingang zonder pull-up; een uitgang wordt pas actief na het laden van de instellingen.

### Fase C — Pagina's apart en opgefrist

**Waarom apart.** De landingspagina staat sinds v3.1 in een eigen bestand (`landing.h`). De andere pagina's (`/advanced`, `/settings`, `/update`) worden nog stukje per stukje in de `.ino` opgebouwd. Elke pagina in een eigen `.h`-bestand geeft overzicht (de `.ino` bevat enkel nog logica) en voorkomt dat de Arduino IDE het JavaScript in een `.ino` beschadigt. Geheugen win je er niet mee.

**Hoe.** Vaste HTML + JavaScript; live-waarden uit `/json`, vaste waarden (versie, IP, instellingen) uit een klein JSON-adres dat alleen bij het laden gelezen wordt. Bij elke verhuis meteen drie opfrissingen:
1. **Live en vaste gegevens scheiden**, live-waarden via een vaste `id` per veld (nu zoekt het script de rij via de labeltekst).
2. **Statuscirkels** zoals de Sjalay en de landingspagina (wit = rust, lichtrood met rode rand = actief).
3. **Korte, eenduidige labels**, consequent in het Nederlands.

| Versie | Pagina | Opmerking |
|---|---|---|
| v3.10 | `/update` → `ota.h` | Klein: eerste test van de aanpak. Met voortgangsbalk en uitleg over het juiste `.bin`-bestand |
| v3.11 | `/advanced` → `advanced.h` | Grootste winst in onderhoudbaarheid |
| v3.12 | `/settings` → `settings.h` | Grootste en meest vervlochten pagina, daarom als laatste |

### Fase D — Later

| Versie | Wat |
|---|---|
| v3.13 | **LDR-schaal gelijk met de Sjalay:** 0 = donker, 100 = fel; één instelbare nachtdrempel. Raakt ook het Dashboard en de Sheet: samen aanpassen |
| — | **Uitrol:** per kamer een ESP32 met deze sketch in plaats van de Photon; instellingen via `/settings`; RPi-bridge en Dashboard krijgen de nieuwe room erbij |

### Beslist (5 nov 2026)

| # | Vraag | Beslissing |
|---|---|---|
| 1 | Effectief setpoint `ak` ook loggen in de Sheet? | **ja** (GAS v1.6) |
| 2 | Dauwpuntbeveiliging ook bij Thuis + thermostaatcontact? | **ja** (v3.8) |
| 3 | Maximum aantal DS18B20's per room: 4 of 6? | **6**, nieuwe keys `an`, `ao` (v3.7) |

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.4.3** | 9 nov 2026 | **Ventilatie op CO2 instelbaar** in `/settings`: "0 % bij" (standaard 600 ppm) en "100 % bij" (standaard 1200 ppm), rechtlijnig daartussen (was vast 400–800 ppm: te hevig). NVS `co2_lo`, `co2_hi`. `/json` ongewijzigd |
| **v3.4.2** | 9 nov 2026 | **I2C op IO13 werkt:** IO12/IO13 zijn op de ESP32-C6 de ingebouwde USB (D−/D+); die hield IO13 vast, waardoor de SCD41 (en vroeger de TSL2561) nooit antwoordde. Nu gecompileerd met USB CDC On Boot = Disabled en bij het opstarten de USB-functie van IO12/IO13 uitgeschakeld. Serieel en flashen via de poort "UART". Shield ongewijzigd |
| **v3.4.1** | 9 nov 2026 | Diagnose SCD41: bij "ontbreekt" toont `/settings` → Diagnose de mislukte stap, de foutmelding van de bibliotheek en een **I2C-scan** (welke adressen antwoorden). Aanleiding: SCD41 op room80 antwoordde niet na de flash van v3.4. Verder ongewijzigd |
| **v3.4** | 9 nov 2026 | SCD41 via I2C (IO13/IO11, Sensirion-bibliotheek, niet-blokkerend: elke 5 s), CO2 via PWM weg (IO18 vrij). Nieuwe keys `al` (SCD-temp) en `am` (SCD-vocht) achteraan; `k` = CO2 van de SCD41. `/settings`: SCD41 aan/uit, ASC, temperatuuroffset, kalibreren in buitenlucht (FRC 425 ppm); Diagnose: SCD41-toestand. Geen sensormetingen (en geen verwarmingsvraag) in AP-mode. 1,21 MB |
| **v3.3** | 7 nov 2026 | Stabiliteit: TSL2561 volledig weg (bibliotheek, I2C-start, instelling "Zonlicht", UI; key `n` = 0). "Laatste stap" in RTC-geheugen (loop en webserver apart); na een herstart in NVS bewaard en in `/settings` → Diagnose getoond ("Laatste stap ervoor", "Vorige herstarts": laatste drie, met reden en uptime ervoor). Gecompileerd met core 3.3.12: 25 KB kleiner dan v3.2. Geen nieuwe `/json`-keys |
| **v3.2** | 5 nov 2026 | Versie en diagnose zichtbaar: `ROOM_VERSION` op elke pagina en bij het opstarten; in `/settings` een blok Diagnose (uptime, reden laatste herstart, aantal opstarts, geheugen, WiFi). "Crashteller" heet nu "Lage-heap meldingen". Geen nieuwe `/json`-keys |
| v3.1 | 3 nov 2026 | Landingspagina `/` (landing.h); technische statuspagina naar `/advanced`; 🏠 in het menu |
| v3.0 | 3 nov 2026 | Matter verwijderd; Apple Home via de RPi-bridge. Verder ongewijzigd (JSON-keys, endpoints, NVS) |
| v2.24 | 8 mei 2026 | Gedeelde CSS-helper (heap), versieregel op serial verwijderd |
| v2.22 | 1 mei 2026 | `/capabilities` voor de verlichting in de Portal |
| v2.21 | 13 apr 2026 | Thuis/Weg vereenvoudigd (heating_mode/vent_mode weg) |
| v2.20 | 13 apr 2026 | Stabiliteit: crash-log in NVS, CO2-timeout 50 ms |
| v2.8 | mrt 2026 | Laatste versie vóór Matter (in `old/`) |

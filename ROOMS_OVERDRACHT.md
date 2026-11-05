# Overdracht — project "Rooms" (Zarlar ROOM-controllers)

*Stand 5 november 2026, 22:15 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een nieuw gesprek over de **ROOM-sketch** en de overstap van de Photon-rooms naar ESP32-C6. Het vat samen wat er tot nu toe (HVAC-control-gesprekken tot 5/11) beslist en gebouwd is.

---

## 1. Werkwijze (altijd)

- **Nederlands.** ESP32-C6 in de **Arduino IDE**.
- **Nooit een sketch of script maken zonder akkoord vooraf** (GO). Eerst een kort plan, dan code.
- **Eén stap per versie**, met **versiekop + changelog** in elke sketch, en daarna een **48 u-test** (Sheet: geheugen en herstarts).
- **README's bijwerken** bij elke stap: beknopt, logisch, geen achterhaalde info. Ook het Master-document en het Portal Plan bijhouden.
- **`/json`-keys en bedieningsadressen niet wijzigen** zonder uitdrukkelijk akkoord: het Dashboard, de Portal, de RPi-bridge en de Google Sheets hangen ervan af.
- **Logs lezen:** via de Google Drive-koppeling (`download_file_content`, export `text/csv`, base64 decoderen; volledige Sheet) of FiDel plakt de deellink. Let op: in de export staat de uptime met punten als duizendtal-scheiding (`99.000`).

## 2. GitHub-repos (FidelDworp)

| Repo | Inhoud |
|---|---|
| [`ESP32C6_ROOMS`](https://github.com/FidelDworp/ESP32C6_ROOMS) | **ROOM-sketch** (`TESTROOM/ESP32_C6_ROOM.ino` + `landing.h`), `ROOM_GoogleScript.gs`, `partitions.csv`, README met het **Verbeterplan** |
| [`PHOTON_ROOMS`](https://github.com/FidelDworp/PHOTON_ROOMS) | Oude Photon-sketches die nog in huis draaien: R1-BandB, R2-BADK, R3-INKOM, R4-KEUK, R5-WASPL, R6-EETPL, R7-ZITPL, R8-ACCESS (+ S-HVAC, S-ECO_SOLAR, S-OUTSIDE, archief) |
| [`ESP32C6_HVAC`](https://github.com/FidelDworp/esp32c6_hvac) | HVAC-controller (`ESP32C6_HVAC/ESP32C6_HVAC.ino`), v2.0 zonder Matter, AnaBoX-ventilatie |
| [`ESP32C6_ECO-boiler`](https://github.com/FidelDworp/ESP32C6_ECO-boiler) | ECO-boiler-controller, v2.0 zonder Matter (`Productie/ESP32-C6_ECO-boiler/`), `ECO_GoogleScript.gs` |
| [`ZarlarApp`](https://github.com/FidelDworp/ZarlarApp) (lokaal en als push-doel: `Zarlardinge_Dashboards`) | `ESP32_Zarlar/`: **Master-overnamedocument**, Dashboard-sketch, `zarlar-rpi/` (**server.js**, plugin **matterbridge-zarlar**, `Zarlar_Portal_Plan.md`, `public/`) |
| [`SJALAY-YSERMONDE`](https://github.com/FidelDworp/SJALAY-YSERMONDE) | Sjalay-controller (vakantiehuis Recht) v1.8: bron van de "goodies" voor ROOM v3.4/v3.5 |
| [`HVAC_WONING`](https://github.com/FidelDworp/HVAC_WONING) | Woning HVAC (Maarten): `ESP32_PWM_Test.ino` + `pagina.h` v1.5, AnaBoX 0–10 V voor de Begetube 350 R |
| [`ESP32C6_Smart-Energy`](https://github.com/FidelDworp/ESP32C6_Smart-Energy) | S-ENERGY-controller (HomeWizard), post niet zelf naar Google |

**Nog te pushen** (gemaakt op 5/11, nog niet in de repo's):
- `ESP32C6_ROOMS`: v3.2 (`ESP32_C6_ROOM.ino`, `landing.h`) in `TESTROOM/`, `README.md` (root, met het nieuwe plan), `README_ROOM_ACCESS_aansluitingen.md` en `Zarlar_Photon_inventaris_v3.xlsx`. Op GitHub stond op 5/11 nog v2.24 (met Matter).
- `ESP32C6_ECO-boiler`: v2.0 + nieuwe README, oude bestanden opruimen (`CHANGELOG.md`, `Productie/-Zarlar_Overnamedocument.md`, `Productie/partitions.csv` naar de sketchmap, Photon-bestand naar `old/`).
- `ZarlarApp`: Master-document (met §17 Optimalisatieplan) en Portal Plan, als FiDel ze nog niet pushte.

## 3. Het systeem in het kort

| Controller | Adres | Sketch | Stand 5/11/2026 |
|---|---|---|---|
| **room80** (TESTROOM-testbord, nu in de eetplaats) | 192.168.0.80 | ROOM | **v3.2**, geflasht 5/11 ±22:00, 48 u-test loopt |
| HVAC | 192.168.0.70 | ESP32C6_HVAC | v2.0, zonder Matter, stabiel (239–251 KB) |
| ECO-boiler | 192.168.0.71 | ESP32-C6_ECO-boiler | v2.0, zonder Matter, 48 u-test liep af 5/11 22:15 (op 5/11 20:00: 45,8 u zonder herstart, 251 KB) |
| Dashboard (matrix) | 192.168.0.60 | Dashboard | v6.3, zonder Matter; pollt alle `/json` en **logt elke 5 min naar Google Sheets** (controllers posten zelf niets) |
| RPi (portal + Matter-bridge) | 192.168.0.50:3000 | server.js v2.11 + plugin matterbridge-zarlar v0.8 | Apple Home woning **Zarlardinge**: knoppen, room80 ("Eetplaats"), HVAC, ECO-boiler |
| Photon-rooms | — | PHOTON_ROOMS | nog in productie, behalve de B&B (defect) |

- **Geen enkele ESP32 heeft nog Matter.** Testwoning "Matter Home" is verwijderd. Matter kostte ±60 KB; zonder: grootste blok ±240–250 KB.
- **Sheets:** R-ROOMS Log `1UiaexdpgpL3XjPWHNdMEbJ18_H9yp1w_VN8VBdVCpY0` · S-ECO `1nn_GBmLhS9zZhp5On_3r-LRLonetSjnHCuHZiNOc4Q0` · S-HVAC `1LL7swNRIVl2qfREHxAU7nCq9fH9zTUxa37s0IduHTXY`.

## 4. ROOM-sketch: stand en plan

**Versies:** v3.0 Matter weg · v3.1 landingspagina (`landing.h`), technische pagina op `/advanced` (stabiel getest: 68 %, 247 KB, herstarts alleen door werk aan het bord) · **v3.2** versie (`ROOM_VERSION`) op elke pagina + blok Diagnose in `/settings` (herstartreden, opstarts, geheugen, WiFi).

**Verbeterplan** (volledig in de README van `ESP32C6_ROOMS`; beslist: **eerst functies, dan pagina's**):

| Versie | Wat |
|---|---|
| v3.3 | WiFi robuuster (geen modemslaap, periodiek opnieuw verbinden) + mDNS `<kamer>.local` |
| v3.4 | **Temperatuurbewaking zoals de Sjalay:** DS18B20-foutenreeks (3 → "ontbreekt"), 85 °C negeren, DHT22 laatste geldige waarde + herstart na 5 fouten, terugval alleen < 5 min oud, melding groen/oranje/rood, verwarming geblokkeerd zonder betrouwbare sensor; tabel "Alle DS18B20-sensoren" |
| v3.5 | **Verwarmingslogica zoals de Sjalay:** effectief setpoint = max(setpoint, dauwpunt + marge), dauwpunt aan/uit, marge −10…+10, hysterese 0,2–5,0 (± band), minimum setpoint (onderkant schuifbalk, standaard 10), landingspagina "IST (→ SOLL)", nieuwe key `ak` |
| v3.6 | **SCD41** (I2C, CO2 + temp + vocht), CO2-PWM weg, keys `al`/`am`, ASC/offset/FRC in `/settings`, **GAS v1.6** → daarna **B&B live** |
| v3.7 | Stofsensor en lichtstraal volledig weg (ook keys `l`, `aa`, `ab`) |
| v3.8 | Buitenlicht oost (B&B, PWM op IO2) |
| v3.9 | Pixelklok (ZITPL) |
| v3.10–v3.12 | Pagina's naar `ota.h`, `advanced.h`, `settings.h` |
| v3.13 | LDR-schaal 0 = donker (zoals Sjalay), samen met Dashboard en Sheet |

**Nog te beslissen (FiDel):**
1. `ak` (effectief setpoint) ook loggen in de Sheet? (voorstel: ja)
2. Dauwpuntbeveiliging ook bij "Thuis + thermostaatcontact"? (nu volgt de room dan de wandthermostaat)
3. Maximum DS18B20's per room: 4 (zoals nu, keys `f`, `ah`–`aj`) of 6?

**Bekende `/json`-keys ROOM** (v3.2): a uptime · b heating_on · c setpoint · d tstat · e temp DHT · f temp DS primair · g vent % · h vocht · i dauwpunt · j dauwpunt-alert · k CO2 · l stof · m LDR · n zon (lux) · o nacht · p bed · q/r/s RGB · t pixels aan · u pixelmodus · v thuis · w/x MOV1/2 triggers · y/z MOV1/2 licht · aa/ab beam · ac RSSI · ad heap % · ae grootste blok · af laagste heap · ag DS-aantal · ah–aj extra DS. Gepland: ak, al, am.

## 5. Hardware: van Photon naar ESP32 (per room)

Volledig in `README_ROOM_ACCESS_aansluitingen.md` (v0.4) en `Zarlar_Photon_inventaris_v3.xlsx`.

- **Regel 1:** RoomSense-kant op **3,3 V** (shield-jumper Sense pin 8 op 3V3); elke RoomSense-box ombouwen zoals de B&B (AM312-PIR, DHT22 en LDR op 3V3).
- **Regel 2:** Option-kant mag 5 V voeden (MOV2 = HC-SR501, uitgang 3,3 V), maar **geen signaal boven 3,3 V**: elke ader meten vóór aansluiten.
- **Geen eigen printje meer** voor de rooms. Alleen een **buffer 3,3 → 5 V** (74AHCT1G125 of 74HCT125) voor de LED-modules buiten: 4× ACCESS, 1× B&B (PWM-E).
- **CO2:** standaard wordt de **Sensirion SCD41** (GY-SCD41, I2C, 3,3 V) bij de controller, op de I2C-bus met de TSL2561 (IO13/IO11). 4 stuks besteld (5/11). Plaatsen: B&B, BADK, ZITPL. Bij ontvangst: draadkleuren doormeten, 15 min buitenlucht (420–430 ppm, anders FRC), ASC per kamer.
- **Vervalt:** stofsensor, alert-LDR, CO2-PWM (CM1106), de 2 relais van R7 (worden Powerpixels), TSL2561 is optioneel (die op het testbord is defect).
- **Eerste room: B&B** (Photon defect): MOV2 meten, SCD41, buffer PWM-E + kabel naar de LED-modules oost (de oostkant gaat van ACCESS naar de B&B).
- **ACCESS (R8, buitenlicht):** blijft een aparte controller met 4 zones (W1, W2, S, N1), nieuwe sketch `ESP32_C6_ACCESS` later.

## 6. Andere open punten (buiten de ROOM-sketch)

- **ECO v2.0:** resultaat 48 u-test bekijken (afgelopen 5/11 22:15), meting in de ECO-README, repo opruimen. Later ECO v2.1: dode `publishHVAC()` weg, mDNS.
- **HVAC v2.1:** eigen Google Sheets-push weg (de Dashboard logt al).
- **Dashboard:** HOME/UIT-tegel via de RPi (`/home_json`, `/set_home_global`), nieuwe matrixlegende, Master bijwerken voor v6.1–v6.3.
- **Master §17 Optimalisatieplan:** fase 1 afronden, fase 2 opruimversies + mDNS overal, fase 3 statuspagina op de RPi, fase 4 ROOM-uitrol.

## 7. Praktisch

- **Compileren:** board ESP32C6 Dev Module, **Flash 16 MB**, Partition Custom met `partitions.csv` in de sketchmap. JavaScript altijd in een `.h`-bestand met raw string (de IDE beschadigt JS in een `.ino`).
- **OTA:** *Sketch → Export Compiled Binary*, alleen `<sketch>.ino.bin` uit `build/esp32.esp32.esp32c6/` uploaden via `/update`.
- **RPi deployen** (Mac-terminal): bestanden in `~/Downloads`, dan `bash ~/deploy.sh "omschrijving"` (repo, git push, RPi update). Plugin: tar.gz-procedure uit het Portal Plan.
- **Google Apps Script bijwerken:** altijd "Implementaties beheren → potlood → nieuwe versie", nooit "nieuwe implementatie" (andere URL). Na kolomwijzigingen `setupHeaders()` één keer uitvoeren.

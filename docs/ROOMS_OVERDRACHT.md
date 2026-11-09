# Overdracht — project "Rooms" (Zarlar ROOM-controllers)

*Stand 7 november 2026, 16:30 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een **nieuw gesprek met Claude** over de ROOM-sketch en de overstap van de Photon-rooms naar ESP32-C6. Het vat samen hoe we werken, wat er beslist en gebouwd is, en wat nog openstaat. Overkoepelend (alle repo's, systeem, compileren in de sessie): [`ZARLAR_OVERDRACHT.md`](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md). Wie het systeem wil begrijpen, begint beter bij de [README](../README.md) en de [ROOM-standaard](ROOM_STANDAARD.md).

---

## 1. Werkwijze (altijd)

- **Nederlands.** ESP32-C6 in de **Arduino IDE**.
- **Nooit een sketch of script maken zonder akkoord vooraf** (GO). Eerst een kort plan, dan code. **Eén stap per keer**, met feedback ertussen.
- **Eén stap per versie**, met **versiekop + changelog** in elke sketch, en daarna een **48 u-test** (Sheet: geheugen en herstarts).
- **Alles via OTA**: diagnose en updates zonder seriële kabel.
- **README's bijwerken** bij elke stap: beknopt, logisch, geen achterhaalde info. Ook het Master-document en het Portal Plan bijhouden.
- **`/json`-keys en bedieningsadressen niet wijzigen** zonder uitdrukkelijk akkoord: het Dashboard, de Portal, de RPi-bridge en de Google Sheets hangen ervan af.
- **Logs lezen:** via de Google Sheets-koppeling (`get_values`, tab `Data`, oudste rij bovenaan, nieuwste onderaan). De uptime staat met punten als duizendtal-scheiding (`99.000`).
- **Bij twijfel over IP's, namen of verschillen tussen documenten: eerst met FiDel bespreken**, dan pas aanpassen.

### Werkwijze GitHub

- Claude koppelt de repo's van FidelDworp met **schrijfrechten** aan de sessie (`add_repo`, access `push`) en werkt docs, sketches en mappen **rechtstreeks** bij: één commit per stap, vóór het pushen eerst `fetch` + `rebase` (FiDel commit soms zelf tussendoor).
- **Indeling van elke repo:** `README.md` in de root; **alle andere documenten in `docs/`**; sketchmap `<SketchNaam>/` (ino + `.h` + `partitions.csv`); `GoogleScript/`; `bin/` (laatste gecompileerde `.bin`); `old/`.
- **Elke repo heeft een overdracht** `docs/<NAAM>_OVERDRACHT.md` voor een nieuw gesprek (afspraak 7/11). Claude houdt die bij.
- Een **nieuwe repo** maakt FiDel zelf aan (leeg, public); Claude vult hem daarna.
- **Niet mogelijk voor Claude:** repo-beschrijving ("About") en andere instellingen, GitHub-releases. Dat doet FiDel.
- **Na GO:** sketch in de repo, gecompileerd in de sessie (zie §8), de `.bin` in `bin/` (vorige versie weg) en een zip van de sketchmap naar FiDel. FiDel uploadt via `/update`. Photon-sketches: FiDel flasht via de Particle Web IDE.

## 2. GitHub-repos (FidelDworp)

**Overkoepelend:** [`Zarlardinge-Home-Automation`](https://github.com/FidelDworp/Zarlardinge-Home-Automation): `README.md` = overzicht van alle controllers, `docs/ARCHITECTUUR.md`, `docs/UITLEG_VOOR_MAARTEN.md`, `ESP32_Zarlar/` (Master, Dashboard-sketchmap, `zarlar-rpi/`). Lokale map op de Mac blijft `~/Zarlardinge_Dashboards` (deploy.sh).

| Repo | Inhoud |
|---|---|
| `ESP32C6_ROOMS` | `ESP32_C6_ROOM/`, `bin/`, `docs/ROOM_STANDAARD.md` (v1.1), `docs/ROOMS_OVERDRACHT.md` (dit document), `docs/aansluitingen.md` |
| `ESP32C6_HVAC` | `ESP32C6_HVAC/` |
| `ESP32C6_ECO-boiler` | `ESP32-C6_ECO-boiler/` |
| `ESP32C6_Smart-Energy` | `ESP32_C6_ENERGY/`, `Tesla/TeslaChargeTest/` |
| `ESP32C6_ACCESS` | `ESP32_C6_ACCESS/` (nog te ontwerpen) · `old/R8-ACCESS.ino` |
| `SJALAY-YSERMONDE` | `ESP32-C6_SJALAY_CONTROLLER/` |
| `HVAC_WONING` | `ESP32_PWM_Test/` |
| `PHOTON_ROOMS` | Particle-build; R8-ACCESS kreeg 6/11 `JSON_status` |
| `ESP32C6_Experiments` | elke test in een eigen map, Matter-sims in `old/` |

**Open (Claude):** S-ENERGY-kop noemt nog `partitions_16mb.csv` → rechtzetten bij v1.31.

## 3. Het systeem in het kort

**Eén netwerk** achter de Telenet-router: woning en schuur zien elkaar rechtstreeks. Telenet laat **geen DHCP-reservering** toe: vaste IP's worden in de controller zelf ingesteld. Bereik **.50–.84 is voor de schuur**.

| Controller | Adres | Sketch | Stand 7/11/2026 |
|---|---|---|---|
| **room80** (experimenteerbord, op tafel in de eetplaats) | 192.168.0.80 | ROOM | **v3.3**, geflasht 7/11 ±16:00; **48 u-test geslaagd** (50 u 47 min zonder herstart, heap 67–68 %, grootste blok 239–255 KB). 9/11 18:48 herstart = stroom eraf door FiDel om de **SCD41** (I2C, 3,3 V) aan te sluiten. TSL2561 weg |
| HVAC (testbord slaapkamer) | 192.168.0.70 | ESP32C6_HVAC | v2.0 |
| ECO-boiler (bordje slaapkamer) | 192.168.0.71 | ESP32-C6_ECO-boiler | v2.0 |
| Smart Energy (bordje slaapkamer) | 192.168.0.73 · `smart-energy.local` | ESP32_C6_ENERGY | v1.30 (6/11), 48 u-test loopt |
| Tesla-proxy (wasplaats) | DHCP · `tesla-key-esp32.local` | firmware 0Bu/tesla-key-esp32 | S-ENERGY vindt hem op naam |
| Dashboard (matrix, zitplaats) | 192.168.0.60 | Dashboard | v6.4 (6/11), in test. Logt elke 5 min naar Google Sheets |
| RPi (portal + Matter-bridge, zitplaats) | 192.168.0.50:3000 | server.js v2.12 + matterbridge-zarlar v0.8 | Apple Home woning Zarlardinge |
| Photon-rooms | — | PHOTON_ROOMS | nog in productie, behalve de B&B (defect) |

**IP-plan:** .50 RPi · .60 Dashboard · .70 HVAC · .71 ECO · .72 S-OUTSIDE (alleen een idee, adres gereserveerd) · .73 S-ENERGY · .74 vrij · .75 B&B · .76 Badkamer · .77 Inkom · .78 Keuken · .79 Wasplaats · .80 Eetplaats (room80) · .81 Zitplaats · .82/.83/.84 ACCESS1/2/3.

**Dashboard-matrix:** rij 0 HVAC · 1 ECO · 2 S-ENERGY · 3 ACCESS (W·Z·N·O) · 4 scheiding · 5–11 rooms · 12–15 leeg.

- **Geen enkele ESP32 heeft nog Matter.** Zonder Matter: grootste blok ±240–250 KB.
- **Sheets:** R-ROOMS Log `1UiaexdpgpL3XjPWHNdMEbJ18_H9yp1w_VN8VBdVCpY0` · S-ECO `1nn_GBmLhS9zZhp5On_3r-LRLonetSjnHCuHZiNOc4Q0` · S-HVAC `1LL7swNRIVl2qfREHxAU7nCq9fH9zTUxa37s0IduHTXY`.

## 4. ROOM-sketch: stand en plan

**Versies:** v3.0 Matter weg · v3.1 landingspagina · v3.2 versie + Diagnose in `/settings` · **v3.3** (7/11, geflasht ±16:00): TSL2561 weg, "laatste stap vóór herstart" in Diagnose. Eerste opstart na de OTA had nog geen stap (v3.2 noteerde er geen); "Lage-heap meldingen" gewist.

**48 u-test v3.2 (5/11 21:45 → 7/11 15:05): niet gehaald.** 4 herstarts (6/11 17:10 na 19,4 u; 7/11 06:31 na 13,3 u; 07:16; 07:29), reden **"Watchdog (interrupt)"**. Geheugen in orde (grootste blok 239–247 KB, laagste 225–228 KB), RSSI −54…−62 dBm. Verdachten: **defecte TSL2561** (I2C), DHT22-uitlezing (zet interrupts uit), NeoPixel + WiFi. "Lage-heap meldingen 59" is een oude teller in NVS.

**Na het flashen van v3.3:** in `/settings` → Diagnose staan "Laatste stap ervoor" en "Vorige herstarts" (laatste drie: reden, uptime ervoor, laatste stap in loop en web). Daarmee de 48 u-test beoordelen.

**Verbeterplan** (volledig in de README; eerst functies, dan pagina's):

| Versie | Wat |
|---|---|
| **v3.3** | Stabiliteit: TSL2561 weg, laatste stap vóór herstart. **Geflasht 7/11, 48 u-test geslaagd (9/11)** |
| v3.4 | WiFi robuuster (geen modemslaap, periodiek opnieuw verbinden) + mDNS `<kamer>.local` (zoals S-ENERGY v1.30) |
| v3.5 | Temperatuurbewaking zoals de Sjalay; max 6 DS18B20's (keys `an`, `ao`) |
| v3.6 | Verwarmingslogica zoals de Sjalay (key `ak`; dauwpuntbeveiliging ook bij Thuis + thermostaatcontact) |
| v3.7 | SCD41 (keys `al`/`am`), CO2-PWM weg, GAS v1.6 (+ kolommen `aj`, `ak`, `an`, `ao`, `ap`–`as`) → B&B live |
| v3.8 | Stofsensor en lichtstraal weg (keys `l`, `aa`, `ab`) |
| v3.9 | Generieke I/O GP1–GP4 (IO12, IO7, IO2 = enige ADC, IO18), keys `ap`–`as`, bediening `/set_gp?n=1..4&v=0..100` |
| v3.10–v3.12 | Pagina's naar `ota.h`, `advanced.h`, `settings.h` |
| v3.13 | LDR-schaal 0 = donker, samen met Dashboard en Sheet |

**Beslist:** `ak` loggen (5/11) · dauwpuntbeveiliging ook bij Thuis + thermostaat (5/11) · max 6 DS18B20's (5/11) · keys `ap`–`as` en `/set_gp` (7/11) · TSL2561 nooit meer op een room, wel op ACCESS/OUTSIDE (7/11).

**`/json`-keys ROOM** (v3.3): a uptime · b heating_on · c setpoint · d tstat · e temp DHT · f temp DS primair · g vent % · h vocht · i dauwpunt · j dauwpunt-alert · k CO2 · l stof · m LDR · n zon (altijd 0 sinds v3.3) · o nacht · p bed · q/r/s RGB · t pixels aan · u pixelmodus · v thuis · w/x MOV1/2 triggers · y/z MOV1/2 licht · aa/ab beam · ac RSSI · ad heap % · ae grootste blok · af laagste heap · ag DS-aantal · ah–aj extra DS 2–4. Gepland: an/ao (v3.5), ak (v3.6), al/am (v3.7), ap–as (v3.9). Dashboard en Portal gebruiken ag–aj niet; GAS v1.5 logt ag–ai, niet aj.

## 5. Hardware

Standaard: [`ROOM_STANDAARD.md`](ROOM_STANDAARD.md). Ombouw: [`aansluitingen.md`](aansluitingen.md) en `Zarlar_Photon_inventaris_v3.xlsx`.

- RoomSense-kant op **3,3 V**; Option-kant mag 5 V voeden, maar **geen signaal boven 3,3 V** (elke ader meten).
- **CO2:** Sensirion **SCD41** (I2C, IO13/IO11), 4 stuks besteld (5/11) voor B&B, BADK, ZITPL.
- **Vervalt:** stofsensor, alert-LDR, CO2-PWM, de relais van R7 (worden Powerpixels), TSL2561 (naar ACCESS/OUTSIDE).
- **Eerste room: B&B** (Photon defect), op basis van room80. Daarna Badkamer, Inkom, Keuken, Wasplaats, Zitplaats.

## 6. ACCESS (buitenverlichting) — repo `ESP32C6_ACCESS`

- Photon **R8-ACCESS** bedient vandaag W1 Voordeur, W2 Veldkant, S Terras; `JSON_status` naar het Dashboard (rij 3).
- **Nog te ontwerpen:** ACCESS1 (.82), ACCESS2 (.83, noord, B&B), ACCESS3 (.84, oost, B&B), één sketch. Zonlicht (TSL2561) komt op een ACCESS- of OUTSIDE-controller.
- **Checkpunten ACCESS1:** terraslicht gaat niet aan; koppeling W1 → W2; PIR W2 defect.

## 7. Andere open punten

- S-ENERGY v1.30 en Dashboard v6.4: 48 u-test bekijken.
- ECO v2.1: dode `publishHVAC()` weg, mDNS. HVAC v2.1: eigen Sheets-push weg.
- Dashboard: HOME/UIT-tegel via de RPi, nieuwe matrixlegende, Master bijwerken voor v6.1–v6.4.
- Master §17 Optimalisatieplan: opruimversies + mDNS overal, statuspagina op de RPi, ROOM-uitrol.
- Particle-token leesbaar in `worker-status.js`: bewust zo gelaten (Photons verdwijnen).

## 8. Praktisch

- **Compileren (FiDel):** ESP32C6 Dev Module, **Flash 16 MB**, Partition Custom (`partitions.csv` in de sketchmap). JavaScript altijd in een `.h`-bestand.
- **OTA:** enkel `<sketch>.ino.bin` uploaden via `http://<ip>/update`. De partitietabel kan niet via OTA veranderen.
- **Compileren in de Claude-sessie** (werkte op 7/11): `arduino-cli` 1.3.1 van GitHub-releases; ESP32-index `package_esp32_index.json` van de GitHub-release 3.3.12 (espressif.github.io en downloads.arduino.cc zijn niet bereikbaar), ingekort tot `esp-rv32`, `esptool_py` en `esp32c6-libs` en als `file://`-index gebruikt; bibliotheken via `git clone` (ESP32Async/ESPAsyncWebServer + AsyncTCP, adafruit DHT + Adafruit_Sensor + NeoPixel, pstolarz/OneWireNg); `--build-property runtime.tools.ctags.path=` naar een lege `ctags` (de ROOM-sketch heeft geen automatische prototypes nodig). FQBN `esp32:esp32:esp32c6:FlashSize=16M,PartitionScheme=custom,CDCOnBoot=cdc`.
- **RPi:** `ssh fidel@192.168.0.50 'bash ~/update.sh'` voor wat al in de repo staat; vanaf de Mac `bash ~/deploy.sh "omschrijving"`.
- **Google Apps Script:** "Implementaties beheren → potlood → nieuwe versie", nooit "nieuwe implementatie". Na kolomwijzigingen `setupHeaders()` één keer uitvoeren.

# ESP32C6_ROOMS — Zarlar ROOM-controllers

Room-controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings` (bewaard in NVS).

- **Wat er in een room zit (en wat niet):** [`docs/ROOM_STANDAARD.md`](docs/ROOM_STANDAARD.md). Eerst lezen.
- **Ombouw per room, B&B in detail:** [`docs/aansluitingen.md`](docs/aansluitingen.md).
- **Nieuw gesprek met Claude:** [`docs/ROOMS_OVERDRACHT.md`](docs/ROOMS_OVERDRACHT.md).
- **Systeembreed:** [Master-overnamedocument](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/Zarlar_Master_Overnamedocument.md) §7 (ROOM).

## Status (25 november 2026)

| | |
|---|---|
| **Sketch** | **v3.8b.1** — map `ESP32_C6_ROOM/`, klaar om te uploaden: `bin/ESP32_C6_ROOM_v3.8b.1.ino.bin` |
| **Draait op** | room80: `192.168.0.80` · `http://eetplaats.local/` — testbord met alle sensoren, rij R-EETPL op de Dashboard-matrix |
| **Volgende room** | B&B (`.75`, R-BandB), in montage |
| **Apple Home** | Via de RPi-bridge (`matterbridge-zarlar`), toestel "Eetplaats". Geen Matter op de ESP32 |
| **Logging** | Het Dashboard (`.60`) haalt `/json` op en post **elke 5 min** naar de Sheet "R-ROOMS Log". De room post zelf niets. Een nieuwe room verschijnt pas in de Sheet als hij op het Dashboard → `/settings` op **actief** staat |
| **Portal (RPi)** | `192.168.0.50:3000` gebruikt `/json`, `/capabilities`, `/toggle_pixel_mode`, `/toggle_pixel`, `/setcolor`, `/set_home` |

## Wat de sketch doet

- **Temperatuur:** DS18B20 (max. 6, één primair), terugval op DHT22 en daarna SCD41; zonder geldige meting vorstbeveiliging (15 min per uur verwarmen). Geldig bereik −10…40 °C.
- **Verwarming:** effectief setpoint = max(setpoint, dauwpunt + marge) als de dauwpuntbeveiliging aan staat; hysterese (standaard 1,0 °C); altijd uit vanaf 30 °C; bij Thuis + thermostaatcontact volgt hij de thermostaat of de dauwpuntbeveiliging.
- **Ventilatie:** volgens CO2 van de SCD41 (0 % bij 600, 100 % bij 1200 ppm, instelbaar).
- **Pixels:** per pixel manueel, beweging MOV1/MOV2 of nachtlampje, eigen kleur optioneel; bed-modus houdt beweging- en nachtlampjes uit. Eén **effectstrook** (bv. treden) met effect vast, samen ademen, één voor één ademen of looplicht.
- **Generic ADC** op GP3 (IO2), in volt, met bijnaam en factor.
- **Betrouwbaarheid:** zelf herverbinden met WiFi, nood-AP zonder router (normale werking blijft), veilige OTA, herstartgeschiedenis met de laatste stap.

## Pagina's

| Adres | Voor wie | Inhoud |
|---|---|---|
| `/` | iedereen | **Landingspagina:** temperatuur met het echte doel en schuifbalk, 🔥 = verwarming vraagt warmte, een tegel per lamp (beweging: tik = automatisch ↔ altijd aan; nachtlampje: tik = handmatig), kleur, 🛏️ bed-modus, 🏠 thuis/weg |
| `/advanced` | FiDel | Alle metingen, overzicht van alle temperatuursensoren |
| `/settings` | FiDel | Instellingen + **Diagnose** (versie, herstarts met laatste stap, geheugen, WiFi, mDNS, SCD41, DHT22, OTA, NVS) |
| `/set_fx` | `/settings` | Effectstrook meteen toepassen en bewaren (sinds v3.8b.1) |
| `/update` | FiDel | OTA met voortgangsbalk; meldt "Terug online" en gaat naar `/advanced` |
| `/ver` | OTA-pagina | `versie\|uptime_s` |
| `/json` | Dashboard, Portal, RPi-bridge | Live-gegevens (zie hieronder) |

## Compileren en uploaden

**Arduino IDE:** ESP32C6 Dev Module · Flash Size **16 MB** · Partition: `partitions.csv` in de schetsmap (automatisch) · **USB CDC On Boot = Disabled** (anders foutmelding) · Core Debug Level None. Bibliotheken: ESPAsyncWebServer, AsyncTCP, OneWireNg, Adafruit NeoPixel, Sensirion I2C SCD4x (+ Sensirion Core). Geen DHT-bibliotheek (eigen uitlezing).

**USB:** voeding, eerste flash en seriële monitor (115200) via de poort **"UART"** van de devkit, nooit via "USB" (IO12/IO13 zijn shield-pinnen).

**Uploaden (OTA):** enkel `ESP32_C6_ROOM.ino.bin` (niet `.bootloader`, `.partitions` of `.merged`) op `http://<ip>/update`, of de `.bin` uit `bin/`.

**Van op afstand** (Mac via mobiele data + Tailscale; **niet** via de wifi in Nieuwpoort, die gebruikt ook 192.168.0.x). Download naar Downloads en upload via de OTA-pagina, of rechtstreeks met curl:
```
curl -L -o ~/Downloads/room.bin https://raw.githubusercontent.com/FidelDworp/ESP32C6_ROOMS/main/bin/ESP32_C6_ROOM_v3.8b.1.ino.bin
ls -l ~/Downloads/room.bin
curl -F "update=@$HOME/Downloads/room.bin" http://192.168.0.80/update
```

**Nieuwe room of overstap van een oude sketch:** `/settings` → Diagnose → eenmaal "Ongebruikte NVS-gegevens wissen".

## `/json` (contract)

Keys en bedieningsadressen wijzigen alleen met uitdrukkelijk akkoord: Dashboard, Portal, RPi-bridge en Sheet hangen ervan af. Nieuwe keys komen achteraan.

| Key | Inhoud | Key | Inhoud |
|---|---|---|---|
| `a` | uptime (s) | `t` | stand per pixel (`P=0101…`) |
| `b` | verwarming vraagt (0/1) | `u` | per pixel: 1 = altijd aan / handmatig |
| `c` | ingesteld setpoint (°C) | `v` | thuis (0/1) |
| `d` | thermostaatcontact (0/1) | `w`, `x` | MOV1/MOV2 triggers per min |
| `e` | DHT22-temp (°C of `null`) | `y`, `z` | een MOV1-/MOV2-pixel brandt |
| `f` | DS18B20 primair (°C of `null`) | `ac` | WiFi RSSI (dBm) |
| `g` | ventilatie (%) | `ad` | vrij geheugen (%) |
| `h` | vocht (%, DHT22, anders SCD41) | `ae`, `af` | grootste blok, laagste heap (KB) |
| `i` | dauwpunt (°C) | `ag` | aantal DS18B20 |
| `j` | dauwpuntalarm | `ah`–`aj`, `an`, `ao` | DS18B20 2–6 (°C of `null`) |
| `k` | CO2 (ppm, SCD41) | `ak` | effectief setpoint (°C) |
| `m` | LDR: 100 = licht, 0 = donker | `al`, `am` | SCD41-temp (°C), -vocht (%) |
| `o` | nacht (0/1) | `ar` | Generic ADC GP3 (V of `null`) |
| `p` | bed (0/1) | *gepland* | `ap`, `aq`, `as` = GP1, GP2, GP4 (v3.9) |
| `q`, `r`, `s` | kamerkleur RGB | | |

## Google Sheet

Sheet "R-ROOMS Log", tabblad **"Data"** (GAS v1.6, `GoogleScript/ROOM_GoogleScript.gs`): één rij per room per 5 min, kamerfilter in **C1**. Ontbrekende meting of `null` = lege cel.
- **Kolom N "Licht LDR":** vóór 22/11 21:07 = 100 donker, erna = 100 licht.
- **Script wijzigen:** Apps Script → Implementeren → Implementaties beheren → potlood → **Nieuwe versie** (nooit "Nieuwe implementatie": dan verandert de URL). Na een kolomwijziging `archiveerEnNieuweKoppen()` of `setupHeaders()` uitvoeren.

## Plan

**Spelregels:** één versie per stap · plan, dan code na GO · na elke flash testen (Sheet: geheugen en herstarts) · `/json` is een contract · README en versiekop bij elke stap bijwerken.

| # | Versie | Doel | Voor |
|---|---|---|---|
| 1 | **v3.8.1** | **Vorstbeveiliging na de opstart.** Nu: vóór de eerste meting geldt "geen geldige temperatuur", en de vorstbeveiliging begint meteen met haar kwartier "aan" → na elke herstart kort een verwarmingsvraag (`b` = 1, gezien in de Sheet 25/11 22:10). Oplossing: de vorstbeveiliging pas laten ingrijpen als er **3 min na de opstart** nog geen geldige temperatuur is; daarna telt haar ritme vanaf het begin van de storing — ✅ geflasht 26/11 | alle rooms |
| — | *(geschrapt 26/11)* | **Lessen uit het Vakantiehuizen-gesprek (26/11) die voor de ROOM-sketch NIET nodig zijn.** Het Ysermonde-bord herstartte thuis elke ±5 min en postte niet. Achteraf bleek de echte oorzaak het netwerk: vast IP **.212 lag in de DHCP-reeks** (ook aan een Apple Watch gegeven) en de DNS van de router antwoordde dat bord niet. De tussenstappen daar waren voorbarig en worden in ROOM **niet** overgenomen: (1) **SNTP weghalen** (v1.14c): de crash zat wel in SNTP, maar enkel omdat de naamopzoeking misliep; room80 (vast IP .80, onder .100) heeft daar nooit last van gehad; (2) **8.8.8.8 als reserve-DNS** (v1.14d): ROOM zoekt enkel `pool.ntp.org` op en dat werkt; (3) **setup-/nood-AP als "enkel AP"** (v1.14b): het wegvallen van het AP viel samen met die herstartlus en is dus niet bewezen een kanaalprobleem; (4) **Async TCP-versie** en **spanningsdip van het relais**: niet de oorzaak. **Wel overgenomen:** geen verwarmingsvraag bij de opstart (= v3.8.1) en de huisregel **vaste IP's onder .100, buiten de DHCP-reeks**. Het nood-AP van ROOM eerst thuis testen (router uit); pas aanpassen als het daar echt wegvalt | — |
| 2 | → | **B&B gaat live** (montage door FiDel; opstarten volgens `aansluitingen.md` §2.4) | B&B |
| 3 | v3.8b | **Pixel-effecten:** één effectstrook (pixel X tot Y, naam, modus zoals de pixels, effect vast / samen ademen / één voor één ademen / looplicht, kleur, snelheid, helderheid), één tegel; DS18B20 niet-blokkerend — 🔨 gebouwd 26/11; v3.8b.1: strook live instellen ("Toepassen", `/set_fx`, geen herstart) en instellingenformulier via POST. B&B: pixels 5–16 (8 treden deur + 4 badkamer), "één voor één ademen" | B&B, BADK, SLPK |
| 4 | v3.9 | **Generieke I/O** op GP1 (IO12), GP2 (IO7), GP4 (IO18): per pin uit · digitale ingang · digitale uitgang · PWM, met bijnaam; `/json` `ap`, `aq`, `as`; bediening `/set_gp?n=1..4&v=0..100`. Pas als er een toepassing is | alle rooms |
| 5 | v3.10–v3.12 | **Pagina's apart** in `.h`-bestanden (`/update`, `/advanced`, `/settings`), met vaste `id` per veld i.p.v. zoeken op labeltekst. Onderhoud, geen zichtbaar nut | alle rooms |
| 6 | — | **Uitrol:** elke Photon-room krijgt een ESP32 met deze sketch (na de B&B: BADK, INKOM, KEUKEN, WASPL, ZITPL) | |

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.8b.1** | 26/11 | Effectstrook live instellen (eigen formulier, `/set_fx`, geen herstart); instellingen via POST (strookvelden kwamen via GET nooit aan) |
| v3.8b | 26/11 | **Pixel-effecten:** effectstrook met 4 effecten, één tegel; DS18B20 niet-blokkerend (geen 0,75 s stilstand van de lus meer) |
| v3.8.1 | 26/11 | Vorstbeveiliging pas 3 min na de opstart (geen verwarmingsvraag meer bij een herstart), ritme telt vanaf het begin van de storing |
| v3.8a | 25/11 | **Pixelmodi** per pixel (manueel, beweging MOV1/MOV2, nachtlampje), eigen kleur per pixel; `u` één teken per pixel |
| v3.8 | 25/11 | **Verwarmingslogica zoals de Sjalay:** effectief setpoint `ak`, hysterese, dauwpuntbeveiliging aan/uit, minimum setpoint, 30 °C-veiligheid |
| v3.7–v3.7.1 | 25/11 | **Temperatuurbewaking:** max. 6 DS18B20's (`an`, `ao`), fouten per sensor, DHT22 5 min geldig, vocht valt terug op de SCD41, sensoroverzicht; SCD41-terugval ook op de landingspagina |
| v3.6–v3.6.3 | 22–25/11 | **WiFi robuuster + mDNS** (nood-AP met normale werking), OTA-voortgangspagina + `/ver`, paginatitels met de kamernaam, CO2-referentie SCD41 instelbaar (420 ppm) |
| v3.5–v3.5.3 | 20–22/11 | **Temperatuurveiligheid** (−10 °C, terugval, `null`, vorstbeveiliging), veilige OTA, opruimen (stof, zon, lichtstraal weg; Generic ADC `ar`), LDR omgekeerd (100 = licht) |
| v3.4–v3.4.5 | 9–11/11 | SCD41 via I2C (`al`, `am`), I2C op IO13 (USB CDC uit), ventilatie op CO2 instelbaar, eigen DHT22-uitlezing (einde van de watchdog-herstarts), NVS opruimen |
| v3.0–v3.3 | 3–7/11 | Matter weg (grootste vrije blok van ±30 naar ±245 KB), landingspagina, versie + diagnose, TSL2561 weg |

Details per versie staan in de kop van `ESP32_C6_ROOM.ino`.

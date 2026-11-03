# ESP32C6_ROOMS — Zarlar ROOM controllers

Room controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings`; ze worden bewaard in NVS.

**Systeembrede referentie:** [`Zarlar_Master_Overnamedocument.md`](https://github.com/FidelDworp/ZarlarApp/blob/main/ESP32_Zarlar/Zarlar_Master_Overnamedocument.md), §7 (ROOM) en §3 (softwareregels). Hardware, pinout en de `/json`-keys staan daar, niet in deze repo.

## Status (3 november 2026)

| | |
|---|---|
| **Actieve sketch** | **v3.0** (3 nov 2026), **zonder Matter** — `TESTROOM/ESP32_C6_ROOM.ino` |
| **Draait op** | room80 (`192.168.0.80`): het TESTROOM-testbord met alle sensoren. Rij R-EETPL op de matrix van het Dashboard |
| **Board** | ESP32-C6-DevKitM-1 (clone), 32-pin, 16 MB, op het Zarlar shield |
| **Apple Home** | Via de RPi-bridge (`server.js` + plugin `matterbridge-zarlar`): toestel "Eetplaats" in de woning Zarlardinge. De ESP32 zelf doet geen Matter meer |
| **Logging** | Het Zarlar Dashboard (`192.168.0.60`) haalt `/json` op en stuurt het naar Google Sheets (`ROOM_GoogleScript.gs`, Sheet "R-ROOMS Log"). De controller post zelf niets |
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

**Meetplan v3.0** (na het flashen, 48 u):

| Meting v3.0 | Waarde |
|---|---|
| Opstart (Serial: "Free heap" / "Largest block" aan het einde van `setup()`) | |
| Vrij geheugen en grootste blok (Sheet, mediaan / laagste over 48 u) | |
| Herstarts in 48 u | |

## Bestanden

| Pad | Inhoud |
|---|---|
| `TESTROOM/ESP32_C6_ROOM.ino` | Actieve ROOM-sketch (versie in de kop van het bestand) |
| `TESTROOM/ROOM_GoogleScript.gs` | Google Apps Script: 37 kolommen A–AK |
| `TESTROOM/partitions.csv` | Partitietabel 16 MB (2 × 6 MB app, voor OTA). Moet naast het `.ino`-bestand staan |
| `TESTROOM/old/` | Archief: v2.8 (maart 2026). Alleen ter referentie |

## Compileren en uploaden

**Arduino IDE:** board **ESP32C6 Dev Module** · Flash Size **16 MB** · Partition Scheme: `partitions.csv` in de schetsmap (wordt automatisch gebruikt) · USB CDC On Boot **Enabled** · Core Debug Level **None**.

**Uploaden:**
1. De eerste keer via de USB-kabel.
2. Daarna via OTA: *Sketch → Export Compiled Binary*. In `build/esp32.esp32.esp32c6/` staan meerdere `.bin`-bestanden: kies **enkel `ESP32_C6_ROOM.ino.bin`**, niet `.bootloader.bin`, `.partitions.bin` of `.merged.bin`. Upload dat op `http://<ip>/update`; de controller herstart vanzelf.
3. Staat Flash Size per ongeluk op 4 MB (zichtbaar als `FlashSize=4M` in de compile-uitvoer), dan kan de OTA mislukken: eerst op 16 MB zetten en opnieuw exporteren.

**Na een update naar v3.0:** de oude Eetplaats-tegels in de testwoning "Matter Home" verdwijnen. De tegels in Zarlardinge (via de RPi) blijven werken. Daarna mag de testwoning "Matter Home" weg (Portal Plan AP26).

## Volgende versies

Telkens één wijziging per versie, na GO (Master §7.9):

| Versie | Wat |
|---|---|
| v3.1 | `#define Serial Serial0` beslissen: poort "UART" of "USB" |
| v3.2 | Geen sensormetingen in AP-mode, zodat `/settings` vlot blijft |
| v3.3 | mDNS terug (`<kamer>.local`), enkel na een geldige WiFi-verbinding |
| v3.4 | Firmwareversie zichtbaar onderaan de statuspagina en in `/settings` (niet in `/json`) |
| later | Sjalay-ideeën (live-JSON vs. waarden bij het laden, statuscirkels, korte sectietitels); LDR-schaal omkeren naar 0 = donker, 100 = fel (zoals Sjalay v1.7); CO2 niet-blokkerend |

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.0** | 3 nov 2026 | Matter verwijderd; Apple Home via de RPi-bridge. Verder ongewijzigd (JSON-keys, endpoints, NVS) |
| v2.24 | 8 mei 2026 | Gedeelde CSS-helper (heap), versieregel op serial verwijderd |
| v2.22 | 1 mei 2026 | `/capabilities` voor de verlichting in de Portal |
| v2.21 | 13 apr 2026 | Thuis/Weg vereenvoudigd (heating_mode/vent_mode weg) |
| v2.20 | 13 apr 2026 | Stabiliteit: crash-log in NVS, CO2-timeout 50 ms |
| v2.8 | mrt 2026 | Laatste versie vóór Matter (in `old/`) |

# ESP32C6_ROOMS — Zarlar ROOM controllers

Room controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): ESP32-C6 op het Zarlar shield, één sketch voor alle kamers. De kamer, pixels en opties worden per controller ingesteld via `/settings` en bewaard in NVS.

**Systeembrede referentie:** [`ZarlarApp/ESP32_Zarlar/Zarlar_Master_Overnamedocument.md`](https://github.com/FidelDworp/ZarlarApp/blob/main/ESP32_Zarlar/Zarlar_Master_Overnamedocument.md), §7 (ROOM) en §3 (softwareregels). Hardware, pinout, `/json`-keys en het plan voor v3.0 staan dáár, niet in deze repo.

## Status (16 oktober 2026)

| Item | Waarde |
|---|---|
| Actieve sketch | **v2.24** (8 mei 2026), met Matter. Draait op room80 (`192.168.0.80`, Eetplaats). |
| Volgende versie | **v3.0**: Matter verwijderd, gecentraliseerd op de RPi (Master §7.9 en §16) |
| Board | ESP32-C6-DevKitM-1 (clone), 32-pin, 16 MB, op het Zarlar shield |
| Logging | Via het Zarlar Dashboard (192.168.0.60) naar Google Sheets (`ROOM_GoogleScript.gs` v1.5) |
| Portal | RPi `192.168.0.50:3000` gebruikt `/json`, `/capabilities`, `/toggle_pixel_mode`, `/toggle_pixel` en `/setcolor` |

## Bestanden

| Pad | Inhoud |
|---|---|
| `TESTROOM/ESP32_C6_ROOM.ino` | Actieve ROOM-sketch (versie: zie header) |
| `TESTROOM/ROOM_GoogleScript.gs` | Google Apps Script, 37 kolommen A–AK |
| `TESTROOM/partitions.csv` | Custom partitietabel 16 MB (2× 6 MB app + OTA). Arduino IDE gebruikt dit bestand automatisch als het in de schetsmap staat. |
| `TESTROOM/old/` | Archief: v2.8 (maart 2026), laatste versie zonder Matter |

## Compileren

Arduino IDE · Board **ESP32C6 Dev Module** · Flash **16 MB** · `partitions.csv` in de schetsmap · Core Debug Level **None**. Eerste keer via USB, daarna OTA via `/update`.

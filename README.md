# ESP32C6_ROOMS — Zarlar ROOM controllers

Room controllers van het Zarlar-thuisautomatiseringssysteem (Zarlardinge): een ESP32-C6 op het Zarlar shield, met **één sketch voor alle kamers**. Kamernaam, pixels en opties stel je per controller in via `/settings`; ze worden bewaard in NVS.

**Systeembrede referentie:** [`Zarlar_Master_Overnamedocument.md`](https://github.com/FidelDworp/ZarlarApp/blob/main/ESP32_Zarlar/Zarlar_Master_Overnamedocument.md), §7 (ROOM) en §3 (softwareregels). Hardware, pinout en de `/json`-keys staan daar, niet in deze repo.

## Status (3 november 2026)

| | |
|---|---|
| **Actieve sketch** | **v3.1** (3 nov 2026), zonder Matter, met landingspagina — `TESTROOM/ESP32_C6_ROOM.ino` + `TESTROOM/landing.h` |
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

**Nog in te vullen na 48 u** (de test start opnieuw bij het flashen van v3.1):

| Meting v3.1 | Waarde |
|---|---|
| Vrij geheugen en grootste blok (Sheet, mediaan / laagste over 48 u) | |
| Herstarts in 48 u | |

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

**Na een update naar v3.0:** de oude Eetplaats-tegels in de testwoning "Matter Home" verdwijnen. De tegels in Zarlardinge (via de RPi) blijven werken. Daarna mag de testwoning "Matter Home" weg (Portal Plan AP26).

## Verbeterplan

Dit is de sketch voor **alle** rooms: ze vervangt stap voor stap de Photons. Daarom eerst betrouwbaarheid, dan gebruiksgemak. Telkens één versie per stap, code pas na GO; JSON-keys en adressen blijven gelijk, tenzij uitdrukkelijk afgesproken (Dashboard, Portal, bridge en Sheet hangen ervan af).

### A. Betrouwbaarheid

| Versie | Wat | Waarom |
|---|---|---|
| v3.2 | **Versie en diagnose zichtbaar:** één `#define ROOM_VERSION`, getoond op elke pagina, in `/settings` en bij het opstarten. In `/settings` ook uptime, reden van de laatste herstart en vrij geheugen | Nu is nergens te zien welke versie op een room draait; bij 8 rooms onmisbaar |
| v3.3 | **WiFi robuuster:** geen modem-slaap (`WiFi.setSleep(false)`) en periodiek opnieuw verbinden | Les uit de PWM-test (onbereikbaar in de kelder) |
| v3.4 | **CO₂ niet-blokkerend** (nu wacht `pulseIn()` tot 2 × 50 ms per meting) en **geen sensormetingen in AP-mode** | Minder haperingen van webserver en pixels; `/settings` vlot bij de eerste installatie |
| v3.5 | **mDNS terug:** `<kamer>.local`, enkel na een geldige WiFi-verbinding | Rooms vind je op naam i.p.v. op IP |

### B. Eén sketch voor alle rooms

| Versie | Wat |
|---|---|
| ✅ v3.1 | **Landingspagina voor gewone gebruikers** (zoals de Sjalay), zie hieronder en "Pagina's" |
| v3.6 | **LDR-schaal gelijk met de Sjalay:** 0 = donker, 100 = fel. Nu zijn er twee vaste drempels (`LDR_DARK_THRESHOLD` en een harde "> 50" voor de nacht); dat wordt er één, instelbaar in `/settings`. Raakt ook het Dashboard (matrix) en de Sheet: samen aanpassen |
| v3.7 | **OTA met voortgangsbalk** en uitleg over het juiste `.bin`-bestand op de pagina zelf |
| — | **Gelijk met de Photons:** nagaan welke functies de Photon-rooms hebben die deze sketch nog mist (Photon-room-sketch nodig) |

**Landingspagina (v3.1, gedaan).** Tot v3.0 opende `http://<room>/` meteen de volledige technische statuspagina (HVAC, verlichting, beweging, bewaking, controller), met waarden als RSSI, heap en PIR-tellers. Voor een huisgenoot of gast is dat te veel. Naar het voorbeeld van de Sjalay:

| Pagina | Voor wie | Inhoud |
|---|---|---|
| `/` **landingspagina** | iedereen | Grote kamertemperatuur met het setpoint en een schuifbalk · tegels per lamp (met de pixelnamen uit `/settings`): tikken = aan/uit · kleurkeuze · bed-modus · thuis/weg · onderaan ⚙ naar de technische pagina |
| `/advanced` (de huidige statuspagina) | Filip | Alle metingen en diagnose, zoals nu |
| `/settings`, `/update` | Filip | Ongewijzigd |

Op **elke** pagina staat bovenaan een **🏠-icoon** dat terugleidt naar de landingspagina. `/json` en alle bedieningsadressen blijven gelijk, dus Dashboard, Portal en Apple Home merken niets.

### C. Webpagina's opfrissen (later, telkens een kleine versie)

**1. Live-gegevens en vaste gegevens scheiden.** De statuspagina haalt nu elke 3 s de volledige `/json` op (32 waarden, dezelfde als voor de Sheet) en zoekt daarna elke tabelrij op **via de tekst van het label** (bv. "bevat het label *Humidity*?"). Dat werkt, maar is breekbaar: hernoem je een label, dan wordt die waarde stil niet meer bijgewerkt. Ook waarden die niet veranderen terwijl je kijkt (versie, IP, instellingen zoals dimsnelheid of lichttijd) gaan mee in die cyclus.
- *Voorstel:* vaste gegevens één keer invullen bij het laden van de pagina (door de ESP32 zelf), en de live-waarden bijwerken via een vaste `id` per veld in plaats van via de labeltekst.
- *Voordeel:* labels mag je vrij aanpassen (bv. vertalen) zonder dat iets stilvalt, het script wordt korter en overzichtelijker, en de browser doet per 3 s minder werk. `/json` zelf blijft ongewijzigd. Geheugenwinst op de ESP32 is er nauwelijks (sinds v3.0 is er ruim genoeg); het gaat om onderhoudbaarheid.

**2. Statuscirkels.** Aan/uit-toestanden (verwarming, thermostaat, thuis, bed, beweging, dauwpuntalarm, pixels) staan nu als kleine bolletjes van 14 px in een gekleurde of grijze tint.
- *Voorstel (Sjalay `.icring`):* een cirkel rond het icoon (🔥, 🛏️, 👋…): **wit = rust**, **lichtrood gevuld met rode rand = actief**.
- *Voordeel:* op een gsm in één oogopslag leesbaar, ook van op afstand en voor wie kleuren slecht onderscheidt (de vorm verandert, niet alleen de kleur), en overal dezelfde betekenis.

**3. Korte, eenduidige titels en labels.** De sectietitels zijn al kort (HVAC, VERLICHTING, BEWEGING, BEWAKING, CONTROLLER), maar de labels mengen Nederlands en Engels ("Room temp", "Humidity", "Heating setpoint" naast "Dauwpunt", "Ventilatie snelheid").
- *Voorstel:* alles in het Nederlands, kort en consequent (bv. "Temperatuur", "Vochtigheid", "Gewenst").
- *Voordeel:* rustiger en duidelijker voor iedereen; samen met punt 1 te doen, want dan hangt er geen code meer aan de labeltekst.

## Versiegeschiedenis

| Versie | Datum | Wat |
|---|---|---|
| **v3.1** | 3 nov 2026 | Landingspagina `/` (landing.h); technische statuspagina naar `/advanced`; 🏠 in het menu |
| v3.0 | 3 nov 2026 | Matter verwijderd; Apple Home via de RPi-bridge. Verder ongewijzigd (JSON-keys, endpoints, NVS) |
| v2.24 | 8 mei 2026 | Gedeelde CSS-helper (heap), versieregel op serial verwijderd |
| v2.22 | 1 mei 2026 | `/capabilities` voor de verlichting in de Portal |
| v2.21 | 13 apr 2026 | Thuis/Weg vereenvoudigd (heating_mode/vent_mode weg) |
| v2.20 | 13 apr 2026 | Stabiliteit: crash-log in NVS, CO2-timeout 50 ms |
| v2.8 | mrt 2026 | Laatste versie vóór Matter (in `old/`) |

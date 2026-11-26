# Zarlar ROOM: aansluitingen bij de overstap van Photon naar ESP32-C6

*v0.5 — 25 nov 2026 — Filip Delannoy (FiDel). Status: plan; B&B in montage.*
> **De standaard staat in [`ROOM_STANDAARD.md`](ROOM_STANDAARD.md).** Bij twijfel geldt de standaard.

*Bron: inventaris `Zarlar_Photon_inventaris_v3.xlsx` (Photon-sketches R1–R8, tekening "ACCESS-WASPL bedrading").*

**Historiek**
- v0.5 (25 nov 2026): opgeruimd: sketch-eisen (allemaal uitgevoerd in ROOM v3.4–v3.8a), ACCESS-buffer (hoort in `ESP32C6_ACCESS`) en afgehandelde open punten weg; B&B op 5 Powerpixels + pixelmodi.
- v0.1–v0.4 (4–16 nov 2026): eerste voorstellen, geen eigen printje voor de rooms, CO2 via de SCD41, B&B-verlichting via Powerpixels.

De Photons in de rooms worden vervangen door een ESP32-C6 op het Zarlar shield. **Een ESP32-pin mag nooit meer dan 3,3 V zien.** Dit document beschrijft wat er per room aangesloten wordt (§1), wat er per room aan de bestaande sensoren moet gebeuren (§2), de B&B in detail (§2.4) en wat er bij een nieuwe SCD41 te doen is (§3). ACCESS (buitenlicht) staat in de repo `ESP32C6_ACCESS`.

---

## 1. Rooms (R1–R7)

Photon-pinnen nagekeken in de productiesketches; ESP32-IO volgens de shield-mapping van ROOM v3.1.

| Signaal | Photon | Connector | ESP32-C6 | Toestel / spanning | R1 B&B | R2 BADK | R3 INKOM | R4 KEUK | R5 WASPL | R6 EETPL | R7 ZITPL |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DS18B20 | D3 | RoomSense | IO3 | 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| DHT22 | D6 | RoomSense | IO6 | 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| PIR MOV1 | D5 | RoomSense | IO5 | AM312, 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| LDR kamerlicht | A3 | RoomSense | IO1 | 3,3 V (analoog) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Pixels / Powerpixels | D4 | Pixel-line | IO4 | 5 V-strip, data 3,3 V | ✅ 5 Powerpixels + 12 pixels | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ 2 Powerpixels + 12 klokpixels |
| **SCD41** CO2 + temp + vocht | — | I2C bij de controller | IO13 SDA / IO11 SCL | 3,3 V | ✅ | ✅ | — | — | — | — | ✅ tv-wand |
| Thermostaatcontact | A6 | Option pin 2 | IO10 | contact naar GND | ✅ | ✅ | ✅ | via HVAC | ✅ | via HVAC | via HVAC |
| (vrij) | A4 | Option pin 4 | IO18 | — | reserve (GP4) |||||||
| PIR MOV2 | A5 | Option pin 5 | IO19 | HC-SR501 op 5 V (uitgang 3,3 V), alleen of via PIReX-diodebordje | ✅ trap | — | ✅ toilet + vestiaire | — | ✅ 2× plafond | — | — |

**Vervalt:**
- CO2 via PWM (CM1106 / MH-Z19), want de SCD41 meet via I2C;
- de stofsensor Sharp (D7/A2) en de alert-LDR (A7);
- de TSL2561 (zonlicht hoort op ACCESS of OUTSIDE);
- de 2 relais van R7 (D2/A0), want die worden Powerpixels;
- de vaste DS18B20-adressen.

**SCD41:** 4× GY-SCD41 (5 nov). De sensor komt bij de controller op de I2C-bus (IO13/IO11): minstens 10–20 cm van de controller en de pixels, en niet erboven. Hij vervangt de CM1106 in de B&B en op de TESTROOM; de keukensensor verhuist naar de tv-wand in de ZITPL. De DHT22 blijft.

**Thermostaat:** KEUK, EETPL en ZITPL zijn rechtstreeks bedraad naar de HVAC. B&B, BADK, INKOM en WASPL sturen een bericht naar de HVAC.

**HC-SR501:** voeding 5 V, maar de uitgang is 3,3 V (er zit een 3,3 V-regelaar op het moduletje). Een serieweerstand met klemdiodes volstaat dus als bescherming.

## 2. Per room: wat doen met de bestaande sensoren

### 2.1 Twee regels voor alle rooms

1. **RoomSense-kant op 3,3 V.** Shield-jumper Sense pin 8 op **3V3** en elke RoomSense-box ombouwen zoals de B&B: PIR → **AM312**, DHT22 en LDR op 3V3. De oude boxen hangen vermoedelijk op 5 V (de Photon-sketch waarschuwt: LDR "not 5V tolerant, 22k series resistor"). Dan zouden DHT22 (IO6) en LDR (IO1) boven 3,3 V uitkomen.
2. **Option-kant: voeding mag 5 V blijven, de signalen niet.** De Option-kabel levert zowel 5 V (pin 1) als 3V3 (pin 8). De MOV2-PIR's (HC-SR501) blijven op 5 V, alleen of samengevoegd via een PIReX-diodebordje. Hun uitgang is 3,3 V. Controle: **meten** (§2.3).

### 2.2 Per room

| Room | RoomSense-kant | Option-kant | Nieuw |
|---|---|---|---|
| **R1 B&B** | ✅ al op 3V3 | TSTAT: blijft. MOV2 (HC-SR501, trap): blijft op 5 V, signaal meten. CM1106: **verwijderd**. | SCD41 bij de controller. 5 Powerpixels (TRIO) + 12 pixels, zie §2.4. |
| **R2 BADK** | ombouwen naar 3V3 | TSTAT: blijft. | SCD41 bij de controller. |
| **R3 INKOM** | ombouwen naar 3V3; **stofsensor (Sharp) loskoppelen** | TSTAT: blijft. MOV2 (toilet + vestiaire, 2 PIR's): blijft op 5 V, signaal meten. **Alert-LDR loskoppelen.** | — |
| **R4 KEUK** | ombouwen naar 3V3 | niets (TSTAT via HVAC); Option-kabel niet nodig | — (CO2 verhuist naar ZITPL) |
| **R5 WASPL** | ombouwen naar 3V3 | TSTAT: blijft. MOV2 (2 PIR's plafond): blijft op 5 V, signaal meten. | — |
| **R6 EETPL** | ombouwen naar 3V3 | niets (TSTAT via HVAC) | — |
| **R7 ZITPL** | **nieuwe RoomSense-box** (heeft er nu geen) | niets (TSTAT via HVAC) | SCD41 bij de controller (tv-wand, naast de tv); 2 Powerpixels + 12 klokpixels in de plaats van de 2 relais. |

### 2.3 Meten vóór je de ESP32 aansluit

Per room, met de kabels aangesloten op de sensoren maar **nog niet op het shield**:

1. Voeding aanleggen zoals het shield die zal geven: RoomSense pin 8 = 3V3; Option pin 1 = 5 V en pin 8 = 3V3. Kan ook met de labovoeding.
2. Op elke **signaalader** van beide RJ45's tegen GND meten: in rust, en bij beweging voor de PIR's.
3. **Nooit meer dan 3,3 V.** Een MOV2 via PIReX-diodebordje geeft ±3 V bij beweging: in orde (drempel ESP32 ±2,5 V).
4. Hoger? Niet aansluiten: eerst de oorzaak zoeken (sensor op 5 V, pull-up naar 5 V).

Tip: een **1 kΩ in serie** op de MOV2-ader (bv. in het klemmenbordje) beschermt de ESP32 als er toch eens iets misloopt, en hindert de werking niet.

### 2.4 B&B: verlichting via Powerpixels (16 nov 2026)

Regels: [`ROOM_STANDAARD.md`](ROOM_STANDAARD.md) §1.3. Bron: tekening "ACCESS1-BB bedrading" (16/11).

| Pixel | Powerpixel | Lichtkring (12 V) | Stroom (tekening 18/11) | Zekering | Externe schakelaar (override) |
|---|---|---|---|---|---|
| 0 | TRIO, 3 kanalen parallel | **Spanlampen** — modus beweging MOV1 | ±3 A | 4 A | ✅ ingangsdeur |
| 1 | TRIO, 3 kanalen parallel | Bedlampjes (bolletjes 0,1 A) + **ledstrip vloermat ingang** (0,5 A) | ±0,6 A | 2 A (1 A volstaat) | — |
| 2 | TRIO, 3 kanalen parallel | Tafelspot (lamp nog te kiezen) | ≤ 2 A | 2 A | ✅ ingangsdeur |
| 3 | TRIO, 3 kanalen parallel | Badkamer: glasdallen 3 A + WC 1 A | 4 A | 5 A | ✅ badkamer |
| 4 | TRIO, 3 kanalen parallel | **Leeslampjes** (lampjes nog te kiezen) | ≤ 2 A | 2 A | — |
| 5–16 | gewone pixels | 12 pixels (treden: 3 naar de badkamer, 9 aan de voet van het bed) | — | — | — |

Volgorde op de pixel-line volgens de tekening "ACCESS3-BB bedrading" van 18/11 (versie 22:26): ESP32 (IO4) → Powerpixel 0 → 1 → 2 → 3 → 4 → 12 pixels = **17 pixels** in `/settings`.

**Metingen 18/11 (FiDel):**
- **Zekeringen op maat** (tekening 18/11): sloten 2 A, leeslampjes 2 A, badkamer 5 A, tafelspot 2 A, bedlampjes + mat 2 A, spanlampen 4 A, fix-draadjes 2 A; 24 V 2 A, 5 V 2 A. Regel: zekering op maat, ±1,25–1,5× de stroom (ROOM-standaard §1.3).
- **Ongebruikte lijnen** vanaf de Powerpixels (nog te beslissen met Mireille, Maarten en Céline): **Douche rechts**, **Douche links** (strips of niet) en **Leeslampje mansarde** (kleinste bed op de halve verdieping). Elk zou een eigen Powerpixel krijgen (één per lichtkring).
- **Ledstrip vloermat ingang:** ±1 m strip op het trapje aan de ingangsdeur, op Powerpixel 1 samen met de bedlampjes; brandt automatisch als het donker is en gaat uit in BED-modus → pixel 1 in `/settings` op **nachtlampje** (sinds ROOM v3.8a).
- **Constante-stroom-drivers:** overal **300 mA**, telkens **2 LED's in serie**: buitenlampen op 24 V (ACCESS), en de leeslampjes in de slaapkamer van het huis (schuur) op 12 V. In de B&B zijn de leeslampjes nog te kiezen.
- **Voedingen (centraal, in metalen kastjes in de kelder):** 5 V (opschrift 70 A, maar vermoedelijk nu een **5V/20A**-voeding; weinig verbruik op 5 V), 12 V / 13 A, 24 V / 6,5 A, voor het hele huis. **Bij het plaatsen van langere pixelstrings:** 5 V meten aan het einde onder last; lange strings eventueel een eigen 5V-voeding dichtbij. Gevoed via een 230V/16A-lijn met **hoofdschakelaar in de zekeringkast aan de keldertrap** (zet alles tegelijk af; eerst daar afzetten vóór je een kastje opent, want 230 V zit er op open verdeelstrips). Vanuit de kastjes lopen **stijgleidingen van 6 mm²** naar de **DC-zekeringkast in de badkamer** (FiDel 18/11). Aandachtspunten: spanningsval op 5 V over de lange stijgleiding (meten onder last), 110/220V-schakelaar van de Chinese voeding op 220V, aarding van kast en voedingen, stof uit de verluchting. Alle B&B-kringen samen aan (±3 + 4 + 2 + 0,6 + 2 A + sloten) kan de 12V-voeding al dicht bij 13 A brengen; in de praktijk branden ze zelden allemaal tegelijk. Bij stap 13 van het opstartplan ook **12 V meten op het B&B-bord met alle lampen aan** (spanningsval over de lange kabel uit de kelder).

- **Fix-draadjes** van de vijf Powerpixels naar **+12V**, via een eigen zekering van 2 A in het 12V-blok (tekening "ACCESS3-BB bedrading", 16/11).
- De drie schakelaars zijn van Maarten (twee aan de ingangsdeur, één in de badkamer). Ze blijven, parallel over OUT–GND: bewust compromis, faalveilig zonder domotica. De controller ziet niet dat een lamp via de schakelaar brandt.
- Bewust **niet** gedaan: de R/G/B-kanalen van één Powerpixel over drie kringen verdelen (zou 2 printjes besparen, maar maakt de sketch complexer).
- Hetzelfde bord draagt ook **ACCESS3**: buitenlicht N2, E1, E2 via **drie 24V-drivers van 300 mA** (één per LED-paar; tekening aan te passen, 18/11). Getest 18/11: **N2 geeft niets** en **E1 is defect** (meestal ±20 mA, af en toe 300 mA); E2 werkt. Herstel met Maarten (ladder). Zie `ACCESS_STANDAARD.md` in `ESP32C6_ACCESS`.

**Veilig monteren en opstarten (B&B + ACCESS3)**

*Montage (alles spanningsloos)*
1. Voedingen uit, 230V los.
2. Alles monteren en bedraden, elke draad aan beide uiteinden labelen. **Zekeringen nog niet** plaatsen, **ESP32's nog niet** insteken.
3. Weerstand meten: +5V, +12V en +24V elk tegen GND én tegen elkaar: nergens kortsluiting.
4. Polariteit nakijken van lampen, LED-drivers en LED-paren buiten.

*Opstarten, stap voor stap*

5. Voedingen aan, nog zonder zekeringen: 5V, 12V en 24V meten op de rails.
6. **5V-zekering** plaatsen: 5V meten op de aansluitingen van beide ESP32's en op de pixel-line.
7. Met de ESP32 nog los **elke signaalader** op Sense en Option meten (PIR's, reeds, PWM-ingang van de drivers): nergens boven **3,3 V**.
8. **Zekering van het fix-draadje** plaatsen: 12V meten op het fix-draadje van elke Powerpixel. Er mag geen lamp branden.
9. **Lampzekeringen één voor één** plaatsen: geen lamp mag vanzelf aangaan. Per kring Maartens schakelaar testen (aan/uit).
10. **24V-zekering** plaatsen: kijken hoe de drivers reageren zonder PWM-signaal (uit, of volledig aan?).
11. **Eerst de B&B-ESP32** insteken (geflasht met de laatste ROOM-sketch), `/json` nakijken. In `/settings`: kamernaam, IP .75, aantal pixels **17**, en per pixel de naam en modus: 0 Spanlampen (beweging MOV1), 1 Bedlampjes + vloermat (nachtlampje), 2 Tafelspot, 3 Badkamer, 4 Leeslampjes (manueel), Powerpixels met eigen kleur wit; effectstrook pixel 5 tot 16 "Treden" (effect één voor één ademen). Elke Powerpixel apart testen. Diagnose → eenmaal "Ongebruikte NVS-gegevens wissen". Op het Dashboard R-BandB op **actief** zetten (logging).
12. Dan de **ACCESS3-ESP32** insteken: PWM E, PWM N2, PIR E, PIR N2 en de reeds testen.
13. Na **een uur** branden: Powerpixels, drivers en zekeringhouders voelen, schroefklemmen natrekken.

## 3. Bij elke nieuwe SCD41

1. **Draadkleuren doormeten** van het meegeleverde kabeltje (op de print: SDA – SCL – GND – VCC). Niet vertrouwen op rood = VCC. VCC op **3,3 V**. Extra pull-ups zijn niet nodig (3,3 kΩ op het shield).
2. **Een kwartier in buitenlucht**: hij hoort rond de ingestelde "CO2 buitenlucht" (standaard 420 ppm) te lezen. Zo niet: knop **"Kalibreren in buitenlucht"** in `/settings`.
3. **ASC** (automatische kalibratie): **aan** in kamers die regelmatig gelucht worden, **uit** in afgesloten ruimtes; daar af en toe in buitenlucht kalibreren.

## 4. Uitrol

1. B&B als eerste room: MOV2 meten (§2.3), SCD41 plaatsen, opstarten volgens §2.4.
2. De andere rooms één per één: RoomSense ombouwen naar 3V3, meten, ESP32 in de plaats van de Photon, zelfde sketch.

## 5. Open punt

- Is de pinvolgorde van de RJ45 op het shield (pin 1–8) dezelfde als de klemvolgorde op de tekening?

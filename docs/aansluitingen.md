# Zarlar ROOM en ACCESS: aansluitingen bij de overstap van Photon naar ESP32-C6

*v0.3 — 5 nov 2026 — Filip Delannoy (FiDel). Status: plan, nog niets omgebouwd.*
> **De standaard staat in [`ROOM_STANDAARD.md`](../ROOM_STANDAARD.md)** (6 nov 2026). Waar dit document afwijkt (buitenlicht oost in de B&B, pixelklok, TSL2561), geldt de standaard: die gaan naar eigen controllers of vervallen.

*Bron: inventaris `Zarlar_Photon_inventaris_v3.xlsx` (Photon-sketches R1–R8, tekening "ACCESS-WASPL bedrading").*

**Historiek**
- v0.3 (5 nov 2026): **geen eigen printje meer** voor de rooms. Per room een actielijst (§4); alleen ACCESS en de B&B (PWM-E) krijgen een kleine schakeling met de hand (§5).
- v0.2 (5 nov 2026): CO2 via de Sensirion SCD41 op I2C in plaats van PWM.
- v0.1 (4 nov 2026): eerste voorstel met een universeel I/O-bordje (vervallen).

De Photons in de rooms en de ACCESS-controller worden vervangen door een ESP32-C6 op het Zarlar shield. **Een ESP32-pin mag nooit meer dan 3,3 V zien.** Dit document beschrijft wat er per room en voor ACCESS aangesloten wordt (§1, §2), de eisen voor de sketches (§3), wat er per room aan de bestaande sensoren moet gebeuren (§4) en de kleine schakeling voor ACCESS en de B&B (§5).

---

## 1. Rooms (R1–R7)

Photon-pinnen nagekeken in de productiesketches; ESP32-IO volgens de shield-mapping van ROOM v3.1.

| Signaal | Photon | Connector | ESP32-C6 | Toestel / spanning | R1 B&B | R2 BADK | R3 INKOM | R4 KEUK | R5 WASPL | R6 EETPL | R7 ZITPL |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DS18B20 | D3 | RoomSense | IO3 | 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| DHT22 | D6 | RoomSense | IO6 | 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| PIR MOV1 | D5 | RoomSense | IO5 | AM312, 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| LDR kamerlicht | A3 | RoomSense | IO1 | 3,3 V (analoog) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Pixels / Powerpixels | D4 | Pixel-line | IO4 | 5 V-strip, data 3,3 V | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ 2 Powerpixels + 12 klokpixels |
| **SCD41** CO2 + temp + vocht | — | I2C bij de controller | IO13 SDA / IO11 SCL | 3,3 V | ✅ | ✅ | — | — | — | — | ✅ tv-wand |
| ~~TSL2561 zonlicht~~ | — | — | — | — | **vervalt in de rooms** (7/11): komt op een ACCESS- of OUTSIDE-controller |||||||
| Thermostaatcontact | A6 | Option pin 2 | IO10 | contact naar GND | ✅ | ✅ | ✅ | via HVAC | ✅ | via HVAC | via HVAC |
| Buitenlicht oost PWM-E | — | Option pin 3 | IO2 | LED-module 0–5 V | ✅ nieuw | — | — | — | — | — | — |
| (vrij) | A4 | Option pin 4 | IO18 | — | reserve |||||||
| PIR MOV2 | A5 | Option pin 5 | IO19 | HC-SR501 op 5 V (uitgang 3,3 V), alleen of via PIReX-diodebordje | ✅ trap | — | ✅ toilet + vestiaire | — | ✅ 2× plafond | — | — |

**Vervalt:**
- CO2 via PWM (CM1106 / MH-Z19), want de SCD41 meet via I2C;
- de stofsensor Sharp (D7/A2);
- de alert-LDR (A7);
- de 2 relais van R7 (D2/A0), want die worden Powerpixels;
- de vaste DS18B20-adressen.

**SCD41:** 4× GY-SCD41 besteld (5 nov). De sensor komt bij de controller op de I2C-bus (IO13/IO11): minstens 10–20 cm van de controller en de pixels, en niet erboven. Hij vervangt de CM1106 in de B&B en op de TESTROOM; de keukensensor verhuist naar de tv-wand in de ZITPL. De DHT22 blijft.

**Thermostaat:** KEUK, EETPL en ZITPL zijn rechtstreeks bedraad naar de HVAC. B&B, BADK, INKOM en WASPL sturen een bericht naar de HVAC.

**Oostkant buiten:** MOV-E en PWM-E gaan van ACCESS naar de **B&B-controller**: PIR binnen (MOV1), PIR buiten bovenaan de trap (MOV2) en uitgang PWM-E op Option pin 3.

**HC-SR501:** voeding 5 V, maar de uitgang is 3,3 V (er zit een 3,3 V-regelaar op het moduletje). Een serieweerstand met klemdiodes volstaat dus als bescherming.

## 2. ACCESS (buitenlicht, 4 zones)

Klemnummers volgens de tekening (klemmenbordjes "Sense (ACCESS)" en "Option (ACCESS)").

| Klem | Signaal | Zone | Photon | ESP32-C6 | Toestel | Status |
|---|---|---|---|---|---|---|
| Sense 1 | MOV-W1 | W1 + D | D3 | IO3 | HC-SR501 (radar aan voordeur) | in gebruik |
| Sense 2 | MOV-W2 | W3 + W4 | D5 | IO5 | HC-SR501 | in gebruik (via TRANSFER) |
| Sense 3 | ~~MOV-E~~ | — | D6 | IO6 | — | vervalt → B&B |
| Sense 4 | MOV-N1 | N1 + N2 | D7 | IO12 | HC-SR501 | bedraad, nog niet in sketch |
| Sense 5 | ~~PWM-E~~ | — | A2 | IO7 | — | vervalt → B&B |
| Sense 6 | PWM-N1 | N1 + N2 | A3 | IO1 | LED-module 0–5 V | bedraad, nog niet in gebruik |
| Sense 7 / 8 | GND / +5 V | | | | | jumper shield op 5 V |
| Option 1 | +5 V | | | | | |
| Option 2 | PWM-S | S1 + S2 | A6 | IO10 | LED-module 0–5 V | in gebruik (via TRANSFER) |
| Option 3 | MOV-S | S1 + S2 | A7 | IO2 | HC-SR501 | in gebruik (via TRANSFER) |
| Option 4 | PWM-W2 | W3 + W4 | A4 | IO18 | LED-module 0–5 V | in gebruik (via TRANSFER) |
| Option 5 | PWM-W1 | W1 + DN/DS | A5 | IO19 | LED-module 0–5 V | in gebruik |
| Option 6 | (vrij) | | D4 | IO4 | eventueel statusbord | niet aangesloten |
| Option 7 / 8 | GND / 3V3 | | | | | |

- LED-modules: 24 V / 300 mA constante stroom, stuuringang 0–5 V. Aanname: **0 V = licht aan**; omkeren kan in de sketch.
- AlarmiX (I2C-expander: deuren, sloten, bel, poort): voorlopig niet nodig, blijft in de planning.

## 3. Eisen voor de sketches

**Werkwijze:** code pas na GO en een duidelijk plan, één stap per versie, telkens een 48 u-test.

### 3.1 ROOM-sketch (bestaande ROOM v3.x)

| # | Eis | Status |
|---|---|---|
| R1 | DS18B20 zelf zoeken op de bus (geen vaste adressen) | ✅ al zo |
| R2 | MOV2 per room aan/uit + eigen lichtgroep | ✅ bestaat (`mov2`) |
| R3 | **SCD41 via I2C, niet-blokkerend** (vervangt CO2-PWM en `pulseIn()`); CO2 vult `/json`-key `k` en stuurt de ventilatie zoals nu; temp en vocht van de SCD41 alleen als extra regels in de UI; geen sensormetingen in AP-mode | gepland v3.4 |
| R4 | Thermostaat: per room instelbaar "lokaal contact" of "via HVAC" | nakijken |
| R5 | **Nachtdrempel (lux) als instelling** per room (Photon: 150–1200) | nieuw |
| R6 | ~~Dag/nacht uit één TSL2561~~: **vervallen** (7/11). Dag/nacht uit de eigen LDR; de TSL2561 komt op ACCESS of OUTSIDE | — |
| R7 | **Powerpixels** als lichtgroepen; R7: 2 Powerpixels (muurspots, tv-achtergrond) | nakijken |
| R8 | **Pixelklok** (12 pixels na de Powerpixels) voor R7 | nieuw |
| R9 | **B&B: buitenlicht oost** — PWM-E (IO2) aan bij MOV2 (en MOV1?), duur en helderheid instelbaar, alleen 's nachts | nieuw |
| R10 | Stofsensor, alert-LDR en CO2-PWM uit de code | opruimen |

### 3.2 ACCESS-sketch (nieuw, `ESP32_C6_ACCESS`)

Zelfde basis als de andere ESP32-sketches: vast IP, `/json`, `/settings`, `/update` (OTA), geen Matter, Apple Home via de RPi-bridge.

| # | Eis |
|---|---|
| A1 | 4 zones (W1, W2, S, N1), elk met PIR-ingang en dimbare uitgang (PWM op elke pin) |
| A2 | Per zone instelbaar: aan/uit, automatisch, duur na beweging (Photon: 30 s), helderheid |
| A3 | Alleen 's nachts automatisch (dag/nacht uit R6) |
| A4 | Koppeling tussen zones instelbaar (vandaag schakelt W1 ook W2 mee) |
| A5 | Uitgang omkeerbaar per zone (0 V = aan, of 5 V = aan) |
| A6 | Veilige opstart: lichten uit tot de sketch draait (zie §5, pull-up op de bufferingang) |
| A7 | `/json` met PIR-status, helderheid per zone, uptime, heap; tegels via de RPi-bridge |

## 4. Per room: wat doen met de bestaande sensoren

### 4.1 Twee regels voor alle rooms

1. **RoomSense-kant op 3,3 V.** Shield-jumper Sense pin 8 op **3V3** en elke RoomSense-box ombouwen zoals de B&B: PIR → **AM312**, DHT22 en LDR op 3V3. De oude boxen hangen vermoedelijk op 5 V (de Photon-sketch waarschuwt: LDR "not 5V tolerant, 22k series resistor"). Dan zouden DHT22 (IO6) en LDR (IO1) boven 3,3 V uitkomen.
2. **Option-kant: voeding mag 5 V blijven, de signalen niet.** De Option-kabel levert zowel 5 V (pin 1) als 3V3 (pin 8). De MOV2-PIR's (HC-SR501) blijven op 5 V, alleen of samengevoegd via een PIReX-diodebordje. Hun uitgang is 3,3 V. Controle: **meten** (§4.3).

### 4.2 Per room

| Room | RoomSense-kant | Option-kant | Nieuw |
|---|---|---|---|
| **R1 B&B** | ✅ al op 3V3 | TSTAT: blijft. MOV2 (HC-SR501, trap): blijft op 5 V, signaal meten. CM1106: **verwijderd**. | SCD41 bij de controller. PWM-E op Option pin 3 met de buffer van §5 + kabel naar de LED-modules oost (E1/E2). |
| **R2 BADK** | ombouwen naar 3V3 | TSTAT: blijft. | SCD41 bij de controller. |
| **R3 INKOM** | ombouwen naar 3V3; **stofsensor (Sharp) loskoppelen** | TSTAT: blijft. MOV2 (toilet + vestiaire, 2 PIR's): blijft op 5 V, signaal meten. **Alert-LDR loskoppelen.** | — |
| **R4 KEUK** | ombouwen naar 3V3 | niets (TSTAT via HVAC); Option-kabel niet nodig | — (CO2 verhuist naar ZITPL) |
| **R5 WASPL** | ombouwen naar 3V3 | TSTAT: blijft. MOV2 (2 PIR's plafond): blijft op 5 V, signaal meten. | — |
| **R6 EETPL** | ombouwen naar 3V3 | niets (TSTAT via HVAC) | — |
| **R7 ZITPL** | **nieuwe RoomSense-box** (heeft er nu geen) | niets (TSTAT via HVAC) | SCD41 bij de controller (tv-wand, naast de tv); 2 Powerpixels + 12 klokpixels in de plaats van de 2 relais. |

### 4.3 Meten vóór je de ESP32 aansluit

Per room, met de kabels aangesloten op de sensoren maar **nog niet op het shield**:

1. Voeding aanleggen zoals het shield die zal geven: RoomSense pin 8 = 3V3; Option pin 1 = 5 V en pin 8 = 3V3. Kan ook met de labovoeding.
2. Op elke **signaalader** van beide RJ45's tegen GND meten: in rust, en bij beweging voor de PIR's.
3. **Nooit meer dan 3,3 V.** Een MOV2 via PIReX-diodebordje geeft ±3 V bij beweging: in orde (drempel ESP32 ±2,5 V).
4. Hoger? Niet aansluiten: eerst de oorzaak zoeken (sensor op 5 V, pull-up naar 5 V).

Tip: een **1 kΩ in serie** op de MOV2-ader (bv. in het klemmenbordje) beschermt de ESP32 als er toch eens iets misloopt, en hindert de werking niet.

## 5. ACCESS en B&B: buffer 3,3 → 5 V voor de LED-modules

De LED-modules buiten (24 V / 300 mA) hebben een stuuringang van 0–5 V. Met 3,3 V uit de ESP32 halen ze hun volle bereik niet, en bij "0 V = aan" gaat het licht dan ook niet volledig uit. Per uitgang een kleine buffer, met de hand op een gaatjesprint of op het bestaande klemmenbordje:

```
                 +3V3                      +5V
                   │                        │ VCC
                  R3 10k        ┌───────────┴──────┐
                   │            │ 74AHCT1G125      │
 ESP32-pin ────────┴────────────┤ A              Y ├──── 47 Ω ────► stuuringang LED-module
                                │ OE aan GND       │
                                └──────────────────┘     100 nF tussen VCC en GND
```

- **R3 (10k naar 3V3)** houdt de uitgang op 5 V zolang de ESP32 opstart: **lichten uit** (bij 0 V = aan).
- Alternatief zonder SMD: een **74HCT125** (DIP-14, 4 buffers in één IC) voor de 4 uitgangen van ACCESS.

| Waar | Uitgang | ESP32 | Aantal buffers |
|---|---|---|---|
| ACCESS Sense 6 | PWM-N1 | IO1 (op het shield al 10k pull-up naar 3V3) | 1 |
| ACCESS Option 2, 4, 5 | PWM-S, PWM-W2, PWM-W1 | IO10, IO18, IO19 | 3 |
| B&B Option 3 | PWM-E | IO2 | 1 |

**ACCESS-ingangen** (HC-SR501, uitgang 3,3 V): **1 kΩ in serie**, verder niets. Op Sense 1 (IO3) zeker niet groter dan 1 kΩ: IO3 heeft op het shield een 4,7 kΩ pull-up voor de T-BUS. Met 1 kΩ blijft "laag" ±0,6 V, onder de drempel van 0,8 V. Shield-jumper Sense pin 8 voor ACCESS op **5 V** (HC-SR501's en buffer).

## 6. Volgende stappen

### 6.1 Te doen bij ontvangst van de SCD41's (4× GY-SCD41)

| # | Taak | Status |
|---|---|---|
| S1 | **Draadkleuren doormeten** van het meegeleverde kabeltje (op de print: SDA – SCL – GND – VCC). Niet vertrouwen op rood = VCC. VCC op **3,3 V**. | |
| S2 | **Elke sensor een kwartier in buitenlucht.** Hij hoort rond **420–430 ppm** te lezen. Zo niet: **geforceerde kalibratie (FRC)** in buitenlucht. | |
| S3 | **ASC per kamer:** automatische kalibratie **aan** in kamers die regelmatig gelucht worden; **uit** in afgesloten ruimtes (slaapkamer), en daar af en toe in buitenlucht kalibreren. Wordt een instelling in `/settings` (ROOM v3.4). | |
| S4 | **CM1106 in de B&B:** zolang die nog via PWM met de formule `5000 × …` uitgelezen wordt, nakijken of het een **2000- of 5000 ppm-versie** is. Een 2000 ppm-versie leest **2,5× te hoog**: oude CO2-waarden en de ventilatiesturing zijn dan te hoog geweest. Vervalt zodra de SCD41 de CM1106 vervangt. | |

### 6.2 Uitrol

1. B&B als eerste room: MOV2 meten (§4.3), SCD41 plaatsen, buffer PWM-E maken, kabel oost trekken.
2. ROOM-sketch: eerst een plan voor R3, R5, R6 en R9 (§3.1), code pas na GO.
3. De andere rooms één per één: RoomSense ombouwen, meten, ESP32 in de plaats van de Photon.
4. ACCESS: buffers maken, ACCESS-sketch (§3.2) eerst op het testbord.

## 7. Open punten

| # | Vraag |
|---|---|
| 1 | Is de pinvolgorde van de RJ45 op het shield (pin 1–8) dezelfde als de klemvolgorde op de tekening? |
| 2 | Kabel van de LED-modules oost (E1/E2) naar de B&B-controller: welke weg? |
| 3 | B&B buitenlicht oost: alleen bij MOV2 (buiten), of ook bij MOV1 (binnen)? |
| 4 | ~~Welke room krijgt de TSL2561?~~ Vervallen: geen enkele (7/11) |

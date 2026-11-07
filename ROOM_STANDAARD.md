# ROOM-standaard (Zarlar)

*v1.1 — 7 nov 2026 — Filip Delannoy (FiDel). Status: **vastgelegd**.*

Elke room in Zarlardinge gebruikt **dezelfde hardware en dezelfde sketch**. Dit document legt vast wat er in een room zit en wat er bewust níet in zit. Het is het antwoord op jaren "feature creep": telkens een nieuwe sensor op de Photon-rooms deed hard- en software steeds verder uit elkaar lopen.

Wat per room verschilt, is een **instelling** in `/settings` (opgeslagen in NVS), nooit een aparte sketch of een `#ifdef` per kamer.

---

## 1. Hardware

| Onderdeel | Standaard |
|---|---|
| Controller | ESP32-C6-DevKitM-1 (32 pinnen, 16 MB flash) op het **Zarlar shield** |
| Sensorbox | **RoomSense** op **3,3 V** (shield-jumper Sense pin 8 op 3V3): DS18B20, DHT22, PIR (AM312), LDR |
| Optielijn | **Option**-kabel: levert 5 V (pin 1) én 3V3 (pin 8) |
| Pixel-line | Pixels en Powerpixels (zie §1.3) |
| I2C-bus | Bij de controller, SDA IO13 / SCL IO11, 3,3 V |

**Gouden regel: een ESP32-pin ziet nooit meer dan 3,3 V.** De voeding van een sensor mag 5 V zijn, het signaal niet. Elke ader meten vóór aansluiten ([`docs/aansluitingen.md`](docs/aansluitingen.md) §4.3).

### 1.1 Kern: in elke room

| Connector | Pin | ESP32 | Functie |
|---|---|---|---|
| RoomSense | 1 | IO3 | **DS18B20-bus**: 1 tot 6 sensoren, de sketch zoekt ze zelf; één is primair (kamertemperatuur) |
| RoomSense | 2 | IO5 | **MOV1**: PIR, AM312 op 3,3 V |
| RoomSense | 3 | IO6 | **DHT22**: temperatuur en vocht (dauwpunt) |
| RoomSense | 6 | IO1 | **LDR** kamerlicht (ADC), 10 kΩ pull-up op het shield |
| Pixel-line | data | IO4 | **Pixels en Powerpixels**, aantal en lichtgroepen instelbaar |

### 1.2 Opties: vaste pin, aan of uit in `/settings`

| Connector | Pin | ESP32 | Optie |
|---|---|---|---|
| Option | 2 | IO10 | **Thermostaatcontact** (contact naar GND, interne pull-up). Uit in rooms waar de thermostaat rechtstreeks naar de HVAC gaat (KEUK, EETPL, ZITPL) |
| Option | 5 | IO19 | **MOV2**: tweede PIR met eigen lichtgroep. Bij voorkeur een AM312 op 3,3 V; een of meer 5 V-PIR's (HC-SR501, uitgang 3,3 V), al of niet via het **PIReX**-diodebordje, mag ook. Signaal altijd meten |
| I2C | — | IO13 / IO11 | **SCD41**: CO2, temperatuur en vocht (Sensirion, adres 0x62) |

### 1.3 Pixel-line: pixels en Powerpixels

Op de pixel-line kunnen gewone adresseerbare RGB-pixels én **Powerpixels** hangen, door elkaar.

**Powerpixel** = eigen ontwerp van FiDel: hij wordt aangestuurd **zoals een gewone pixel** (zelfde dataprotocol, zelfde plaats in de reeks), maar de chip stuurt **MOSFET's** in plaats van ledjes. Zo bedient één "pixel" zware LED-verlichting of een RGB-strip (muurspots, tv-achtergrond, plafondlicht).

Voor de sketch is een Powerpixel dus een pixel in de reeks met een eigen bijnaam en lichtgroep; voor de hardware is het een vermogensuitgang.

### 1.4 Generieke I/O: de vrije pinnen

De pinnen van RoomSense en Option die niet voor de kern of de opties gebruikt worden, blijven **beschikbaar als generieke I/O**. Zo kun je er later iets mee schakelen of monitoren zonder de sketch aan te passen.

| Naam | Connector | Pin | ESP32 | Mogelijk |
|---|---|---|---|---|
| **GP1** | RoomSense | 4 | IO12 | digitaal in / uit / PWM |
| **GP2** | RoomSense | 5 | IO7 | digitaal in / uit / PWM |
| **GP3** | Option | 3 | IO2 | digitaal in / uit / PWM, **analoog in (ADC)** |
| **GP4** | Option | 4 | IO18 | digitaal in / uit / PWM |

- Per pin in `/settings`: **modus** (uit · digitale ingang · analoge ingang (alleen GP3) · digitale uitgang · PWM-uitgang), **bijnaam**, voor ingangen pull-up aan/uit.
- **Zichtbaar** op `/advanced` (en de landingspagina als hij aan staat), in `/json` en dus in de Sheet.
- Uitgangen zijn bedienbaar via een eigen adres (gepland, zie README).
- Standaardmodus na de eerste flash: **uit** (pin is ingang zonder pull-up: veilig).
- Op de ESP32-C6 is **alleen IO0–IO6 analoog** (ADC1). IO7 en IO12 zijn dus enkel digitaal. (Daarom werkte de Sharp-stofsensor op IO7 nooit op de ESP32.)
- Ook hier: **maximaal 3,3 V** op de pin. Een LED-module of relais op 5 V gaat via een buffer of transistor.
- Option pin 6 (IO4) is dezelfde pin als de pixel-line en is dus **niet** vrij.

### 1.5 Niet in de standaard

| Wat | Waarom / waar wel |
|---|---|
| Stofsensor (Sharp), alert-LDR (lichtstraal), CO2 via PWM (CM1106, MH-Z19) | Vervallen. Werd haast niet gebruikt of werkte niet op de ESP32 |
| Relais op de controller | Worden Powerpixels |
| TSL2561 (zonlicht, lux) | **Nooit meer op een room-controller** (7/11: de defecte TSL2561 op het testbord is verdacht van herstarts). Hoort op een **ACCESS- of OUTSIDE-controller** (buiten). In de room komt dag/nacht uit de eigen LDR met een instelbare nachtdrempel |
| OLED, keypad, vaste DS18B20-adressen | Vervallen |
| **Buitenlicht** (oost- en noordgevel bij de B&B) | Eigen controller(s) **ACCESS2, ACCESS3…** met de ACCESS-sketch |
| **Pixelklok** en andere extra's in de ZITPL | Eigen extra ESP32 in de zitplaats |

---

## 2. Software: één sketch

- **Eén sketch** `ESP32_C6_ROOM` voor alle rooms. Kamernaam, pixels, sensoren en opties stel je per controller in via `/settings`.
- **Vast IP** per room, Apple Home via de RPi-bridge (geen Matter op de ESP32), logging via het Dashboard naar de Sheet (de room post zelf niets).
- **`/json` is een contract.** Keys en bedieningsadressen wijzigen alleen met uitdrukkelijk akkoord (Dashboard, Portal, RPi-bridge en Sheet hangen ervan af). Nieuwe keys komen achteraan.
- **Een uitgeschakelde optie** voert geen code uit, maar haar `/json`-keys blijven bestaan (waarde 0 of leeg). Zo blijven Dashboard en Sheet voor elke room gelijk.

## 3. Spelregels tegen feature creep

1. **Iets nieuws komt in de room alleen als het in de standaard past:** een generieke I/O-pin of de I2C-bus, 3,3 V, en zinvol voor meer dan één room.
2. **Past het niet**, dan komt het op een **aparte controller** (zoals ACCESS2 of de klok in de ZITPL), niet in de ROOM-sketch.
3. **Eerst dit document aanpassen**, dan pas hardware en code.
4. **Experimenten** horen in `ESP32C6_Experiments`, nooit in de ROOM-sketch.

## 4. Historiek

| Versie | Datum | Wat |
|---|---|---|
| v1.1 | 7 nov 2026 | TSL2561 nooit meer op een room, wel op ACCESS of OUTSIDE |
| v1.0 | 6 nov 2026 | Eerste vastgelegde standaard: kern, opties, Powerpixels, generieke I/O (GP1–GP4), buitenlicht en klok naar eigen controllers, TSL2561 uit de room |

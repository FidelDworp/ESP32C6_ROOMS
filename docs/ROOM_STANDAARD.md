# ROOM-standaard (Zarlar)

*v1.6 — 25 nov 2026 — Filip Delannoy (FiDel). Status: **vastgelegd**.*

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
| I2C-bus | Bij de controller, SDA IO13 / SCL IO11, 3,3 V. Pull-ups 3,3 kΩ op het shield |
| USB van de devkit | **Alleen de poort "UART"** (voeding, flashen, seriële monitor). IO12/IO13 zijn de ingebouwde USB (D−/D+); de ROOM-sketch schakelt die USB uit en gebruikt ze als gewone pinnen. Compileren met **USB CDC On Boot = Disabled** (sinds v3.4.2, 9/11) |

**Gouden regel: een ESP32-pin ziet nooit meer dan 3,3 V.** De voeding van een sensor mag 5 V zijn, het signaal niet. Elke ader meten vóór aansluiten ([`aansluitingen.md`](aansluitingen.md) §2.3).

### 1.1 Kern: in elke room

| Connector | Pin | ESP32 | Functie |
|---|---|---|---|
| RoomSense | 1 | IO3 | **DS18B20-bus**: 1 tot 6 sensoren, de sketch zoekt ze zelf; één is primair (kamertemperatuur) |
| RoomSense | 2 | IO5 | **MOV1**: PIR, AM312 op 3,3 V |
| RoomSense | 3 | IO6 | **DHT22**: temperatuur en vocht (dauwpunt) |
| RoomSense | 6 | IO1 | **LDR** kamerlicht (ADC), 10 kΩ pull-up op het shield. In `/json` 100 = licht, 0 = donker; "donker" = onder de instelbare drempel |
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

**Regels voor Powerpixels (16 nov 2026):**

| Regel | Waarom |
|---|---|
| **Eén Powerpixel per lichtkring**, de drie kanalen (R, G, B) **parallel** op de klem van die kring | De R/G/B-kanalen niet verdelen over verschillende kringen: dat zou de ROOM-sketch complexer maken (lamp = pixel + kanaal). Powerpixels zijn er genoeg, en onderhoud blijft eenvoudig |
| **De sketch verandert niet:** elke pixel houdt zijn vrije RGB-kleur | Een Powerpixel met een RGB-strip moet zijn kleuren blijven gebruiken. Voor een Powerpixel met parallelle kanalen kies je in `/settings` gewoon wit (of een grijswaarde om te dimmen): de hoogste van de drie waarden bepaalt de helderheid |
| Type: **PowerPiXel-TRIO** (EasyEDA rev 5.0, print v4.0): WS2811 → PC817-opto's → NCE30H12K (30 V / 120 A, DPAK), 470R-indicatie-LED per kanaal | Opto-gescheiden: de GND van de pixel-line en de power-GND (PGND) zijn **apart**; geen stripstroom door de pixelkabel |
| **Fix-draadje "+5-12V" naar +12V** (de lampvoeding), **niet** naar 5 V | De net "+5-12V" (collector van de opto's, gate-sturing en indicatie-LEDs) heeft geen klem: op de PowerPiXel v4.0 (EasyEDA) komt het draadje in het soldeerpunt **"+5-12V !!!"** onder de WS2811 (dun draadje, 0,25–0,5 mm², vastzetten); op de PowerQuad v1.0 op pin 4 van een PC817. Op 12 V krijgt de gate ±11,8 V; op 5 V maar ±4,8 V én gaat de scheiding verloren |
| Lamp tussen **+12V en OUT**, GND-klem naar de min van de 12V-voeding; per kring een **zekering op maat** (±1,25–1,5× de stroom van de kring, minstens 2 A; bv. 4 A → 5 A) | Laag-zijdig schakelen. Met de 3 kanalen parallel verbruikt de TRIO bij 4 A samen ±0,1 W: koeling is niet nodig |
| **Externe schakelaar** via de **SMD poke-in connector van TE** (2-polig, met ontgrendeling) op het OUT/GND-padpaar van één kanaal ("Conn" op de tekening). Draad: **18–22 AWG** (0,35–0,75 mm²), massief of licht vertind soepel. De lampdraden (OUT's parallel) en GND worden op de andere pads gesoldeerd, met een kort stuk soepele draad naar de klem van de lichtkring | Tool-vrij aan- en afkoppelen van Maartens schakelaars. Gesoldeerde draden op vlakke SMD-pads kunnen bij trekken de pad van de print scheuren: kabels vastzetten (kabelbinder door het montagegat) |
| **Externe schakelaar** parallel over OUT–GND mag (override) | Faalveilig: werkt ook als controller of wifi uitvalt. Gevolg: staat de schakelaar aan, dan brandt de lamp altijd volledig en weet de controller het niet (de indicatie-LED op de print brandt wel). Terugmelding later eventueel via een PC817 op een vrije ingang |
| Data 3,3 V van de ESP32 rechtstreeks op de pixel-line | Werkt al jaren met de Photons (ook 3,3 V). De eerste WS2811 regenereert het signaal op 5 V. Pas bij flikkeren van de eerste pixel een 74AHCT125 |
| Dimmen: de laagste niveaus (±0–5 %) doen weinig | De WS2811 dimt met PWM van enkele honderden Hz; de PC817 met 10K pulldown kan de kortste pulsen niet volgen |

De oudere PowerpiXel v4.0 (Eagle, 2018, SI2300-inversie, zonder opto) mag blijven waar hij werkt, maar nieuwe kringen krijgen een TRIO.

### 1.4 Generieke I/O: de vrije pinnen

De pinnen van RoomSense en Option die niet voor de kern of de opties gebruikt worden, blijven **beschikbaar als generieke I/O**. Zo kun je er later iets mee schakelen of monitoren zonder de sketch aan te passen.

| Naam | Connector | Pin | ESP32 | Mogelijk |
|---|---|---|---|---|
| **GP1** | RoomSense | 4 | IO12 | digitaal in / uit / PWM |
| **GP2** | RoomSense | 5 | IO7 | digitaal in / uit / PWM |
| **GP3** | Option | 3 | IO2 | **Generic ADC** in volt (sinds v3.5.2): bijnaam en factor voor een spanningsdeler in `/settings`, `/json` `ar`. Max. ±3,1 V op de pin |
| **GP4** | Option | 4 | IO18 | digitaal in / uit / PWM |

- GP3 is de analoge ingang. GP1, GP2 en GP4 krijgen in v3.9 per pin in `/settings` een **modus** (uit · digitale ingang · digitale uitgang · PWM-uitgang), **bijnaam** en pull-up aan/uit.
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
- **Pixels:** elke pixel heeft een modus (manueel · beweging MOV1 · beweging MOV2 · nachtlampje) en optioneel een eigen kleur, in `/settings` (sinds v3.8a). Een Powerpixel met parallelle kanalen krijgt eigen kleur wit.
- **WiFi:** blijft zelf herverbinden; zonder router een nood-AP `ROOM-<naam>` (192.168.4.1) met normale werking; bereikbaar als `<kamernaam>.local` (sinds v3.6).
- **DHT22 met de eigen uitlezing** van de sketch (sinds v3.4.4), nooit met de Adafruit-DHT-bibliotheek (interrupt-watchdog-herstarts).
- **Overstap van een oude sketch (met Matter):** na de eerste flash in `/settings` → Diagnose eenmaal "Ongebruikte NVS-gegevens wissen" (sinds v3.4.5).
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
| v1.6 | 25 nov 2026 | Pixelmodi en eigen kleur per pixel (ROOM v3.8a) |
| v1.5 | 22 nov 2026 | GP3 = Generic ADC in volt (`ar`); LDR-schaal 100 = licht; WiFi-gedrag en mDNS (ROOM v3.5.2–v3.6) |
| v1.4 | 18 nov 2026 | Zekering per lichtkring op maat in plaats van vast 2 A (B&B: badkamer 4 A, spanlampen 3 A) |
| v1.3 | 16 nov 2026 | Regels voor Powerpixels (§1.3): één TRIO per lichtkring met de kanalen parallel (geen R/G/B-verdeling), sketch ongewijzigd (vrije RGB per pixel blijft voor RGB-strips), fix-draadje naar +12V, externe schakelaar als override toegelaten |
| v1.2 | 11 nov 2026 | USB-poort "UART" i.p.v. "USB" (IO12/IO13 = USB D−/D+), I2C-pull-ups 3,3 kΩ op het shield, DHT22 alleen met de eigen uitlezing, NVS opruimen na overstap van Matter |
| v1.1 | 7 nov 2026 | TSL2561 nooit meer op een room, wel op ACCESS of OUTSIDE |
| v1.0 | 6 nov 2026 | Eerste vastgelegde standaard: kern, opties, Powerpixels, generieke I/O (GP1–GP4), buitenlicht en klok naar eigen controllers, TSL2561 uit de room |

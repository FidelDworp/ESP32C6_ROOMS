# Zarlar Matter Integratie — Briefing voor nieuw gesprek - 27feb26

## Context
Filip Delannoy, thuisautomatisering "Zarlar" in Zarlardinge (BE).
Drie ESP32-C6 controllers (16MB flash), één webserver per controller (AsyncWebServer),
vaste IP adressen op 192.168.0.x subnet.
Ervaring: Arduino IDE, HomeSpan (eerder gebruikt), Homebridge (vervangen).
Doel: Matter integratie voor Apple HomeKit + Google Home + Amazon Alexa.

---

## Hardware
| Controller | IP | MAC | Functie |
|---|---|---|---|
| ESP32_ECO-boiler | 192.168.0.71 | 58:8C:81:32:2B:D4 | Zonneboiler |
| ESP32C6_HVAC | 192.168.0.70 | 58:8C:81:32:29:54 | Verwarming/ventilatie |
| ESP32_ROOM | 192.168.0.80 | 58:8C:81:5D:B0:88 | Kamer multi-sensor |

Alle drie: ESP32-C6, 16MB flash, Arduino IDE, huge_app partition scheme.

opm: Op dit ogenblik is ESP32_ROOM nog een oudere ESP32-WROOM-32 processor, maar we maakten net een nieuwe sketch voor een C6 processor. Deze moet in de eerstkomende dagen nog getest worden.

---

## Gekozen architectuur: Thread voor Matter + WiFi voor webUI

- **Matter** loopt via **Thread radio** (ingebouwd in C6)
- **Webserver** blijft op **WiFi** (fixed IP, AsyncWebServer poort 80)
- Geen resource conflict tussen beide
- Vereist een **Thread border router**: HomePod Mini of Apple TV 4K (2021+)
- ESP32-C6 ondersteunt WiFi + Thread simultaan

---

## Wat in HomeKit per controller (subset — details blijven in webUI)

### ECO Boiler
**Zien:**
- Boiler temperatuur (gemiddelde van 6 sensoren)
- Collector temperatuur (Tsun)
- Pomp status (aan/uit + %)
- Energie vandaag (yield_today kWh)

**Bedienen:**
- Pomp override aan/uit

**Matter accessory type:** Temperature Sensor + Switch

---

### HVAC
**Zien:**
- Aanvoer temperatuur
- Retour temperatuur
- Verwarmingsstatus per circuit (3 circuits)
- Buitentemperatuur

**Bedienen:**
- Verwarming aan/uit per circuit

**Matter accessory type:** Thermostat + Switch (per circuit)

---

### ROOM (meest complex)
**Zien:**
- Kamertemperatuur (primaire DS18B20 sensor)
- Alle DS18B20 sensoren (1-4, dynamisch ontdekt)
- Luchtvochtigheid (DHT22)
- CO₂ (indien ingeschakeld)
- Beweging MOV1 / MOV2
- Daglicht (LDR)

**Bedienen:**
- NeoPixels aan/uit per pixel (4-8 stuks, elk als lamp)
- Verwarming setpoint
- Bed modus aan/uit

**Matter accessory types:**
- Temperature Sensor (per DS18B20)
- Humidity Sensor
- Air Quality Sensor (CO₂)
- Occupancy Sensor (PIR)
- Light Sensor (LDR)
- Dimmable Light (per NeoPixel)
- Thermostat
- Switch (Bed)

---

## Aanpak: stap-voor-stap

### Fase 1 — Experiment (blank sketch)
- Nieuwe lege sketch op spare C6 (4MB is voldoende voor test)
- Matter library installeren
- Één temperatuursensor + Thread
- Pairen met Apple Home, Google Home testen
- Resource monitoring (heap, flash)

### Fase 2 — Evaluatie
- 24u stabiliteitstest
- Heap meten: moet >30% vrij blijven
- Pairing proces documenteren

### Fase 3 — Integratie per sketch
- Beginnen met ROOM (meest gebruik, meest sensoren)
- Dan HVAC
- Dan ECO

---

## Technische keuzes

- **Library:** Espressif Arduino Matter library (Board Manager esp32 3.0+)
- **Partition:** huge_app (3MB) — voldoende voor Matter + webserver op 16MB
- **Thread border router:** HomePod Mini of Apple TV 4K aanwezig?
  *(Filip: bevestig dit voor start Fase 1)*
- **#define Serial Serial0** — vaste fix voor alle C6 sketches

---

## Bestaande sketch features die BEWAARD blijven
- Volledige webUI (/, /settings, /json, /update)
- AsyncWebServer op poort 80
- Alle sensorlogica
- NVS opslag
- OTA updates
- mDNS (.local namen)
- AP fallback bij WiFi fout

---

## Vraag voor start Fase 1
Heb je een Thread border router (HomePod Mini / Apple TV 4K 2021+)?
Antwoord: Jazeker: Beneden een apple tv 4k en een homepod mini, boven idem.

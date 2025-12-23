# ESP32 Testroom Controller

**Project README – status december 2025**

---

## 1. Projectdoel

Dit project is een ESP32-gebaseerde **room controller** met:

* een volledige webinterface (desktop & mobiel)
* persistente configuratie via **NVS**
* optionele sensoren die dynamisch aan/uit gezet kunnen worden
* integratie met **Matter / Home Assistant** via JSON
* robuuste fallback naar **AP-mode met captive portal**

Het systeem is bedoeld als een **stabiele basis** waarop stapsgewijs extra functionaliteit kan worden toegevoegd, **zonder regressies of instabiliteit**.

---

## 2. Huidige status – wat werkt volledig en stabiel

### 2.1 Webinterface

* Volledig uitgewerkte, **responsive web UI**

  * header
  * sidebar
  * warning box
  * hints
  * table-layout
* Werkt correct op mobiel (**iPhone getest**)

---

### 2.2 `/settings` pagina (volledig functioneel)

Configuratie via webinterface:

#### **Algemeen**

* Room naam
* WiFi SSID / password
* Static IP

#### **HVAC defaults**

* heating setpoint (default)
* vent request (default)
* dew margin
* home mode (default)
* LDR dark threshold
* beam alert threshold

#### **Optionele sensoren (checkboxes)**

* CO₂
* Stof
* Zonlicht
* MOV2 PIR
* Hardware thermostaat
* Beam sensor

⚠️ **Belangrijk:**
Deze pagina is opgebouwd met `R"rawliteral()"`.

Dit is **extreem kwetsbaar**:
bij elke wijziging moet expliciet gecontroleerd worden dat de rawliteral-structuur **niet breekt**.

**Checkboxes mogen niet herschreven of verplaatst worden zonder testen.**

**Nog niet geïmplementeerd:**

* Nicknames voor optionele sensoren (zie §6)

---

### 2.3 Persistent gedrag (na reboot / power-cycle)

Permanent opgeslagen in NVS:

* **NeoPixel defaults**

  * aantal pixels (1–30)
  * RGB-kleur
* Factory Reset knop
* **JavaScript validatie**

  * IP-formaat
  * verplichte velden
  * confirm dialogs

**Checkbox-afhandeling**

* Betrouwbaar via `method="get"` + `request->hasArg("naam")`

---

### 2.4 Runtime-persistent states (Matter-exposed)

Deze blijven behouden over reboot en zijn beschikbaar via JSON / Matter:

* `bed` (AAN / UIT)
* `heating_setpoint` (actuele waarde)
* `fade_duration` (1–10 s)
* `home_mode` (Thuis / Uit)

Niet-kritieke states worden persistent gehouden;
kritieke states krijgen **veilige defaults**.

---

## 3. Optionele sensoren – dynamisch gedrag

* Uitschakelen in `/settings` resulteert in:

  * verdwijnen van rijen in de hoofdpagina (`/`)
  * verdwijnen van bijbehorende seriële output
* Lege sectietitels (bv. **BEWAKING**) verdwijnen automatisch
* Labels zijn overal geharmoniseerd en consistent
* **JSON endpoint stuurt altijd alle waarden**

  * vereist voor Matter / Home Assistant

**Veilig fallback-gedrag bij reboot:**

* `vent_percent`
* pixel modes
* `heating_mode`
* `vent_mode`
  → vallen altijd terug naar defaults

**Alle core functionaliteit blijft intact:**

* OTA
* JSON endpoint
* NeoPixel kleurkiezer
* sliders en toggles met live update
* seriële logging

---

## 4. Pixel nicknames (reeds geïmplementeerd)

* Elke NeoPixel kan een **vrije gebruikersnaam** krijgen
* Zinvolle defaults (bv. *“Testroom Pixel 0”*)
* Volledig opgeslagen en geladen via NVS
* Voorbereiding voor **Siri / Matter voice control**

---

## 5. Belangrijke technische keuzes

* **NVS** voor alle permanente instellingen
* Eenvoudige, robuuste checkbox-logica (`GET + hasArg`)
* Runtime-persistent states bewust beperkt tot niet-kritieke parameters
* Web UI opgebouwd met:

  * `R"rawliteral()"`
  * responsive tabellen
  * conditionele HTML via `if (xxx_enabled)`

---

## 6. Nog te doen (technisch noodzakelijk)

### 6.1 Betere foutafhandeling bij sensoren

**Probleem**

* Defecte of niet-aangesloten sensoren tonen momenteel `0` of onzinwaarden

**Doel**

* In **web UI** en **serial output** expliciet tonen:

  * “sensor defect”
  * “niet beschikbaar”

**Voorbeelden**

* DS18B20 defect → fallback naar DHT22
* DS18B20 + DHT22 defect → duidelijke foutmelding
* CO₂, Dust, TSL2561, Beam, etc. krijgen defect-detectie

---

### 6.2 Matter exposure koppelen aan `/settings`

Met ESP32-R6 controllers:

* Optionele sensoren die uitgeschakeld zijn in `/settings`:

  * verdwijnen ook in Matter / HomeKit
  * of worden als `unavailable` gemarkeerd

**Momenteel**

* Matter toont **altijd alles** via JSON
  → dit moet consistent worden met de webconfiguratie

---

## 7. Nice-to-have

### `/reset_runtime` endpoint

* Webpagina of GET-endpoint (`/reset_runtime`)
* Wist **alleen** runtime-persistent states:

  * `bed`
  * `heating_setpoint`
  * `fade_duration`
  * `home_mode`
* **Geen factory reset**
* Bedoeld om snel terug te keren naar defaults

---

## 8. Werkfocus vanaf 22 december 2025

### 8.1 Doel 1 – Captive portal in AP-mode

Bij verbinding met AP:

```
ROOM-<room_id>
```

* iOS / Android opent automatisch:

  ```
  http://192.168.4.1/settings
  ```
* Geen manueel IP-typen nodig
* Gedrag vergelijkbaar met Sonoff / Tasmota

---

### 8.2 Doel 2 – Sensor nicknames

Labels op hoofdpagina (`/`) worden:

```
<room_id> <korte_sensornaam>
```

Voorbeelden:

* *“Testroom temperatuur”*

* *“Testroom CO₂”*

* Alle labels bewerkbaar in `/settings` (18 tekstvelden)

* Leeg veld → automatisch fallback naar `<room_id> + default`

* Serial output blijft **ongewijzigd** (vaste NL-teksten)

**Default korte namen (lowercase, consistent):**

```cpp
const char* default_sensor_labels[18] = {
  "temperatuur",
  "vochtigheid",
  "dauwpunt",
  "dauwalarm",
  "CO₂",
  "stof",
  "verwarming",
  "verwarming aan",
  "thermostaat",
  "zonlicht",
  "omgevingslicht",
  "nachtmodus",
  "MOV1 licht",
  "MOV2 licht",
  "MOV1 beweging",
  "MOV2 beweging",
  "lichtstraal",
  "lichtstraal alarm"
};
```

Deze functionaliteit wordt toegevoegd als helperfunctie
(na `void updateFadeInterval()` of vóór `void pushEvent(...)`).

---

## 9. Problemen uit het verleden (lessons learned)

* Ongeldige static IP default (`192.168.xx.xx`)
  → WiFi stack instabiel
  → AP-mode + reboot loops
* Lange `delay()` in `setup()` → watchdog resets
* Blocking sensor reads (vooral `pulseIn()` bij CO₂)
  → webserver niet responsief
  → reboot bij pagina-load
* Arduino-ESP32 core v3.x watchdog issues
* Cumulatie van kleine wijzigingen → instabiliteit

---

## 10. Nieuwe aanpak (strikt!)

### Clean start

* Start van de **laatste stabiele werkende sketch**
* Geen code genereren tenzij **Filip dit expliciet vraagt**

### Eerst stabiliteit, dan features

1. Static IP default leeg (DHCP standaard)
2. Sensor reads **skippen in AP-mode**
3. Captive portal robuust maken

Pas daarna:

* Sensor nicknames, stap voor stap

---

## 11. Werkwijze (niet onderhandelbaar)

* **Geen volledige code dumps**
* **Geen refactors**
* Eén wijziging per stap
* Exacte **ankerregel** (“na deze regel…”)
* Exacte insert-code
* Geen onnodige HTML-wijzigingen
* Extra voorzichtigheid bij `rawliteral()`
* Na elke HTML-wijziging:

  * testen op mobiel
  * check sliders & toggles
* Na elke stap testen
* Volgende stap pas na expliciete goedkeuring

---

## 12. Overname van Grok → ChatGPT

Grok leverde een instabiele evolutie.
Deze README beschrijft de **ChatGPT-versie van 23 dec 2025**, met duidelijke verbeteringen in AP-mode.

### Wat werd correct verbeterd

* Captive portal (DNS hijack + OS-detectie)
* Betere mDNS lifecycle
* Duidelijke serial logging in AP-mode

### Wat bewust niet werd aangeraakt

* Geen sensor nicknames
* Geen rawliteral HTML-wijzigingen
* Geen refactors

### Wat nog fout loopt (kritisch)

* Sensor reads draaien nog in AP-mode
* `pulseIn()` blokkeert → webserver starvation
* Watchdog reset bij openen `/settings`

**Volgende stap (veilig):**

* Sensor reads conditioneel skippen in AP-mode
  → webserver altijd responsief houden

---


**WERKDOCUMENT Voor AI**

* een **compacte AI-prompt** voor herstart van het project, of
* deze README opsplitsen in `README.md` + `DESIGN_RULES.md`

Prima. Hieronder krijg je **twee concrete deliverables**, klaar om te gebruiken.

---

## 1️⃣ Compacte AI-prompt om het project opnieuw op te starten

Deze prompt kan je **letterlijk kopiëren** in een nieuw ChatGPT-gesprek, samen met je `TESTROOM.ino`.

---

### 🔹 AI-STARTPROMPT (aanbevolen)

```
Je werkt aan een bestaand ESP32-project genaamd “ESP32 Testroom Controller”.

Context:
- De sketch is groot, stabiel en in productie.
- Stabiliteit heeft absolute prioriteit boven nieuwe features.
- Er mag NIETS gerefactord worden tenzij expliciet gevraagd.
- HTML is opgebouwd met R"rawliteral()" en is extreem kwetsbaar.
- Wijzigingen gebeuren stap voor stap, met exacte ankerregels.

Wat reeds correct werkt:
- Webinterface (desktop + mobiel)
- /settings configuratie via NVS
- Captive portal in AP-mode
- STA + mDNS lifecycle
- JSON endpoint (Matter / Home Assistant)
- OTA, NeoPixels, runtime states

Strikte werkwijze:
- Geen volledige code dumps
- Eén wijziging per stap
- Altijd: “zoek exact deze regel” → “voeg exact dit toe”
- Geen aannames, gebruik ALLEEN de aangeleverde sketch
- Na elke stap wachten op expliciete bevestiging

Huidige focus (technisch noodzakelijk):
1. Sensor reads volledig skippen in AP-mode
2. Webserver moet altijd responsief blijven
3. Geen blocking calls (pulseIn, delay, etc.)

Ik upload nu TESTROOM.ino.
Begin met: analyse van setup() en loop(), geen code schrijven.
```

---

## 2️⃣ Opsplitsing van de README (aanbevolen structuur)

Dit maakt je repository **duidelijker**, en voorkomt dat een AI later regels door elkaar haalt.

---

### 📄 `README.md` (publiek / overzicht)

**Bevat:**

* Projectdoel
* Wat werkt
* Architectuur op hoog niveau
* Werkfocus
* Status

**Korter, leesbaar voor mensen.**

---

### 📄 `DESIGN_RULES.md` (streng, voor AI & future you)

**Bevat:**

* Werkwijze (niet onderhandelbaar)
* Rawliteral-waarschuwingen
* AP vs STA principes
* Lessons learned
* Wat nooit zonder expliciete toestemming mag

Dit bestand is **cruciaal** om regressies te vermijden.

---

### 📄 `KNOWN_ISSUES.md` (optioneel, maar nuttig)

**Bevat:**

* Sensor blocking
* pulseIn-probleem
* watchdog resets
* AP-mode beperkingen

Helpt om niet opnieuw dezelfde fouten te maken.

---

## 3️⃣ Waarom dit werkt (belangrijk)

* Je **begrenst de AI expliciet**
* Je dwingt **ankerpunten af**
* Je vermijdt “goedbedoelde refactors”
* Je bewaart stabiliteit als contract

Dit is exact de aanpak die je project nodig heeft.


---------------------------------------------------------













TESTROOM.ino

Gespreksafspraken & technische inzichten (december 2025)

Dit addendum vat **alle cruciale lessen, afspraken en randvoorwaarden** samen die tijdens de recente ChatGPT-integratie zijn vastgelegd, en die **niet mogen verloren gaan** bij een nieuw gesprek of verdere ontwikkeling.

Dit document is bedoeld als **contextanker** voor elke volgende AI-assistent.
starten van een nieuw ChatGPT-gesprek, samen met:
* TESTROOM.ino sketch
* deze README

---

## A. Niet-onderhandelbare ontwikkelregels (herbevestigd)

Deze regels zijn **hard constraints**:

1. **Geen volledige code dumps**
2. **Geen refactors**
3. **Eén wijziging per stap**
4. **Altijd een exact ankerpunt**

   * letterlijk bestaande code uit `TESTROOM.ino`
   * geen “functioneel equivalent”
   * geen veronderstellingen
5. **Exacte instructie**:

   * “zoek dit blok”
   * “vervang door”
   * “voeg hierna toe”
6. **Geen HTML-wijzigingen zonder expliciete vraag**
7. `R"rawliteral()"` is **heilig**
8. Elke wijziging moet:

   * compileerbaar zijn
   * testbaar zijn
   * revertbaar zijn

Elke AI die hiervan afwijkt, werkt **tegen** het project.

---

## B. AP-mode vs STA-mode: harde scheiding

### 1. AP-mode (fallback / captive portal)

In **AP-mode** geldt:

* Webserver **moet altijd responsief blijven**
* DNS captive portal actief
* `/settings` moet altijd laden
* **GEEN sensor reads**
* **GEEN blocking calls**

  * geen `pulseIn()`
  * geen lange delays
* mDNS:

  * **NOOIT starten**
* Serial logging:

  * expliciet melden: *AP-mode actief*

👉 AP-mode = **configuratie + reddingsmodus**, geen functionaliteit.

---

### 2. STA-mode (normaal bedrijf)

Pas in STA-mode:

* Sensor reads toegestaan
* mDNS starten **alleen als**:

  * `WiFi.status() == WL_CONNECTED`
  * `WiFi.localIP() != 0.0.0.0`
* mDNS:

  * moet **herstartbaar** zijn
  * mag niet crashen bij reconnect
* Webserver bereikbaar via:

  * IP
  * `http://<room_id>.local`

---

## C. mDNS lifecycle – definitieve afspraken

mDNS-gedrag is **strikt**:

* ❌ Niet starten:

  * in AP-mode
  * zonder geldige STA-IP
* ✅ Wel starten:

  * na succesvolle STA-connect
* 🔁 Moet opnieuw kunnen starten:

  * na WiFi reconnect
  * na DHCP renew
* ❌ Nooit:

  * dubbele `MDNS.begin()`
  * starten op `0.0.0.0`

Serial logging moet dit duidelijk tonen.

---

## D. Captive portal – vastgelegd gedrag

Bij verbinden met AP:

```
ROOM-<room_id>
```

Doelgedrag (getest):

* iOS / Android openen automatisch:

  ```
  http://192.168.4.1/settings
  ```
* Apple captive portal detectie aanwezig:

  * `hotspot-detect.html`
* DNS hijack:

  * alle domeinen → `192.168.4.1`

⚠️ Safari kan wispelturig zijn:

* vertraging is acceptabel
* handmatige refresh soms nodig
* **dit is geen bug**

---

## E. Sensor reads – kritiek probleem (nog op te lossen)

### Huidige fout (bekend & erkend)

* Sensor reads lopen nog in AP-mode
* `pulseIn()` (CO₂) blokkeert
* Gevolg:

  * webserver starvation
  * watchdog reset bij `/settings`

### Vastgelegde oplossing (volgende veilige stap)

* **Sensor reads conditioneel skippen in AP-mode**
* Geen uitzonderingen
* Geen “snelle workaround”
* Eerst stabiliteit, dan features

Dit is **prioriteit #1** vóór verdere uitbreiding.

---

## F. Static IP – definitieve regel

* Static IP default = **leeg**
* DHCP is standaard
* Ongeldige defaults hebben eerder geleid tot:

  * WiFi stack instabiliteit
  * AP-loops
  * reboot storms

Deze fout mag **nooit** terugkomen.

---

## G. Samenwerking met AI – expliciete instructie

Bij start van een nieuw gesprek moet dit expliciet vermeld worden:

> *“Gebruik mijn README + addendum als bindende context.
> Genereer geen code tenzij expliciet gevraagd.
> Werk uitsluitend met exacte ankerpunten uit mijn TESTROOM.ino.”*

Zonder deze context is elke AI-output **onbetrouwbaar**.

---

## H. Status aan het einde van dit gesprek

✅ Werkend en stabiel:

* AP-mode
* Captive portal
* `/settings`
* STA-connectie
* mDNS lifecycle
* Web UI
* NVS persistentie

❗ Nog te doen (veilig, afgebakend):

1. Sensor reads skippen in AP-mode
2. Defect-detectie sensoren
3. Sensor nicknames
4. Matter exposure synchroniseren met `/settings`
5. `/reset_runtime` endpoint

---

## I. Doel van dit document

Dit addendum zorgt ervoor dat:

* context **niet verloren gaat**
* regressies vermeden worden
* een nieuwe AI **geen aannames hoeft te doen**
* u niet opnieuw dezelfde strijd moet voeren

---

Prima. Hieronder krijg je **twee volledig uitgewerkte documenten**, **klaar om 1-op-1 in je GitHub-repository te plaatsen**.
Ze zijn bewust streng, expliciet en AI-proof geschreven.

---

# 📄 `DESIGN_RULES.md`

## ESP32 Testroom Controller

**Strikte ontwerp- en samenwerkingsregels – bindend**

---

## 1. Doel van dit document

Dit document beschrijft **niet-onderhandelbare ontwerp- en werkwijzeregels** voor het project
**ESP32 Testroom Controller**.

Het is bedoeld voor:

* toekomstige ontwikkeling,
* samenwerking met AI-systemen (ChatGPT, Copilot, Grok, …),
* het vermijden van regressies, instabiliteit en herhaling van fouten.

👉 **Elke wijziging die deze regels schendt, wordt beschouwd als fout.**

---

## 2. Fundamenteel principe

> **Eerst stabiliteit, dan features. Altijd.**

Dit project is **geen experimentele playground**, maar een stabiele controller in reële omgeving.

---

## 3. Werkwijze (NIET onderhandelbaar)

### 3.1 Code discipline

* ❌ Geen volledige code dumps

* ❌ Geen refactors

* ❌ Geen “opkuis”

* ❌ Geen herstructurering

* ❌ Geen “betere aanpak” voorstellen zonder expliciete vraag

* ✅ **Exact één wijziging per stap**

* ✅ Elke wijziging moet:

  * compileerbaar zijn
  * testbaar zijn
  * revertbaar zijn

---

### 3.2 Ankerregel – absoluut verplicht

Elke wijziging moet gebeuren met:

* een **letterlijk bestaande regel** uit `TESTROOM.ino`
* exact benoemd als ankerpunt

Formaat:

```
Zoek exact deze regel:
<letterlijke code>

Voeg DIRECT NA deze regel toe:
<nieuwe code>
```

❌ “Functioneel equivalent”
❌ “Zoek iets gelijkaardigs”
❌ “In setup() ergens”

➡️ **Niet toegestaan**

---

## 4. HTML & Web UI – ZEER GEVOELIG

### 4.1 R"rawliteral()"

* De volledige web UI is opgebouwd met `R"rawliteral()"`
* Dit is **extreem kwetsbaar**

❌ Geen herschrijven
❌ Geen verplaatsen
❌ Geen herformatteren
❌ Geen “kleine aanpassing” zonder testen

### 4.2 Checkboxes

* Afhandeling via:

  ```
  method="get" + request->hasArg("naam")
  ```
* Dit gedrag mag **nooit gewijzigd** worden zonder expliciete toestemming.

---

## 5. AP-mode vs STA-mode – harde scheiding

### 5.1 AP-mode (configuratie / reddingsmodus)

In **AP-mode**:

* ✅ Webserver **moet altijd** responsief zijn

* ✅ DNS captive portal actief

* ✅ `/settings` moet altijd laden

* ❌ GEEN sensor reads

* ❌ GEEN blocking calls

  * geen `pulseIn()`
  * geen lange `delay()`

* ❌ GEEN mDNS

AP-mode = **configuratie**, geen functionaliteit.

---

### 5.2 STA-mode (normale werking)

Pas in STA-mode:

* Sensor reads toegestaan
* JSON updates actief
* mDNS toegestaan (zie §6)

---

## 6. mDNS – lifecycle regels

mDNS mag:

* ✅ Alleen starten als:

  * `WiFi.status() == WL_CONNECTED`
  * `WiFi.localIP() != 0.0.0.0`

mDNS mag NIET:

* ❌ in AP-mode
* ❌ zonder geldige STA-IP
* ❌ dubbel gestart worden
* ❌ starten vóór WiFi connectie stabiel is

mDNS moet:

* 🔁 herstartbaar zijn na reconnect
* 🔁 correct stoppen en opnieuw starten

---

## 7. Static IP – vaste regel

* Static IP default = **LEEG**
* DHCP is standaard

Reden:

* Ongeldige defaults veroorzaakten:

  * WiFi instabiliteit
  * AP-loops
  * reboot storms

➡️ Deze fout mag **nooit** terugkeren.

---

## 8. Sensor reads – kritisch aandachtspunt

### 8.1 Bekend probleem

* Sensor reads draaien nog in AP-mode
* `pulseIn()` (CO₂) blokkeert
* Resultaat:

  * webserver starvation
  * watchdog resets bij `/settings`

---

### 8.2 Vastgelegde oplossing

* Sensor reads **conditioneel skippen in AP-mode**
* Geen uitzonderingen
* Geen workarounds

👉 **Dit is prioriteit #1 vóór nieuwe features**

---

## 9. Samenwerking met AI – expliciete instructie

Bij elk nieuw AI-gesprek moet expliciet vermeld worden:

> “Gebruik mijn DESIGN_RULES.md als bindend contract.
> Genereer geen code tenzij expliciet gevraagd.
> Werk uitsluitend met exacte ankerpunten uit mijn sketch.”

Zonder deze context is AI-output **onbetrouwbaar**.

---

## 10. Slotverklaring

Dit document is **bindend**.

Elke wijziging die hiermee in conflict is:

* wordt verworpen
* wordt teruggedraaid
* wordt niet verder besproken

---

---

# 📄 `KNOWN_ISSUES.md`

## ESP32 Testroom Controller

**Bekende problemen & technische schuld**

---

## 1. Doel

Dit document voorkomt dat **oude fouten opnieuw gemaakt worden**
en bewaart technische context die anders verloren gaat.

---

## 2. Kritische problemen (actueel)

### 2.1 Sensor reads in AP-mode

**Status:** ❌ Nog niet opgelost

* Sensor reads lopen in AP-mode
* Vooral `pulseIn()` (CO₂) is problematisch
* Gevolg:

  * webserver starvation
  * watchdog reset
  * `/settings` laadt niet betrouwbaar

**Oplossing (vastgelegd):**

* Sensor reads conditioneel skippen in AP-mode

---

### 2.2 Safari captive portal gedrag

* Safari (macOS / iOS) is wispelturig
* Captive portal popup verschijnt niet altijd
* Soms manuele refresh nodig

➡️ Dit is **geen bug** in de ESP32-code
➡️ Gedrag is acceptabel

---

## 3. Historische fouten (lessons learned)

### 3.1 Ongeldige static IP defaults

* Voorbeeld:

  ```
  192.168.xx.xx
  ```
* Gevolg:

  * WiFi stack instabiel
  * AP-loops
  * reboot storms

---

### 3.2 Lange delay() in setup()

* Watchdog resets
* Onvoorspelbaar gedrag

---

### 3.3 Arduino-ESP32 core v3.x

* Bekende watchdog issues
* Extra voorzichtigheid vereist
* Geen “trial-and-error” code

---

## 4. Niet-problemen (bewust zo gelaten)

* mDNS werkt correct
* JSON stuurt altijd alle waarden
* Sommige waarden blijven persistent over reboot

➡️ Dit is **bewust ontwerp**, geen bug.

---

## 5. Volgende veilige stap

> **Sensor reads volledig uitschakelen in AP-mode**

Pas daarna:

* sensor defect-detectie
* sensor nicknames
* Matter exposure koppelen aan `/settings`

---

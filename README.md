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

Als u wilt, kan ik hierna ook nog:

* een **compacte AI-prompt** maken voor herstart van het project, of
* deze README opsplitsen in `README.md` + `DESIGN_RULES.md`

Maar enkel op expliciete vraag.


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

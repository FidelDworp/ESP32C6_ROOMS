# Zarlar Thuisautomatisering — Overnamedocument
**Volgende sessie: ROOM Controller — Matter "aparte tegels" + CSS-optimalisatie**

ESP32-C6 · Arduino IDE · Matter · Google Sheets  
15 maart 2026 · Filip Delannoy

---

## 1. Doel van dit document

Volledige staat na **ROOM Matter-integratie + heap-optimalisatie** (15 maart 2026).

Sessiehistorie:
- **HVAC v1.9 → v1.16** (12–13 maart 2026): referentiesketch, volledig geoptimaliseerd
- **ECO v1.21 → v1.22** (13–14 maart 2026): stabiel, in productie
- **ROOM v2.4 → v2.8** (15 maart 2026, Focus 1): licht & stabiel, sensor health indicators ✅
- **ROOM v2.9 → v2.10** (15 maart 2026, Focus 2): Matter + heap-optimalisatie ✅

---

## 2. Systeemoverzicht

| Controller | IP | MAC | Versie | Status |
|---|---|---|---|---|
| **HVAC** | 192.168.0.70 | — | v1.16 | ✅ Productie, stabiel |
| **ECO Boiler** | 192.168.0.71 | 58:8C:81:32:2B:D4 | v1.22 | ✅ Productie, stabiel |
| **ROOM** | 192.168.0.80 | 58:8C:81:32:29:54 | v2.10 | ✅ Matter + heap stabiel |
| **Zarlar Dashboard** | 192.168.0.60 | — | — | Verzamelt JSON, POST naar Google |

---

## 3. ROOM Controller — eindstaat v2.10

### 3.1 Heap-baseline (definitief na alle optimalisaties)

```
Setup:   23% free  (62936 bytes)   Largest block: 45 KB
Runtime: 20% free  (~74 KB)        Largest block: 31 KB  ✅
```

**Crashdrempel: 25 KB — wij zitten op 31 KB → 6 KB marge ✅**

Historisch overzicht:

| Versie | Largest block runtime | Maatregel |
|---|---|---|
| v2.8 (geen Matter) | 243 KB | Baseline |
| v2.9 (Matter toegevoegd) | 29 KB | Matter kost ~214 KB |
| v2.10 String→char[] | 25 KB | pixel_nicknames, room_id, etc. |
| v2.10 heap-optimalisatie | **31 KB** | Captive AP-only, MOV_BUF 50→20, pixel-handlers geconsolideerd |

### 3.2 Heap-optimalisaties doorgevoerd (v2.10)

**String → char[] (permanente heap + BSS winst):**

| Variabele | Oud | Nieuw |
|---|---|---|
| `pixel_nicknames[30]` | `String[]` heap | `char[30][32]` BSS |
| `ds_nicknames[4]` | `String[]` heap | `char[4][48]` BSS |
| `room_id`, `wifi_ssid`, `wifi_pass` | `String` heap | `char[]` BSS |
| `static_ip_str`, `mac_address` | `String` heap | `char[]` BSS |
| `mdns_name` | `String` heap | **Verwijderd** (ongebruikt) |
| `getFormattedDateTime()` | `String` return | `const char*` static buf |
| `matterNuclearReset bk_nick[30]` | `String[]` heap | `char[30][32]` stack |

**Handler en BSS winst:**

| Maatregel | Winst |
|---|---|
| Captive portal handlers → AP-only | ~600 bytes handler-heap |
| `MOV_BUF_SIZE` 50 → 20 | 240 bytes BSS |
| N pixel-handlers → 2 universele (`?idx=`) | ~600 bytes handler-heap |
| `hsvToRgb()` dode code verwijderd | Flash-ruimte |

### 3.3 Matter-endpoints (v2.10, werkend)

| # | Type | Variabele | Opmerking |
|---|---|---|---|
| EP1 | `MatterThermostat` | `room_temp` + `heating_setpoint` | Setpoint instelbaar vanuit Apple Home; mode OFF = manueel stop |
| EP2 | `MatterHumiditySensor` | `humi` | |
| EP3 | `MatterOccupancySensor` | `mov1_light` | MOV1 PIR |
| EP4 | `MatterOccupancySensor` | `mov2_light` | Alleen als `mov2_enabled` |
| EP5 | `MatterColorLight` | `neo_r/g/b` | Kleurpicker; on/off genegeerd (altijd aan) |
| EP6 | `MatterOnOffLight` | `pixel_mode[0]` | SW1: pixel 0 override (MOV1) |
| EP7 | `MatterOnOffLight` | `pixel_on[1]` | SW2: pixel 1 |
| EP8 | `MatterOnOffLight` | `pixel_on[2..N]` | SW3: pixels 2..pixels_num-1 samen |

---

## 4. Matter — geleerde lessen

### 4.1 Compile-fouten

> **⚠️ `#define Serial Serial0` positie is kritiek**
> Verplicht vóór alle `#include` statements. Staat hij erna → 100+ cascade-fouten.

> **⚠️ `void setup() {` mag nooit verdwijnen**
> Zelfde cascade-fouten. Controleer altijd na grote str_replace operaties.

### 4.2 Apple Home — aparte tegels

> **⚠️ "Toon als aparte tegels" — hypothese endpoint-volgorde**
> In de werkende oude sketch (v2.1, 3 maart 2026) stond de thermostat op positie 7 ná 6 sensor-endpoints. Apple Home bood splitsen aan. In v2.10 staat de thermostat op EP1 → Apple Home behandelt het als klimaatapparaat → geen splitsen.
>
> **Te proberen volgende sessie:** thermostat ná de sensoren en licht-endpoints registreren:
> ```
> EP1: MatterTemperatureSensor (los, niet thermostat)
> EP2: MatterHumiditySensor
> EP3: MatterOccupancySensor MOV1
> EP4: MatterOccupancySensor MOV2
> EP5: MatterColorLight
> EP6: MatterOnOffLight SW1
> EP7: MatterOnOffLight SW2
> EP8: MatterOnOffLight SW3
> EP9: MatterThermostat  ← achteraan
> ```
> Dit kost 1 extra endpoint (totaal 9 van max 12) maar repliceeert de structuur die wél werkte.
> Let op: Matter reset + herpairing vereist bij endpoint-volgorde wijziging.

> **⚠️ `MatterEnhancedColorLight` blokkeert aparte tegels**
> Gebruik `MatterColorLight` (EP5). On/off negeren, altijd op `true` houden.

> **⚠️ `espHsvColor_t` vs `HsvColor_t`**
> `MatterEnhancedColorLight` = `espHsvColor_t`; `MatterColorLight` = `HsvColor_t`.

> **⚠️ Pixels buiten `pixels_num` bij boot**
> Fix: `pixels.updateLength(30)` + `clear()` + `show()` eerst, dan `updateLength(pixels_num)`.

> **⚠️ Matter reset verplicht bij endpoint-type of volgorde wijziging**
> `/matter` → "Matter reset (pairing wissen)" vóór herpairing.

> **⚠️ `nvs_flash_erase()` niet vanuit async handler**
> Gebruik vlag in `loop()`: `matter_nuclear_reset_requested = true`.

### 4.3 Pixel-handlers geconsolideerd (v2.10)

In v2.10 vervangen door 2 universele handlers:
- `/toggle_pixel_mode?idx=N` — voor pixels 0 en 1 (MOV-mode)
- `/toggle_pixel?idx=N` — voor pixels 2+

JS-selector `action^="/toggle_pixel_mode"` werkt nog steeds (prefix-match).

---

## 5. Volgende sessie — resterende optimalisaties

### 5.1 CSS naar gedeeld endpoint (medium prioriteit)

De 4 pagina's (/, /update, /settings, /matter) hebben elk een eigen CSS-blok van 1000–2500 chars die bij elke request door AsyncResponseStream worden gestreamd. Dit veroorzaakt fragmentatie.

**Plan:** één `/style.css` endpoint met lange cache-header (`max-age=86400`). Elke pagina vervangt `<style>...</style>` door `<link rel="stylesheet" href="/style.css">`.

Geschatte winst: ~2-3 KB minder fragmentatie per request → stabieler runtime largest block.

```cpp
// Toe te voegen vóór server.begin():
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
  AsyncWebServerResponse *r = request->beginResponse(200, "text/css",
    // gedeelde CSS hier als PROGMEM string
  );
  r->addHeader("Cache-Control", "public, max-age=86400");
  request->send(r);
});
```

### 5.2 Matter aparte tegels (zie §4.2)

Thermostat naar EP9 verplaatsen en losse `MatterTemperatureSensor` als EP1 toevoegen.

---

## 6. Bestanden

| Bestand | Beschrijving |
|---|---|
| `ESP32-C6_MATTER_ROOM_15mar_2200.ino` | ROOM v2.10 — Matter + heap-geoptimaliseerd, stabiel |
| `ROOM_GoogleScript_v1_4.gs` | GAS ROOM — 37 kolommen A–AK |
| `Oude_MATTER_ROOM_3mar.ino` | Referentie: werkende Matter endpoint-volgorde (aparte tegels) |
| `partitions.csv` | Custom partitietabel voor alle ROOM controllers |

---

## 7. Valkuilen — volledig overzicht

*(Alle vorige valkuilen blijven geldig)*

### Heap — aandachtspunten voor ROOM

| Valkuil | Probleem | Oplossing |
|---|---|---|
| `String globals` | Permanente heap-fragmentatie | `char[]` + `strlcpy` (gedaan in v2.10) |
| `pixel_nicknames[30]` als String[] | ~1.5 KB heap-fragmentatie | `char[30][32]` (gedaan) |
| Pixel-handler per pixel | N × ~200 bytes handler-heap | 2 universele handlers met `?idx=` (gedaan) |
| Captive portal altijd geregistreerd | ~600 bytes handler-heap verspild in STA-mode | AP-only registratie (gedaan) |
| `MOV_BUF_SIZE 50` | 400 bytes BSS voor bewegingsbuffers | 20 is ruim voldoende (gedaan) |
| CSS per pagina herhalen | Fragmentatie bij elke request | Gedeeld `/style.css` met cache (volgende sessie) |
| Matter kost ~214 KB heap | Largest block zakt naar ~29 KB | Niet te vermijden; optimaliseer de rest |

### Matter — aandachtspunten

| Valkuil | Probleem | Oplossing |
|---|---|---|
| Thermostat als EP1 | Geen "aparte tegels" in Apple Home | Thermostat ná sensoren (volgende sessie) |
| `MatterEnhancedColorLight` | Geen aparte tegels + andere API | `MatterColorLight` gebruiken |
| `hsvToRgb()` zelf schrijven | Dode code bij MatterColorLight | Verwijderd; MatterColorLight doet eigen HSV |

---

## 8. Instructies voor volgende sessie

**Upload:** `ESP32-C6_MATTER_ROOM_15mar_2200.ino` + dit document.

**Prioriteit 1 — Aparte tegels:**
"Verplaats MatterThermostat naar EP9. Voeg MatterTemperatureSensor toe als EP1 (los van thermostat). Gebruik `Oude_MATTER_ROOM_3mar.ino` als referentie voor de werkende endpoint-volgorde. Matter reset vereist."

**Prioriteit 2 — CSS endpoint (als heap verder krimpt):**
"Maak gedeeld `/style.css` endpoint met `Cache-Control: max-age=86400`. Vervang de 4 inline CSS-blokken door `<link rel='stylesheet' href='/style.css'>`. Gebruik PROGMEM string voor de CSS."

---

*Zarlar project — Filip Delannoy — bijgewerkt 15 maart 2026 (v2.3 na heap-optimalisatie)*

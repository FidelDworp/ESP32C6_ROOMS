### ESP32 Testroom Controller Project (december 2025)

**Huidige status (wat werkt perfect):**
- Volledige, mooie en responsive webinterface (header, sidebar, warning box, hints, table-stijl)
- /settings pagina volledig functioneel:
  - Room naam, WiFi SSID/pass, static IP
  - HVAC defaults (heating setpoint default, vent request default, dew margin, home mode default, LDR dark threshold, beam alert threshold)
  - Optionele sensoren (checkboxes: CO₂, Stof, Zonlicht, MOV2 PIR, Hardware thermostaat, Beam sensor) –
  
**persistent na reboot**
  - NeoPixel defaults (aantal pixels 1-30, RGB)
  - Factory Reset knop
  - JavaScript validatie (IP-formaat, verplichte velden, confirm)
- Checkboxes werken betrouwbaar via method="get" + request->hasArg("naam")
- Runtime persistent states (na power-off/on reboot) en beschikbaar/expose in Matter
  - bed (AAN/UIT)
  - heating_setpoint (huidige gekozen temperatuur)
  - fade_duration (dim-snelheid 1-10s)
  - home_mode (Thuis/Uit)

**Optionele sensoren volledig dynamisch**:
  - Bij uitschakelen in /settings → rijen verdwijnen uit web UI (/) én regels uit seriële output
  - Lege sectietitels (bv. BEWAKING) verdwijnen mee
  - Labels overal geharmoniseerd en consistent
  - JSON endpoint blijft **altijd alles sturen** (voor Matter/Home Assistant)
- Veilig fallback-gedrag: kritieke states (vent_percent, pixel modes, heating_mode, vent_mode) vallen terug naar defaults bij reboot
- Alle core functionaliteit intact: OTA, JSON endpoint, NeoPixel kleurkiezer, sliders/toggles met live update, seriële output, etc.

**Nicknames voor pixels**
- Gebruiker kan elke pixel een vrije naam geven (ideaal voor Siri/Matter later)
- Defaults zijn zinvol ("Testroom Pixel 0")
- Alles veilig opgeslagen en geladen in NVS

**Belangrijke technische keuzes**
- NVS voor alle permanente instellingen
- Simpele, betrouwbare checkbox-afhandeling (GET + hasArg)
- Runtime persistent states beperkt tot niet-kritieke instellingen
- Web UI gebouwd met R"rawliteral()", responsive tables, conditionele HTML via `if (xxx_enabled)`

**Nog te doen**
- Met de nieuwe ESP32-R6 controllers: Optionele sensoren ook uitschakelen in Matter-exposure
- Maakt de configuratie via /settings écht volledig – wat je daar uitzet, verdwijnt overal (UI, serial én HomeKit/Matter).
- Momenteel toont Matter altijd alles via JSON → bij uitschakelen in /settings moeten de bijbehorende entities verdwijnen of als "unavailable" gemarkeerd worden)

**Eventuele nice-to-haves**
1) /reset_runtime endpoint (wis alleen runtime persistent states, geen factory reset)
  = Een simpele webpagina of GET-endpoint (bijv. /reset_runtime)
  Wist alleen de runtime persistent states (bed, heating_setpoint, fade_duration, home_mode)
  Zonder factory reset → ideaal om snel terug te keren naar defaults zonder alles te verliezen
2) Verbeterde foutafhandeling bij sensoren (Bijv. als een sensor defect is, geen waarde meet, of niet aangesloten: Toon in de web UI & serial "sensor defect" of "niet beschikbaar" i.p.v. 0 of rare waarden, hetzelfde in seriële output. Voorbeeld: DS18B20 defect → backup DHT22, en als beide falen duidelijke melding
  - Voorbeeld: DS18B20 defect → backup DHT22, en als beide falen duidelijke melding
  - CO₂, Dust, TSL2561, Beam, etc. krijgen een "defect" detectie

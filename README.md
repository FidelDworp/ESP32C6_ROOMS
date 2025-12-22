### ESP32 Testroom Controller Project (december 2025)

**Huidige status (wat werkt perfect):**
- Volledige, mooie en responsive webinterface (header, sidebar, warning box, hints, table-stijl)
- /settings pagina volledig functioneel:
  - Room naam, WiFi SSID/pass, static IP
  - HVAC defaults (heating setpoint default, vent request default, dew margin, home mode default, LDR dark threshold, beam alert threshold)
  - Optionele sensoren in of uitschakelen (checkboxes: CO₂, Stof, Zonlicht, MOV2 PIR, Hardware thermostaat, Beam sensor) => Opgepast: Dit is een zéér kwetsbare aanpak met "rawliteral"! Opgepast bij wijzigingen! Check altijd dat de "rawliteral" structuur niet gebroken wordt!
  
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
1. Verbeterde foutafhandeling bij sensoren (Bijv. als een sensor defect is, geen waarde meet, of niet aangesloten: Toon in de web UI & serial "sensor defect" of "niet beschikbaar" i.p.v. 0 of rare waarden, hetzelfde in seriële output. Voorbeeld: DS18B20 defect → backup DHT22, en als beide falen duidelijke melding
  - Voorbeeld: DS18B20 defect → backup DHT22, en als beide falen duidelijke melding
  - CO₂, Dust, TSL2561, Beam, etc. krijgen een "defect" detectie
2. "captive portal" realiseren: standaard bij veel devices (zoals Sonoff of Tasmota). Na connect met AP, opent device automatisch een popup met de config pagina (/settings). Je kan direct configureren!
3. Met de nieuwe ESP32-R6 controllers: Optionele sensoren ook uitschakelen in Matter-exposure
=> Maakt de configuratie via /settings écht volledig: Wat je daar uitzet, verdwijnt overal (UI, serial én HomeKit/Matter). Momenteel toont Matter altijd alles via JSON. Bij uitschakelen in /settings moeten de bijbehorende entities verdwijnen of als "unavailable" gemarkeerd worden)

**Eventuele nice-to-haves**
1. /reset_runtime endpoint (wis alleen runtime persistent states, geen factory reset)
  = Een simpele webpagina of GET-endpoint (bijv. /reset_runtime)
  Wist alleen de runtime persistent states (bed, heating_setpoint, fade_duration, home_mode)
  Zonder factory reset → ideaal om snel terug te keren naar defaults zonder alles te verliezen


**Verder werken op 22dec25:**

Doelstellingen:

1) Captive portal in AP mode

Na verbinding met "ROOM-<room_id>" opent iPhone/Android automatisch een popup met http://192.168.4.1/settings.
Geen handmatig typen van IP meer nodig bij first boot of factory reset.

2) Sensor nicknames implementeren

- Labels in de hoofdpagina (/) worden automatisch <room_id> <korte_sensornaam> (bijv. "Testroom temperatuur", "Testroom vochtigheid", "Testroom CO₂"). Deze zijn bewerkbaar in /settings (18 tekstvelden). Indien leeg laten → automatisch terug naar de combinatie met room_id.
- Gebruik: consistente, persoonlijke namen in UI + toekomstige Matter/HomeKit entities (Siri spreekt exact deze naam uit).
- Serial report blijft ongewijzigd (vaste Nederlandse teksten, geen room_id).
- Defaults lijst met korte namen, lowercase voor consistentie:
// Vaste korte sensor-naamdelen (voor automatische combinatie met room_id)
const char* default_sensor_labels[18] = {
  "temperatuur",     // 0: Room temp
  "vochtigheid",     // 1: Humidity
  "dauwpunt",        // 2: Dauwpunt
  "dauwalarm",       // 3: DewAlert
  "CO₂",             // 4: CO₂
  "stof",            // 5: Stof
  "verwarming",      // 6: Heating setpoint
  "verwarming aan",  // 7: Heating aan
  "thermostaat",     // 8: Hardware thermostaat
  "zonlicht",        // 9: Zonlicht
  "omgevingslicht",  // 10: LDR
  "nachtmodus",      // 11: Night mode
  "MOV1 licht",      // 12: MOV1 PIR licht aan
  "MOV2 licht",      // 13: MOV2 PIR licht aan
  "MOV1 beweging",   // 14: MOV1 PIR trig/min
  "MOV2 beweging",   // 15: MOV2 PIR trig/min
  "lichtstraal",     // 16: Beam waarde
  "lichtstraal alarm" // 17: Beam alert
};
=> Voeg deze volledige functie toe na void updateFadeInterval() (of een andere helper, voor void pushEvent(...)):

---

Samenvatting van de belangrijkste problemen die we tegenkwamen

Ongeldige static IP default ("192.168.xx.xx") → WiFi stack instabiel → AP mode + reboots.
Lange delay() in setup() → watchdog triggers.
Blocking sensor reads (vooral pulseIn in CO₂) in AP mode → webserver niet responsief → reboots bij pagina laden.
Watchdog issues in Arduino-ESP32 core v3.x.
Cumulatieve kleine changes → onstabiele toestand.

-----

Hoe we het nú anders gaan doen

Clean start: alleen onze werkende originele sketch als basis.
Geen code genereren tot Filip expliciet vraagt.
Stap-voor-stap plan met voorafgaande goedkeuring van elke stap.
Eerst stabiliteit garanderen:
- Static IP default leeg maken (DHCP standaard).
- Sensor reads skippen in AP mode (zodat /settings altijd bereikbaar is).
- Captive portal toevoegen.

Daarna pas "sensor nicknames" implementeren, in kleine, veilige stappen: Checkboxes, sliders en live update blijven altijd werken.

Nieuw stappenplan (na clean start)
- Stabiliteit fixes (static IP + AP mode robuust maken + captive portal).
- Sensor nicknames globals + regenerate functie.
- Laden uit NVS in setup().
- Opslaan in /save_settings.
- Tekstvelden toevoegen in /settings.
- Labels vervangen in hoofdpagina (/).
- Testen + polish.

Werkwijze:
- Geen volledige code dumps, geen refactor! Dit zorgt voor "rabbit holes"!
- Altijd een testbaar resultaat nastreven.
- Exacte bestaande regel als ankerankerregel (“na deze regel…”)
- Exacte insert code geven: Eén wijziging per stap, maar goed uitgedacht zodat eventueel meerdere gewijzigde regels gegroepeerd worden.
- Géén volledige HTML blokken herschrijven indien niet nodig! Indien wel, opgepast voor de "checkboxes" Extra voorzichtigheid bij rawliteral HTML! Na elke HTML wijziging: testen op mobiel (iPhone) of toggles/sliders nog reageren.
- Na elke stap: testen! Pas daarna volgende stap
=> Werk rustig, één stap per keer, met goedkeuring vóór elke volgende code wijziging.

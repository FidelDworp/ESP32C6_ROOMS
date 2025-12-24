## High-level samenvatting

1. **Bedoeling van de sketch**
   De sketch implementeert een ESP32-gebaseerde room controller voor een testopstelling, als opvolger van een Photon-gebaseerd systeem. Ze combineert netwerkconfiguratie, persistente instellingen, sensoringang, actuator-/pixelsturing en een webinterface (inclusief captive portal) in één geïntegreerd geheel.

2. **Belangrijkste onderdelen**

   * Netwerk & connectiviteit (WiFi, mDNS, captive portal)
   * Persistente configuratie (NVS)
   * Webserver en webinterface
   * Sensoren (analoog en digitaal)
   * Actuatoren en NeoPixel-uitgangen
   * Status- en debuglogging via Serial

3. **Configuratiefilosofie**
   Alle instelbare parameters (room-ID, WiFi, IP, namen van pixels/sensoren, opties) worden persistent opgeslagen in NVS en zijn configureerbaar via de webinterface. Factory reset en herconfiguratie zijn expliciet ondersteund.

4. **Gebruiksscenario**
   De controller draait autonoom op het lokale netwerk, is bereikbaar via vaste URL (mDNS of IP), en kan bij ontbrekende of foutieve netwerkconfiguratie overschakelen naar een captive portal voor setup.

---

## Detailbeschrijving per onderdeel

### 1. Netwerk & connectiviteit

* De ESP32 maakt verbinding met een WiFi-netwerk op basis van in NVS opgeslagen SSID en wachtwoord.
* Ondersteuning voor:

  * DHCP of statisch IP-adres.
  * mDNS (bijvoorbeeld `http://testroom.local`) voor gebruiksvriendelijke toegang.
* Indien geen geldige WiFi-configuratie beschikbaar is of de verbinding faalt:

  * Start een **captive portal** met eigen access point.
  * DNS-verkeer wordt omgeleid zodat elke URL naar de configuratiepagina leidt.

### 2. Persistente opslag (NVS)

* Gebruik van ESP32 NVS voor het veilig bewaren van:

  * Room-ID en device-identiteit
  * WiFi-credentials
  * Netwerkinstellingen (statisch IP)
  * Opties (bv. LDR actief/niet actief)
  * Nicknames van pixels en (voorbereid) sensoren
* Bij opstarten:

  * Worden alle instellingen uit NVS geladen.
  * Worden defaults gebruikt indien waarden ontbreken.
* Factory reset:

  * Wist relevante NVS-keys.
  * Forceert herstart in captive-portal-modus.

### 3. Webserver & webinterface

* Gebaseerd op **ESPAsyncWebServer**.
* Voorziet meerdere HTTP-endpoints, waaronder:

  * Hoofdpagina (`/`) met statusinformatie.
  * Instellingenpagina (`/settings`) voor configuratie.
  * Acties zoals opslaan van instellingen en reset.
* Webinterface is:

  * Lichtgewicht (geen zware externe dependencies).
  * Geschikt voor desktop en mobiel gebruik.
* Pixel-nicknames worden:

  * In `/settings` geconfigureerd.
  * Op de hoofdpagina correct weergegeven (statusweergave).

### 4. Sensoren

De sketch ondersteunt meerdere sensoringangen, waaronder:

* **Analoge ingangen**

  * Sharp sensor (afstand / aanwezigheid).
  * LDR (lichtniveau), optioneel via configuratie.
* **Digitale / PWM-ingangen**

  * CO₂-sensor via PWM-signaal.
  * Thermostaat-ingang.

Sensorwaarden worden:

* Periodiek ingelezen.
* Intern verwerkt voor logica en status.
* (Voorbereid) getoond of gelinkt aan namen/nicknames.

### 5. Actuatoren & outputs

* **NeoPixel LED-strip**

  * Aangestuurd via een gedefinieerde GPIO.
  * Pixels hebben individuele indexen én menselijke leesbare nicknames.
  * Pixelstatussen worden gebruikt voor:

    * Visuele feedback
    * Debug en statusindicatie
* Overige GPIO’s

  * Voorzien voor uitbreiding (relais, LEDs, etc.).

### 6. Logica & programmastroom

* `setup()`:

  * Initialisatie van GPIO’s.
  * Laden van NVS-instellingen.
  * Opstarten van WiFi of captive portal.
  * Initialisatie van webserver en mDNS.
* `loop()`:

  * Afhandeling van sensormetingen.
  * Eventuele periodieke taken.
  * Geen blokkerende delays (asynchroon ontwerp).

### 7. Logging & debug

* Uitgebreide **Serial logging**:

  * Opstartfase (configuratie, netwerkstatus).
  * WiFi- en IP-informatie.
  * Acties zoals reset en opslaan van instellingen.
* Bedoeld voor:

  * Ontwikkeling en debugging.
  * Diagnostiek bij installatie op locatie.

### 8. Ontwerpdoelstellingen

* Modulair en uitbreidbaar ontwerp.
* Maximale autonomie (geen cloud-afhankelijkheid).
* Robuuste configuratie via webinterface.
* Duidelijke scheiding tussen:

  * Configuratie
  * Hardware-aansturing
  * Netwerk en UI

---

Deze samenvatting is bedoeld als functionele documentatie voor de GitHub-repository en beschrijft **wat** de sketch doet en **waarom**, niet de volledige implementatiedetails per functie.

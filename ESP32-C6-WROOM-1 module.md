### 📄 `ESP32-C6-WROOM-1 module voor MATTER.md`

# ESP32-C6-WROOM-1 module (klaar gesoldeerd, met antenne, flash, crystal) is ideaal voor shields: eenvoudige footprint (18x25mm, 0.1” pins), goedkoop (€2-3 bij LCSC/JLCPCB), geen soldering van SoC nodig.	

Voordelen vs bare SoC (ESP32-C6-QFN32):	
	•  Geen fine-pitch QFN soldering (0.4mm pitch, moeilijk handmatig).
	•  Minder risico op RF/antenne-problemen.
	•  Snellere assemblage in China (JLCPCB assembleert modules gratis).
	Moeilijkheidsgraad footprint: laag – kopieer Espressif’s officiële module footprint uit KiCad/Eagle library.
Ga voor WROOM-1 module – toekomstbestendig en productie-vriendelijk.	
	
ESP32-C6-WROOM-1 is uitstekend voor Matter (officieel ondersteund door Espressif SDK voor Wi-Fi en Thread end-devices, future-proof met Wi-Fi 6, BLE 5, Zigbee/Thread).	

Beste alternatieven:
	•  ESP32-C3 (goedkoper, mature Matter Wi-Fi).
	•  ESP32-H2 (pure Thread/Zigbee, laag energie).
	Interne antenne: voldoende voor typische home use case (20-50m indoors, tot 80m open).
	Voor groter bereik/hindernissen: kies WROOM-1U (externe antenne connector).

C6-WROOM-1 is prima start – genoeg voor testroom/sensoren.	
	
Wi-Fi 6 voordelen vs Particle Photon (Wi-Fi 3):

	•  Hogere snelheid (tot 9.6 Gbps vs ~300 Mbps).
	•  Betere efficiëntie in drukke netwerken (meer apparaten tegelijk).
	•  Lager energieverbruik (TWT voor batterijbesparing).
	•  Sterker bereik en minder interferentie (OFDMA, MU-MIMO).
	
Voor sensoren/shield: stabielere verbinding, sneller data (JSON/OTA), lager verbruik in low-power modes. Photon is trager en verbruikt meer in crowded WiFi.

Outdoors (line of sight) is 2-5x beter dan indoors bij interne antenne: indoors ~20-50m, outdoors ~80-200m (typisch voor ESP32-C6 interne antenne; muren/obstakels halveren bereik sterk).
	
Externe antenne vs interne (ESP32-C6):

	•  Bereik indoors: 2-3x beter (50-150m vs 20-50m interne).
	•  Signaalsterkte: +5-15 dB gain, minder obstakelverlies.
	•  Kosten/complexiteit: Module duurder (+€1-2), connector nodig.
	•  Voor jouw use case: Interne voldoende (testroom/home); externe alleen bij muren/ afstand >50m.
	
Welke soorten antennes kan men met de C6 gebruiken?

	Interne PCB-antenne (standaard op ESP32-C6-WROOM-1).
	Externe antenne (via U.FL/IPEX-connector op varianten zoals WROOM-1U/Mini-1U; types: monopole, dipole, rubber duck).
	Software/hardware switching (op sommige boards, via GPIO’s).
	Multi-antenne (tot 16 externe via switches, voor diversity).
	
Van onze oude ESP32 => C6 software:

De aangepaste sketch in Arduino IDE gebruiken we als basis voor Matter over WiFi op ESP32-C6.
Arduino-ESP32 core (versie 3.x) ondersteunt Matter over WiFi via de ingebouwde Matter library en examples (bijv. light, on/off device).

Matter over Thread werkt niet volledig in pre-compiled Arduino core (vereist rebuild of ESP-IDF component).
	Je hoeft geen andere tool (zoals ESP-IDF) – blijf in Arduino IDE, voeg Matter examples toe en integreer in je code.
	Update core naar latest voor beste support.
	
Matter over Wi-Fi op ESP32-C6:

Matter is een universele smart home-standaard voor interoperabiliteit. Bosch Smart Home
“Matter over Wi-Fi” betekent dat je apparaten via Wi-Fi in plaats van mesh-netwerk communiceren. ESP32 Tutorials
De ESP32-C6 ondersteunt Wi-Fi en Thread, dus hij is perfect geschikt voor Matter-apparaten. IoT Assistant
Als je zo’n apparaat maakt, kan het direct met Apple Home werken zonder aparte bridge (mits certificering/compatibiliteit). iCulture

### 📄 `ESP32-C6-WROOM-1 module voor MATTER.md`

## ESP32-C6-WROOM-1 module (klaar gesoldeerd, met antenne, flash, crystal) is ideaal voor shields: eenvoudige footprint (18x25mm, 0.1” pins), goedkoop (€2-3 bij LCSC/JLCPCB), geen soldering van SoC nodig.	

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

- Interne PCB-antenne (standaard op ESP32-C6-WROOM-1).
- Externe antenne (via U.FL/IPEX-connector op varianten zoals WROOM-1U/Mini-1U; types: monopole, dipole, rubber duck).
- Software/hardware switching (op sommige boards, via GPIO’s).
- Multi-antenne (tot 16 externe via switches, voor diversity).
	
Van onze oude ESP32 => C6 software:

- De aangepaste sketch in Arduino IDE gebruiken we als basis voor Matter over WiFi op ESP32-C6.
- Arduino-ESP32 core (versie 3.x) ondersteunt Matter over WiFi via de ingebouwde Matter library en examples (bijv. light, on/off device).
- Matter over Thread werkt niet volledig in pre-compiled Arduino core (vereist rebuild of ESP-IDF component).
- Je hoeft geen andere tool (zoals ESP-IDF) – blijf in Arduino IDE, voeg Matter examples toe en integreer in je code.
- Update core naar latest voor beste support.
	
Matter over Wi-Fi op ESP32-C6:

- Matter is een universele smart home-standaard voor interoperabiliteit.
- “Matter over Wi-Fi” betekent dat je apparaten via Wi-Fi in plaats van mesh-netwerk communiceren. ESP32 Tutorials
- De ESP32-C6 ondersteunt Wi-Fi en Thread, dus hij is perfect geschikt voor Matter-apparaten. IoT Assistant
- Als je zo’n apparaat maakt, kan het direct met Apple Home werken zonder aparte bridge (mits certificering/compatibiliteit).

---------------

## Voorbeeldproject van een ESP32-C6 PCB-ontwerp

Gebruikersproject met een ESP32-C6 module op een PCB die je kunt laten produceren, en wat nuttige context over dit soort modules in productiescenario’s:

### **ESP32-C6 dev board (door user, met PCBWay productie)**

Er is een open project online waarin een hobby-ontwerper een **ESP32-C6-gebaseerde PCB zelf heeft ontworpen en via PCBWay heeft geproduceerd**.

JLCPCB module ESP32_C6_WROOM_1N4:
https://jlcpcb.com/partdetail/ESPRESSIFINC-ESP32_C6_WROOM_1N4/C6034714

Project:
https://fritzenlab.net/fritzenlab-esp32-c6-dev-board/

Development:
https://fritzenlab.net/2024/06/10/develop-a-circuit-board-esp32-c6/

De maker beschrijft dat hij:

* het **ESP32-C6 Xiao-module** (met geïntegreerde antenne en flash) als kern gebruikt,
* een compacte board vormt die direct op een breadboard past,
* sensoren zoals **DHT11, LDR en WS2812 RGB LED** toevoegt,
* het board als **eigen PCB** laat produceren door PCBWay (waarbij hij zijn Gerbers uploadt),
* en positieve ervaringen deelt over de kwaliteit van de PCB-productie en afwerking. ([fritzenlab.net][1])

Dit is precies het soort proces dat je bedoelt:

* **klaar gesoldeerde ESP32-module** op jouw eigen PCB
* productie in China via een service zoals **PCBWay**
* upload van je *Gerber/BOM* en uitlevering van de volledige PCB of ook gelaste boards
* praktische user-ervaring met de realiteit van PCB productie.

---

## Waarom zo’n ESP32-module ideaal is voor jouw PCB

De **ESP32-C6-WROOM-1** (en varianten zoals N4/N8/N16) is een **module** — niet alleen de bare IC — met:

* geïntegreerde **crystal, flash, RF matching en antenne**
* pin-compatibiliteit tot families zoals de WROOM serie
* afmetingen rond **18 × 25 mm** en 0,1″ pinheaders — ideaal voor shields/boards
* veel functionaliteit: Wi-Fi 6, Bluetooth, Zigbee/Thread (afhankelijk model) ([espboards.dev][2])

Dat betekent voor jouw PCB-project:

* je hoeft **niet de ESP32-SoC zelf te routeren** met RF matching etc.
* in plaats daarvan haal je een **gemakkelijke footprint** (module) binnen.
* productiehouses zoals JLCPCB of PCBWay kunnen deze modules **meelassen tegen lage meerprijs** tijdens PCB-assemblage.
* de prijzen voor de modules zelf zijn **scherp** bij LCSC of in houses zoals JLCPCB Assembly (~€2–€4 per stuk) ([jlcpcb.com][3]).

---

**Welke module kiezen?**

Bij JLCPCB vind ik deze Espressif module "esp32-c6-devkitc-1-n8" maar de pin layout (2x 16p) verschilt van mijn drie (2x 15p) modules die ik gekocht heb. 

Varianten:

1) Officiele module van Espressif: "ESP32-C6-devkitc-1-n8" (2x 16p)
https://jlcpcb.com/partdetail/21893411-ESP32_C6_DEVKITC_1N8/C20554022

2) Modules die ik kocht: "ESP32-C6-WROOM-1-N8" of "ESP32-C6-WROOM-1-N16" Development Board by Dorhea
https://www.amazon.com/dp/B0DKF4NQHC   => VIDEO: https://youtu.be/LyMUUUmBnLI

Espressif product selector: https://products.espressif.com/#/product-selector?names=ESP32-C6-WROOM-1-N8



----------------

## Hoe een dergelijk project eruit ziet (user-ervaring)

Hoewel ik geen exacte blogpost met dit specifiekscenario kon vinden (ESP32-C6-WROOM-1 + China PCB productie + user story), bestaan er bruikbare voorbeelden zoals:

### ✔ Een ESP32-board maken en produceren

In een user-gedeeld project legt iemand stap-voor-stap uit:

* hoe hij een PCB ontwerpt rond een ESP32-module
* het PCB laat assembleren via JLCPCB
* gebruik van tools zoals EasyEDA of KiCad
* en hoe de productie verloopt — inclusief kwaliteit en prijs
  → Dit soort projecten zijn typerend voor hobbyisten die PCB’s + SMT assembly (incl. modules) bestellen. ([hackster.io][4])

---

### Samengevat — waarom dit aansluit bij jouw vraag

| Component                  | Wat het betekent                                                                                 |
| -------------------------- | ------------------------------------------------------------------------------------------------ |
| **ESP32-C6-WROOM-1**       | Module ideaal voor PCB’s — RF, flash en oscillator al op module aanwezig. ([espboards.dev][2])   |
| **PCB productie in China** | Services zoals PCBWay / JLCPCB accepteren Gerbers + BOM + Assembly opties. ([fritzenlab.net][1]) |
| **User project voorbeeld** | Hobbyist ontwerpt ESP32-C6 bord en laat produceren; deelt ervaring. ([fritzenlab.net][1])        |

---

## Conclusie

Wat jij wilt — een PCB die een **klaar module-ESP32** bevat en die ** volledig in China wordt geproduceerd** — is **exact de workflow die andere makers gebruiken**: ze ontwerpen rond een module zoals de ESP32-C6, uploaden bestanden, kiezen productie + SMT assembly, en ontvangen complete boards. ([fritzenlab.net][1])

Wil je dat ik je **een voorbeeld van een realtime blog of GitHub-repo** geef waarin iemand dit *stapsgewijs uitlegt* (inclusief BOM / Gerbers / productie tips)? Zeg “ja, toon link”.

[1]: https://fritzenlab.net/2024/07/30/esp32-c6-dev-board-pcbway-partnership/?utm_source=chatgpt.com "ESP32-C6 dev board - PCBWay partnership - FritzenLab electronics"
[2]: https://www.espboards.dev/esp32/microcontroller/esp32c6/esp32-c6-wroom-1/?utm_source=chatgpt.com "ESP32-C6-WROOM-1 Revisions"
[3]: https://jlcpcb.com/partdetail/ESPRESSIFINC-ESP32_C6_WROOM_1N4/C6034714?utm_source=chatgpt.com "ESP32-C6-WROOM-1-N4 | Espressif Systems | WiFi Modules | JLCPCB"
[4]: https://www.hackster.io/electroboy001/how-i-made-own-esp32-development-board-8af733?utm_source=chatgpt.com "How I made Own ESP32 Development board - Hackster.io"


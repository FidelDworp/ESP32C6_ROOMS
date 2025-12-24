📄 AI_CONTEXT.md

Je werkt aan een bestaand ESP32-project: “ESP32 Testroom Controller”.

Dit is een stabiele, in-productie zijnde sketch (TESTROOM.ino).
Stabiliteit heeft absolute prioriteit boven features.

Dit document vormt de enige geldige context voor samenwerking met AI-systemen (ChatGPT, Copilot, Grok, …) aan de sketch TESTROOM.ino.

BINDENDE REGELS:
- Geen code genereren tenzij ik dit expliciet vraag
- Geen refactors, geen herstructurering, geen “opkuis”
- Geen volledige code dumps
- Exact één wijziging per stap
- Altijd werken met LETTERLIJKE ankerregels uit mijn sketch
- Formaat: “zoek exact deze regel” → “voeg DIRECT NA toe”
- Geen HTML-wijzigingen zonder expliciete vraag
- R"rawliteral()" is extreem kwetsbaar

ARCHITECTUUR:
- AP-mode = configuratie / reddingsmodus
  * webserver moet altijd responsief zijn
  * GEEN sensor reads
  * GEEN blocking calls (pulseIn, delay)
  * GEEN mDNS
- STA-mode = normale werking
  * sensor reads toegestaan
  * mDNS alleen bij geldige STA-IP

HUIDIGE PRIORITEIT #1:
- Sensor reads volledig skippen in AP-mode
- Webserver mag nooit blokkeren

Werkwijze:
- Gebruik uitsluitend mijn TESTROOM.ino + DESIGN_RULES.md als waarheid
- Doe eerst analyse, schrijf GEEN code
- Wacht na elke stap op expliciete goedkeuring

Ik upload nu TESTROOM.ino.
Begin met een analyse van setup() en loop().

-----

Bindende context voor samenwerking met AI-systemen
Elke AI-output die deze context negeert, is onbetrouwbaar en niet bruikbaar.

1. Projectstatus (samenvatting)

Groot, bestaand ESP32-project

Draait in reële omgeving

Stabiliteit is belangrijker dan features

Webinterface gebouwd met R"rawliteral()" (extreem kwetsbaar)

AP-mode fungeert als reddings- en configuratiemodus

2. Nieuwe aanpak (strikt)
2.1 Clean start

Vertrek uitsluitend van de laatste stabiele werkende sketch

Geen code genereren tenzij Filip dit expliciet vraagt

2.2 Eerst stabiliteit, dan features

Absolute prioriteiten:

Static IP default = leeg (DHCP standaard)

Sensor reads volledig uitschakelen in AP-mode

Captive portal robuust en responsief houden

Pas daarna:

Sensor nicknames

Extra logica

Verdere uitbreidingen

3. Werkwijze (niet onderhandelbaar)

Deze regels zijn hard constraints:

❌ Geen volledige code dumps

❌ Geen refactors

❌ Geen herstructurering

❌ Geen “betere aanpak” voorstellen

✅ Exact één wijziging per stap

✅ Altijd werken met letterlijke ankerregels

✅ Exacte instructies:

“zoek exact deze regel”

“voeg hierna toe”

“vervang dit blok door”

❌ Geen HTML-wijzigingen zonder expliciete vraag

❌ Geen onnodige wijzigingen in R"rawliteral()"

Na elke HTML-wijziging:

testen op mobiel

sliders en toggles controleren

Na elke stap:

testen

wachten op expliciete goedkeuring

4. Overname van Grok → ChatGPT

Grok leverde een instabiele evolutie.

Deze repository beschrijft de ChatGPT-versie van 23 december 2025, met duidelijke verbeteringen in AP-mode.

4.1 Correct verbeterd

Captive portal (DNS hijack + OS-detectie)

mDNS lifecycle duidelijker

Heldere serial logging in AP-mode

4.2 Bewust NIET aangepast

Geen sensor nicknames

Geen rawliteral HTML-wijzigingen

Geen refactors

4.3 Kritische resterende fouten

Sensor reads lopen nog in AP-mode

pulseIn() blokkeert → webserver starvation

Watchdog reset bij openen van /settings

4.4 Volgende veilige stap

Sensor reads conditioneel uitschakelen in AP-mode
→ webserver moet altijd responsief blijven

5. Verplichte AI-startinstructie

Bij elk nieuw AI-gesprek moet expliciet vermeld worden:

“Gebruik mijn AI_CONTEXT.md en DESIGN_RULES.md als bindend contract.
Genereer geen code tenzij expliciet gevraagd.
Werk uitsluitend met exacte ankerpunten uit mijn TESTROOM.ino.”

Zonder deze context is AI-output niet bruikbaar.

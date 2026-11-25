# Overdracht — project "Rooms" (Zarlar ROOM-controllers)

*Stand 22 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Start van een **nieuw gesprek met Claude** over de ROOM-sketch en de overstap van de Photon-rooms naar ESP32-C6. Plan, `/json`-keys en versiegeschiedenis staan in de [README](../README.md); wat er in een room zit in de [ROOM-standaard](ROOM_STANDAARD.md). Overkoepelend: [`ZARLAR_OVERDRACHT.md`](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md).

## 1. Werkwijze

- **Nederlands**, ESP32-C6 in de **Arduino IDE**.
- **Nooit code zonder akkoord:** eerst een kort plan, dan code na **GO**. Eén stap per versie, met versiekop + changelog, daarna testen.
- **Alles via OTA.** Terminalcommando's voor Mac/RPi kant-en-klaar geven.
- **`/json`-keys en bedieningsadressen niet wijzigen** zonder akkoord.
- **Docs bijwerken bij elke stap**, beknopt, zonder achterhaalde info. De repo's zijn de enige bron. Geen sketches of `.md` in de chat tonen tenzij gevraagd.
- **Vragen één voor één**, duidelijk.
- **Logs lezen:** Google Sheets-koppeling, tab `Data` (nieuwste onderaan; uptime met punt als duizendtal).
- **Eén gesprek per repo tegelijk.** Bij twijfel over IP's, namen of verschillen tussen documenten: eerst met FiDel bespreken.

### GitHub

- Claude koppelt de repo's met schrijfrechten (`add_repo`, `push`) en commit rechtstreeks naar `main`, één commit per stap.
- **Indeling:** `README.md` in de root, documenten in `docs/`, sketchmap, `GoogleScript/`, `bin/` (alleen de laatste `.bin`), `old/`. Elke repo heeft een `docs/<NAAM>_OVERDRACHT.md`.
- Nieuwe repo's, "About" en releases doet FiDel.

## 2. Controllers

| Controller | Adres | Stand |
|---|---|---|
| **room80** (testbord, eetplaats) | 192.168.0.80 · `eetplaats.local` | **ROOM v3.6.2** (25/11) |
| Dashboard (matrix, logging) | 192.168.0.60 | v6.5 (22/11) |
| RPi (portal + Apple Home-bridge) | 192.168.0.50:3000 | `update.sh` 22/11 |
| HVAC · ECO-boiler · S-ENERGY | .70 · .71 · .73 | eigen repo's |
| Photon-rooms | — | nog in productie, behalve de B&B (defect) |

**IP-plan:** .50 RPi · .60 Dashboard · .70 HVAC · .71 ECO · .72 S-OUTSIDE (gereserveerd) · .73 S-ENERGY · .75 B&B · .76 Badkamer · .77 Inkom · .78 Keuken · .79 Wasplaats · .80 Eetplaats · .81 Zitplaats · .82/.83/.84 ACCESS1/2/3. Telenet laat geen DHCP-reservering toe: vaste IP's staan in de controller.

**Dashboard-matrix:** rij 0 HVAC · 1 ECO · 2 S-ENERGY · 3 ACCESS · 5–11 rooms. Kolom 12 = LDR (sinds v6.5 nieuwe schaal).

**Sheets:** R-ROOMS Log `1UiaexdpgpL3XjPWHNdMEbJ18_H9yp1w_VN8VBdVCpY0` (GAS v1.6) · S-ECO `1nn_GBmLhS9zZhp5On_3r-LRLonetSjnHCuHZiNOc4Q0` · S-HVAC `1LL7swNRIVl2qfREHxAU7nCq9fH9zTUxa37s0IduHTXY`.

## 3. ROOM-sketch: stand

- **v3.5 → v3.6 op 22/11 geflasht** (v3.5.1 veilige OTA, v3.5.2 opruimen + Generic ADC, v3.5.3 LDR omgekeerd, v3.6 WiFi + mDNS). Diagnose telkens in orde. v3.5 haalde vooraf 44 u zonder herstart.
- **Thuis nog te testen** (FiDel):
  1. Sensoren lostrekken: DS18B20, daarna DHT22 → terugval en melding op `/advanced`.
  2. IO2 met een draadje aan GND → Generic ADC ±0,00 V.
  3. `http://eetplaats.local/`.
  4. Router 2–3 min uit → room80 komt vanzelf terug, Diagnose "1 keer weggevallen" met reden. Optioneel: router en room80 samen herstarten → kort nood-AP, dan terug op het netwerk.
  5. Bewegingslamp 's avonds aan, overdag niet (LDR-drempel omgerekend).
- **Volgende stappen:** zie het plan in de README. **v3.6.1 OTA-voortgangspagina** (uit Vakantiehuizen v1.13) geflasht en getest 25/11: voortgangsbalk, "Terug online" en sprong naar /advanced werken. **v3.6.2** (paginatitels met de kamernaam) geflasht 25/11 via de nieuwe OTA-pagina: in orde. Open vraag aan FiDel: **pixelmodi** vóór de B&B live gaat (per pixel manueel / beweging / nachtlampje — aan bij donker, uit in BED-modus — en een kleur per pixel; nodig voor de vloermat op pixel 1).

**Technisch om te onthouden**
- IO12/IO13 = USB D−/D+ van de C6: compileren met CDC uit + `releaseUsbPins()`; I2C (SCD41) op IO13/IO11.
- DHT22 alleen met de eigen uitlezing (`dhtRead22`), nooit de Adafruit-bibliotheek.
- Geplande herstarts (OTA, /reboot, instellingen) gaan via `restart_at` in `loop()`, met `restart_cause` voor de herstartgeschiedenis.
- `ap_mode_active` = setupmodus (geen SSID, geen metingen/verwarming); `ap_rescue` = nood-AP met normale werking.
- mDNS-naam komt uit de kamernaam in `/settings` (room80 "Eetplaats").

## 4. Hardware

Standaard: [`ROOM_STANDAARD.md`](ROOM_STANDAARD.md) · ombouw: [`aansluitingen.md`](aansluitingen.md) en `Zarlar_Photon_inventaris_v3.xlsx`.
- RoomSense op 3,3 V; geen signaal boven 3,3 V op een ESP32-pin (elke ader meten).
- **SCD41** (I2C) voor B&B, BADK, ZITPL (4 besteld). Pull-ups 3,3 kΩ op het shield.
- **Powerpixels:** één PowerPiXel-TRIO per lichtkring, kanalen parallel, fix-draadje naar +12V, zekering per kring op maat. B&B-invulling: `aansluitingen.md` §4.4 (17 pixels: 5 Powerpixels + 12 trede-pixels).
- **Eerste room: B&B** (.75, R-BandB). Bij ingebruikname: flashen, kamernaam, IP .75, 17 pixels, NVS opruimen, en op het Dashboard → `/settings` R-BandB op **actief** zetten (anders geen logging).

## 5. Andere open punten

- **ACCESS** (buitenverlichting): eigen gesprek en repo `ESP32C6_ACCESS`. N2/E1-drivers defect (met Maarten).
- **S-ENERGY:** kop noemt nog `partitions_16mb.csv` → rechtzetten bij v1.31.
- **Sjalay:** eigen gesprek; v1.12 wacht op de SCD41.

## 6. Praktisch

- **Compileren in de sessie:** `cd /home/claude/tools && python3 gen_protos.py <sketchmap>`, daarna `./arduino-cli --config-file cli.yaml compile --fqbn esp32:esp32:esp32c6:FlashSize=16M,PartitionScheme=custom,CDCOnBoot=default --build-path build --build-property "runtime.tools.ctags.path=/home/claude/tools/fakectags" /tmp/bp/ESP32_C6_ROOM`. Structs bovenaan de sketch (vóór de automatische prototypes). Dashboard en andere C6-sketches: `CDCOnBoot=cdc`.
- **Flashen van op afstand:** vanaf de Mac via mobiele data + Tailscale met `curl` (commando's in de README); de wifi in Nieuwpoort gebruikt ook 192.168.0.x en botst met de Tailscale-route. Alternatief via de RPi: overkoepelende `docs/DEPLOY.md` §A2.
- **RPi bijwerken:** `ssh -t fidel@192.168.0.50 'bash ~/update.sh'`.
- **Google Apps Script:** altijd "Nieuwe versie", nooit "Nieuwe implementatie".

# Overdracht — project "Rooms" (Zarlar ROOM-controllers)

*Stand 25 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Start van een **nieuw gesprek met Claude** over de ROOM-sketch en de overstap van de Photon-rooms naar ESP32-C6. Wat de sketch doet, het plan, de `/json`-keys en de versies staan in de [README](../README.md); wat er in een room zit in de [ROOM-standaard](ROOM_STANDAARD.md); de ombouw en de B&B in [aansluitingen.md](aansluitingen.md). Overkoepelend: [`ZARLAR_OVERDRACHT.md`](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md).

## 1. Werkwijze

- **Nederlands**, ESP32-C6 in de **Arduino IDE**.
- **Nooit code zonder akkoord:** eerst een kort plan, dan code na **GO**. Eén stap per versie, met versiekop + changelog, daarna testen.
- **Alles via OTA.** Terminalcommando's voor Mac/RPi kant-en-klaar geven.
- **`/json`-keys en bedieningsadressen niet wijzigen** zonder akkoord.
- **Docs bijwerken bij elke stap**, beknopt, zonder achterhaalde info. De repo's zijn de enige bron. Geen sketches of `.md` in de chat tonen tenzij gevraagd.
- **Vragen één voor één**, duidelijk.
- **Eén gesprek per repo tegelijk.** Bij twijfel over IP's, namen of verschillen tussen documenten: eerst met FiDel bespreken.
- **GitHub:** Claude koppelt de repo's met schrijfrechten (`add_repo`, `push`) en commit rechtstreeks naar `main`. Indeling: `README.md` in de root, documenten in `docs/`, sketchmap, `GoogleScript/`, `bin/` (alleen de laatste `.bin`).

## 2. Stand

| Controller | Adres | Stand |
|---|---|---|
| **room80** (testbord, eetplaats) | 192.168.0.80 · `eetplaats.local` | **ROOM v3.10** (26/11), v3.11 klaar |
| B&B | 192.168.0.75 · R-BandB | in montage (Trespa-plaat), opstarten volgens `aansluitingen.md` §2.4 |
| Dashboard (matrix, logging) | 192.168.0.60 | v6.6 |
| RPi (portal + Apple Home-bridge) | 192.168.0.50:3000 | bijgewerkt 26/11 (bridge v0.8.1, tab Lichten met namen) |
| Photon-rooms | — | nog in productie, behalve de B&B |

**IP-plan:** .50 RPi · .60 Dashboard · .70 HVAC · .71 ECO · .73 S-ENERGY · .75 B&B · .76 Badkamer · .77 Inkom · .78 Keuken · .79 Wasplaats · .80 Eetplaats · .81 Zitplaats · .82/.83/.84 ACCESS1/2/3. Vaste IP's staan in de controller zelf (Telenet laat geen DHCP-reservering toe).

**Sheet** R-ROOMS Log: `1UiaexdpgpL3XjPWHNdMEbJ18_H9yp1w_VN8VBdVCpY0`, tab `Data` (nieuwste onderaan; uptime met punt als duizendtal). Lezen via de Google Sheets-koppeling.

**v3.8.1** (vorstbeveiliging pas 3 min na de opstart, `FROST_GRACE_MS`, `frost_since`) geflasht 26/11 ±21:04 (eerste rij uptime 120 s: HEAT 0). **v3.8b** (effectstrook: `FxCfg fx` = NVS-blob "fx", `updateEffect()` ±40 beelden/s, `inFx()`; DS18B20 niet-blokkerend `dsStartConversion()`/`dsReadResults()`) gebouwd 26/11. Strookvelden kwamen via het lange GET-formulier nooit aan → **v3.8b.1**: eigen formulier + `/set_fx` (live, `fx_apply_req` in loop), groot formulier via POST (getest, werkt). **v3.9** (LDR elke 2 s, `updateLdr()`, `is_dark` met hysterese + 10 s) gebouwd 26/11. v3.9 geflasht en goed bevonden. **v3.10** lange effectstrook gebouwd 26/11: `FxCfg.len` (0–300) na de gewone pixels (`MAX_PIXELS` 20), strook = lamp-index `FXI` = `pixels_num` in t/u/tegels/tik, effecten 4 regenboog en 5 kleurwissel, stroomgrens `maxA`, migratie van de v3.8b-blob (`FxCfgV38`); geflasht, werkt. **v3.11** gebouwd 26/11: /advanced-pixelrij `data-m` (1 = beweging → schakelaar uit `u`, anders echte stand uit `t`), live strookformulier `fxa` onder de strookrij (zelfde `/set_fx`, pagina herlaadt na Toepassen), snelheid 2–60, `/capabilities` + `fx.name` op index `FXI`; RPi index.html tab Lichten haalt namen uit `/api/capabilities/room80?refresh=1`. Daarna: B&B live (zie plan in de README).

**Nog thuis te testen (FiDel):** IO2 aan GND → Generic ADC ±0,00 V · `http://eetplaats.local/` · router 2–3 min uit → room80 komt vanzelf terug, Diagnose "1 keer weggevallen" · bewegingslamp 's avonds aan, overdag niet.

## 3. Referentie uit de Sheet (25/11, room80)

Om later mee te vergelijken:
- **Geheugen:** vrij 64–66 %, grootste blok 223–239 KB, laagste heap ooit 212–223 KB, geen daling over de versies. RSSI −54…−63 dBm.
- **Stabiliteit:** v3.6 liep 72 u zonder herstart. Enige onverwachte herstart: **23/11 ±18:12 = stroompanne** (Dashboard logde niet van 18:04 tot 19:21); room80 kwam vanzelf terug. Daarna 49 u zonder herstart tot de flashes van 25/11 (alle herstarts die dag = OTA).
- **Kamer 's avonds:** 23,2–23,7 °C, vocht 28 %, dauwpunt ±9 °C; CO2 540 → 700 ppm, ventilatie 16–37 %; SCD41 23,0–23,4 °C / 35 %.
- **Logging:** elke 5 min, kolommen kloppen (`ak` in kolom AK sinds v3.8; DS 2–6 leeg want room80 heeft 1 DS18B20).

## 4. Technisch om te onthouden

- **IO12/IO13 = USB D−/D+** van de C6: compileren met CDC uit + `releaseUsbPins()`; I2C (SCD41) op IO13/IO11.
- **DHT22** alleen met de eigen uitlezing (`dhtRead22`), nooit de Adafruit-bibliotheek (interrupt-watchdog).
- **Geplande herstarts** (OTA, /reboot, instellingen) via `restart_at` in `loop()`, met `restart_cause` voor de herstartgeschiedenis. Nooit `delay()` of `ESP.restart()` in een webserver-callback.
- **Lessen Vakantiehuizen 26/11:** de herstarts en mislukte POSTs van het Ysermonde-bord kwamen door het netwerk (vast IP in de DHCP-reeks, DNS van de router antwoordde niet). SNTP weghalen, reserve-DNS 8.8.8.8, "enkel AP" en een andere Async TCP-versie zijn daar voorbarig toegepast en zijn voor ROOM **niet** nodig (zie README, geschrapt item). Huisregel: vaste IP's onder .100.
- **WiFi:** `ap_mode_active` = setupmodus (geen SSID, geen metingen/verwarming); `ap_rescue` = nood-AP met normale werking. mDNS-naam komt uit de kamernaam (room80 "Eetplaats").
- **Pixels:** `px_mode[]` (0 manueel, 1 MOV1, 2 MOV2, 3 nachtlampje), `px_ovr[]`, `px_col[]`; NVS `px_modes`, `px_ovr`, `px_cols`; `pxTap()` achter `/toggle_pixel` en `/toggle_pixel_mode`.
- **/advanced-JavaScript** zoekt rijen op labeltekst (behalve pixelrijen: `data-px`). Een nieuw label mag geen bestaand zoekwoord bevatten (bv. "Dauwpunt", "CO", "Pixel").
- **RPi-startpagina** leest de eetplaats als `room80` uit `/api/status` (de alias `room` wordt daar overgeslagen).

## 5. Praktisch

- **Compileren in de sessie:** `cd /home/claude/tools && python3 gen_protos.py <sketchmap>`, daarna `./arduino-cli --config-file cli.yaml compile --fqbn esp32:esp32:esp32c6:FlashSize=16M,PartitionScheme=custom,CDCOnBoot=default --build-path build --build-property "runtime.tools.ctags.path=/home/claude/tools/fakectags" /tmp/bp/ESP32_C6_ROOM`. Structs bovenaan de sketch (vóór de automatische prototypes). JavaScript nakijken met `node --check`. Dashboard en andere C6-sketches: `CDCOnBoot=cdc`.
- **Flashen van op afstand:** Mac via mobiele data + Tailscale (commando's in de README). Niet via de wifi in Nieuwpoort (zelfde 192.168.0.x).
- **RPi bijwerken:** `ssh -t fidel@192.168.0.50 'bash ~/update.sh'`.
- **Google Apps Script:** altijd "Nieuwe versie", nooit "Nieuwe implementatie".

## 6. Andere gesprekken

- **ACCESS** (buitenverlichting): repo `ESP32C6_ACCESS`. N2/E1-drivers in de B&B defect (met Maarten).
- **Vakantiehuizen** (Sjalay/Ysermonde): repo `VAKANTIEHUIZEN`.

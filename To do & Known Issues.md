📄 To do & Known Issues.md

ESP32 Testroom Controller

Bekende problemen & technische schuld

1. Actuele kritische problemen
   
1.1 Sensor reads in AP-mode
   
- Status: ❌ Niet opgelost
- pulseIn() blokkeert
- Webserver starvation
- Watchdog resets bij /settings

Vastgelegde oplossing: Sensor reads conditioneel uitschakelen in AP-mode

1.2 Safari captive portal gedrag

- Onvoorspelbaar
- Soms manuele refresh nodig

➡️ Geen bug, aanvaard gedrag

2. Historische fouten (lessons learned)

Ongeldige static IP defaults
Lange delay() in setup()
ESP32 core v3.x watchdog-gevoeligheid

3. Volgende veilige stap

Sensor reads volledig uitschakelen in AP-mode
Daarna pas:
- sensor defect-detectie
- sensor nicknames
- Matter-koppeling
- extra endpoints

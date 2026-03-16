// ============================================================
// ROOM CONTROLLER DATA LOGGER - Google Apps Script v1.4
// Ontvangt JSON-push van de Zarlar Dashboard (room controllers)
// en logt naar Google Sheet — één sheet voor alle kamers.
//
// v1.4 (15mar26):
//   - Headers aangepast (jouw finale titels), 10pt wit op zwart,
//     niet vet, niet italic, gecentreerd, word wrap, smalle kolommen
//   - MAX_ROWS limiet toegevoegd (default 1000, makkelijk aanpasbaar)
//     → oudste rijen worden automatisch verwijderd als limiet bereikt
//   - DEPLOYMENT: gebruik ALTIJD "Bestaande implementatie bewerken"
//     → nooit "Nieuwe implementatie" — dat geeft een nieuwe URL!
//     → Zarlar Dashboard moet dan ook de nieuwe URL krijgen
//
// v1.3 (15mar26): volledig schema a..ak, 37 kolommen
// v1.2 (14mar26): P= prefix pixel string
// v1.1 (14mar26): JSON-keys hernummerd naar schema v2.5
//
// ⚠️  DEPLOYMENT INSTRUCTIE (BELANGRIJK):
//   Gebruik bij elke update: "Implementeren" → "Implementaties beheren"
//   → potlood-icoon (Bewerken) → versienummer verhogen → Implementeren
//   De URL blijft dan DEZELFDE → Zarlar Dashboard hoeft niet aangepast!
//   "Nieuwe implementatie" geeft ALTIJD een nieuwe URL → updates stoppen.
//
// ============================================================

// ============================================================
// ⚙️  CONFIGURATIE — pas hier aan zonder de rest aan te raken
// ============================================================
const MAX_ROWS = 1000;  // Maximum aantal datarijen (excl. header)
                        // Oudste rijen worden verwijderd als limiet bereikt
                        // Pas aan naar wens (bv 500, 2000, 5000)
// ============================================================


function doPost(e) {
  try {
    const data = JSON.parse(e.postData.contents);
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

    const timestamp = Utilities.formatDate(
      new Date(),
      "Europe/Brussels",
      "yyyy-MM-dd HH:mm:ss"
    );

    // pixel_on_str: sketch stuurt "P=00111000" — geen pure cijferstring
    // → appendRow() converteert niet naar getal → leading zeros bewaard
    const pixelOnStr = data.t || "P=00000000";

    const row = [
      timestamp,              // A:  Tijdstempel
      data.a   || 0,          // B:  Uptime (s)
      data.room || "?",       // C:  Kamer
      data.b   || 0,          // D:  HEAT - Heating_on (0/1)
      data.c   || 0,          // E:  Set (°C) - Heating_setpoint
      data.d   || 0,          // F:  TSTAT (0/1)
      data.e   || 0,          // G:  Temp1 (°C) - DHT22
      data.f   || 0,          // H:  Temp2 (°C) - DS18B20 primair
      data.g   || 0,          // I:  Vent (%) - ventilatie
      data.h   || 0,          // J:  Vocht (%) - vochtigheid
      data.i   || 0,          // K:  Dauwpt (°C)
      data.j   || 0,          // L:  Dew Alert (0/1)
      data.k   || 0,          // M:  CO2 (ppm)
      data.l   || 0,          // N:  Stof (raw)
      data.m   || 0,          // O:  Licht LDR (0-100)
      data.n   || 0,          // P:  Zon (lux)
      data.o   || 0,          // Q:  Nacht (0/1)
      data.p   || 0,          // R:  Bed (0/1)
      data.q   || 0,          // S:  R (0-255)
      data.r   || 0,          // T:  G (0-255)
      data.s   || 0,          // U:  B (0-255)
      pixelOnStr,             // V:  Pixels aan (bv "P=10001000")
      data.u   || "",         // W:  Pixel mode
      data.v   || 0,          // X:  Thuis (0/1)
      data.w   || 0,          // Y:  MOV1 trig/min
      data.x   || 0,          // Z:  MOV2 trig/min
      data.y   || 0,          // AA: MOV1 licht (0/1)
      data.z   || 0,          // AB: MOV2 licht (0/1)
      data.aa  || 0,          // AC: Beam waarde (0-100)
      data.ab  || 0,          // AD: Beam alert (0/1)
      data.ac  || 0,          // AE: RSSI (dBm)
      data.ad  || 0,          // AF: Mem (%)
      data.ae  || 0,          // AG: Heap block (KB)
      data.af  || 0,          // AH: Heap min (KB)
      data.ag  || 0,          // AI: DS count
      data.ah  || "",         // AJ: Tds2 (°C)
      data.ai  || "",         // AK: Tds3 (°C)
    ];

    sheet.appendRow(row);

    // MAX_ROWS bewaking: verwijder oudste datarij als limiet overschreden
    // Rij 1 = header, datarijen starten op rij 2
    const dataRows = sheet.getLastRow() - 1;  // excl. header
    if (dataRows > MAX_ROWS) {
      sheet.deleteRow(2);  // verwijder oudste rij (rij 2, net onder header)
      Logger.log("MAX_ROWS (" + MAX_ROWS + ") bereikt — oudste rij verwijderd");
    }

    return ContentService
      .createTextOutput(JSON.stringify({
        status:    "success",
        message:   "Room data gelogd",
        timestamp: timestamp,
        room:      data.room || "?",
        uptime:    data.a    || 0
      }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    Logger.log("doPost fout: " + error.toString());
    return ContentService
      .createTextOutput(JSON.stringify({
        status:  "error",
        message: error.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}


// ============================================================
// SETUP — voer eenmalig uit via Uitvoeren → setupHeaders
// ============================================================
function setupHeaders() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

  // Verwijder bestaande koprij als aanwezig
  if (sheet.getLastRow() > 0) {
    const firstCell = sheet.getRange(1, 1).getValue();
    if (firstCell === "Tijdstempel") {
      sheet.deleteRow(1);
      Logger.log("Bestaande koprij verwijderd.");
    }
  }

  // Jouw finale kolomtitels
  const headers = [
    "Tijdstempel",    // A  - breed
    "Uptime (s)",     // B
    "Kamer",          // C  - breed, bevroren
    "HEAT",           // D
    "Set (°C)",       // E
    "TSTAT",          // F
    "Temp1 (°C)",     // G
    "Temp2 (°C)",     // H
    "Vent (%)",       // I
    "Vocht (%)",      // J
    "Dauwpt (°C)",    // K
    "Dew Alert",      // L
    "CO2 (ppm)",      // M
    "Stof",           // N
    "Licht LDR",      // O
    "Zon (lux)",      // P
    "Nacht",          // Q
    "Bed",            // R
    "R",              // S
    "G",              // T
    "B",              // U
    "Pixels aan",     // V  - breed
    "Pixel mode",     // W
    "Thuis",          // X
    "MOV1",           // Y
    "MOV2",           // Z
    "MOV1 licht",     // AA
    "MOV2 licht",     // AB
    "Beam waarde",    // AC
    "Beam alert",     // AD
    "RSSI (dBm)",     // AE
    "Mem (%)",        // AF
    "Heap block",     // AG
    "Heap min",       // AH
    "DS count",       // AI
    "Tds2 (°C)",      // AJ
    "Tds3 (°C)",      // AK
  ];

  sheet.insertRowBefore(1);
  const headerRange = sheet.getRange(1, 1, 1, headers.length);
  headerRange.setValues([headers]);

  // Opmaak: 10pt, wit op zwart, niet vet, niet italic, gecentreerd, wrap
  headerRange.setFontSize(10);
  headerRange.setFontWeight("normal");
  headerRange.setFontStyle("normal");
  headerRange.setFontColor("#ffffff");
  headerRange.setBackground("#000000");
  headerRange.setHorizontalAlignment("center");
  headerRange.setVerticalAlignment("middle");
  headerRange.setWrap(true);  // woorden wrappen → 2 rijen mogelijk

  // Rijhoogte header: genoeg voor 2 regels tekst op 10pt
  sheet.setRowHeight(1, 40);

  // Kolombreedtes: smal genoeg om woorden te laten wrappen
  sheet.setColumnWidth(1,  130);  // A: Tijdstempel
  sheet.setColumnWidth(2,   60);  // B: Uptime
  sheet.setColumnWidth(3,   80);  // C: Kamer
  sheet.setColumnWidth(4,   45);  // D: HEAT
  sheet.setColumnWidth(5,   55);  // E: Set (°C)
  sheet.setColumnWidth(6,   45);  // F: TSTAT
  sheet.setColumnWidth(7,   60);  // G: Temp1
  sheet.setColumnWidth(8,   60);  // H: Temp2
  sheet.setColumnWidth(9,   50);  // I: Vent
  sheet.setColumnWidth(10,  50);  // J: Vocht
  sheet.setColumnWidth(11,  60);  // K: Dauwpt
  sheet.setColumnWidth(12,  55);  // L: Dew Alert
  sheet.setColumnWidth(13,  60);  // M: CO2
  sheet.setColumnWidth(14,  45);  // N: Stof
  sheet.setColumnWidth(15,  55);  // O: Licht LDR
  sheet.setColumnWidth(16,  55);  // P: Zon
  sheet.setColumnWidth(17,  45);  // Q: Nacht
  sheet.setColumnWidth(18,  40);  // R: Bed
  sheet.setColumnWidth(19,  35);  // S: R
  sheet.setColumnWidth(20,  35);  // T: G
  sheet.setColumnWidth(21,  35);  // U: B
  sheet.setColumnWidth(22, 110);  // V: Pixels aan
  sheet.setColumnWidth(23,  70);  // W: Pixel mode
  sheet.setColumnWidth(24,  45);  // X: Thuis
  sheet.setColumnWidth(25,  45);  // Y: MOV1
  sheet.setColumnWidth(26,  45);  // Z: MOV2
  sheet.setColumnWidth(27,  55);  // AA: MOV1 licht
  sheet.setColumnWidth(28,  55);  // AB: MOV2 licht
  sheet.setColumnWidth(29,  60);  // AC: Beam waarde
  sheet.setColumnWidth(30,  55);  // AD: Beam alert
  sheet.setColumnWidth(31,  60);  // AE: RSSI
  sheet.setColumnWidth(32,  50);  // AF: Mem
  sheet.setColumnWidth(33,  60);  // AG: Heap block
  sheet.setColumnWidth(34,  55);  // AH: Heap min
  sheet.setColumnWidth(35,  55);  // AI: DS count
  sheet.setColumnWidth(36,  60);  // AJ: Tds2
  sheet.setColumnWidth(37,  60);  // AK: Tds3

  sheet.setFrozenRows(1);
  sheet.setFrozenColumns(3);  // A+B+C zichtbaar bij horizontaal scrollen

  Logger.log("Headers aangemaakt! " + headers.length + " kolommen (A t/m AK)");
  Logger.log("MAX_ROWS instelling: " + MAX_ROWS);
}


// ============================================================
// TEST — simuleer een POST zoals de dashboard die stuurt
// Voer uit via Uitvoeren → test
// ============================================================
function test() {
  const testData = {
    postData: {
      contents: JSON.stringify({
        "room": "R-EETPLAATS",
        "a":  3600,
        "b":  1,
        "c":  21,
        "d":  1,
        "e":  21.4,
        "f":  20.9,
        "g":  25,
        "h":  58,
        "i":  12.3,
        "j":  0,
        "k":  620,
        "l":  45,
        "m":  72,
        "n":  310,
        "o":  0,
        "p":  0,
        "q":  255,
        "r":  128,
        "s":  0,
        "t":  "P=10001000",
        "u":  "00",
        "v":  1,
        "w":  3,
        "x":  1,
        "y":  1,
        "z":  0,
        "aa": 35,
        "ab": 0,
        "ac": -61,
        "ad": 68,
        "ae": 42,
        "af": 38,
        "ag": 2,
        "ah": 19.5,
        "ai": 22.1
      })
    }
  };

  const result = doPost(testData);
  Logger.log(result.getContent());
}

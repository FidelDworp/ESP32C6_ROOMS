// ============================================================
// ROOM CONTROLLER DATA LOGGER - Google Apps Script v1.6
// Ontvangt JSON-push van de Zarlar Dashboard (room controllers)
// en logt naar Google Sheet, tabblad "Data" — één tabblad voor alle kamers.
//
// v1.6 (20nov26):
//   - Nieuwe kolomindeling (43 kolommen). A–M ongewijzigd (grafiek "CO2" blijft werken).
//     Eruit: Stof (l), Zon (n), Beam waarde/alert (aa, ab).
//     Erbij: Tds4–Tds6 (aj, an, ao), Eff. setpoint (ak), SCD temp/vocht (al, am),
//     GP1–GP4 (ap, aq, ar = volt, as).
//   - Eén kolomlijst (COLUMNS) voor koppen én rijen: nooit meer verschuiven.
//   - Ontbrekende of null-waarden worden een LEGE cel (niet 0) voor metingen
//     (temperaturen, vocht, SCD, GP): geen duik naar nul in grafieken.
//     ROOM v3.5 stuurt null voor een ongeldige DHT22/DS18B20-meting.
//   - Schrijft altijd naar het tabblad "Data" (niet het actieve tabblad).
//   - MAX_ROWS 20000 (±35 dagen voor 2 rooms om de 5 minuten).
//   - Kamerfilter: keuzelijst in C1 (ALL + alle rooms). onEdit zet een filter op
//     kolom C; doPost voegt nieuwe rooms toe aan de lijst en past het filter toe
//     op nieuwe rijen. Titel van rij 1 staat nu in D1.
//   - archiveerEnNieuweKoppen(): eenmalig — kopieert "Data" naar "Archief v1.5",
//     maakt "Data" leeg en zet de nieuwe koppen + keuzelijst.
//
// v1.5 (18mar26): 2 bevroren rijen (titel+URL, kolomtitels), HEADER_ROWS = 2
// v1.4 (15mar26): headers 10pt wit op zwart, MAX_ROWS
// v1.3 (15mar26): volledig schema a..ak, 37 kolommen
// v1.2 (14mar26): P= prefix pixel string
// v1.1 (14mar26): JSON-keys hernummerd naar schema v2.5
//
// ⚠️  DEPLOYMENT INSTRUCTIE (BELANGRIJK):
//   Gebruik bij elke update: "Implementeren" → "Implementaties beheren"
//   → potlood-icoon (Bewerken) → Versie: "Nieuwe versie" → Implementeren
//   De URL blijft dan DEZELFDE → Zarlar Dashboard hoeft niet aangepast!
//   "Nieuwe implementatie" geeft ALTIJD een nieuwe URL → updates stoppen.
//
// ============================================================

// ============================================================
// ⚙️  CONFIGURATIE — pas hier aan zonder de rest aan te raken
// ============================================================
const SHEET_NAME   = "Data";          // tabblad waar gelogd wordt
const ARCHIVE_NAME = "Archief v1.5";  // tabblad voor de oude rijen (archiveerEnNieuweKoppen)
const MAX_ROWS     = 20000;           // maximum aantal datarijen (excl. koppen)
const HEADER_ROWS  = 2;               // rij 1 = keuzelijst (C1) + titel (D1), rij 2 = kolomtitels
const FILTER_CELL  = "C1";            // keuzelijst kamerfilter (boven de kolom "Kamer")
const ROOM_COL     = 3;               // kolom C = Kamer
// ============================================================

// Kolomlijst: [titel, JSON-key, soort]
//   "num0"  ontbrekend → 0      (tellers, vlaggen, standen)
//   "leeg"  ontbrekend/null → lege cel (metingen)
//   "tekst" ontbrekend → lege cel
//   speciale keys: "_ts" (tijdstempel), "room", "t" (pixelstring)
const COLUMNS = [
  ["Tijdstempel",   "_ts",  "tekst"],  // A
  ["Uptime (s)",    "a",    "num0"],   // B
  ["Kamer",         "room", "tekst"],  // C
  ["HEAT",          "b",    "num0"],   // D
  ["Set (°C)",      "c",    "num0"],   // E
  ["TSTAT",         "d",    "num0"],   // F
  ["Temp1 (°C)",    "e",    "leeg"],   // G  DHT22
  ["Temp2 (°C)",    "f",    "leeg"],   // H  DS18B20 primair
  ["Vent (%)",      "g",    "num0"],   // I
  ["Vocht (%)",     "h",    "leeg"],   // J
  ["Dauwpt (°C)",   "i",    "leeg"],   // K
  ["Dew Alert",     "j",    "num0"],   // L
  ["CO2 (ppm)",     "k",    "leeg"],   // M
  ["Licht LDR",     "m",    "num0"],   // N
  ["Nacht",         "o",    "num0"],   // O
  ["Bed",           "p",    "num0"],   // P
  ["R",             "q",    "num0"],   // Q
  ["G",             "r",    "num0"],   // R
  ["B",             "s",    "num0"],   // S
  ["Pixels aan",    "t",    "tekst"],  // T
  ["Pixel mode",    "u",    "tekst"],  // U
  ["Thuis",         "v",    "num0"],   // V
  ["MOV1",          "w",    "num0"],   // W
  ["MOV2",          "x",    "num0"],   // X
  ["MOV1 licht",    "y",    "num0"],   // Y
  ["MOV2 licht",    "z",    "num0"],   // Z
  ["RSSI (dBm)",    "ac",   "num0"],   // AA
  ["Mem (%)",       "ad",   "num0"],   // AB
  ["Heap block",    "ae",   "num0"],   // AC
  ["Heap min",      "af",   "num0"],   // AD
  ["DS count",      "ag",   "num0"],   // AE
  ["Tds2 (°C)",     "ah",   "leeg"],   // AF
  ["Tds3 (°C)",     "ai",   "leeg"],   // AG
  ["Tds4 (°C)",     "aj",   "leeg"],   // AH
  ["Tds5 (°C)",     "an",   "leeg"],   // AI
  ["Tds6 (°C)",     "ao",   "leeg"],   // AJ
  ["Eff. set (°C)", "ak",   "leeg"],   // AK
  ["SCD temp (°C)", "al",   "leeg"],   // AL
  ["SCD vocht (%)", "am",   "leeg"],   // AM
  ["GP1",           "ap",   "leeg"],   // AN
  ["GP2",           "aq",   "leeg"],   // AO
  ["GP3 (V)",       "ar",   "leeg"],   // AP
  ["GP4",           "as",   "leeg"],   // AQ
];
const NCOLS = COLUMNS.length;  // 43


// ============================================================
// ONTVANGEN — elke 5 minuten per room, vanuit de Zarlar Dashboard
// ============================================================
function doPost(e) {
  try {
    const data  = JSON.parse(e.postData.contents);
    const sheet = getDataSheet_();

    const timestamp = Utilities.formatDate(new Date(), "Europe/Brussels", "yyyy-MM-dd HH:mm:ss");
    const room = data.room || "?";

    const row = COLUMNS.map(function (col) {
      const key = col[1], kind = col[2];
      if (key === "_ts")  return timestamp;
      if (key === "room") return room;
      if (key === "t")    return data.t || "P=00000000";  // "P=" → leading zeros blijven bewaard
      const v = data[key];
      if (v === undefined || v === null || v === "") return (kind === "num0") ? 0 : "";
      return v;
    });

    sheet.appendRow(row);

    // MAX_ROWS bewaking: verwijder oudste datarij (rij 3) als limiet overschreden
    if (sheet.getLastRow() - HEADER_ROWS > MAX_ROWS) {
      sheet.deleteRow(HEADER_ROWS + 1);
    }

    // Kamerfilter: nieuwe room in de keuzelijst, actief filter ook op de nieuwe rij
    const lijstGewijzigd = addRoomToList_(sheet, room);
    const keuze = String(sheet.getRange(FILTER_CELL).getValue() || "ALL");
    if (keuze !== "ALL" || lijstGewijzigd) applyRoomFilter_(sheet, keuze);

    return ContentService
      .createTextOutput(JSON.stringify({ status: "success", message: "Room data gelogd",
                                         timestamp: timestamp, room: room, uptime: data.a || 0 }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    Logger.log("doPost fout: " + error.toString());
    return ContentService
      .createTextOutput(JSON.stringify({ status: "error", message: error.toString() }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}


// ============================================================
// KAMERFILTER — keuzelijst in C1
// ============================================================

// Simple trigger: draait vanzelf als iemand de Sheet bewerkt.
function onEdit(e) {
  if (!e || !e.range) return;
  const sheet = e.range.getSheet();
  if (sheet.getName() !== SHEET_NAME) return;
  if (e.range.getA1Notation() !== FILTER_CELL) return;
  applyRoomFilter_(sheet, String(e.value || "ALL"));
}

// Filter op kolom C: één room tonen, of alles (ALL). Verbergt alleen rijen, wist niets.
function applyRoomFilter_(sheet, keuze) {
  const bestaand = sheet.getFilter();
  if (bestaand) bestaand.remove();
  if (!keuze || keuze === "ALL") return;
  const lastRow = Math.max(sheet.getLastRow(), HEADER_ROWS + 1);
  const range = sheet.getRange(HEADER_ROWS, 1, lastRow - HEADER_ROWS + 1, NCOLS);
  const filter = range.createFilter();
  filter.setColumnFilterCriteria(ROOM_COL,
    SpreadsheetApp.newFilterCriteria().whenTextEqualTo(keuze).build());
}

// Voeg een room toe aan de keuzelijst als die nog niet bestaat. Geeft true als de lijst wijzigde.
function addRoomToList_(sheet, room) {
  if (!room || room === "?") return false;
  const props = PropertiesService.getDocumentProperties();
  const rooms = JSON.parse(props.getProperty("rooms") || "[]");
  if (rooms.indexOf(room) !== -1) return false;
  rooms.push(room);
  rooms.sort();
  props.setProperty("rooms", JSON.stringify(rooms));
  setRoomDropdown_(sheet, rooms);
  return true;
}

function setRoomDropdown_(sheet, rooms) {
  const cell = sheet.getRange(FILTER_CELL);
  const rule = SpreadsheetApp.newDataValidation()
    .requireValueInList(["ALL"].concat(rooms), true)
    .setAllowInvalid(false)
    .build();
  cell.setDataValidation(rule);
  if (!cell.getValue()) cell.setValue("ALL");
}


// ============================================================
// EENMALIG BIJ DE OVERSTAP NAAR v1.6 — Uitvoeren → archiveerEnNieuweKoppen
// ============================================================
function archiveerEnNieuweKoppen() {
  const ss    = SpreadsheetApp.getActiveSpreadsheet();
  const sheet = getDataSheet_();

  // 1. Kopie van het hele tabblad als archief (oude kolomindeling blijft leesbaar)
  if (sheet.getFilter()) sheet.getFilter().remove();
  let naam = ARCHIVE_NAME;
  if (ss.getSheetByName(naam)) {
    naam = ARCHIVE_NAME + " " + Utilities.formatDate(new Date(), "Europe/Brussels", "yyyy-MM-dd HH:mm");
  }
  const archief = sheet.copyTo(ss).setName(naam);
  ss.setActiveSheet(archief);
  ss.moveActiveSheet(ss.getNumSheets());   // archief achteraan
  Logger.log("Archief gemaakt: " + naam + " (" + (sheet.getLastRow() - HEADER_ROWS) + " rijen)");

  // 2. "Data" leegmaken: alle datarijen weg, één lege rij laten staan
  const maxRows = sheet.getMaxRows();
  if (maxRows > HEADER_ROWS) {
    sheet.getRange(HEADER_ROWS + 1, 1, maxRows - HEADER_ROWS, sheet.getMaxColumns()).clear();
  }
  if (maxRows > HEADER_ROWS + 1) {
    sheet.deleteRows(HEADER_ROWS + 2, maxRows - HEADER_ROWS - 1);
  }

  // 3. Nieuwe koppen + keuzelijst; lijst van rooms opnieuw opbouwen bij de eerste logregels
  PropertiesService.getDocumentProperties().deleteProperty("rooms");
  setupHeaders();
  ss.setActiveSheet(sheet);
  Logger.log("Data leeggemaakt en nieuwe koppen v1.6 gezet.");
}


// ============================================================
// KOPPEN — wordt door archiveerEnNieuweKoppen aangeroepen; mag ook apart
// ============================================================
function setupHeaders() {
  const ss    = SpreadsheetApp.getActiveSpreadsheet();
  const sheet = getDataSheet_();

  // genoeg kolommen?
  if (sheet.getMaxColumns() < NCOLS) {
    sheet.insertColumnsAfter(sheet.getMaxColumns(), NCOLS - sheet.getMaxColumns());
  }

  // rij 1 en 2 helemaal leegmaken (ook oude validatie en opmaak)
  const kop = sheet.getRange(1, 1, HEADER_ROWS, sheet.getMaxColumns());
  kop.clear();
  kop.clearDataValidations();

  // --- Rij 1: A1 "Kamer:", C1 keuzelijst, D1 titel + URL ---
  sheet.getRange(1, 1, 1, sheet.getMaxColumns())
    .setBackground("#222222").setFontColor("#cccccc").setFontSize(9).setVerticalAlignment("middle");
  sheet.getRange("A1").setValue("Kamerfilter →").setHorizontalAlignment("right").setFontStyle("italic");
  const keuze = sheet.getRange(FILTER_CELL);
  keuze.setBackground("#ffffff").setFontColor("#000000").setFontSize(10).setFontWeight("bold")
       .setHorizontalAlignment("center");
  setRoomDropdown_(sheet, JSON.parse(PropertiesService.getDocumentProperties().getProperty("rooms") || "[]"));
  keuze.setValue("ALL");
  sheet.getRange("D1").setValue("ROOM CONTROLLER DATA LOGGER v1.6  |  " + ss.getUrl())
       .setFontStyle("italic").setHorizontalAlignment("left");
  sheet.setRowHeight(1, 24);

  // --- Rij 2: kolomtitels ---
  const titels = COLUMNS.map(function (c) { return c[0]; });
  sheet.getRange(2, 1, 1, NCOLS).setValues([titels])
    .setFontSize(10).setFontWeight("normal").setFontStyle("normal")
    .setFontColor("#ffffff").setBackground("#000000")
    .setHorizontalAlignment("center").setVerticalAlignment("middle").setWrap(true);
  sheet.setRowHeight(2, 36);

  // --- Kolombreedtes ---
  for (let c = 1; c <= NCOLS; c++) sheet.setColumnWidth(c, 55);
  sheet.setColumnWidth(1, 135);   // Tijdstempel
  sheet.setColumnWidth(3, 90);    // Kamer (keuzelijst)
  sheet.setColumnWidth(20, 85);   // Pixels aan

  sheet.setFrozenRows(HEADER_ROWS);
  sheet.setFrozenColumns(1);
  Logger.log("Koppen v1.6 gezet: " + NCOLS + " kolommen.");
}


// ============================================================
// HULPFUNCTIES
// ============================================================
function getDataSheet_() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  return ss.getSheetByName(SHEET_NAME) || ss.insertSheet(SHEET_NAME, 0);
}

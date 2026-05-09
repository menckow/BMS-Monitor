//***************************************************
// Zentrales Modern-CSS für das helle/neutrale Design
//***************************************************
String getModernCSS() {
  return "<style>"
         "body{font-family:system-ui,-apple-system,sans-serif;background-color:#f4f7f6;color:#333;margin:0;padding:15px;line-height:1.5}"
         ".container{max-width:800px;margin:0 auto}"
         "h2,h3{color:#2c3e50;margin-top:0}"
         ".card{background:#fff;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 6px rgba(0,0,0,0.05)}"
         "table{width:100%;border-collapse:collapse;margin-bottom:10px}"
         "td{padding:10px;border-bottom:1px solid #eee}"
         "table.info-table td:first-child{color:#6c757d;font-weight:500;width:40%}"
         ".badge{display:inline-block;padding:4px 8px;border-radius:12px;font-size:0.85em;font-weight:bold}"
         ".badge.green{background:#d4edda;color:#155724}"
         ".badge.red{background:#f8d7da;color:#721c24}"
         ".badge.blue{background:#cce5ff;color:#004085}"
         ".badge.yellow{background:#fff3cd;color:#856404}"
         ".btn{display:inline-block;padding:10px 20px;background:#007bff;color:#fff;text-decoration:none;border-radius:8px;border:none;cursor:pointer;font-size:1em;text-align:center;transition:0.2s}"
         ".btn:hover{background:#0056b3}"
         ".btn-danger{background:#dc3545}.btn-danger:hover{background:#c82333}"
         ".progress-container{width:100%;height:14px;background:#e9ecef;border-radius:7px;overflow:hidden;margin-top:4px}"
         ".progress-bar{height:100%;border-radius:7px;transition:width 0.5s ease-in-out}"
         ".top-bar{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}"
         ".nav-bar{display:flex;gap:10px;margin-bottom:20px;flex-wrap:wrap}"
         "</style>";
}

void mache_HTML_Seite()                                                        // HTML Startseite aufbereiten
{
  DateTime now = myRTC.now();

  Minute = String(now.minute());
  if (now.minute() < 10)
  {
    Minute = "0" + Minute;                                                       // für Uhrzeit in Überschrift
  }

  if (aktuell_Strom < 0)                                                         // entladen
  {
    Hinweis  = "Zeit bis leer:";
    RestZeit = -1 * (RestLadung / aktuell_Strom);
  }
  else if (aktuell_Strom > 0)                                                    // laden
  {
    Hinweis  = "Zeit bis voll:";
    RestZeit = (balance_Capacity - RestLadung) / aktuell_Strom;
  }
  else                                                                           // Ruhezustand
  {
    RestZeit = 0;
  }

  sRestZeit = String(int(RestZeit)) + ":";                                       // ergibt Restzeit Stunden

  if (60 * (RestZeit - int(RestZeit)) < 10)                                      // Minuten, führende Null zufügen
  {
    sRestZeit += "0";
  }
  sRestZeit += String(int(60 * (RestZeit - (int(RestZeit)))));                   // Restzeit in Minuten
  sRestZeit += " Std:Min";

  if (aktuell_Strom == 0) sRestZeit    = "";

  // HTTP-Antwort starten (Streaming-Modus)
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String chunk;
  chunk = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
          "<link rel=\"icon\" type=\"image/vnd.microsoft.icon\" href=\"favicon.ico\">"
          "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
          "<meta http-equiv='refresh' content='60'>"
          "<title>BMS-Logger</title>";
  chunk += getModernCSS();
  chunk += "<script>window.history.pushState({}, document.title, window.location.pathname);</script></head><body><div class='container'>";
  server.sendContent(chunk);
  
  // Header / Navigation
  chunk = "<div class='top-bar'><h2>BMS-Logger</h2><span class='badge blue'>" + String(now.day(), DEC) + "." + String(now.month(), DEC) + "." + String(now.year(), DEC) + " &nbsp;" + String(now.hour(), DEC) + ":" + Minute + "</span></div>";
  chunk += "<div class='nav-bar'><a class='btn' href='/'>Aktualisieren</a><a class='btn' href='Chart'>Diagramme</a><a class='btn' href='Listen'>Dateien</a><a class='btn' href='Setup'>Setup</a></div>";
  server.sendContent(chunk);

  // System Info Card
  chunk = "<div class='card'><h3>System Info</h3><table class='info-table'>";
  chunk += "<tr><td>Läuft seit</td><td>" + bootZeit + "</td></tr>";
  chunk += "<tr><td>BT Name</td><td>" + BT_Name + "</td></tr>";
  chunk += "<tr><td>BMS Typ</td><td>" + packVersionsInfo.sBMSTyp + "</td></tr>";
  chunk += "<tr><td>BMS Version</td><td>0x" + String(packBasicInfo.BMS_Version, HEX) + "</td></tr>";
  chunk += "<tr><td>Anzahl Zyklen</td><td>" + String(Anzahl_Zyklen) + "</td></tr>";
  chunk += "</table></div>";
  server.sendContent(chunk);

  // Battery Status Card
  chunk = "<div class='card'><h3>Batterie Status</h3><table class='info-table'>";
  chunk += "<tr><td>Ladezustand</td><td><b>" + String(aktuell_Ladezustand) + " %</b></td></tr>";
  chunk += "<tr><td>Gesamtspannung</td><td><b>" + String(aktuell_Spannung, 3) + " V</b></td></tr>";
  chunk += "<tr><td>Strom</td><td>" + String(aktuell_Strom) + " A</td></tr>";
  chunk += "<tr><td>Leistung</td><td>" + String(packBasicInfo.Watts) + " W</td></tr>";
  chunk += "<tr><td>Restkapazität</td><td>" + String(RestLadung) + " Ah / " + String(balance_Capacity) + " Ah</td></tr>";
  if (aktuell_Strom != 0) {
    chunk += "<tr><td>" + Hinweis + "</td><td>" + sRestZeit + "</td></tr>";
  }
  
  chunk += "<tr><td>NTC's (Temperaturen)</td><td>";
  chunk += String(packBasicInfo.Temp1 / 10.0, 1) + " &deg;C, " + String(packBasicInfo.Temp2 / 10.0, 1) + " &deg;C</td></tr>";
  chunk += "</table></div>";
  server.sendContent(chunk);

  // Cell Info Card mit Bar-Charts
  chunk = "<div class='card'><h3>Zellen Info</h3>";
  
  float maxVal = 0, minVal = 5000;
  uint8_t minZelle = 0, maxZelle = 0;
  for (uint8_t i = 0 ; i < Anzahl_Zellen ; i++) {
    if (packCellInfo.CellVolt[i] > maxVal) { maxVal = packCellInfo.CellVolt[i]; maxZelle = i + 1; }
    if (packCellInfo.CellVolt[i] < minVal) { minVal = packCellInfo.CellVolt[i]; minZelle = i + 1; }
  }

  chunk += "<p style='margin-top:0'>Zellendifferenz (" + String(maxZelle) + " - " + String(minZelle) + "): <b>" + String((maxVal - minVal), 0) + " mV</b></p>";
  chunk += "<table style='width:100%'>";
  server.sendContent(chunk);
  chunk = "";

  for (uint8_t i = 0; i < Anzahl_Zellen; i++) {
    float v = packCellInfo.CellVolt[i] / 1000.0;
    int pct = int((v - 2.8) / (4.2 - 2.8) * 100.0);
    if(pct < 0) pct = 0; if(pct > 100) pct = 100;
    
    String pColor = "#28a745"; // green
    if(v < 3.0) pColor = "#dc3545"; // red
    else if(v < 3.2) pColor = "#ffc107"; // yellow

    chunk += "<tr><td style='width:20%;font-weight:bold'>C" + String(i + 1);
    if(bitRead(packBasicInfo.BalanceCodeLow, i)) chunk += " <span style='color:#007bff;font-size:1.2em' title='Balancing aktiv'>*</span>";
    chunk += "</td>";
    chunk += "<td style='width:25%'>" + String(v, 3) + " V</td>";
    chunk += "<td style='width:55%'><div class='progress-container'><div class='progress-bar' style='width:" + String(pct) + "%;background-color:" + pColor + "'></div></div></td></tr>";
    
    if (chunk.length() > 500) { server.sendContent(chunk); chunk = ""; }
  }
  chunk += "</table></div>";
  server.sendContent(chunk);

  // FET Control Card
  chunk = "<div class='card'><h3>System Steuerung</h3><table class='info-table'>";
  int currentDis = bitRead(packBasicInfo.MosfetStatus, 1);
  int currentChg = bitRead(packBasicInfo.MosfetStatus, 0);
  int Discharge = (!currentDis) + (currentChg * 2);
  int Charge = currentDis + ((!currentChg) * 2);

  String disHtml = currentDis ? "<span class='badge green'>AN</span>" : "<span class='badge red'>AUS</span>";
  String chgHtml = currentChg ? "<span class='badge green'>AN</span>" : "<span class='badge red'>AUS</span>";

  chunk += "<tr><td>Entlade-FET</td><td>" + disHtml + "</td><td style='text-align:right'><a class='btn' href='/?SetzeMosFet=" + String(Discharge) + "'>Umschalten</a></td></tr>";
  chunk += "<tr><td>Lade-FET</td><td>" + chgHtml + "</td><td style='text-align:right'><a class='btn' href='/?SetzeMosFet=" + String(Charge) + "'>Umschalten</a></td></tr>";
  
  if (packBasicInfo.ProtectionStatus != 0) {
    chunk += "<tr><td style='color:#dc3545'><b>Fehlermeldungen!</b></td><td><span class='badge red'>Aktiv</span></td><td style='text-align:right'><a class='btn btn-danger' href='/StatusAbfrage'>Anzeigen</a></td></tr>";
  } else {
    chunk += "<tr><td>System Status</td><td colspan='2'><span class='badge green'>OK</span></td></tr>";
  }

  chunk += "</table></div></div></body></html>\r\n";
  server.sendContent(chunk);
  server.sendContent("");
  webServerLastActive = millis();
}

//----------------------------------------------------
void handleRoot()                                                                   // HTML Startseite
{
  if (server.hasArg("SetzeMosFet"))                                                 // Wurde der Button geklickt?
  {
    String mosFetArg = server.arg("SetzeMosFet");
    Serial.println("Web-Command: SetzeMosFet = " + mosFetArg);
    
    if (String(packBasicInfo.MosfetStatus) != mosFetArg)                            // nur neuer Wert
    {
      bool flag_charge, flag_discharge;

      if (mosFetArg == "0") { flag_discharge = 0; flag_charge = 0; }
      if (mosFetArg == "1") { flag_discharge = 1; flag_charge = 0; }
      if (mosFetArg == "2") { flag_discharge = 0; flag_charge = 1; }
      if (mosFetArg == "3") { flag_discharge = 1; flag_charge = 1; }

      Serial.println("Sende BLE Befehl an BMS...");
      set_mosfet_control(flag_discharge, flag_charge);                            // Setzte MOS_FET ein/aus
      Variable_X = packBasicInfo.MosfetStatus;                                    // warten bis FET gesetzt
      
      for (nX = 1; nX <= 10; nX++)                                                // max 10 Sekunden warten
      {
        yieldWeb(1000);
        bleRequestData();                                                         // BMS abfragen
        if (Variable_X != packBasicInfo.MosfetStatus) {
          Serial.println("BMS hat Status erfolgreich geändert!");
          break;
        }
      }
      mache_HTML_Seite();                                                         // HTML neu machen
    }
  }
  
  Serial.println("HTML - Anfrage Root");

  mache_HTML_Seite();                                                         // Jetzt wird gestreamt!
  
  if (Screensaver >= 600)                                                   // Bildschirm schonen xx Sek.
  {
    tft.fillScreen(TFT_BLACK);
  }
  Screensaver = 0;                                                                   // Bildschirmschoner aus
  webServerLastActive = millis();
}

//----------------------------------------------------
void Listen()                                                                       // Listen downloaden
{
  Serial.println("Listen - Anfrage");

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  uint16_t nAnzahl = 0;                                                             // Anzahl der Einträge im Array
  String chunk;

  chunk = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
          "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
          "<title>BMS-Dateimanager</title>";
  chunk += getModernCSS();
  chunk += "<script>function janein(text){return confirm(text);}</script></head><body><div class='container'>\n"
           "<script>window.history.pushState({}, document.title, window.location.pathname);</script>";
  chunk += "<div class='top-bar'><h2>Dateimanager</h2><a class='btn' href='/'>Zurück</a></div>";
  server.sendContent(chunk);

  File root = getFS().open("/");                                                      // Aktuell im Speicher vorhandenen Dateien
  File file = root.openNextFile();
  while (file) {
    nAnzahl ++;                                                                     // Anzahl zählen
    file = root.openNextFile();
  }
  root.close();

  if (nAnzahl > 0)
  {
    sArr = new name_t[nAnzahl];                                                      // Array anlegen
    if (sArr == nullptr) return;                                                     // kein freier Speicher

    nAnzahl = 0;
    char copy[60];                                                                   // max. Anzahl Zeichen pro Zeile
    String sTemp;

    root = getFS().open("/");                                                    // Aktuell im Speicher vorhandenen Dateien
    file = root.openNextFile();

    while (file)                                                                     // Verzeichniss abarbeiten
    {
      sTemp = String(file.name()) + "#" + String(file.size());                       // Dateiname#Länge
      sTemp.toCharArray(copy, sTemp.length() + 1);                                   // nach "copy" kopieren
      strcpy(sArr[nAnzahl], copy);                                                   // und ins Array
      nAnzahl ++;
      file = root.openNextFile();
    }
    bsort(sArr, nAnzahl, false);                                                     // Array sortieren, Groß- und Kleinschreibung egal
    root.close();

    chunk = "<div class='card'><h3>" + String(sdCardActive ? "SD Karte" : "SPIFFS Speicher") + "</h3>";
    chunk += "<p>Belegt: <b>" + formatBytes(getFSUsedBytes()) + "</b> / " + formatBytes(getFSTotalBytes()) + "</p>";
    
    int spiffsPct = (getFSTotalBytes() > 0) ? (getFSUsedBytes() * 100) / getFSTotalBytes() : 0;
    chunk += "<div class='progress-container' style='margin-bottom:15px'><div class='progress-bar' style='width:" + String(spiffsPct) + "%;background-color:#007bff'></div></div>";
    
    if (!sdCardActive) {
        chunk += "<form method='POST' action='/format'><input type='submit' value='Speicher Formatieren' class='btn btn-danger' onclick=\"return janein('Wirklich formatieren? Alle Daten gehen restlos verloren!')\"></form>";
    }
    chunk += "</div>";
    server.sendContent(chunk);

    chunk = "<div class='card'><table class='info-table'>";
    for (uint16_t i = 0; i < nAnzahl; i++)                                           // HTML-Liste aufbauen
    {
      sTemp = sArr[i];
      String   sName = sTemp.substring(0, sTemp.indexOf("#"));
      String   sSize = sTemp.substring(sTemp.indexOf("#") + 1);
      uint32_t iSize = sSize.toInt();
      
      chunk += "<tr><td style='font-family:monospace;width:50%'><a style='text-decoration:none;' href='" + sName + "' download>" + sName + "</a></td>";
      chunk += "<td>" + formatBytes(iSize) + "</td>";
      chunk += "<td><a class='btn btn-danger' style='padding:6px 12px;font-size:0.85em' href='Loeschen?File=" + sName + "' onclick=\"return janein('" + sName + " wirklich löschen?')\">Löschen</a></td></tr>";
      
      if (chunk.length() > 1000) { server.sendContent(chunk); chunk = ""; }
    }
    chunk += "</table></div>";
    server.sendContent(chunk);
    
    delete[] sArr;                                                                    // Speicher des Arrays freigeben !!!
  }

  // Datei Hochladen
  chunk = "<div class='card'><h3>Datei hochladen</h3>";
  chunk += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  chunk += "<div style='display:flex;gap:10px;align-items:center'><input type='file' name='upload' style='font-family:inherit' required>";
  chunk += "<input type='submit' value='Upload' class='btn'></div></form></div>";
  chunk += "</div></body></html>\n";
  server.sendContent(chunk);
  server.sendContent("");
  webServerLastActive = millis();
}

//----------------------------------------------------
void StatusAbfrage()                                                                  // FehlerStatus
{
  Serial.println("Status - Anfrage");

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String sUE = "&Uuml;";
  String chunk;

  chunk = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
          "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
          "<title>BMS-Status</title>";
  chunk += getModernCSS();
  chunk += "<script>function janein(text) {return confirm(text);}</script></head><body><div class='container'>";
  chunk += "<div class='top-bar'><h2>Fehlermeldungen</h2><a class='btn' href='/'>Zurück</a></div>";
  chunk += "<div class='card'><table class='info-table'>";
  server.sendContent(chunk);

  if (packBasicInfo.ProtectionStatus == 0)
  {
    server.sendContent("<tr><td>Status</td><td style='text-align:right'><span class='badge green'>KEINE FEHLER</span></td></tr>");
  }
  else 
  {
    chunk = "";
    if (bitRead(packBasicInfo.ProtectionStatus, 0)) chunk += "<tr><td>Zellen-" + sUE + "berspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 1)) chunk += "<tr><td>Zellen-Unterspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 2)) chunk += "<tr><td>Batterie-" + sUE + "berspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 3)) chunk += "<tr><td>Batterie-Unterspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 4)) chunk += "<tr><td>Laden " + sUE + "bertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 5)) chunk += "<tr><td>Laden Untertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 6)) chunk += "<tr><td>Entladen " + sUE + "bertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 7)) chunk += "<tr><td>Entladen Untertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    server.sendContent(chunk);

    chunk = "";
    if (bitRead(packBasicInfo.ProtectionStatus, 8)) chunk += "<tr><td>Laden " + sUE + "berstrom</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 9)) chunk += "<tr><td>Entladen " + sUE + "berstrom</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 10)) chunk += "<tr><td>Kurzschluss</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 11)) chunk += "<tr><td>IC Fehler</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 12)) chunk += "<tr><td>MOS-Software Lock-in</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    server.sendContent(chunk);
  }

  server.sendContent("</table></div></div></body></html>\n");
  server.sendContent(""); // Ende
  webServerLastActive = millis();

  Serial.println();
  yieldWeb(1000);
}

//----------------------------------------------------
void SetUpDaten()                                                                     // HTML-Setup-Seite
{
  Serial.println("Setup - Anfrage");

  DateTime now = myRTC.now();
  uint16_t intYear = now.year();
  uint8_t  intMonth = now.month(), intDay = now.day(), intHour = now.hour(), intMinute = now.minute();

  if (server.args() != 0)
  {
    for (uint8_t i = 0; i < server.args(); i++)                                        // wenn Werte übergeben wurden
    {
      if (server.argName(i) == "Intervall") { Intervall = server.arg(i); putEEprom(0, Intervall); }
      if (server.argName(i) == "myPassword") { myPassword = server.arg(i); putEEprom(10, myPassword); }
      if (server.argName(i) == "SSID_WLAN") { WLAN_SSID = server.arg(i); putEEprom(60, WLAN_SSID); }
      if (server.argName(i) == "PASS_WLAN") { WLAN_PASS = server.arg(i); putEEprom(110, WLAN_PASS); }
      if (server.argName(i) == "BMS_MAC_1") { BMS_MAC_1 = server.arg(i); putEEprom(210, BMS_MAC_1); }
      if (server.argName(i) == "BMS_MAC_2") { BMS_MAC_2 = server.arg(i); putEEprom(240, BMS_MAC_2); }

      if (server.argName(i) == "YEAR")    intYear   = server.arg(i).toInt();
      if (server.argName(i) == "MONTH")   intMonth  = server.arg(i).toInt();
      if (server.argName(i) == "DAY")     intDay    = server.arg(i).toInt();
      if (server.argName(i) == "Stunden") intHour   = server.arg(i).toInt();
      if (server.argName(i) == "Minuten") intMinute = server.arg(i).toInt();
    }

    EEPROM.commit();                                                 // EEPROM abschliessen
    myRTC.adjust(DateTime(intYear, intMonth, intDay, intHour, intMinute, 0));
    now = myRTC.now();
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String chunk;
  chunk = "<!DOCTYPE html><html lang='de'><head><meta charset='UTF-8'>"
          "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
          "<title>BMS-Logger Setup</title>";
  chunk += getModernCSS();
  chunk += "<style>input[type=text],input[type=number]{width:100%;padding:8px;border:1px solid #ccc;border-radius:6px;box-sizing:border-box} .date-inputs input{width:auto;display:inline-block;margin-right:2px;text-align:center}</style>";
  chunk += "<script>function jn(text){return confirm(text);}</script></head><body><div class='container'>\n"
           "<script>window.history.pushState({}, document.title, window.location.pathname);</script>";
  chunk += "<div class='top-bar'><h2>Einstellungen</h2><a class='btn' href='/'>Zurück</a></div>";
  server.sendContent(chunk);

  chunk = "<div class='card'><h3>Geräteinfo</h3>";
  chunk += "<p><b>Version:</b> " + Version + "<br><b>Build:</b> " + Build + "<br><b>IP:</b> " + sIPAdresse + " &nbsp; <b>RSSI:</b> " + WiFi.RSSI() + " dBm<br>";
  chunk += "<b>BMS-Typ:</b> " + packVersionsInfo.sBMSTyp + "<br><b>BT-Name:</b> " + BT_Name + "<br><b>BT RSSI:</b> " + String(myRSSI, 0) + " dBm</p></div>";
  server.sendContent(chunk);

  chunk = "<form method='GET'><div class='card'><h3>Systemzeit & Speichern</h3><table class='info-table'>";
  chunk += "<tr><td>Datum & Uhrzeit<br><small>dd.mm.yyyy, hh:mm</small></td><td class='date-inputs'>"
           "<input name='DAY' value='" + String(now.day()) + "' size='2' maxLength='2'> . "
           "<input name='MONTH' value='" + String(now.month()) + "' size='2' maxLength='2'> . "
           "<input name='YEAR' value='" + String(now.year()) + "' size='4' maxLength='4'> &nbsp; "
           "<input name='Stunden' value='" + String(now.hour()) + "' size='2' maxLength='2'> : "
           "<input name='Minuten' value='" + String(now.minute()) + "' size='2' maxLength='2'>"
           "</td></tr>";
  chunk += "<tr><td>Speicherintervall<br><small>(1-60 Minuten)</small></td><td><input name='Intervall' type='number' min='1' max='60' step='1' value='" + Intervall + "'></td></tr>";
  chunk += "</table></div>";
  server.sendContent(chunk);

  chunk = "<div class='card'><h3>Netzwerk</h3><table class='info-table'>";
  chunk += "<tr><td>WLAN SSID</td><td><input type='text' name='SSID_WLAN' minlength='8' maxlength='50' value='" + WLAN_SSID + "'></td></tr>";
  chunk += "<tr><td>WLAN Passwort</td><td><input type='text' name='PASS_WLAN' minlength='8' maxlength='50' value='" + WLAN_PASS + "'></td></tr>";
  chunk += "<tr><td>AccessPoint Passwort</td><td><input type='text' name='myPassword' minlength='8' maxlength='50' value='" + myPassword + "'></td></tr>";
  chunk += "</table></div>";
  server.sendContent(chunk);

  chunk = "<div class='card'><h3>BMS Batterie Zuordnungen</h3><table class='info-table'>";
  chunk += "<tr><td>Batterie 1 MAC<br><small>Wird für Fast-Switch per Taste benötigt</small></td><td><input type='text' name='BMS_MAC_1' maxlength='17' placeholder='AA:BB:CC:DD:EE:FF' value='" + BMS_MAC_1 + "'></td></tr>";
  chunk += "<tr><td>Batterie 2 MAC</td><td><input type='text' name='BMS_MAC_2' maxlength='17' placeholder='AA:BB:CC:DD:EE:FF' value='" + BMS_MAC_2 + "'></td></tr>";
  chunk += "</table><br><button type='submit' class='btn' style='width:100%;margin-bottom:10px'>Speichern & Anwenden</button></form></div>";
  server.sendContent(chunk);

  chunk = "<div class='card'><h3>Aktionen</h3><table class='info-table'>";
  chunk += "<tr><td>ESP32 Hard Reset</td><td style='text-align:right'><form method='POST' action='/Reset' style='margin:0'><button type='submit' class='btn btn-danger' onclick=\"return jn('Hardware sicher resetten?')\">Neustart</button></form></td></tr>";
  chunk += "</table></div>";
  chunk += "</div></body></html>\n";
  server.sendContent(chunk);
  server.sendContent("");
  webServerLastActive = millis();
  Serial.println();
}

//-------------------------------------------------
String formatBytes(size_t bytes)                                                       // lesbare Anzeige der Speichergrößen
{
  if (bytes < 1024) return String(bytes) + " Byte ";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB ";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB ";
  return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB ";
}
// --------------------------------------------------------------------
String lTrim(String Str, uint8_t Laenge)                                               // String vorne mit . ergänzen
{
  String  S = "";
  uint8_t x = Str.length();                                                            // Länge der Zahl
  for ( uint8_t i = x; i < Laenge; i++) S = S + ".";
  S += Str;
  return (S);
}
// --------------------------------------------------------------------
String rTrim(String Str, uint8_t Laenge)                                                    // String hinten mit . ergänzen
{
  String  S = "";
  uint8_t x = Str.length();                                                              // Länge der Zahl
  for ( uint8_t i = x; i < Laenge; i++) S = S + ".";
  Str += S;
  return (Str);
}
// --------------------------------------------------------------------
String getContentType(String filename) {                                                // ContentType für den Browser
  String fn = filename;
  fn.toLowerCase();
  if (fn.endsWith(".htm") || fn.endsWith(".html")) return "text/html";
  else if (fn.endsWith(".css"))  return "text/css";
  else if (fn.endsWith(".js"))   return "application/javascript";
  else if (fn.endsWith(".png"))  return "image/png";
  else if (fn.endsWith(".gif"))  return "image/gif";
  else if (fn.endsWith(".jpg"))  return "image/jpeg";
  else if (fn.endsWith(".ico"))  return "image/x-icon";
  else if (fn.endsWith(".kml"))  return "application/googleearth.exe";
  else if (fn.endsWith(".xml"))  return "text/xml";
  else if (fn.endsWith(".csv"))  return "text/csv";
  else if (fn.endsWith(".pdf"))  return "application/x-pdf";
  else if (fn.endsWith(".zip"))  return "application/x-zip";
  else if (fn.endsWith(".gz"))   return "application/x-gzip";
  return "text/plain";
}

// --------------------------------------------------------------------
void handleFileUpload()
{ // Dateien vom Rechnenknecht oder Klingelkasten ins SPIFFS schreiben
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    if (upload.filename.length() > 30)
    {
      upload.filename = upload.filename.substring(upload.filename.length() - 30, upload.filename.length());  // Dateinamen auf 30 Zeichen kürzen
    }
    fsUploadFile = SPIFFS.open("/" + server.urlDecode(upload.filename), "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
      fsUploadFile.close();
    yieldWeb(10);
    Listen();
  }
  webServerLastActive = millis();
}

//-------------------------------------------------
void Fehlerloeschen()                                                                    // Status ändern
{
  Serial.println("\nFehlerloeschen");
  "bms._fault_counts()";
  yieldWeb(1000);
  StatusAbfrage();
  webServerLastActive = millis();
}
//-------------------------------------------------
void Reset()
{ Serial.println("R E S E T");
  yieldWeb(100);
  ESP.restart();
}

void ChartSeite()
{
  Serial.println("Chart - Anfrage");

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String chunk;
  chunk = "<!DOCTYPE html><html lang='de'><head><meta charset='UTF-8'>"
          "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
          "<title>BMS-Diagramme</title>";
  chunk += getModernCSS();
  chunk += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body><div class='container'>";
  chunk += "<div class='top-bar'><h2>Batterie Diagramme</h2>";
  chunk += "<select id='bmsSelect' class='btn' style='background:#444; border:none; color:#fff; cursor:pointer; margin-right:10px'></select>";
  chunk += "<a class='btn' href='/'>Zurück</a></div>";
  server.sendContent(chunk);

  chunk = "<div class='card' style='height:300px'><canvas id='chartVoltage'></canvas></div>";
  chunk += "<div class='card' style='height:300px'><canvas id='chartCells'></canvas></div>";
  chunk += "<div class='card' style='height:300px'><canvas id='chartCurrent'></canvas></div>";
  chunk += "<div class='card' style='height:300px'><canvas id='chartCapacity'></canvas></div>";
  server.sendContent(chunk);

  chunk = "<script>\n"
          "let chartV, chartC, chartA, chartCap;\n"
          "let allData = {};\n"
          "fetch('/BMS_LOG.CSV').then(r => r.text()).then(csv => {\n"
          "  const rows = csv.trim().split('\\n').slice(1);\n"
          "  rows.forEach(r => {\n"
          "    const c = r.split(';');\n"
          "    if(c.length > 9) {\n"
          "      let bmsName = c[0] || 'Unbekannt';\n"
          "      if(!allData[bmsName]) allData[bmsName] = { labels:[], volts:[], amps:[], c1:[], c2:[], c3:[], c4:[], caps:[] };\n"
          "      const d = allData[bmsName];\n"
          "      d.labels.push(c[2]);\n"
          "      d.volts.push(parseFloat(c[4].replace(',', '.')));\n"
          "      d.amps.push(parseFloat(c[5].replace(',', '.')));\n"
          "      d.c1.push(parseFloat(c[6].replace(',', '.')));\n"
          "      d.c2.push(parseFloat(c[7].replace(',', '.')));\n"
          "      d.c3.push(parseFloat(c[8].replace(',', '.')));\n"
          "      d.c4.push(parseFloat(c[9].replace(',', '.')));\n"
          "      d.caps.push(parseFloat(c[3].replace(',', '.')));\n"
          "    }\n"
          "  });\n"
          "  const sel = document.getElementById('bmsSelect');\n"
          "  Object.keys(allData).forEach(k => { const opt = document.createElement('option'); opt.value = k; opt.textContent = k; sel.appendChild(opt); });\n"
          "  if(Object.keys(allData).length > 0) updateCharts(Object.keys(allData)[0]);\n"
          "  sel.onchange = (e) => updateCharts(e.target.value);\n"
          "});\n";
  server.sendContent(chunk);

  chunk = "function updateCharts(bms) {\n"
          "  const d = allData[bms];\n"
          "  const config = (l, dat, lbl, col) => ({ type:'line', data:{ labels:l, datasets:[{ label:lbl, data:dat, borderColor:col, tension:0.1, pointRadius:1 }] }, options:{responsive:true, maintainAspectRatio:false}});\n"
          "  if(chartV) { chartV.destroy(); chartC.destroy(); chartA.destroy(); chartCap.destroy(); }\n"
          "  chartV = new Chart(document.getElementById('chartVoltage'), config(d.labels, d.volts, 'Spannung (V)', '#007bff'));\n"
          "  chartA = new Chart(document.getElementById('chartCurrent'), config(d.labels, d.amps, 'Strom (A)', '#28a745'));\n"
          "  chartCap = new Chart(document.getElementById('chartCapacity'), config(d.labels, d.caps, 'SOC (%)', '#ffc107'));\n"
          "  chartC = new Chart(document.getElementById('chartCells'), { type:'line', data:{ labels:d.labels, datasets:[\n"
          "    {label:'Z1', data:d.c1, borderColor:'#ff6384', tension:0.1, pointRadius:1},\n"
          "    {label:'Z2', data:d.c2, borderColor:'#36a2eb', tension:0.1, pointRadius:1},\n"
          "    {label:'Z3', data:d.c3, borderColor:'#cc65fe', tension:0.1, pointRadius:1},\n"
          "    {label:'Z4', data:d.c4, borderColor:'#ffce56', tension:0.1, pointRadius:1}\n"
          "  ]}, options:{responsive:true, maintainAspectRatio:false}});\n"
          "}\n"
          "</script></body></html>\r\n";
  server.sendContent(chunk);
  server.sendContent("");
  webServerLastActive = millis();
}

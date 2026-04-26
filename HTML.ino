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

  Content = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
            "<link rel=\"icon\" type=\"image/vnd.microsoft.icon\" href=\"favicon.ico\">"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<meta http-equiv='refresh' content='60'>"
            "<title>BMS-Logger</title>";
  Content += getModernCSS();
  Content += "<script>window.history.pushState({}, document.title, window.location.pathname);</script></head><body><div class='container'>";
  
  // Header / Navigation
  Content += "<div class='top-bar'><h2>BMS-Logger</h2><span class='badge blue'>" + String(now.day(), DEC) + "." + String(now.month(), DEC) + "." + String(now.year(), DEC) + " &nbsp;" + String(now.hour(), DEC) + ":" + Minute + "</span></div>";
  Content += "<div class='nav-bar'><a class='btn' href='/'>Aktualisieren</a><a class='btn' href='Listen'>Dateien</a><a class='btn' href='Setup'>Setup</a></div>";

  // System Info Card
  Content += "<div class='card'><h3>System Info</h3><table class='info-table'>";
  Content += "<tr><td>Läuft seit</td><td>" + bootZeit + "</td></tr>";
  Content += "<tr><td>BT Name</td><td>" + BT_Name + "</td></tr>";
  Content += "<tr><td>BMS Typ</td><td>" + packVersionsInfo.sBMSTyp + "</td></tr>";
  Content += "<tr><td>BMS Version</td><td>0x" + String(packBasicInfo.BMS_Version, HEX) + "</td></tr>";
  Content += "<tr><td>Anzahl Zyklen</td><td>" + String(Anzahl_Zyklen) + "</td></tr>";
  Content += "</table></div>";

  // Battery Status Card
  Content += "<div class='card'><h3>Batterie Status</h3><table class='info-table'>";
  Content += "<tr><td>Ladezustand</td><td><b>" + String(aktuell_Ladezustand) + " %</b></td></tr>";
  Content += "<tr><td>Gesamtspannung</td><td><b>" + String(aktuell_Spannung, 3) + " V</b></td></tr>";
  Content += "<tr><td>Strom</td><td>" + String(aktuell_Strom) + " A</td></tr>";
  Content += "<tr><td>Leistung</td><td>" + String(packBasicInfo.Watts) + " W</td></tr>";
  Content += "<tr><td>Restkapazität</td><td>" + String(RestLadung) + " Ah / " + String(balance_Capacity) + " Ah</td></tr>";
  if (aktuell_Strom != 0) {
    Content += "<tr><td>" + Hinweis + "</td><td>" + sRestZeit + "</td></tr>";
  }
  
  Content += "<tr><td>NTC's (Temperaturen)</td><td>";
  Content += String(packBasicInfo.Temp1 / 10.0, 1) + " &deg;C, " + String(packBasicInfo.Temp2 / 10.0, 1) + " &deg;C</td></tr>";
  Content += "</table></div>";

  // Cell Info Card mit Bar-Charts
  Content += "<div class='card'><h3>Zellen</h3>";
  
  float maxVal = 0, minVal = 5000;
  uint8_t minZelle = 0, maxZelle = 0;
  for (uint8_t i = 0 ; i < Anzahl_Zellen ; i++) {
    if (packCellInfo.CellVolt[i] > maxVal) { maxVal = packCellInfo.CellVolt[i]; maxZelle = i + 1; }
    if (packCellInfo.CellVolt[i] < minVal) { minVal = packCellInfo.CellVolt[i]; minZelle = i + 1; }
  }

  Content += "<p style='margin-top:0'>Zellendifferenz (" + String(maxZelle) + " - " + String(minZelle) + "): <b>" + String((maxVal - minVal), 0) + " mV</b></p>";
  
  Content += "<table style='width:100%'>";
  for (uint8_t i = 0; i < Anzahl_Zellen; i++) {
    float v = packCellInfo.CellVolt[i] / 1000.0;
    
    // Fortschrittsbalken-Farbe // LiFePO4: 2.8 bis 3.6, LiIon: 3.0 bis 4.2
    int pct = int((v - 2.8) / (4.2 - 2.8) * 100.0);
    if(pct < 0) pct = 0; if(pct > 100) pct = 100;
    
    String pColor = "#28a745"; // green
    if(v < 3.0) pColor = "#dc3545"; // red
    else if(v < 3.2) pColor = "#ffc107"; // yellow

    Content += "<tr><td style='width:20%;font-weight:bold'>C" + String(i + 1);
    if(bitRead(packBasicInfo.BalanceCodeLow, i)) Content += " <span style='color:#007bff;font-size:1.2em' title='Balancing aktiv'>*</span>";
    Content += "</td>";
    Content += "<td style='width:25%'>" + String(v, 3) + " V</td>";
    Content += "<td style='width:55%'><div class='progress-container'><div class='progress-bar' style='width:" + String(pct) + "%;background-color:" + pColor + "'></div></div></td></tr>";
  }
  Content += "</table></div>";

  // FET Control Card
  Content += "<div class='card'><h3>System Steuerung</h3><table class='info-table'>";
  
  // Berechnung zum Umschalten der FETs (exakte Kopie alte Logik)
  int currentDis = bitRead(packBasicInfo.MosfetStatus, 1);
  int currentChg = bitRead(packBasicInfo.MosfetStatus, 0);
  int Discharge = (!currentDis) + (currentChg * 2);
  int Charge = currentDis + ((!currentChg) * 2);

  String disHtml = currentDis ? "<span class='badge green'>AN</span>" : "<span class='badge red'>AUS</span>";
  String chgHtml = currentChg ? "<span class='badge green'>AN</span>" : "<span class='badge red'>AUS</span>";

  Content += "<tr><td>Entlade-FET</td><td>" + disHtml + "</td><td style='text-align:right'><a class='btn' href='/?SetzeMosFet=" + String(Discharge) + "'>Umschalten</a></td></tr>";
  Content += "<tr><td>Lade-FET</td><td>" + chgHtml + "</td><td style='text-align:right'><a class='btn' href='/?SetzeMosFet=" + String(Charge) + "'>Umschalten</a></td></tr>";
  
  if (packBasicInfo.ProtectionStatus != 0) {
    Content += "<tr><td style='color:#dc3545'><b>Fehlermeldungen!</b></td><td><span class='badge red'>Aktiv</span></td><td style='text-align:right'><a class='btn btn-danger' href='/StatusAbfrage'>Anzeigen</a></td></tr>";
  } else {
    Content += "<tr><td>System Status</td><td colspan='2'><span class='badge green'>OK</span></td></tr>";
  }

  Content += "</table></div>";
  Content += "</div></body></html>\r\n";
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
        delay(1000);
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

  server.send(200, "text/html", Content);
  if (Screensaver >= 600)                                                   // Bildschirm schonen xx Sek.
  {
    tft.fillScreen(TFT_BLACK);
  }
  Screensaver = 0;                                                                   // Bildschirmschoner aus
}

//----------------------------------------------------
void Listen()                                                                       // Listen downloaden
{
  Serial.println();
  Serial.println("Listen - Anfrage");

  uint16_t nAnzahl = 0;                                                             // Anzahl der Einträge im Array
  String Content;

  Content = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>BMS-Dateimanager</title>";
  Content += getModernCSS();
  Content += "<script>function janein(text){return confirm(text);}</script></head><body><div class='container'>\n"
             "<script>window.history.pushState({}, document.title, window.location.pathname);</script>";
             
  Content += "<div class='top-bar'><h2>Dateimanager</h2><a class='btn' href='/'>Zurück</a></div>";

  File root = SPIFFS.open("/");                                                      // Aktuell im Speicher vorhandenen Dateien
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

    root = SPIFFS.open("/");                                                    // Aktuell im Speicher vorhandenen Dateien
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

    Content += "<div class='card'><table class='info-table'>";
    for (uint16_t i = 0; i < nAnzahl; i++)                                           // HTML-Liste aufbauen
    {
      sTemp = sArr[i];
      String   sName = sTemp.substring(0, sTemp.indexOf("#"));
      String   sSize = sTemp.substring(sTemp.indexOf("#") + 1);
      uint32_t iSize = sSize.toInt();
      
      Content += "<tr><td style='font-family:monospace;width:50%'><a style='text-decoration:none;' href='" + sName + "' download>" + sName + "</a></td>";
      Content += "<td>" + formatBytes(iSize) + "</td>";
      Content += "<td><a class='btn btn-danger' style='padding:6px 12px;font-size:0.85em' href='Loeschen?File=" + sName + "' onclick=\"return janein('" + sName + " wirklich löschen?')\">Löschen</a></td></tr>";
    }
    Content += "</table></div>";
    delete sArr;                                                                      // Speicher des Arrays freigeben !!!
  }

  // Speicher Info
  Content += "<div class='card'><h3>SPIFFS Speicher</h3>";
  Content += "<p>Belegt: <b>" + formatBytes(SPIFFS.usedBytes()) + "</b> / " + formatBytes(SPIFFS.totalBytes()) + "</p>";
  
  int spiffsPct = (SPIFFS.totalBytes() > 0) ? (SPIFFS.usedBytes() * 100) / SPIFFS.totalBytes() : 0;
  Content += "<div class='progress-container' style='margin-bottom:15px'><div class='progress-bar' style='width:" + String(spiffsPct) + "%;background-color:#007bff'></div></div>";
  
  Content += "<form method='POST' action='/format'><input type='submit' value='Speicher Formatieren' class='btn btn-danger' onclick=\"return janein('Wirklich formatieren? Alle Daten gehen restlos verloren!')\"></form>";
  Content += "</div>";

  // Datei Hochladen
  Content += "<div class='card'><h3>Datei hochladen</h3>";
  Content += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  Content += "<div style='display:flex;gap:10px;align-items:center'><input type='file' name='upload' style='font-family:inherit' required>";
  Content += "<input type='submit' value='Upload' class='btn'></div></form></div>";

  Content += "</div></body></html>\r\n";
  server.send(200, "text/html", Content);
}

//----------------------------------------------------
void StatusAbfrage()                                                                  // FehlerStatus
{
  Serial.println();
  Serial.println("Status - Anfrage");

  String Content = "";                                                                // für die HTML-Seite
  String sUE = String(char(154));                                                     // ü via special ASCII mapping, or just &Uuml;
  sUE = "&Uuml;";                                                                     // Use HTML Entity!

  Content = "<!DOCTYPE html>\n<html lang='de'>\n<head><meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>BMS-Status</title>";
  Content += getModernCSS();
  Content += "<script>function janein(text) {return confirm(text);}</script></head><body><div class='container'>";
  Content += "<div class='top-bar'><h2>Fehlermeldungen</h2><a class='btn' href='/'>Zurück</a></div>";

  Content += "<div class='card'><table class='info-table'>";

  if (packBasicInfo.ProtectionStatus == 0)
  {
    Content += "<tr><td>Status</td><td style='text-align:right'><span class='badge green'>KEINE FEHLER</span></td></tr>";
  }
  else 
  {
    if (bitRead(packBasicInfo.ProtectionStatus, 0)) Content += "<tr><td>Zellen-" + sUE + "berspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 1)) Content += "<tr><td>Zellen-Unterspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 2)) Content += "<tr><td>Batterie-" + sUE + "berspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 3)) Content += "<tr><td>Batterie-Unterspannung</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";

    if (bitRead(packBasicInfo.ProtectionStatus, 4)) Content += "<tr><td>Laden " + sUE + "bertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 5)) Content += "<tr><td>Laden Untertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 6)) Content += "<tr><td>Entladen " + sUE + "bertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 7)) Content += "<tr><td>Entladen Untertemperatur</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";

    if (bitRead(packBasicInfo.ProtectionStatus, 8)) Content += "<tr><td>Laden " + sUE + "berstrom</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 9)) Content += "<tr><td>Entladen " + sUE + "berstrom</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 10)) Content += "<tr><td>Kurzschluss</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 11)) Content += "<tr><td>IC Fehler</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
    if (bitRead(packBasicInfo.ProtectionStatus, 12)) Content += "<tr><td>MOS-Software Lock-in</td><td style='text-align:right'><span class='badge red'>Aktiv</span></td></tr>";
  }

  Content += "</table></div></div></body></html>\n";
  server.send(200, "text/html", Content);                                             // und weg damit

  Serial.println();
  delay(1000);
}

//----------------------------------------------------
void SetUpDaten()                                                                     // HTML-Setup-Seite
{
  Serial.println();
  Serial.println("Setup - Anfrage");

  DateTime now = myRTC.now();
  String Content = "";

  uint16_t intYear;
  uint8_t  intMonth, intDay, intHour, intMinute;

  if (server.args() != 0)
  {
    for (uint8_t i = 0; i < server.args(); i++)                                        // wenn Werte übergeben wurden
    {
      if (server.argName(i) == "Intervall") { Intervall = server.arg(i); putEEprom(0, Intervall); }
      if (server.argName(i) == "myPassword") { myPassword = server.arg(i); putEEprom(10, myPassword); }
      if (server.argName(i) == "SSID_WLAN") { WLAN_SSID = server.arg(i); putEEprom(60, WLAN_SSID); }
      if (server.argName(i) == "PASS_WLAN") { WLAN_PASS = server.arg(i); putEEprom(110, WLAN_PASS); }

      if (server.argName(i) == "YEAR")    intYear   = server.arg(i).toInt();
      if (server.argName(i) == "MONTH")   intMonth  = server.arg(i).toInt();
      if (server.argName(i) == "DAY")     intDay    = server.arg(i).toInt();
      if (server.argName(i) == "Stunden") intHour   = server.arg(i).toInt();
      if (server.argName(i) == "Minuten") intMinute = server.arg(i).toInt();

      Serial.print(server.argName(i) + ": ");
      Serial.println(server.arg(i));
    }

    EEPROM.commit();                                                 // EEPROM abschliessen
    myRTC.adjust(DateTime( intYear, intMonth,  intDay,  intHour,  intMinute, 0));
    now = myRTC.now();
  }
  now = myRTC.now();

  Content = "<!DOCTYPE html><html lang='de'><head><meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  Content += "<title>BMS-Logger Setup</title>";
  Content += getModernCSS();
  Content += "<style>input[type=text],input[type=number]{width:100%;padding:8px;border:1px solid #ccc;border-radius:6px;box-sizing:border-box} .date-inputs input{width:auto;display:inline-block;margin-right:2px;text-align:center}</style>";
  Content += "<script>function jn(text){return confirm(text);}</script></head><body><div class='container'>\n"
             "<script>window.history.pushState({}, document.title, window.location.pathname);</script>";
             
  Content += "<div class='top-bar'><h2>Einstellungen</h2><a class='btn' href='/'>Zurück</a></div>";

  Content += "<div class='card'><h3>Geräteinfo</h3>";
  Content += "<p><b>Version:</b> " + Version + "<br><b>Build:</b> " + Build + "<br><b>IP:</b> " + sIPAdresse + " &nbsp; <b>RSSI:</b> " + WiFi.RSSI() + " dBm<br>";
  Content += "<b>BMS-Typ:</b> " + packVersionsInfo.sBMSTyp + "<br><b>BT-Name:</b> " + BT_Name + "<br><b>BT RSSI:</b> " + String(myRSSI, 0) + " dBm</p></div>";

  Content += "<form method='GET'><div class='card'><h3>Systemzeit & Speichern</h3><table class='info-table'>";
  Content += "<tr><td>Datum & Uhrzeit<br><small>dd.mm.yyyy, hh:mm</small></td><td class='date-inputs'>"
             "<input name='DAY' value='" + String(now.day()) + "' size='2' maxLength='2'> . "
             "<input name='MONTH' value='" + String(now.month()) + "' size='2' maxLength='2'> . "
             "<input name='YEAR' value='" + String(now.year()) + "' size='4' maxLength='4'> &nbsp; "
             "<input name='Stunden' value='" + String(now.hour()) + "' size='2' maxLength='2'> : "
             "<input name='Minuten' value='" + String(now.minute()) + "' size='2' maxLength='2'>"
             "</td></tr>";
  Content += "<tr><td>Speicherintervall<br><small>(1-60 Minuten)</small></td><td><input name='Intervall' type='number' min='1' max='60' step='1' value='" + Intervall + "'></td></tr>";
  Content += "</table></div>";

  Content += "<div class='card'><h3>Netzwerk</h3><table class='info-table'>";
  Content += "<tr><td>WLAN SSID</td><td><input type='text' name='SSID_WLAN' minlength='8' maxlength='50' value='" + WLAN_SSID + "'></td></tr>";
  Content += "<tr><td>WLAN Passwort</td><td><input type='text' name='PASS_WLAN' minlength='8' maxlength='50' value='" + WLAN_PASS + "'></td></tr>";
  Content += "<tr><td>AccessPoint Passwort</td><td><input type='text' name='myPassword' minlength='8' maxlength='50' value='" + myPassword + "'></td></tr>";
  Content += "</table><br><button type='submit' class='btn' style='width:100%;margin-bottom:10px'>Speichern & Anwenden</button></form></div>";

  Content += "<div class='card'><h3>Aktionen</h3><table class='info-table'>";
  Content += "<tr><td>ESP32 Hard Reset</td><td style='text-align:right'><form method='POST' action='/Reset' style='margin:0'><button type='submit' class='btn btn-danger' onclick=\"return jn('Hardware sicher resetten?')\">Neustart</button></form></td></tr>";
  Content += "</table></div>";

  Content += "</div></body></html>\n";

  server.send(200, "text/html", Content);                                              // und weg damit
  Serial.println();
  delay(1000);
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
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css"))  return "text/css";
  else if (filename.endsWith(".js"))   return "application/javascript";
  else if (filename.endsWith(".png"))  return "image/png";
  else if (filename.endsWith(".gif"))  return "image/gif";
  else if (filename.endsWith(".jpg"))  return "image/jpeg";
  else if (filename.endsWith(".ico"))  return "image/x-icon";
  else if (filename.endsWith(".kml"))  return "application/googleearth.exe";
  else if (filename.endsWith(".xml"))  return "text/xml";
  else if (filename.endsWith(".csv"))  return "text/csv";
  else if (filename.endsWith(".pdf"))  return "application/x-pdf";
  else if (filename.endsWith(".zip"))  return "application/x-zip";
  else if (filename.endsWith(".gz"))   return "application/x-gzip";
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
    delay(10);
    Listen();
  }
}

//-------------------------------------------------
void Fehlerloeschen()                                                                    // Status ändern
{
  Serial.println("\nFehlerloeschen");
  "bms._fault_counts()";
  delay(1000);
  StatusAbfrage();
}
//-------------------------------------------------
void Reset()
{ Serial.println("R E S E T");
  delay(100);
  ESP.restart();
}

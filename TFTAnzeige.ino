//**************************************************
// Helper for drawing boxed values with FreeFonts WITHOUT FLICKER using GFXcanvas1
void printValueBox(String valStr, String unitStr, int x, int y, int w, int h, uint16_t color, const uint8_t *valFont, const uint8_t *unitFont, uint8_t alignMode = 0) {
  TFT_eSprite canvas = TFT_eSprite(&tft);
  if (canvas.createSprite(w, h) == nullptr) {
    // Kein Heap für Sprite — direkt auf TFT zeichnen als Fallback
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    if (valFont) tft.loadFont(valFont);
    tft.drawString(valStr + " " + unitStr, x + 2, y + h / 2);
    if (valFont) tft.unloadFont();
    return;
  }
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextColor(color, TFT_BLACK); 
  
  if (alignMode == 1) {
     if (unitStr.length() > 0) {
        if (unitFont == NULL) canvas.setTextFont(1);
        else canvas.loadFont(unitFont);
        
        canvas.setTextDatum(MR_DATUM);
        int unitW = canvas.textWidth(unitStr);
        canvas.drawString(unitStr, w - 2, h / 2);
        if (unitFont != NULL) canvas.unloadFont();
        
        if (unitStr == "C") canvas.drawCircle(w - 2 - unitW + 2, (h/2)-10, 2, color);
        
        if (valFont == NULL) canvas.setTextFont(1);
        else canvas.loadFont(valFont);
        
        canvas.drawString(valStr, w - unitW - 8, h / 2);
        if (valFont != NULL) canvas.unloadFont();
     } else {
        if (valFont == NULL) canvas.setTextFont(1);
        else canvas.loadFont(valFont);
        canvas.setTextDatum(MR_DATUM);
        canvas.drawString(valStr, w - 2, h / 2);
        if (valFont != NULL) canvas.unloadFont();
     }
  } else if (alignMode == 2) {
     // Center aligned
     if (valFont == NULL) canvas.setTextFont(1);
     else canvas.loadFont(valFont);
     canvas.setTextDatum(MC_DATUM);
     canvas.drawString(valStr, w / 2, h / 2);
     if (valFont != NULL) canvas.unloadFont();
  } else {
     // Left aligned
     if (valFont == NULL) canvas.setTextFont(1);
     else canvas.loadFont(valFont);
     canvas.setTextDatum(ML_DATUM);
     canvas.drawString(valStr, 2, h / 2);
     int valW = canvas.textWidth(valStr);
     if (valFont != NULL) canvas.unloadFont();
     
     if (unitStr.length() > 0) {
        if (unitFont == NULL) canvas.setTextFont(1);
        else canvas.loadFont(unitFont);
        canvas.drawString(unitStr, 2 + valW + 6, h / 2);
        if (unitFont != NULL) canvas.unloadFont();
        
        if (unitStr == "C") canvas.drawCircle(2 + valW + 4, (h/2)-10, 2, color);
     }
  }
  
  canvas.pushSprite(x, y);
  canvas.deleteSprite();
}

// Draw a modern Battery Arc / Stack directly on TFT, using a Micro-Sprite only for text (Saves 19.5 KB RAM!)
void drawBattery(int x, int y, int percent, bool force = false) {
  static int last_percent = -1;
  if(force) last_percent = -1;
  if(percent == last_percent && percent != -1) return; // Only draw when changed!
  last_percent = percent;

  // Battery shell (Direct drawing, no huge memory allocation needed)
  tft.drawRoundRect(x, y + 5, 55, 175, 4, TFT_DARKGREY);
  tft.fillRect(x + 20, y, 15, 6, TFT_DARKGREY); // Tip
  tft.fillRect(x + 2, y + 7, 51, 171, TFT_BLACK); // clear inner

  // set color based on SOC
  uint16_t color = TFT_GREEN;
  if (percent < 20) color = TFT_RED;
  else if (percent < 40) color = TFT_ORANGE;
  else if (percent < 60) color = TFT_YELLOW;
  
  // segment height calc
  int fillHeight = map(percent, 0, 100, 0, 169);
  int fillY = (y + 5 + 174) - fillHeight;
  
  if (fillHeight > 0) {
    tft.fillRect(x + 3, fillY, 49, fillHeight, color);
  }

  // --- MICRO SPRITE FÜR GESTOCHEN SCHARFE TEXT-GLÄTTUNG ---
  // Ein 46x26 Sprite frisst nur 2.3 KB RAM anstelle von 21 KB für die komplette Batterie.
  // Das verhindert den "StoreProhibited" Core-Panic bei aktiven WLAN/Webserver-Requests!
  TFT_eSprite txtSprite = TFT_eSprite(&tft);
  int tW = 46;
  int tH = 26;
  int tX = x + 4;  // ca. Mitte der Batterie (x+27)
  int tY = y + 82; // y+95 ist die Mitte 
  
  if (txtSprite.createSprite(tW, tH) != nullptr) {
      // Exakten Hintergrund im Mini-Fenster mathematisch rekonstruieren (split color)
      for (int sy = 0; sy < tH; sy++) {
          uint16_t rowColor = ((tY + sy) >= fillY) ? color : TFT_BLACK;
          txtSprite.drawFastHLine(0, sy, tW, rowColor);
      }
      
      txtSprite.loadFont(NotoSansBold15);
      txtSprite.setTextColor((fillHeight > 90) ? TFT_BLACK : TFT_WHITE);
      txtSprite.setTextDatum(MC_DATUM);
      txtSprite.drawString(String(percent) + "%", tW / 2, tH / 2);
      txtSprite.unloadFont();
      
      txtSprite.pushSprite(tX, tY);
      txtSprite.deleteSprite();
  }
}

void voltage(){
  digitalWrite(14, HIGH);
  yieldWeb(1);
  float measurement = (float) analogRead(35); //VBAT Pin 35=T4 34=TS
  float battery_voltage = (measurement / 4095.0) * 7.26;
  digitalWrite(14, LOW);
  
  uint16_t bColor = (battery_voltage < 2.8) ? TFT_RED : TFT_DARKGREY;
  // Akku rechtsbündig, ohne 'LiPo' für mehr Platz
  printValueBox(String(battery_voltage, 1), "V", 260, 0, 60, 20, bColor, NotoSansBold15, NotoSansBold15, 1);
}

//TFT 240 x 320 Modern Design
void TFT_Anzeige()
{
  tft.setTextSize(1); // STRIKTES Reset für alle FreeFonts
  String sTemp;

  if (Screensaver >= 60)                                                      // Bildschirm schonen xx Sek.
  {
    analogWrite(Backlight, 50);
    int PosX = random(5, 120);                                                // x-Werte anpassen für grosse Schrift
    int PosY = random(35, 75);                                                // y-Werte anpassen für grosse Schrift
    tft.fillScreen(TFT_BLACK);                                            // Hintergrundfüllung
    
    tft.loadFont(NotoSansBold36);
    uint16_t color = daheim ? TFT_GREEN : TFT_WHITE;
    tft.setTextColor(color, TFT_BLACK);
    
    tft.setCursor(PosX, PosY);
    tft.print(String(packBasicInfo.Volts / 1000.0, 2) + " V");
    tft.setCursor(PosX, PosY + 40);
    tft.print(String(packBasicInfo.Amps / 1000.0, 1) + " A");
    tft.setCursor(PosX, PosY + 80);
    tft.print(String(packBasicInfo.CapacityRemainAh / 1000.0, 1) + " Ah");
    
    tft.setCursor(PosX, PosY + 120);
    digitalWrite(14, HIGH);
    yieldWeb(1);
    float measurement = (float) analogRead(35); //VBAT Pin 35=T4 34=TS
    float battery_voltage = (measurement / 4095.0) * 7.26;
    digitalWrite(14, LOW);
    tft.print(String(battery_voltage, 1) + " V LiPo");
    tft.unloadFont();
  }
  else
  {
    analogWrite(Backlight, 200);

    // Zeit aus sDatum_Zeit abschneiden (vor dem Komma)
    String nurDatum = sDatum_Zeit;
    int commaIdx = sDatum_Zeit.indexOf(',');
    if (commaIdx > 0) nurDatum = sDatum_Zeit.substring(0, commaIdx);

    // ================= TOP BAR (DATE/TIME, BT-NAME & INTERNER AKKU) =================
    // Links (x=0, w=85): Nur Datum, Linksbündig
    printValueBox(nurDatum, "", 0, 0, 85, 20, TFT_DARKGREY, NotoSansBold15, NULL, 0);
    
    // Mitte (x=85, w=175): BT Name des ausgewaehlten BMS, Zentriert mit maximalem Platz!
    printValueBox(BT_Name, "", 85, 0, 175, 20, TFT_DARKGREY, NotoSansBold15, NULL, 2);
    // Rechts (x=220): TTGO interner ESP32 Akku, aufgeräumt ohne Lade-FETs zu stören!
    voltage(); 

    // ================= LEFT COL (CELL VOLTAGES) =================
    uint8_t i;
    for (i = 0; i < Anzahl_Zellen; i++)
    {
      float v = packCellInfo.CellVolt[i] / 1000.0;
      uint16_t cColor = (v < 3.0) ? TFT_RED : TFT_WHITE;
      String cName = "C" + String(i + 1);
      String cBal = bitRead(packBasicInfo.BalanceCodeLow, i) ? " *" : "";
      
      printValueBox(cName, "", 5, 25 + (i * 24), 30, 23, TFT_CYAN, NotoSansBold15, NotoSansBold15);
      printValueBox(String(v, 3), cBal, 35, 25 + (i * 24), 70, 23, cColor, NotoSansBold15, NotoSansBold15);
    }
    
    // Cell Diff
    uint16_t diffColor = (packCellInfo.CellDiff > 25) ? TFT_RED : TFT_WHITE;
    printValueBox("Diff", "", 5, 30 + (i * 24), 40, 23, TFT_DARKGREY, NotoSansBold15, NotoSansBold15);
    printValueBox(String(packCellInfo.CellDiff), "mV", 45, 30 + (i * 24), 60, 23, diffColor, NotoSansBold15, NotoSansBold15);

    // ================= MIDDLE COL (MAIN METRICS) =================
    uint16_t vColor = (packBasicInfo.Volts / 1000.0 < 12.0) ? TFT_RED : TFT_WHITE;
    printValueBox(String(packBasicInfo.Volts / 1000.0, 2), "V", 110, 25, 135, 35, vColor, NotoSansBold36, NotoSansBold15, true);

    uint16_t wColor = TFT_WHITE;
    float wVal = packBasicInfo.Watts / 1.0;
    if (wVal < 0) wColor = TFT_RED;
    else if (wVal > 0) wColor = TFT_GREEN;
    printValueBox(String(packBasicInfo.Watts / 1.0, 0), "W", 110, 65, 135, 35, wColor, NotoSansBold36, NotoSansBold15, true);

    uint16_t aColor = TFT_WHITE;
    float aVal = packBasicInfo.Amps / 1000.0;
    if (aVal < 0) aColor = TFT_RED;
    else if (aVal > 0) aColor = TFT_GREEN;
    printValueBox(String(aVal, 1), "A", 110, 105, 135, 35, aColor, NotoSansBold36, NotoSansBold15, true);

    uint16_t ahColor = (RestLadung < 60) ? TFT_RED : TFT_WHITE;
    printValueBox(String(RestLadung, 0), "Ah", 110, 145, 135, 35, ahColor, NotoSansBold36, NotoSansBold15, true);
    
    // ================= RIGHT COL (BATTERY / SOC) =================
    extern bool force_battery_redraw;
    static bool was_in_screensaver = false;
    
    if (Screensaver < 60 && was_in_screensaver) {
      force_battery_redraw = true; // Wakeup from screensaver
    }
    was_in_screensaver = (Screensaver >= 60);

    // Batterie ist beim Original ganz rechts. Position X = 250, Y = 25 um bis unten auszustrahlen.
    drawBattery(255, 25, packBasicInfo.CapacityRemainPercent, force_battery_redraw);

    // Protection & FET Fault indicator
    bool show_f = (packBasicInfo.ProtectionStatus != 0) || ((packBasicInfo.MosfetStatus & 0x03) != 0x03);
    static bool last_show_f = false;
    
    if (show_f != last_show_f || force_battery_redraw) {
      if (show_f)
      {
        tft.fillSmoothCircle(120, 195, 15, TFT_RED, TFT_BLACK);
        tft.loadFont(NotoSansBold15);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("F", 120, 196); // exakt mittig ueber dem 120,195 kreis, leicht nach unten korrigiert
        tft.unloadFont();
      }
      else
      {
        tft.fillCircle(120, 195, 15, TFT_BLACK); // clear
      }
      last_show_f = show_f;
    }

    // ================= BOTTOM BAR (FETS & TEMPS) =================
    // Keine "ON/OFF" Texte mehr, stattdessen nur farbige Indikatoren in der Fusszeile
    printValueBox("Entl.-FET", "", 5, 215, 85, 25, bitRead(packBasicInfo.MosfetStatus, 1) ? TFT_GREEN : TFT_RED, NotoSansBold15, NULL);
    printValueBox("Lade-FET", "", 95, 215, 75, 25, bitRead(packBasicInfo.MosfetStatus, 0) ? TFT_GREEN : TFT_RED, NotoSansBold15, NULL);

    // Temperatur Box T: 22.0 / 22.0 °C
    String temps = "T: " + String(packBasicInfo.Temp1 / 10.0, 1) + " / " + String(packBasicInfo.Temp2 / 10.0, 1);
    // x=175, breite = 140 um genug Platz fuer die Temperatur plus °C zu haben
    printValueBox(temps, "C", 175, 215, 140, 25, TFT_CYAN, NotoSansBold15, NotoSansBold15);
    
    force_battery_redraw = false; // Endgültiger Reset am Ende der UI-Routine!
    tft.unloadFont();
  }
}

//**************************************************
void lcdDisconnect()
{
  tft.fillScreen(TFT_BLACK); //
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setCursor(3, 25);
  tft.setTextColor(TFT_RED);
  tft.println("BMS lost");

  tft.setTextColor(TFT_WHITE);
  tft.println("");
  tft.println("reconnecting....");
  tft.println("(it may take a while)");
}

//**************************************************
void lcdConnected()
{
  tft.loadFont(NotoSansBold15);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.println("BMS connected!");
}

//**************************************************
void lcdConnectionFailed()
{
  Serial.println("We have failed to connect to the server; there is nothin more we will do.");
  tft.loadFont(NotoSansBold15);
  tft.setTextSize(1);
  tft.setTextColor(TFT_RED, TFT_BLACK); // Hintergrund erzwingen für scharfe Kanten!
  tft.println("connection failed!");
}

//**************************************************
void lcdConnectingStatus(uint8_t state)
{
  tft.loadFont(NotoSansBold15);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Hintergrund erzwingen für scharfe Kanten!
  if (state == 0) tft.setCursor(5, 25); // start from top baseline
  switch (state)
  {
    case 0:
      tft.println("connecting to BMS...");
      break;
    case 1:
      tft.println("created client");
      break;
    case 2:
      tft.println("connected to server");
      break;
    case 3:
      tft.println("service not found");
      break;
    case 4:
      tft.println("found service");
      break;
    case 5:
      tft.println("char. not found");
      break;
    case 6:
      tft.println("char. found");
      break;
    default:
      break;
  }
}

//**************************************************
void Fehler_Anzeige()
{
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  Serial.print("Fehlermeldungen");

  tft.loadFont(NotoSansBold15);
  tft.setCursor(5, 15);
  tft.println("Fehlermeldungen:");

  if (packBasicInfo.ProtectionStatus == 0)
  {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 45);
    tft.println("-KEINE-");
  }
  tft.setCursor(5, 45);
  tft.setTextColor(TFT_RED, TFT_BLACK);

  if (bitRead(packBasicInfo.ProtectionStatus, 0)) tft.println("Zellen-Ueberspannung");
  if (bitRead(packBasicInfo.ProtectionStatus, 1)) tft.println("Zellen-Unterspannung");
  if (bitRead(packBasicInfo.ProtectionStatus, 2)) tft.println("Batterie-Ueberspannung");
  if (bitRead(packBasicInfo.ProtectionStatus, 3)) tft.println("Batterie-Unterspannung");

  if (bitRead(packBasicInfo.ProtectionStatus, 4)) tft.println("Laden Uebertemperatur");
  if (bitRead(packBasicInfo.ProtectionStatus, 5)) tft.println("Laden Untertemperatur");
  if (bitRead(packBasicInfo.ProtectionStatus, 6)) tft.println("Entladen Uebertemperatur");
  if (bitRead(packBasicInfo.ProtectionStatus, 7)) tft.println("Entladen Untertemperatur");

  if (bitRead(packBasicInfo.ProtectionStatus, 8)) tft.println("Laden Ueberstrom");
  if (bitRead(packBasicInfo.ProtectionStatus, 9)) tft.println("Entladen Ueberstrom");
  if (bitRead(packBasicInfo.ProtectionStatus, 10)) tft.println("Kurzschluss");
  if (bitRead(packBasicInfo.ProtectionStatus, 11)) tft.println("IC Fehler");
  if (bitRead(packBasicInfo.ProtectionStatus, 12)) tft.println("MOS-Software Lock-in");

  while (digitalRead(BUTTON_Mitte) == HIGH)
  {
    yieldWeb(100);
    esp_task_wdt_reset();                                            // Watchdog reset
  }
  yieldWeb(100);
  tft.unloadFont();
  tft.fillScreen(TFT_BLACK);
}

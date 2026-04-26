//**************************************************
// Helper for drawing boxed values with FreeFonts WITHOUT FLICKER using GFXcanvas1
void printValueBox(String valStr, String unitStr, int x, int y, int w, int h, uint16_t color, const uint8_t *valFont, const uint8_t *unitFont, uint8_t alignMode = 0) {
  TFT_eSprite canvas = TFT_eSprite(&tft);
  canvas.createSprite(w, h);
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

// Global persistent battery sprite to prevent heap fragmentation failures
TFT_eSprite batSprite(&tft);
bool batSpriteCreated = false;

// Draw a modern Battery Arc / Stack
void drawBattery(int x, int y, int percent, bool force = false) {
  static int last_percent = -1;
  if(force) last_percent = -1;
  if(percent == last_percent && percent != -1) return; // Only draw when changed!
  last_percent = percent;

  // Create Sprite once to reserve the 21KB RAM permanently, preventing fragmentation deaths
  if (!batSpriteCreated) {
      if (batSprite.createSprite(60, 180) == nullptr) {
          Serial.println("ERROR: Battery Sprite RAM allocation failed!");
          return; // Give up gracefully if no RAM
      }
      batSpriteCreated = true;
  }

  batSprite.fillSprite(TFT_BLACK);

  // Battery shell
  batSprite.drawRoundRect(0, 5, 55, 175, 4, TFT_DARKGREY);
  batSprite.fillRect(20, 0, 15, 6, TFT_DARKGREY); // Tip

  // clear inner
  batSprite.fillRect(2, 7, 51, 171, TFT_BLACK);

  // set color based on SOC
  uint16_t color = TFT_GREEN;
  if (percent < 20) color = TFT_RED;
  else if (percent < 40) color = TFT_ORANGE;
  else if (percent < 60) color = TFT_YELLOW;
  
  // segment height calc
  int fillHeight = map(percent, 0, 100, 0, 169);
  int fillY = (5 + 174) - fillHeight;
  
  if (fillHeight > 0) {
    batSprite.fillRect(3, fillY, 49, fillHeight, color);
  }

  // Print Percent inside. Since it's a Sprite, TFT_eSPI will natively alpha-blend 
  // the smooth font reading from the RAM buffer, perfectly blending over the split background!
  batSprite.loadFont(NotoSansBold15);
  batSprite.setTextColor((fillHeight > 90) ? TFT_BLACK : TFT_WHITE);
  batSprite.setTextDatum(MC_DATUM);
  batSprite.drawString(String(percent) + "%", 27, 95);
  batSprite.unloadFont();
  
  // Push physical pixels
  batSprite.pushSprite(x, y);
  // Absicht: deleteSprite() entfernt, um RAM dauerhaft zu reservieren!
}

void voltage(){
  digitalWrite(14, HIGH);
  delay(1);
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
    int PosX = random(5, 150);                                                // x-Werte 0 bis rechts
    int PosY = random(20, 160);                                               // y-Werte 0 bis unten
    tft.fillScreen(TFT_BLACK);                                            // Hintergrundfüllung
    
    tft.loadFont(NotoSansBold15);
    uint16_t color = daheim ? TFT_GREEN : TFT_WHITE;
    tft.setTextColor(color);
    
    tft.setCursor(PosX, PosY);
    tft.print(String(packBasicInfo.Volts / 1000.0, 2) + " V");
    tft.setCursor(PosX, PosY + 25);
    tft.print(String(packBasicInfo.Amps / 1000.0, 1) + " A");
    tft.setCursor(PosX, PosY + 50);
    tft.print(String(packBasicInfo.CapacityRemainAh / 1000.0, 1) + " Ah");
    
    tft.loadFont(NotoSansBold15);
    tft.setCursor(PosX, PosY + 75);
    digitalWrite(14, HIGH);
    delay(1);
    float measurement = (float) analogRead(35); //VBAT Pin 35=T4 34=TS
    float battery_voltage = (measurement / 4095.0) * 7.26;
    digitalWrite(14, LOW);
    tft.print(String(battery_voltage, 1) + " V LiPo");
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
  tft.setTextColor(TFT_YELLOW);

  String sUE = String(char(154));                        // https://theasciicode.com.ar/ ASCII-Tabelle

  Serial.print("Fehlermeldungen");

  tft.setTextFont(1); // Reset to classic built-in font for the error log!
  tft.setTextSize(2);
  tft.setCursor(5, 10);
  tft.println("Fehlermeldungen:");

  if (packBasicInfo.ProtectionStatus == 0)
  {
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(5, 45);
    tft.println("-KEINE-");
  }
  tft.setCursor(5, 35);
  tft.setTextColor(TFT_RED);

  tft.println(bitRead(packBasicInfo.ProtectionStatus, 0) ? "Zellen-" + sUE + "berspannung" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 1) ? "Zellen-Unterspannung" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 2) ? "Batterie-" + sUE + "berspannung" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 3) ? "Batterie-Unterspannung" : " ");

  tft.println(bitRead(packBasicInfo.ProtectionStatus, 4) ? "Laden " + sUE + "bertemperatur" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 5) ? "Laden Untertemperatur" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 6) ? "Entladen " + sUE + "bertemperatur" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 7) ? "Entladen Untertemperatur" : " ");

  tft.println(bitRead(packBasicInfo.ProtectionStatus, 8) ? "Laden " + sUE + "berstrom" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 9) ? "Entladen " + sUE + "berstrom" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 10) ? "Kurzschluss" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 11) ? "IC Fehler" : " ");
  tft.println(bitRead(packBasicInfo.ProtectionStatus, 12) ? "MOS-Software Lock-in" : " ");

  while (digitalRead(BUTTON_Mitte) == HIGH)
  {
    delay(100);
    esp_task_wdt_reset();                                            // Watchdog reset
  }
  delay(100);
  tft.fillScreen(TFT_BLACK);
}

void WLAN_Ein()
{
  daheim = false;
  tft.fillScreen(TFT_BLUE);
  tft.loadFont(NotoSansBold15);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); // Verhindert verwaschene Kanten bei Smooth Fonts
  tft.setTextSize(1);
  Serial.println();
  Serial.println("SSID's...");
  int n  = WiFi.scanNetworks();                                  // SSID's suchen
  for (int i = 0; i < n; i++)
  {
    Serial.print(rTrim(WiFi.SSID(i), 25));
    Serial.print(": ");
    Serial.print(WiFi.RSSI(i));
    if (WiFi.SSID(i) == WLAN_SSID)                                // eigene SSID da?
    {
      daheim = true;
      Serial.println(" dBm   ok!");
      tft.setTextColor(TFT_YELLOW, TFT_BLUE);
      tft.setCursor(1, 20);
      tft.print("WiFi verbinden ");
      tft.print(WiFi.SSID(i));
      tft.setCursor(1, 40);
      tft.print("RSSI ");
      tft.print(WiFi.RSSI(i));
      tft.println(" dBm");
      break;
    }
    else  Serial.println(" dBm");
  }
  Serial.println();

  if (daheim)                                                    // wenn eigenes WLAN da ist
  {
    n = 0;
    WiFi.mode(WIFI_STA);                                         // Stations-Modus --> Esp im Heimnetzwerk einloggen
    WiFi.hostname("BMS_Logger");

    char ssid[WLAN_SSID.length() + 1];
    WLAN_SSID.toCharArray(ssid, WLAN_SSID.length() + 1);

    char pass[WLAN_PASS.length() + 1];
    WLAN_PASS.toCharArray(pass, WLAN_PASS.length() + 1);

    WiFi.begin(ssid, pass);                                       // einloggen

    tft.setCursor(1, 60);
    tft.print("WiFi-Init ");

    Serial.print("Connecting WLAN ");
    while ((WiFi.status() != WL_CONNECTED) && (n <= 10))         // max. 10 Versuche
    {
      yield();
      delay(2000);
      Serial.print(".");
      tft.print("*");
      n++;
    }

    if (WiFi.status() == WL_CONNECTED)                           // Verbunden
    {
      Serial.println("ok");
      
      // NTP Zeit holen und RTC synchronisieren
      Serial.println("NTP Sync...");
      configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
      
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) {                               // Warte max 5 Sek auf Zeit
        myRTC.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        Serial.println("RTC synchronisiert.");
      }
      Serial.print("RSSI:      "  );
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");

      Serial.print("SSID:       ");
      Serial.println(WLAN_SSID);
      Serial.print("Passwort:   ");
      Serial.println(WLAN_PASS);

      sIPAdresse = WiFi.localIP().toString();
      Serial.print("IP address: ");
      Serial.println (sIPAdresse);                               // aktuelle IP

      tft.setCursor(1, 80);
      tft.print("IP: ");
      tft.println(WiFi.localIP());
    }
    else
    {
      Serial.println("WLAN Fehler");
      tft.setCursor(1, 80);
      tft.print("WLAN Fehler");
      delay(1000);
      connectet = false;
    }
  }
  else
  {
    connectet = false;
  }

  if (!connectet)
  {
    Serial.println("AP - Mode an,");
    tft.fillScreen(TFT_BLUE);
    tft.loadFont(NotoSansBold15);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(1);

    char cPASS[myPassword.length() + 1] ;                        // AP-Passwort in Charakter
    myPassword.toCharArray(cPASS, myPassword.length() + 1);

    WiFi.mode(WIFI_AP);                                          // Access-Point-Modus --> Access-Point Adresse http://192.168.4.1/
    if (WiFi.softAP("BMS-LOGGER", cPASS))                        // Netzwerkname, Passwort
    {
      Serial.print("Netzname BMS-LOGGER");
      Serial.println (", Adresse " + sIPAdresse);
      tft.setCursor(1, 25);
      tft.println("Netzwerk \"BMS_LOGGER\"");
      tft.println("IP 192.168.4.1 oeffnen.");
    }
    else
    {
      Serial.println(" Fehler!");
    }
  }
  server.on("/", handleRoot);
  server.on("/Chart", ChartSeite);
  server.on("/format", formatSpeicher);
  server.on("/Setup",  SetUpDaten);                                            // Daten für das Beispiel xxx.xxx.xxx.xxx/Setup
  server.on("/Fehlerloeschen", Fehlerloeschen);
  server.on("/Listen", Listen);                                                // Daten für das Beispiel xxx.xxx.xxx.xxx/Listen
  server.on("/StatusAbfrage", StatusAbfrage);
  server.on("/Loeschen", Loeschen);
  server.on("/Reset",    Reset);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "fertig");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  server.begin();
  Serial.println("HTTP Server gestarted");
  delay(2000);
}
//----------------------------------------------

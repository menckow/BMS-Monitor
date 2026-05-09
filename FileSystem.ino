// SD-Karte on-demand mounten — gibt ~20 KB Heap frei wenn nicht aktiv genutzt.
// Damit bleibt genug Heap für stabilen Webserver-/WiFi-Betrieb.

#define FLUSH_PERCENT 90                                             // Flush wenn SPIFFS zu 90% voll
#define LOG_FILENAME "/BMS_LOG.CSV"

bool sdMount() {
  if (!sdCardPresent) return false;
  webServerLastActive = millis();                                    // BLE-Polling im loop() pausieren (Heap-Schutz)
  if (sdCardActive) return true;                                     // schon gemountet
  if (SD.begin(13, sdSPI, 4000000)) {
    sdCardActive = true;
    return true;
  }
  return false;
}

void sdUnmount() {
  if (sdCardActive) {
    SD.end();
    sdCardActive = false;
  }
  webServerLastActive = millis();                                    // noch 1 Sek BLE-Pause für Beruhigung
}

fs::FS& getFS() {
  if (sdCardActive) return SD;
  return SPIFFS;
}

//-------------------------------------------------
// SPIFFS-Buffer auf SD-Karte verschieben.
// - Erster Flush: kompletter Inhalt (inkl. Header) auf SD.
// - Folge-Flush: erste Zeile (Header) überspringen, Rest anhängen.
// - SPIFFS-Datei wird erst NACH erfolgreichem SD-Schreiben gelöscht (Stromausfall-sicher).
// Rückgabe: true wenn erfolgreich, false bei Fehler (SPIFFS bleibt dann erhalten).
bool flushBufferToSD() {
  if (!sdCardPresent) return false;
  if (!SPIFFS.exists(LOG_FILENAME)) return true;                     // nichts zu flushen

  Serial.println("[FLUSH] Starte SPIFFS -> SD Transfer...");
  uint32_t heapVor = ESP.getFreeHeap();

  if (!sdMount()) {
    Serial.println("[FLUSH] SD-Mount fehlgeschlagen!");
    return false;
  }

  bool sdHasFile = SD.exists(LOG_FILENAME);
  File src = SPIFFS.open(LOG_FILENAME, "r");
  if (!src) {
    Serial.println("[FLUSH] SPIFFS-Datei nicht lesbar!");
    sdUnmount();
    return false;
  }

  File dst = SD.open(LOG_FILENAME, sdHasFile ? "a" : "w");
  if (!dst) {
    Serial.println("[FLUSH] SD-Datei nicht schreibbar!");
    src.close();
    sdUnmount();
    return false;
  }

  // Wenn SD-Datei existiert: erste Zeile (Header) aus SPIFFS-Quelle überspringen
  if (sdHasFile) {
    while (src.available() && src.read() != '\n') { /* skip header */ }
  }

  // Daten in 512-Byte-Blöcken kopieren — schont Heap, gibt yield()-Möglichkeit
  uint8_t buf[512];
  size_t totalCopied = 0;
  while (src.available()) {
    size_t n = src.read(buf, sizeof(buf));
    if (n == 0) break;
    if (dst.write(buf, n) != n) {
      Serial.println("[FLUSH] Schreibfehler auf SD!");
      dst.close();
      src.close();
      sdUnmount();
      return false;                                                  // SPIFFS bleibt erhalten
    }
    totalCopied += n;
    yield();
    esp_task_wdt_reset();
  }

  dst.flush();
  dst.close();
  src.close();
  sdUnmount();

  // Erst nach bestätigtem Schreiben: SPIFFS-Buffer löschen
  SPIFFS.remove(LOG_FILENAME);

  Serial.println("[FLUSH] OK: " + String(totalCopied) + " Byte verschoben. Heap: "
                 + String(heapVor) + " -> " + String(ESP.getFreeHeap()));
  return true;
}

//-------------------------------------------------
String schreibeDatei(String Text, String filename, bool Flag)                        // Flag true ist anhängen.
{
  String sFileName = filename;
  if (!sFileName.startsWith("/")) sFileName = "/" + filename;

  // Logging-Daten landen IMMER in SPIFFS (schnell, kein SD-Mount).
  // Periodisches Flushing nach SD passiert weiter unten.
  if (SPIFFS.totalBytes() - SPIFFS.usedBytes() <= 10000) {
    Serial.println("SPIFFS voll!");
    return "voll";
  }

  Serial.print("[SPIFFS] Schreibe: ");
  Serial.println(sFileName);

  File sFile = SPIFFS.open(sFileName, Flag ? "a" : "w");
  if (sFile) {
    sFile.print(Text);
    sFile.close();
  }

  // SPIFFS zu voll? Dann Logdatei auf SD verschieben um Platz zu schaffen.
  // Nicht während aktiver Webserver-Verbindung (dann hat WiFi Vorrang).
  if (sFileName == LOG_FILENAME && sdCardPresent) {
    size_t threshold = (SPIFFS.totalBytes() * FLUSH_PERCENT) / 100;
    if (SPIFFS.usedBytes() >= threshold && (millis() - webServerLastActive > 5000)) {
      Serial.println("[FLUSH] SPIFFS bei " + String((SPIFFS.usedBytes() * 100) / SPIFFS.totalBytes())
                     + "% (" + String(SPIFFS.usedBytes()) + "/" + String(SPIFFS.totalBytes()) + ") -> Flush");
      flushBufferToSD();
    }
  }

  return "ok";
}

//-------------------------------------------------
bool handleFileRead(String path)
{
  if (path.endsWith("/")) path += "index.html";
  path = server.urlDecode(path);

  // Spezialfall: Logdatei wird angefordert — vorher Buffer auf SD flushen,
  // dann SD-Datei streamen. Browser bekommt vollständigen Datensatz.
  if (path == LOG_FILENAME && sdCardPresent) {
    flushBufferToSD();                                               // alle aktuellen Daten erst nach SD
    if (sdMount()) {
      if (SD.exists(LOG_FILENAME)) {
        File file = SD.open(LOG_FILENAME, "r");
        size_t fileSize = file.size();
        Serial.println("[SD] Stream Logdatei (" + String(fileSize) + " Byte) — BLE pausiert");
        webServerLastActive = millis();                              // BLE bleibt pausiert
        server.streamFile(file, "text/csv");
        file.close();
        sdUnmount();
        Serial.println("[SD] Stream fertig, BLE wird gleich wieder aktiv");
        return true;
      }
      sdUnmount();
    }
    // Fallback wenn SD-Datei nicht existiert: SPIFFS-Buffer ausliefern
  }

  String pathWithGz = path + ".gz";

  // SPIFFS prüfen (immer aktiv)
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {
    if (SPIFFS.exists(pathWithGz)) path += ".gz";
    Serial.println("[SPIFFS] Lese: " + path);
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }

  // Sonst: SD on-demand (z.B. archivierte ältere Dateien)
  if (sdCardPresent && sdMount()) {
    if (SD.exists(pathWithGz) || SD.exists(path)) {
      if (SD.exists(pathWithGz)) path += ".gz";
      Serial.println("[SD] Lese: " + path);
      File file = SD.open(path, "r");
      server.streamFile(file, getContentType(path));
      file.close();
      sdUnmount();
      return true;
    }
    sdUnmount();
  }
  return false;
}

//-------------------------------------------------
void Loeschen()
{
  String sFileName = server.arg(0);
  if (!sFileName.startsWith("/")) sFileName = "/" + sFileName;

  if (SPIFFS.exists(sFileName)) {
    Serial.println("[SPIFFS] Loesche: " + sFileName);
    SPIFFS.remove(sFileName);
  } else if (sdCardPresent && sdMount()) {
    Serial.println("[SD] Loesche: " + sFileName);
    SD.remove(sFileName);
    sdUnmount();
  }

  Listen();
}

//-------------------------------------------------
void formatSpeicher() {
  Serial.println("[SPIFFS] Formatiere Speicher...");
  SPIFFS.format();
  Listen();
}

size_t getFSTotalBytes() { return SPIFFS.totalBytes(); }
size_t getFSUsedBytes()  { return SPIFFS.usedBytes(); }

//-------------------------------------------------
// Direkter Zugriff auf die Logdatei im SPIFFS (ohne Flush, ohne SD-Mount)
void logFromSpiffs() {
  Serial.println("Log SPIFFS - Anfrage");
  if (SPIFFS.exists(LOG_FILENAME)) {
    File f = SPIFFS.open(LOG_FILENAME, "r");
    server.streamFile(f, "text/csv");
    f.close();
  } else {
    server.send(200, "text/csv", CSV_Titel);                         // nur Header (leerer Buffer)
  }
  webServerLastActive = millis();
}

// Direkter Zugriff auf die Logdatei auf der SD-Karte (ohne vorherigen Flush)
void logFromSd() {
  Serial.println("Log SD - Anfrage");
  if (sdCardPresent && sdMount()) {
    if (SD.exists(LOG_FILENAME)) {
      File f = SD.open(LOG_FILENAME, "r");
      webServerLastActive = millis();
      Serial.println("[SD] Stream Logdatei (" + String(f.size()) + " Byte) - BLE pausiert");
      server.streamFile(f, "text/csv");
      f.close();
      sdUnmount();
      return;
    }
    sdUnmount();
  }
  server.send(200, "text/csv", CSV_Titel);                           // nichts auf SD
  webServerLastActive = millis();
}

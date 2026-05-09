# BMS-Logger ESP32 (TTGO T4)

Modernes Bluetooth-Auslese-Dashboard für Smart-BMS (Batterie-Management-Systeme wie JBD / LTT Power) auf dem **LilyGO TTGO T4 v1.4** (ESP32 + ILI9341 TFT 320×240).

Das Programm verbindet sich komplett drahtlos per BLE mit Deinem Batterie-BMS, wertet Spannungen, Ströme und Zellinformationen aus und präsentiert sie:
- auf einem **modernen TFT-Display** mit kantengeglätteten Vektorschriften
- über einen **eingebauten Webserver** (responsive für Smartphone & Desktop)
- als **historische CSV-Daten** mit Diagramm-Visualisierung

**Wichtiges Credit:** Dieses Projekt basiert auf der hervorragenden Grundarchitektur von Uwe — [UK1408/BMS-und-ESP32](https://github.com/UK1408/BMS-und-ESP32). Großer Dank für die JBD-Protokoll-Implementierung!

---

## ✨ Kern-Features

### TFT-Anzeige
* **Vektorschriften (NotoSansBold)** — kantengeglättete Schrift statt verpixelter Bitmap-Fonts
* **Flackerfreies Sprite-Rendering** — Live-Werte werden im RAM vorgerendert und atomar aufs Display geschoben (`TFT_eSprite`)
* **Heap-Schutz** — Sprite-Allokationen werden geprüft, bei wenig RAM Fallback auf direktes TFT-Drawing
* **Live-Status-Anzeige** für Zellspannungen, Strom, Spannung, Watt, Restkapazität, Temperaturen, FET-Status, Fehlermeldungen
* **Top-Bar mit 175 px Mittelfeld** — auch lange BMS-Namen werden nicht abgeschnitten

### Multi-BMS Live-Wechsel (ohne Reboot)
* Beim Start scannt der Logger BLE-Geräte und erkennt mehrere konfigurierte BMS
* **Obere Taste**: Bei ≥2 erkannten BMS öffnet sich ein Auswahlmenü, sauberer Reconnect ohne Memory-Leaks
* **Untere Taste**: Schneller Toggle zwischen den zwei in der Web-Konfig hinterlegten BMS-MAC-Adressen

### Webserver — Hauptseite (`/`)
* **Responsive Design** — passt sich dynamisch an Smartphone/Tablet/Desktop an (Media-Queries für ≤600 px und ≤380 px)
* Live-Status: SOC, Gesamtspannung, Strom, Leistung, Restkapazität, Lade-/Entladezeit
* Zellspannungen mit Balken-Visualisierung und Balancing-Anzeige
* FET-Steuerung per Klick (Lade/Entlade-FET ein/aus)
* Auto-Refresh alle 60 Sekunden
* Sofortiger Redirect nach FET-Schaltung — kein Hängen, kein doppeltes Klicken

### Diagramme (`/Chart`)
* Vier Live-Charts (Chart.js): Spannung, Zellen, Strom, SOC
* **Datenquellen-Auswahl**: ESP-internen Speicher (aktuelle Daten) oder SD-Karte (Archiv)
* **BMS-Auswahl** in der CSV (filtern bei Multi-Akku-Setups)
* Status-Zeile zeigt Anzahl geladener Datenpunkte oder Fehler-Hinweis

### Dateimanager (`/Listen`)
* Listet **SPIFFS und SD-Karte** getrennt mit Auslastungs-Balken
* Direkter Download und Löschen einzelner Dateien
* Datei-Upload in SPIFFS
* SPIFFS-Format-Funktion

### Setup (`/Setup`)
* WLAN-SSID + Passwort konfigurieren
* Access-Point-Passwort setzen
* RTC-Datum/Uhrzeit
* Speicher-Intervall (1–60 Min)
* Zwei BMS-MAC-Adressen für Multi-Akku-Toggle
* Hardware-Reset-Knopf
* **NTP-Sync** beim Verbinden ins Heimnetzwerk (CET mit Sommer-/Winterzeit)

### Status (`/StatusAbfrage`)
* Detailansicht aller BMS-Schutz-Bits (Über-/Unterspannung, Übertemperatur, Kurzschluss, IC-Fehler, …)

---

## 💾 Intelligentes Speicher-Management (Hybrid SPIFFS + SD)

Der ESP32 hat begrenzten Heap (~250 KB) — die SD-Bibliothek würde permanent ~20 KB belegen und WLAN/Webserver destabilisieren. Daher dieses Konzept:

```
┌─────────────────┐  Logentry alle X Min
│  SPIFFS         │ ◄─────────────────────
│  /BMS_LOG.CSV   │   (schneller Buffer, kein SD-Mount nötig)
└────────┬────────┘
         │ ab 90 % SPIFFS-Auslastung:
         │ flushBufferToSD()
         ▼
┌─────────────────┐
│  SD-Karte       │  (Hauptarchiv, wächst kontinuierlich)
│  /BMS_LOG.CSV   │
└─────────────────┘
```

* **Im Normalbetrieb**: SD ist nicht gemountet → ~20 KB mehr Heap → stabiler Webserver
* **Logging schreibt in SPIFFS**: schnell, kein SPI-Bus-Konflikt, kein Mount-Overhead
* **Bei 90 % SPIFFS-Auslastung**: SD wird kurz gemountet, Buffer wird in 512-Byte-Blöcken auf SD angehängt, dann wieder unmountet
* **Header-Handling**: Erster Flush schreibt CSV-Header, alle weiteren überspringen die erste Zeile (kein Header in der Mitte der Datei)
* **Stromausfall-sicher**: SPIFFS-Buffer wird **erst nach** bestätigtem SD-Schreiben gelöscht
* **BLE-Pause während SD-Zugriff**: Über `webServerLastActive`-Flag, schont RAM und vermeidet SPI-Konflikte
* **Log-Download-Button** (`/BMS_LOG.CSV`): triggert Flush, streamt dann komplette SD-Datei

### Drei Datenquellen-Endpunkte
| URL | Verhalten |
|---|---|
| `/BMS_LOG.CSV` | Komplettdatei: erst Flush, dann SD-Stream (für Downloads) |
| `/log_spiffs` | Nur ESP-internen Buffer (für Diagramme: aktuelle Daten) |
| `/log_sd` | Nur SD-Datei (für Diagramme: Archiv, kein Flush) |

---

## 🔌 Webserver-Stabilität (Hard-fought lessons)

Auf dem ESP32 mit gleichzeitig BLE + WiFi + WebServer + TFT + SD ist der Heap der kritische Faktor. Folgende Optimierungen halten das System stabil:

* **Single-Send statt Chunked-Encoding** — Hauptseite und Listen-Seite werden komplett in einem `String` gebaut und mit einem einzigen `server.send()` ausgeliefert. Vermeidet `errno: 11 / EAGAIN`-Probleme bei chunked TCP-Writes
* **`String.reserve()`** — verhindert Heap-Fragmentierung beim Aufbau der Antwort
* **`mache_HTML_Seite()` nur im Handler-Kontext** — `server.send()` außerhalb eines Request-Handlers korrumpiert den WebServer-State und führt zu "erst nach mehrfachem Klicken"-Symptomen
* **Sofortiger HTTP-302-Redirect** nach FET-Schaltbefehlen — kein 10-Sekunden-Warten auf BLE-Antwort, kein Browser-Timeout
* **`yieldWeb()`** — Spezial-Delay das `server.handleClient()` während Wartezyklen weiterlaufen lässt
* **BLE-Pause bei Webserver-Aktivität** — `loop()` skipt `bleRequestData()` wenn die letzte Web-Request <1 Sekunde her ist

---

## 🎨 Responsive Design (Mobile-First)

CSS mit drei Breakpoints:
* **Desktop (>600 px)**: 800 px max-width, klassisches Card-Layout, Top-Bar zweispaltig
* **Mobile (≤600 px)**: 8 px Padding, Top-Bar gestapelt, Navigation als 2-Spalten-Grid, kompaktere Buttons (44 px Touch-Target), Datums-Eingaben auf Setup-Seite bleiben inline
* **Sehr klein (≤380 px)**: Navigation auf 1 Spalte, jeder Button volle Breite

Alle Buttons erfüllen den Apple/Google-Touch-Target-Standard von 44 px Mindesthöhe.
Inputs verhindern iOS-Auto-Zoom durch korrekte Schriftgröße.

---

## 🚀 Bedienung der Tasten (TTGO Hardware)

| Taste | Funktion |
|---|---|
| **Obere Taste (Links)** | Bei ≥2 erkannten BMS: Auswahlmenü öffnen. Sonst: Toggle Entlade-FET |
| **Mittlere Taste (Mitte)** | Manueller TFT-Refresh. **Beim Booten gehalten**: löscht WLAN/EEPROM und setzt Standard-Werte |
| **Untere Taste (Rechts)** | Schneller Toggle zwischen Akku 1 und Akku 2 (anhand der konfigurierten MAC-Adressen) |

---

## 🛠 Technik-Fakten

* **Plattform**: PlatformIO + Arduino Core für ESP32
* **Hardware**: LilyGO TTGO T4 v1.4 (ESP32, ILI9341 TFT 320×240, SD-Slot über HSPI)
* **TFT**: TFT_eSPI mit Smooth Fonts (NotoSansBold15 / NotoSansBold36)
* **BLE**: ESP32 BLEDevice Stack, JBD-Protokoll für Smart-BMS
* **SD**: HSPI (CLK=14, MISO=2, MOSI=15, CS=13), 4 MHz für Stabilität, **on-demand mount**
* **TFT**: VSPI (separat von SD)
* **WLAN**: WiFi-STA mit NTP-Zeit-Sync, AP-Fallback "BMS-LOGGER"
* **Watchdog**: 50 s WDT verhindert Hänger
* **EEPROM**: 300 Bytes für Konfiguration (WLAN, MACs, Intervall, Datum)
* **CSV-Format**: `Batterie;Datum;Zeit;SOC[%];Uges[V];I[A];Cell1-N[V];balKap[Ah];Temp1[°C];Temp2[°C]`
* **Heap-Schutz**: BLE-Speicher-Cleanup bei BMS-Wechsel, Sprite-Null-Checks im TFT-Code
* **TCP-Stabilität**: Content-Length statt Chunked-Encoding für robuste WLAN-Übertragung

---

## 🐛 Troubleshooting

| Symptom | Ursache | Lösung |
|---|---|---|
| Webseite lädt nicht / Browser-Timeout | Zu wenig Heap | SD-Karte korrekt eingesteckt? Heap-Anzeige beim Booten prüfen (TFT) |
| `errno: 11 / EAGAIN` im Serial Log | TCP-Sendepuffer-Overflow | Sollte nach Refactor weg sein. Wenn doch: weitere Reduktion der HTML-Größe nötig |
| TFT zeigt keine großen Zahlen | Sprite-Allokation fehlgeschlagen | Heap zu fragmentiert. Reboot. Bei Wiederholung: TFT-Sprite-Größen reduzieren |
| Upload-Fehler "Wrong boot mode" | ESP nicht im Download-Modus | BOOT-Taste während "Connecting…" halten |
| Logdatei verschwindet | Geflusht auf SD | Log-Download-Button in der Web-UI nutzen → flusht und lädt SD-Datei |

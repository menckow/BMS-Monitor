# BMS-Logger ESP32 (TTGO)
Modernes, hochperformantes Bluetooth-Auslese-Dashboard für Smart-BMS (Batterie-Management-Systeme wie JBD / LTT Power). 
Das Programm verbindet sich komplett drahtlos per BLE (Bluetooth Low Energy) mit Deinem Batterie-BMS, wertet die Spannungen und Ströme aus und präsentiert diese auf einem modernen TFT-Display (ESP32 TTGO). Gleichzeitig spannt das Modul einen kleinen Webserver für die Konfiguration im WLAN auf.

**Wichtiges Credit:**
Dieses Projekt basiert in seinem Kern auf der hervorragenden Entwicklung von Uwe! 
Ein großer Dank geht daher an [UK1408/BMS-und-ESP32](https://github.com/UK1408/BMS-und-ESP32) für die Grundarchitektur und C++ Daten-Extrahierung der JBD-Protokolle!

## ✨ Kern-Features & Neuheiten (Version 2026 Refactor)

Wir haben dieses Projekt optisch und technisch massiv überholt, um die Stabilität und Nutzererfahrung auf ein extrem professionelles, neues Level zu heben:

* **TFT_eSPI & Anti-Aliasing (Vektor-Schriften)**
  Das komplette User Interface wurde auf die hochperformante `TFT_eSPI`-Bibliothek umgezogen. Das Interface nutzt modernste, kantengeglättete Schriftarten (`NotoSansBold`) anstelle der alten, verpixelten Standard-Pixel-Fonts.
  
* **Flackerfreies Hardware-Rendering (Sprites)**
  Die Live-Aktualisierung der Prozent-Anzeige und der Akku-Leiste flimmert nicht länger. Wir haben C++ internen RAM (`TFT_eSprite`) reserviert. Damit wird die gesamte Batterie-Grafik verdeckt im Prozessor-RAM vorberechnet und inklusive echten Alpha-Kanal Blends gestochen scharf und ohne Bildflackern ("Flicker-Free") auf das Display geworfen!

* **Dynamischer Multi-BMS Live-Wechsel (Ohne Reboot!)**
  Hast du mehr als einen Akku verbaut? Beim Start scannt der Logger automatisch die Umgebung. Werden zwei oder mehr bekannte BMS gefunden, erhält die "Obere Taste" magische Kräfte:
  * Durch Drücken der oberen Hardware-Taste öffnet sich im Live-Betrieb sofort eine kleine Auswahlliste.
  * Du kannst das BMS direkt wechseln und die Daten des zweiten Akkus auslesen – der ESP32 trennt dazu im Hintergrund restlos den Speicher der alten Bluetooth-Verbindung und führt einen komplett nahtlosen "Reconnect" durch.
  * *Kein Neustart des ESP32 nötig. Keine Memory-Leaks.*

* **Intelligentes Top-Bar Layout**
  Die obere Statusleiste passt sich nun Deinen Bedürfnissen an: Das Datum ist dezent linksbündig platziert, was der Mitte des Displays **175 Pixel** Platz freischaufelt. Dadurch werden selbst komplizierte oder extrem lange BMS-Namen im Interface nicht mehr brutal und unleserlich weggeschnitten. Die Spannung Deines Lipo-Systemakkus rechts oben leuchtet nun aggressiv rot, sobald die Zellen kritisch tief (unter 2.8V) fallen!

* **Repariertes WLAN-EEPROM (Web-Settings)**
  Alle Webserver-Einstellungen zum WLAN bleiben jetzt felsenfest gespeichert. Das Bug, bei welchem das Interface überschrieben wurde, weil das Flash-Gedächtnis den Null-Wert `0xFF` nicht korrekt ausgelesen hatte, wurde endgültig ausradiert.

## 🚀 Bedienung der Tasten (TTGO Hardware)
- **Obere Taste (Links):**  Bist du mit >1 BMS gestartet, ruft diese Taste direkt das "BMS Wechseln" Fenster auf. Befindet sich nur 1 einziges BMS in deiner Nähe, ist diese Funktion blockiert und die Taste agiert wie früher als Fallback (Umschaltung des Entlade-FETs).
- **Mittlere Taste (Mitte):** Zwingt den Bildschirm zu einem sofortigen "sauberen" manuellen Refresh. Wird sie **beim Booten** gedrückt gehalten, löscht sie den WLAN/EEPROM-Speicher und initialisiert die Standard-Werte für deine Konfiguration!
- **Untere Taste (Rechts):** Schaltet per Hardware manuell den _Lade-FET_ deines BMS ein oder aus.

## 🛠 Technik-Fakten
- PlatformIO & Arduino Core für ESP32
- Exklusiver Speicherschutz (RAM Leak Prevention) bei BLE Verbindungs-Loops.
- Lokales Logging via SPIFFS als CSV für Historien-Auswertungen.

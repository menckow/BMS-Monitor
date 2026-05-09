/*
  TTGO ESP32 TS V1.4 Bluetooth Wifi Modul mit ILI9341

  Diverse Hinweise
  https://github.com/kolins-cz/Smart-BMS-Bluetooth-ESP32
  https://blog.ja-ke.tech/2020/02/07/ltt-power-bms-chinese-protocol.html
  https://github.com/FurTrader/OverkillSolarBMS/blob/master/JBD%20Protocol%20English%20version.pdf
  https://www.lithiumbatterypcb.com/smart-bms-software-download/
  https://github.com/Xinyuan-LilyGO/LilyGo_Txx#t4

*/
const String Version  = "BMS-Logger, Vers. 27.04.26";                                         // inkl. Fehlermeldungen auf neuer TFT-Seite 23.01.23
const String Build    = "Build " + String(__DATE__) + ",\n" + String(  __TIME__) ;


#include <time.h>                                                  // NTP Zeit
#include <esp_task_wdt.h>                                            // Watchdog
#include <rom/rtc.h>                                                 // ResetGrund
#include <RTClib.h>                                                  // in RTClib ist WIRE.LIB drin
#include <WebServer.h>
#include <FS.h>
#include <TFT_eSPI.h>                                                // TFT_eSPI Hardware Driver
#include "NotoSansBold15.h"                                          // Anti-Aliased Smooth Font für Dashboard
#include "NotoSansBold36.h"                                          // Anti-Aliased Smooth Font für Hauptwerte
#include "BLEDevice.h"
#include "mydatatypes.h"
#include "EEPROM.h"
#include "SPIFFS.h"
#include <analogWrite.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "soc/soc.h"                                                 // Disable brownour problems
#include "soc/rtc_cntl_reg.h"


#define WDT_TIMEOUT 50                                               // 50 Sekunden Watchdog
#define TFT_MISO  12                                                 // NC
#define TFT_CS    27                                                 // CS
#define TFT_DC    32                                                 // DC/RS
#define TFT_MOSI  23                                                 // SD/MOSI
#define TFT_CLK   18                                                 // SCK
#define TFT_RST   05                                                 // Reset

#define Backlight 4                                                  // LED

#define BUTTON_Mitte  37
#define BUTTON_Links  38
#define BUTTON_Rechts 39

#define EEPROM_Size 300 // Erweitert von 200 auf 300 für 2x MAC Adressen

TFT_eSPI tft = TFT_eSPI();

RTC_DS1307 myRTC;
WebServer  server(80);                                               // HTTP-Server auf Port 80

bool     connectet      = true,
         daheim         = false,
         newPacketReceived = false,
         toggle         = true,
         Schleife_ok    = false,
         Fehler_aktuell = false;

const uint8_t  Anzahl_Zellen   = 4;

uint8_t  WLAN_Fehler      = 0,
         FehlerNr,
         Variable_X;

uint16_t oldProtectionStatus = 0;

uint32_t Screensaver      = 0,
         last_Blink,
         last_update,
         last_screen_save = 0,
         nX,
         nY = 0,
         AnzahlMessungen  = 0,
         Anzahl_Zyklen    = 0;

String   sLogFileName = "Log.txt",
         WLAN_SSID   = "",
         WLAN_PASS   = "",
         myPassword  = "",
         sIPAdresse  = "192.168.4.1",
         BMS_MAC_1   = "",
         BMS_MAC_2   = "",
         CSV_Titel   = "",
         Fehler,
         BT_Name,
         Intervall   = "",
         Datum_alt   = "",
         sTemp       = "",
         sDatum_Zeit,
         bootZeit,
         Minute       = "",
         Hinweis      = "Leerlauf",
         sRestZeit    = "",
         ResetGrund   = "";

bool     sdCardActive = false;                                       // Flag für SD-Karte (AKTUELL DEAKTIVIERT)
int      lastLogMinute = -1;                                         // Merker für Intervall-Log
uint32_t webServerLastActive = 0;                                    // Zeitstempel für Webserver-Aktivität

// SPIClass sdSPI(HSPI);                                                // Deaktiviert wegen RAM-Mangel

String   FehlerName[] = {"Zellen Ueberspannung", "Zellen Unterspannung",
                         "Batterie Ueberspannung", "Batterie-Unterspannung",
                         "Laden Uebertemperatur", "Laden Untertemperatur",
                         "Entladen Uebertemperatur", "Entladen Untertemperatur",
                         "Laden Überstrom", "Entladen Ueberstrom",
                         "Kurzschluss", "IC Fehler",
                         "MOS-Software Lock-in", "LadeFET gesperrt",
                         "EntladeFET gesperrt",
                        };

float    RestLadung,
         aktuell_Strom,
         aktuell_Ladezustand,
         aktuell_Spannung,
         RestZeit,
         state_of_charge  = 0,
         Prozent          = 0,
         Voltage          = 0,
         Current          = 0,
         balance_Capacity = 0,
         temp             = 0,
         CellVoltage[Anzahl_Zellen],
         myRSSI = 0;

#define FilterFaktor 0.2                                             // Filter Einstellung BT RSSI

static BLEClient*  mypClient;
static boolean doConnect = false;
static boolean BLE_client_connected = false;
static boolean doScan = false;

packBasicInfoStruct packBasicInfo;                                   //here shall be the latest data got from BMS
packEepromStruct    packEeprom;                                      //here shall be the latest data got from BMS
bool force_battery_redraw = false;
packCellInfoStruct  packCellInfo;                                    //here shall be the latest data got from BMS
packVersionsInfoStruct packVersionsInfo;                             // VersionsInfo

unsigned long previousMillis = 0;
const long interval = 2000;

const byte cBasicInfo3   = 3;                                        //type of packet 3= basic info
const byte cCellInfo4    = 4;                                        //type of packet 4= individual cell info
const byte cVersionsInfo = 5;                                        //type of packet 5= Versions Info
const byte cMOS_FET_SW   = 225;                                      //type of packet 225 = MOSFET umschalten

const uint8_t namelen = 32;                                          // Maximale Länge des Filenamens + 1
typedef char  name_t[namelen];                                       // einen Namen als Typ definieren (macht den Arrayzugriff einfacher)
name_t *sArr;                                                        // Das wird unser dynamisches Array zum Array sortieren


// Funktionen deklarieren --------------------------------------------------------------------
void    setup();
void    loop();
void    yieldWeb(uint32_t ms);
String  FuelleLinks(String Str, uint8_t Laenge);                            // String vorne mit Space ergänzen
void    WLAN_Ein();                                                  // WLAN zur Parametereinabe aktivieren
void    handleRoot();                                                // HTML-Seite senden
void    handleNotFound();                                            // Seite nicht gefunden
String  print_reset_reason(RESET_REASON reason);
String  schreibeDatei(String msg, String path, bool append);
String  getEEprom(int addr);
void    putEEprom(int addr, String data);
void    bleStartup();
void    bleRequestData();
void    get_bms_name();
void    Reset();
void    Fehler_Anzeige();
void    set_mosfet_control(bool dis, bool chg);
void    TFT_Anzeige();
void    mache_HTML_Seite();
// -------------------------------------------------------------------

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  while (!Serial)
  {
    yieldWeb(200);
  };

  esp_task_wdt_init(WDT_TIMEOUT, true);               // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                             // add current thread to WDT watch

  ResetGrund  = "Resetgrund CPU 0: " + print_reset_reason(rtc_get_reset_reason(0));
  ResetGrund += ", CPU 1: " + print_reset_reason(rtc_get_reset_reason(1));

  Serial.println(__FILE__);

  Serial.println(Version);
  Serial.println(Build);
  Serial.println(ResetGrund);

  pinMode(BUTTON_Links,  INPUT_PULLUP);
  pinMode(BUTTON_Mitte,  INPUT_PULLUP);
  pinMode(BUTTON_Rechts, INPUT_PULLUP);

  EEPROM.begin(EEPROM_Size);

  SPIFFS.begin(true);

  /* SD-Karte deaktiviert wegen Webserver-Problemen (RAM)
  sdSPI.begin(14, 2, 15, 13); 
  if (SD.begin(13, sdSPI)) {
    sdCardActive = true;
    Serial.println("TTGO T4 SD-Karte gefunden und aktiv.");
  } else {
    Serial.println("TTGO T4 SD-Karte NICHT gefunden, nutze internen Speicher.");
  }
  */
  sdCardActive = false;

  pinMode(Backlight, OUTPUT);                                        // LED als Output definieren
  analogWrite(Backlight, 200);                                       // Helligkeit vorgeben

  tft.begin();                                                       // initialize a ILI9341 chip
  tft.setRotation(1);
  tft.setTextColor(TFT_YELLOW);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 25);
  tft.println(Version);

  // Werte aus EEPROM Lesen

  Intervall  = getEEprom(0);                                         // ab Speicheradresse 0
  myPassword = getEEprom(10);                                        // ab 10
  WLAN_SSID  = getEEprom(60);                                        // Werte aus EEPROM
  WLAN_PASS  = getEEprom(110);
  Datum_alt  = getEEprom(160);
  BMS_MAC_1  = getEEprom(210);
  BMS_MAC_2  = getEEprom(240);

  // Wenn das EEPROM noch leer/unbeschrieben ist, Standard-Werte setzen
  if (WLAN_SSID.length() == 0 || WLAN_SSID.length() >= 49)
  {
    myPassword = "";                                                 // hier können feste Werte vorgegeben werden
    Intervall  = "10";                                               // alle 10 Minuten speichern
    WLAN_SSID  = "";                                      // eigene WLAN SSID eintragen
    WLAN_PASS  = "";                                  // eigenes WLAN PW eintragen
    Datum_alt  = "01.01.2026";
    
    // Nach einmaligem Erst-Booten fest ins EEPROM schreiben, damit es künftig gelesen werden kann
    putEEprom(0, Intervall);
    putEEprom(10, myPassword);
    putEEprom(60, WLAN_SSID);
    putEEprom(110, WLAN_PASS);
    putEEprom(160, Datum_alt);
    // BMS MACs lassen wir absichtlich auf leer (""), bis sie im Web-UI befüllt werden oder der Auto-Scanner anschlägt
    EEPROM.commit();
  }

  Serial.println("EEPROM loeschen mittlere Taste");

  tft.setCursor(1, 110);
  tft.println("EEPROM loeschen Taste --->");
  tft.setCursor(1, 125);

  uint16_t j = 0;
  while (j <= 51)
  {
    tft.print("_");
    j++;
    yieldWeb(50);
    if (digitalRead(BUTTON_Mitte) == LOW)
    {
      myPassword = "";
      Intervall  = "10";                                             // alle 10 Minuten speichern
      WLAN_SSID  = "";
      WLAN_PASS  = "";
      Datum_alt  = "01.01.2022";
      
      putEEprom(0, Intervall);
      putEEprom(10, myPassword);
      putEEprom(60, WLAN_SSID);
      putEEprom(110, WLAN_PASS);
      putEEprom(160, Datum_alt);
      putEEprom(210, BMS_MAC_1); // Beim Factory-Reset auch die leeren MACs überschreiben
      putEEprom(240, BMS_MAC_2);
      EEPROM.commit();
      
      tft.setCursor(1, 60);
      tft.print("EEPROM geloescht");
      break;
    }
  }
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 25);


  Serial.println("Intervall  " + Intervall);
  Serial.println("AP-PASS    " + myPassword);
  Serial.println("WLAN-SSID  " + WLAN_SSID);
  Serial.println("WLAN-PASS  " + WLAN_PASS);
  Serial.println("altesDatum " + Datum_alt);

  tft.println("Intervall " + Intervall);
  tft.println("AP-PASS   " + myPassword);
  yieldWeb(2000);

  Serial.println("DS1307RTC Read Test");
  Serial.println("-------------------");

  myRTC.begin();

  if (!myRTC.isrunning())
  {
    Serial.println("RTC is NOT running!");
    tft.println();
    tft.println("Uhr laeuft nicht !!");

    // following line sets the RTC to the date & time this sketch was compiled
    myRTC.adjust(DateTime(__DATE__, __TIME__));
    nX = 10;
    while (nX > 0)
    {
      yieldWeb(100);
      nX--;
      Serial.println(nX);
    }
  }
  DateTime now = myRTC.now();
  sDatum_Zeit  = String(now.day(),   DEC) + ".";
  sDatum_Zeit += String(now.month(), DEC) + ".";
  sDatum_Zeit += String(now.year(),  DEC) + ", ";
  sDatum_Zeit += String(now.hour(),  DEC) + ":";
  Variable_X   = now.minute();

  if (Variable_X < 10)
  {
    sDatum_Zeit += "0";
  }
  sDatum_Zeit += String(Variable_X, DEC);
  bootZeit = sDatum_Zeit;

  Serial.println(sDatum_Zeit);
  Serial.println();

  schreibeDatei("Neustart    " + sDatum_Zeit + ", " + ResetGrund + "\n", sLogFileName, true);

  tft.println("\nBMS pruefen");
  bleStartup();

  for (nX = 0; nX <= 5; nX++)                                            // ein paar mal wiederholen
  {
    bleRequestData();                                                      // BMS initial abfragen, ausserhalb der Schleife wegen Print

    Serial.print(5 - nX);
    Serial.print(" ");
    tft.print(5 - nX);
    tft.print(" ");
    yieldWeb(1000);
  }
  tft.println();
  Serial.println();

  WLAN_Ein();

  tft.fillScreen(TFT_BLACK);

  if (BLE_client_connected == true)
  {
    get_bms_name();                                                        // BMS-Name holen
    yieldWeb(100);
  }
  else
  {
    Serial.print("kein Bluetooth-Device, bitte Reset");
    tft.print(" kein BT-Device,\n bitte Reset");
    //    while (true) {}                                                  // Endlosschleife ohne BMS
  }

  CSV_Titel = "Batterie;Datum;Zeit;SOC [%];Uges [V];I [A];";
  for (nX = 1; nX <= Anzahl_Zellen; nX++)
  {
    CSV_Titel = CSV_Titel + "Cell " + String(nX) + " [V];";
  }
  CSV_Titel = CSV_Titel + "bal. Kap. [Ah];";
  for (nX = 1; nX <= 2; nX++)
  {
    CSV_Titel = CSV_Titel + "Temp " + String(nX) + " [C];";
  }
  CSV_Titel = CSV_Titel + "\n";

  tft.fillScreen(TFT_BLACK);
}

//----------------------------------------------------
void loop()
{
  server.handleClient();                                             // WLAN HTTP Abfrage
  yieldWeb(10);
  Variable_X = 0;

  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);

  if (digitalRead(BUTTON_Mitte) == LOW)
  {
    tft.fillScreen(TFT_BLACK);
    tft.loadFont(NotoSansBold15);
    tft.setCursor(0, 40);
  }
  while (digitalRead(BUTTON_Mitte) == LOW)
  {
    Screensaver = 0;
    if (Variable_X == 5)
    {
      tft.print("  ");
      tft.setTextColor(TFT_RED, TFT_BLACK);
    }

    tft.print("*");
    if (Variable_X >= 23)
    {
      tft.unloadFont();
      tft.fillScreen(TFT_BLACK);
      Reset();
    }
    yieldWeb(200);
    Variable_X++;
    Serial.println(Variable_X);
    if (digitalRead(BUTTON_Mitte) != LOW) break;
  }
  if (Variable_X != 0) {
    tft.unloadFont();
    tft.fillScreen(TFT_BLACK);
    force_battery_redraw = true;
  }

  if (Variable_X > 5) {
    Fehler_Anzeige();
    force_battery_redraw = true; // Fehler Anzeige also clears the screen at the end!
  }

  if (digitalRead(BUTTON_Links) == LOW)
  {
    Serial.println("links (obere Taste) gedrueckt");
    extern int foundDevicesCount;
    extern void selectBmsMenu(); // Prototype for the BLE.ino function
    
    if (foundDevicesCount > 1) {
       // --- Neue Funktion: BMS Auswahl Menu ---
       if (BLE_client_connected && mypClient != nullptr) {
           Serial.println("Trenne alte BMS Verbindung...");
           mypClient->disconnect();
           yieldWeb(400); // Dem BLE Stack Zeit geben zum Schliessen
           delete mypClient; // Speicher und Stack-Resourcen restlos freigeben! Verhindert Reboot-Bug beim Re-Connect!
           mypClient = nullptr;
       }
       BLE_client_connected = false;
       
       tft.fillScreen(TFT_BLACK);
       selectBmsMenu();
       
       force_battery_redraw = true;
       tft.fillScreen(TFT_BLACK);
    } else {
       // --- Alte Funktion: Mosfet Steuerung ---
       set_mosfet_control(!bitRead(packBasicInfo.MosfetStatus, 1),
                          bitRead(packBasicInfo.MosfetStatus, 0));      // Setzte MOS_FET ein/aus
    }
    
    yieldWeb(200);
  }

  if (digitalRead(BUTTON_Rechts) == LOW)
  {
    Serial.println("rechts geklickt - Toggle BMS!");
    if (BMS_MAC_1.length() >= 17 && BMS_MAC_2.length() >= 17) {
        extern BLEAdvertisedDevice* myDevice;
        extern BLEAdvertisedDevice* foundDevices[];
        extern int foundDevicesCount;
        
        if (BLE_client_connected && mypClient != nullptr && myDevice != nullptr) {
            String currMac = String(myDevice->getAddress().toString().c_str());
            String targetMac = currMac.equalsIgnoreCase(BMS_MAC_1) ? BMS_MAC_2 : BMS_MAC_1;
            
            Serial.println("Wechsle BMS von " + currMac + " nach " + targetMac);
            
            // Clean disconnect des aktuellen BMS
            mypClient->disconnect();
            yieldWeb(400); 
            delete mypClient;
            mypClient = nullptr;
            BLE_client_connected = false;
            
            // Ziel in den gecachten Geräten suchen
            bool foundToggle = false;
            for (int i = 0; i < foundDevicesCount; i++) {
                if (String(foundDevices[i]->getAddress().toString().c_str()).equalsIgnoreCase(targetMac)) {
                    myDevice = foundDevices[i];
                    doConnect = true;
                    foundToggle = true; 
                    
                    String newBTName = myDevice->toString().c_str();
                    int commaIndex = newBTName.indexOf(",");
                    if (commaIndex > 6) {
                        BT_Name = newBTName.substring(6, commaIndex);
                    } else {
                        BT_Name = myDevice->getName().c_str();
                        if(BT_Name.length() == 0) BT_Name = "BMS-LOGGER";
                    }
                    
                    break;
                }
            }
            
            if (!foundToggle) {
               Serial.println("BMS ausser Reichweite beim Start. Reboote ESP32 fuer frischen Hardware-Scan...");
               ESP.restart(); // Fallback wenn Batterie 2 beim booten nicht angeschaltet war
            } else {
               // Optische Rückmeldung für Benutzer über den Wechsel
               tft.fillScreen(TFT_BLACK);
               tft.loadFont(NotoSansBold15);
               tft.setTextColor(TFT_YELLOW);
               tft.setTextDatum(MC_DATUM);
               tft.drawString("Wechsle Akku...", 120, 100);
               tft.unloadFont();
            }
        }
    } else {
        // Hinweis falls die Einstellungen noch nicht eingetragen / erkannt wurden!
        tft.fillScreen(TFT_RED);
        tft.loadFont(NotoSansBold15);
        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Fehlende Web-Konfig", 120, 80);
        tft.drawString("Bitte erst 2 MACs eintragen!", 120, 110);
        tft.unloadFont();
        yieldWeb(2000);
        tft.fillScreen(TFT_BLACK);
        force_battery_redraw = true;
    }
    
    yieldWeb(200);
  }

  if (millis() - webServerLastActive > 1000) {
    bleRequestData();                                                // BMS abfragen
  } else {
    // Während Webserver-Aktivität keine BLE-Abfragen, um WLAN stabil zu halten
    yieldWeb(10);
  }

  if (last_Blink + 1000 < millis())                                // Hardbeat 1 Sek.
  {
    esp_task_wdt_reset();                                            // Watchdog reset

    last_Blink = millis();
    if (Screensaver <= 5000)                                        // Zähler für Screensaver
    { // irgendwann ist Schluss
      Screensaver++;
    }

    WLAN_Fehler ++;

    if (daheim)                                                    // aktuelles WLAN prüfen
    {
      if (WiFi.status() != WL_CONNECTED)                           // WLAN nicht verbunden
      {
        if (WLAN_Fehler >= 120)                                     // nach 120 Versuchen = 2 Minuten
        {
          Reset();
        }
      }
      else
      {
        WLAN_Fehler = 0;
      }
    }
    else
    {
      if (WLAN_Fehler >= 120)                                       // nach 2 Minuten
      {
        WLAN_Fehler = 0;

        int n  = WiFi.scanNetworks();                              // SSID's suchen
        for (int i = 0; i < n; i++)
        {
          Serial.println(WiFi.SSID(i));

          if (WiFi.SSID(i) == WLAN_SSID)                           // eigene SSID da?
          {
            Serial.println("Netzwerk da!");
            Reset();
            break;
          }
        }
      }
    }
  }

  DateTime now = myRTC.now();
  sDatum_Zeit  = String(now.day(),   DEC) + ".";
  sDatum_Zeit += String(now.month(), DEC) + ".";
  sDatum_Zeit += String(now.year(),  DEC) + ", ";
  sDatum_Zeit += String(now.hour(),  DEC) + ":";
  Variable_X   = now.minute();
  if (Variable_X < 10)
  {
    sDatum_Zeit += "0";
  }
  sDatum_Zeit += String(Variable_X, DEC);

  if (newPacketReceived)
  {
      aktuell_Ladezustand = packBasicInfo.CapacityRemainPercent;
      aktuell_Spannung    = packBasicInfo.Volts / 1000.0;
      aktuell_Strom       = packBasicInfo.Amps / 1000.0;
      RestLadung         = packBasicInfo.CapacityRemainAh / 1000.0;  // Restl. Kapazität
      balance_Capacity   = packBasicInfo.CapacityNominellAh / 1000.0;     // nominelle Kapazität
      Anzahl_Zyklen      = packBasicInfo.CycleCount;

    if (millis() - last_update >= 2000)                              // alle 2 Sekunden Daten holen
    {
      last_update = millis();

      state_of_charge    += aktuell_Ladezustand;
      Voltage            += aktuell_Spannung;
      Current            += aktuell_Strom;

      AnzahlMessungen ++;

      Serial.println();

      Serial.print(AnzahlMessungen);
      Serial.println(". Messung");

      Serial.print("Bluetooth Name       ");
      Serial.println(BT_Name);

      static uint32_t lastRSSI = 0;
      if (millis() - lastRSSI > 10000 || myRSSI == 0) {
          myRSSI = mypClient->getRssi() * FilterFaktor + myRSSI * (1 - FilterFaktor);
          lastRSSI = millis();
      }
      Serial.print("Bluetooth RSSI        ");
      Serial.println(myRSSI, 0);

      Serial.print("BMS Name              ");
      Serial.println(packVersionsInfo.sBMSTyp);

      Serial.print("Version               0x");
      Serial.println(packBasicInfo.BMS_Version, HEX);

      Serial.print("Zellenzahl            ");
      Serial.println(Anzahl_Zellen);

      Serial.print("Ladezustand [%]       ");
      Serial.println(aktuell_Ladezustand, 1);

      Serial.print("Spannung [V]          ");
      Serial.println(aktuell_Spannung, 3);

      Serial.print("Strom [A]             ");
      Serial.println(aktuell_Strom, 2);

      Serial.print("max. Kapazitaet:      ");
      Serial.print(balance_Capacity, 2);                             // nominelle. Kapazität
      Serial.println(" Ah");

      Serial.print("Verfuegb. Kapazitaet: ");
      Serial.print(RestLadung, 2);                                   // aktuelle Kapazität
      Serial.println(" Ah");

      if (aktuell_Strom < 0)                                         // entladen Restzeit
      {
        Serial.print("noch Entladen:        ");
        Serial.print(RestLadung / aktuell_Strom);
        Serial.println(" Std");
      }
      else if (aktuell_Strom > 0)                                    // laden Restzeit
      {
        Serial.print("noch laden:           ");
        float nachladen = RestLadung - balance_Capacity;
        Serial.print(nachladen / aktuell_Strom);
        Serial.println(" Std");
      }
      else
      {
        Serial.println("kein laden/entladen");
      }

      Serial.println("Zellenspannungen & Balancing: ");
      for (uint8_t i = 0; i < Anzahl_Zellen; i++)
      {
        temp = packCellInfo.CellVolt[i] / 1000.0;                    // Returns the cell voltage, in volts
        Serial.print("                      ");
        Serial.print(temp, 3);
        Serial.print(" V  ");
        Serial.println(bitRead(packBasicInfo.BalanceCodeLow, i) ? "(balancing)" : "");
        CellVoltage[i] +=  temp;                                     // Durchschnittswert für CSV
      }

      Serial.printf("Max cell volt:        %.3f V\n", (float)packCellInfo.CellMax / 1000);
      Serial.printf("Min cell volt:        %.3f V\n", (float)packCellInfo.CellMin / 1000);
      Serial.printf("Difference cell volt: %.3f V\n", (float)packCellInfo.CellDiff / 1000);

      Serial.print("Lade/Enlade-Zyklen    ");
      Serial.print(Anzahl_Zyklen);
      Serial.println(" mal");

      Serial.print("Temperatur: ");
      Serial.print(packBasicInfo.Temp1 / 10.0, 1);
      Serial.print(", ");
      Serial.print(packBasicInfo.Temp2 / 10.0, 1);
      Serial.println(" C");

      if (bitRead(packBasicInfo.MosfetStatus, 0) == 1)
      {
        Serial.println("Lade    FET           ein ");
      }
      else
      {
        Serial.println("Lade    FET           aus ");
      }

      if (bitRead(packBasicInfo.MosfetStatus, 1) == 1)
      {
        Serial.println("Entlade FET           ein");
      }
      else
      {
        Serial.println("Entlade FET           aus");
      }

      if (!packBasicInfo.ProtectionStatus == 0)
      {
        //Fehler_Anzeige();
        Serial.println("Fehlerstatus");
        if (bitRead(packBasicInfo.ProtectionStatus, 0) == 1) Serial.println("           Zellen-Überspannung");
        if (bitRead(packBasicInfo.ProtectionStatus, 1) == 1) Serial.println("           Zellen-Unterspannung");
        if (bitRead(packBasicInfo.ProtectionStatus, 2) == 1) Serial.println("           Batterie-Überspannung");
        if (bitRead(packBasicInfo.ProtectionStatus, 3) == 1) Serial.println("           Batterie-Unterspannung");

        if (bitRead(packBasicInfo.ProtectionStatus, 4) == 1) Serial.println("           Laden Übertemperatur");
        if (bitRead(packBasicInfo.ProtectionStatus, 5) == 1) Serial.println("           Laden Untertemperatur");
        if (bitRead(packBasicInfo.ProtectionStatus, 6) == 1) Serial.println("           Entladen Übertemperatur");
        if (bitRead(packBasicInfo.ProtectionStatus, 7) == 1) Serial.println("           Entladen Untertemperatur");
        if (bitRead(packBasicInfo.ProtectionStatus, 8) == 1) Serial.println("           Laden Überstrom");

        if (bitRead(packBasicInfo.ProtectionStatus, 9) == 1) Serial.println("           Entladen Überstrom");
        if (bitRead(packBasicInfo.ProtectionStatus, 10) == 1) Serial.println("          Kurzschluss");
        if (bitRead(packBasicInfo.ProtectionStatus, 11) == 1) Serial.println("          IC Fehler");
        if (bitRead(packBasicInfo.ProtectionStatus, 12) == 1) Serial.println("          MOS-Software Lock-in");
      }

      //***************************************************
      TFT_Anzeige();                                                    // alles auf dem TFT ausgeben
      //***************************************************
      mache_HTML_Seite();                                              // HTML-Indexseite aufbereiten
      //***************************************************
      // CSV-Zeile aufbauen
      now = myRTC.now();

      if (oldProtectionStatus != packBasicInfo.ProtectionStatus)        // Änderung Fehlerstatus
      {
        FehlerNr = 0;
        Fehler   = "";

        while (FehlerNr <= 15)                                          // alle Bits durchsehen
        {
          if ((bitRead(packBasicInfo.ProtectionStatus, FehlerNr)) >
              (bitRead(oldProtectionStatus, FehlerNr)))                 // neuer Fehler
          {
            Fehler = Fehler + (FehlerName[FehlerNr]) + ", ";            // FehlerName erstellen
          }
          else if ((bitRead(packBasicInfo.ProtectionStatus, FehlerNr)) <
                   (bitRead(oldProtectionStatus, FehlerNr)))            // Fehler weg
          {
            Fehler = Fehler + (FehlerName[FehlerNr]) + " weg, ";            // FehlerName erstellen
          }
          else
          {
            //                                                          Serial.println("keine Änderung");
          }
          FehlerNr++;
        }

        oldProtectionStatus = packBasicInfo.ProtectionStatus;          // merken

        Fehler_aktuell = true;
      }

      if (AnzahlMessungen >= 5)                                       // mindestens 5 Messungen
      {
        if (Fehler_aktuell || (now.minute() % Intervall.toInt() == 0  // wenn Fehler
                               && now.minute() != lastLogMinute      // noch nicht in dieser Minute geloggt
                               && now.second() > 10                  // 10 bis 20 Sek. nach Intervallablauf
                               && now.second() < 20))
        {
          if (!Fehler_aktuell) lastLogMinute = now.minute();         // Minute merken (aber nur bei regulärem Log)

          sTemp = String(state_of_charge / AnzahlMessungen, 0) + ";";
          state_of_charge = 0;

          if (Fehler_aktuell)
          {
            sTemp += String(packBasicInfo.Volts / 1000.0, 3) + ";";
          }
          else
          {
            sTemp += String(Voltage / AnzahlMessungen, 3) + ";";
          }
          Voltage = 0;

          sTemp += String(Current / AnzahlMessungen, 1) + ";";
          Current = 0;

          for (uint8_t i = 0; i < Anzahl_Zellen; i++)
          {
            sTemp += String(CellVoltage[i] / AnzahlMessungen, 3) + ";";
            CellVoltage[i] = 0;
          }

          sTemp += String(RestLadung) + ";";
          sTemp += String(packBasicInfo.Temp1 / 10.0, 1);
          sTemp += ";";
          sTemp += String(packBasicInfo.Temp2 / 10.0, 1);
          sTemp += ";";

          if (Fehler_aktuell)
          {
            sTemp += Fehler + ";";
          }

          sTemp.replace('.', ',');                                    // in CSV , statt .

          if (!SPIFFS.exists("/BMS_LOG.CSV"))                         // Datei nicht vorhanden
          {
            Serial.println("neue Logdatei BMS_LOG.CSV");
            schreibeDatei(CSV_Titel, "BMS_LOG.CSV", false);           // neue Datei
            Datum_alt = "ALT";
          }

          now = myRTC.now();
          String sTag  = String(now.day(), DEC) + ".";
          sTag += String(now.month(), DEC) + ".";
          sTag += String(now.year(), DEC) + ";";

          if (Datum_alt != sTag)                                      // Datumswechsel für EEPROM-Backup
          {
            Datum_alt = sTag;
            putEEprom(160, sTag);
            EEPROM.commit();
          }

          sDatum_Zeit = BT_Name + ";" + sTag;
          EEPROM.commit();                                            // EEPROM abschliessen

          sDatum_Zeit += String(now.hour(), DEC) + ":";
          Variable_X   = now.minute();
          if (Variable_X < 10)
          {
            sDatum_Zeit += "0";
          }
          sDatum_Zeit += String(Variable_X, DEC);

          sDatum_Zeit += ";" + sTemp + "\n";
          Serial.println("Speichern in SPIFFS ");
          Serial.println(sDatum_Zeit);

          AnzahlMessungen = 0;                                        // wieder neu beginnen
          schreibeDatei(sDatum_Zeit, "/BMS_LOG.CSV", true);
          Fehler_aktuell = false;
        }
      }
    }
  }
  else
  {

    if (!BLE_client_connected)                                       // keine BT-Verbindung
    {
      Serial.println("Bluetooth - Fehler");
      TFT_Anzeige();
      mache_HTML_Seite();                                           // HTML-Indexseite aufbereiten
    }

    //  tft.setCursor(0, 48);
    //  tft.fillRect(0, 40, 128, 38, ST7735_BLUE);
    //  tft.print("    kein BT-Device,\n     bitte Reset");

    yieldWeb(1000);
  }
}

// --------------------------------------------------------------------
void putEEprom(int adresse, String Str)                               // String an Adresse ins EEPROM
// scheibt was ins eeprom
{
  uint8_t x = Str.length(),
          i;
  for (i = 0; i <= x; i++)
  {
    EEPROM.put(adresse + i, Str.charAt(i));
  }
  EEPROM.put(adresse + i, "\0");                                      // NUL - Zeichen als Abschluss
}
//-----------------------------------------------------------
String getEEprom(int adresse)                                         // Holt Daten ab der Adresse aus dem EEPROM
// liest was aus dem EEPROM
{
  uint8_t value;
  String out = "";
  for (int i = 0; i < 50; i++)
  {
    value = EEPROM.read(adresse + i);                                 // EEProm ab Adresse 0 lesen
    if (value != 0 && value != 255)
    {
      out = out + (char) value;
    }
    else
    {
      return (out);
    }
  }
  return (out);
}

// --------------------------------------------------------------------
String FuelleLinks(String Str, uint8_t Laenge)                               // String vorne mit Space ergänzen
{ //                                                                  // formatiert auf Laenge
  String  S = "";
  uint8_t x = Str.length();                                           // Länge der Zahl

  for ( uint8_t i = x; i < Laenge; i++)
  {
    S = S + " ";
  }
  S += Str;
  return (S);                                                         // String mit Leerzeichen zurück
}
//----------------------------------------------------
void yieldWeb(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    delay(1);
    esp_task_wdt_reset();
  }
}

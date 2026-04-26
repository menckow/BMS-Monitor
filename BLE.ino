//  ----- BLE stuff -----
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
BLERemoteService *pRemoteService;
// The remote service we wish to connect to. Needs check/change when other BLE module used.
static BLEUUID serviceUUID("0000ff00-0000-1000-8000-00805f9b34fb");             //xiaoxiang bms original module
static BLEUUID charUUID_tx("0000ff02-0000-1000-8000-00805f9b34fb");             //xiaoxiang bms original module
static BLEUUID charUUID_rx("0000ff01-0000-1000-8000-00805f9b34fb");             //xiaoxiang bms original module

// 0000ff01-0000-1000-8000-00805f9b34fb
// NOTIFY, READ
// Notifications from this characteristic is received data from BMS

// 0000ff02-0000-1000-8000-00805f9b34fb
// Write this characteristic to send data to BMS
// READ, WRITE, WRITE NO RESPONSE

#define MAX_BMS_DEVICES 8
BLEAdvertisedDevice* foundDevices[MAX_BMS_DEVICES];
int foundDevicesCount = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{ // this is called by some underlying magic
    /**
      Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      
      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
      {
        bool exists = false;
        for (int i = 0; i < foundDevicesCount; i++) {
            if (foundDevices[i]->getAddress().equals(advertisedDevice.getAddress())) {
                exists = true;
                break;
            }
        }
        if (!exists && foundDevicesCount < MAX_BMS_DEVICES) {
            foundDevices[foundDevicesCount] = new BLEAdvertisedDevice(advertisedDevice);
            foundDevicesCount++;
        }
      }    // Found our server
    }      // onResult
};         // MyAdvertisedDeviceCallbacks

class MyClientCallback : public BLEClientCallbacks
{ //this is called on connect / disconnect by some underlying magic+
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
      BLE_client_connected = false;
      Serial.println("onDisconnect");
      lcdDisconnect();
    }
};
//----------------------------------------------------
void bleRequestData()
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
      lcdConnected();
      delay(400); // Dem Nutzer kurz Zeit geben, die Gruene Erfolgsmeldung zu sehen
      tft.fillScreen(TFT_BLACK); 
      extern bool force_battery_redraw;
      force_battery_redraw = true;
    }
    else
    {
      lcdConnectionFailed();
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (BLE_client_connected)
  {
    if (millis() - previousMillis >= interval) //every time period
    {
      previousMillis = millis();

      if (toggle)                                                    //  alternate info3 and info4
      {
        bmsGetInfo3();
      }
      else
      {
        bmsGetInfo4();
      }
      newPacketReceived = false; // Reset ONLY when sending a new request
      toggle = !toggle;
    }
  }
  else if (doScan)
  {
    BLEDevice::getScan()->start(0);                                  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
}

void selectBmsMenu() {
  int selectedIndex = 0;
  unsigned long menuStartTime = millis();
  bool confirmed = false;
  int lastSecondsRemain = -1;
  int lastSelectedIndex = -1;

  tft.fillScreen(TFT_BLACK);
  tft.loadFont(NotoSansBold36);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(String(foundDevicesCount) + " BMS", 5, 5);
  tft.unloadFont();

  while(millis() - menuStartTime < 10000) { // 10s auto-select timeout
    int currentSecondsRemain = 10 - ((millis() - menuStartTime) / 1000);
    
    // Draw countdown and menu items only if changed
    if (currentSecondsRemain != lastSecondsRemain || selectedIndex != lastSelectedIndex) {
      tft.loadFont(NotoSansBold15);
      if (currentSecondsRemain != lastSecondsRemain) {
         tft.setTextColor(TFT_WHITE, TFT_BLACK);
         tft.drawString("Autostart in " + String(currentSecondsRemain) + "s   ", 5, 45); 
         lastSecondsRemain = currentSecondsRemain;
      }
      
      if (selectedIndex != lastSelectedIndex) {
        for (int i = 0; i < foundDevicesCount; i++) {
            int y = 75 + (i * 25);
            if (i == selectedIndex) {
                tft.fillRect(0, y, 320, 25, TFT_GREEN);
                tft.setTextColor(TFT_BLACK, TFT_GREEN);
            } else {
                tft.fillRect(0, y, 320, 25, TFT_BLACK);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            
            String devName = foundDevices[i]->getName().c_str();
            if (devName.length() == 0) devName = "BMS-LOGGER";
            String devMac = foundDevices[i]->getAddress().toString().c_str();

            tft.drawString(devName + " (" + devMac.substring(9) + ")", 5, y + 2); // MAC kurz fassen 
        }
        lastSelectedIndex = selectedIndex;
      }
      tft.unloadFont();
    }

    // Input loop für ~50ms
    unsigned long waitStart = millis();
    while (millis() - waitStart < 500) { 
       if (digitalRead(BUTTON_Links) == LOW) { // Hoch
           selectedIndex--;
           if (selectedIndex < 0) selectedIndex = foundDevicesCount - 1;
           menuStartTime = millis(); 
           delay(250); 
           break;
       }
       if (digitalRead(BUTTON_Rechts) == LOW) { // Runter
           selectedIndex++;
           if (selectedIndex >= foundDevicesCount) selectedIndex = 0;
           menuStartTime = millis(); 
           delay(250); 
           break;
       }
       if (digitalRead(BUTTON_Mitte) == LOW) { // OK
           confirmed = true;
           delay(250);
           break;
       }
       esp_task_wdt_reset(); 
       delay(20);
    }
    if (confirmed) break;
  }

  myDevice = foundDevices[selectedIndex];
  doConnect = true;
  doScan = true;
  
  if (myDevice != nullptr) {
      String BTName = myDevice->toString().c_str();
      int commaIndex = BTName.indexOf(",");
      if (commaIndex > 6) {
          BT_Name = BTName.substring(6, commaIndex);
      } else {
          BT_Name = myDevice->getName().c_str();
          if(BT_Name.length() == 0) BT_Name = "BMS-LOGGER";
      }
  }
}

//----------------------------------------------------
void bleStartup()
{
  foundDevicesCount = 0;
  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  
  // Scanne volle 5 Sekunden blockierend, ohne Cache:
  pBLEScan->start(5, false);

  if (foundDevicesCount == 1) {
     myDevice = foundDevices[0];
     doConnect = true;
     doScan = true;
  } else if (foundDevicesCount > 1) {
     selectBmsMenu();
  }

  // Korrekten BT_Name extrahieren fuer Globale Variablen
  if (myDevice != nullptr) {
      String BTName = myDevice->toString().c_str();
      int commaIndex = BTName.indexOf(",");
      if (commaIndex > 6) {
          BT_Name = BTName.substring(6, commaIndex);
      } else {
          BT_Name = myDevice->getName().c_str();
          if(BT_Name.length() == 0) BT_Name = "BMS-LOGGER";
      }
  }
}
//----------------------------------------------------
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                           uint8_t *pData, size_t length, bool isNotify) //this is called when BLE server sents data via notofication
{
  bleCollectPacket((char *)pData, length);
}
//----------------------------------------------------
static MyClientCallback GlobalClientCallback;

bool connectToServer()
{
  tft.fillScreen(TFT_BLACK); // Fix: Vor dem ersten Status-Print den Bildschirm loeschen
  Serial.print("Forming a connection to ");
  lcdConnectingStatus(0);
  Serial.println(myDevice->getAddress().toString().c_str());
  if (mypClient == nullptr) {
      BLEClient *pClient = BLEDevice::createClient();
      pClient->setClientCallbacks(&GlobalClientCallback);
      mypClient = pClient;
      Serial.println(" - Created NEW client");
  } else {
      Serial.println(" - Reusing existing client");
  }
  
  BLEClient *pClient = mypClient;
  lcdConnectingStatus(1);

  // Connect to the remove BLE Server.
  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  lcdConnectingStatus(2);
  // Obtain a reference to the service we are after in the remote BLE server.
  //BLERemoteService*

  pRemoteService = pClient->getService(serviceUUID);

  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    lcdConnectingStatus(3);
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    tft.fillScreen(TFT_BLACK); // Clean up GUI on failure
    extern bool force_battery_redraw;
    force_battery_redraw = true;
    return false;
  }
  Serial.println(" - Found our service");
  lcdConnectingStatus(4);

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID_rx);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    lcdConnectingStatus(5);
    Serial.println(charUUID_rx.toString().c_str());
    pClient->disconnect();
    delay(400);
    tft.fillScreen(TFT_BLACK); // Clean up GUI on failure
    extern bool force_battery_redraw;
    force_battery_redraw = true;
    return false;
  }
  Serial.println(" - Found our characteristic");
  lcdConnectingStatus(6);
  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead())
  {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  BLE_client_connected = true;
  return true;
}
//----------------------------------------------------
void sendCommand(uint8_t *data, uint32_t dataLen)
{

  /*
      for (uint8_t i = 0; i < dataLen; i++)                           //debug function
      {
        Serial.print(data[i], HEX);
        Serial.print(", ");
      }
      Serial.println();
  */

  if (BLE_client_connected)                                         // BT verbunden
  {

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID_tx);
    if (pRemoteCharacteristic)
    {
      pRemoteCharacteristic->writeValue(data, dataLen);
    }
    else
    {
      Serial.println("Remote TX characteristic not found");
    }
  }
}
//----------------------------------------------------

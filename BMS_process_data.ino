bool isPacketValid(byte *packet)                                     //check if packet is valid
{
  if (packet == nullptr)
  {
    return false;
  }

  bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
  int checksumLen = pHeader->dataLen + 2; // status + data len + data

  if (pHeader->start != 0xDD)
  {
    return false;
  }

  int offset = 2; // header 0xDD and command type are skipped

  byte checksum = 0;
  for (int i = 0; i < checksumLen; i++)
  {
    checksum += packet[offset + i];
  }

  //printf("checksum: %x\n", checksum);

  checksum = ((checksum ^ 0xFF) + 1) & 0xFF;
  //printf("checksum v2: %x\n", checksum);

  byte rxChecksum = packet[offset + checksumLen + 1];

  if (checksum == rxChecksum)
  {
    //printf("Packet is valid\n");
    return true;
  }
  else
  {
    //printf("Packet is not valid\n");
    //printf("Expected value: %x\n", rxChecksum);
    return false;
  }
}

//----------------------------------------------------
bool processVersionsInfo(packVersionsInfoStruct *output, byte *data, unsigned int dataLen)
{
  output->sBMSTyp = "";
  for (uint8_t i = 0; i < dataLen; i++)                                         // Array mit ASCII-Zeichen
  {
    output->sBMSTyp += char(data[i]);                                           // in String sBMSTyp
  }
  return true;
}

//----------------------------------------------------
bool processBasicInfo(packBasicInfoStruct *output, byte *data, unsigned int dataLen)
{
  // Expected data len

  if (dataLen < 0x1D)
  {
    Serial.print("Achtung, Protokoll weicht ab! dataLen = 0x");
    Serial.println(dataLen, HEX);
    // Erlaube Parse trotzdem, solange wir mind. 23 Bytes haben
    if (dataLen < 0x17) return false; 
  }
  output->Volts = ((uint32_t)two_ints_into16(data[0], data[1])) * 10;          // Resolution 10 mV -> convert to milivolts   eg 4895 > 48950mV
  output->Amps  = ((int32_t)two_ints_into16(data[2], data[3]))  * 10;          // Resolution 10 mA -> convert to miliamps
  output->Watts = output->Volts * output->Amps / 1000000;                      // W
  output->CapacityRemainAh   = ((uint16_t)two_ints_into16(data[4],
                                data[5])) * 10;
  output->CapacityNominellAh = ((uint16_t)two_ints_into16(data[6],
                                data[7])) * 10;
  output->CycleCount = ((uint16_t)two_ints_into16(data[8], data[9]));
  output->BalanceCodeLow   = (two_ints_into16(data[12], data[13]));             // Balancing der ersten Zellen
  output->BalanceCodeHigh  = (two_ints_into16(data[14], data[15]));
  output->ProtectionStatus = (two_ints_into16(data[16], data[17]));

  // MOS-FET Status an den ProtectionStatus anfügen (Bit 13 und 14) by UK

  if (bitRead((byte)data[20], 0) == 0)                              // EntLade FET, 0 ist aus
  {
    bitSet(output->ProtectionStatus, 13);                           // high für Fehler
  }
  else
  {
    bitClear(output->ProtectionStatus, 13);
  }

  if (bitRead((byte)data[20], 1) == 0)                              // Lade FET, 0 ist aus
  {
    bitSet(output->ProtectionStatus, 14);
  }
  else
  {
    bitClear(output->ProtectionStatus, 14);
  }

  output->BMS_Version = data[18];
  output->CapacityRemainPercent = ((uint8_t)data[19]);
  output->CapacityRemainWh = (output->CapacityRemainAh * c_cellNominalVoltage) /
                             1000000 * packCellInfo.NumOfCells;
  output->NumOfNTCs  = ((uint8_t)data[22]);
  output->Temp1   = (((uint16_t)two_ints_into16(data[23], data[24])) - 2731);   // Korrektur mit abs. Nullpunkt
  output->Temp2   = (((uint16_t)two_ints_into16(data[25], data[26])) - 2731);
  output->Temp3   = (((uint16_t)two_ints_into16(data[27], data[28])) - 2731);

  output->MosfetStatus     = ((byte)data[20]);

  return true;
};

//----------------------------------------------------
bool processCellInfo(packCellInfoStruct *output, byte *data, unsigned int dataLen)
{
  uint16_t _cellSum;
  uint16_t _cellMin = 5000;
  uint16_t _cellMax = 0;
  uint16_t _cellAvg;
  uint16_t _cellDiff;

  output->NumOfCells = dataLen / 2;                                                  // Data length * 2 is number of cells !!!!!!

  //go trough individual cells
  for (byte i = 0; i < dataLen / 2; i++)
  {
    output->CellVolt[i] = ((uint16_t)two_ints_into16(data[i * 2], data[i * 2 + 1])); // Resolution 1 mV
    _cellSum += output->CellVolt[i];
    if (output->CellVolt[i] > _cellMax)
    {
      _cellMax = output->CellVolt[i];
    }
    if (output->CellVolt[i] < _cellMin)
    {
      _cellMin = output->CellVolt[i];
    }
  }
  output->CellMin  = _cellMin;
  output->CellMax  = _cellMax;
  output->CellDiff = _cellMax - _cellMin;                                              // Resolution 10 mV -> convert to volts
  output->CellAvg  = _cellSum / output->NumOfCells;

  //----cell median calculation----
  uint16_t n = output->NumOfCells;
  uint16_t i, j;
  uint16_t temp;
  uint16_t x[n];

  for (uint8_t u = 0; u < n; u++)
  {
    x[u] = output->CellVolt[u];
  }

  for (i = 1; i <= n; ++i) //sort data
  {
    for (j = i + 1; j <= n; ++j)
    {
      if (x[i] > x[j])
      {
        temp = x[i];
        x[i] = x[j];
        x[j] = temp;
      }
    }
  }

  if (n % 2 == 0)                                                                                // compute median
  {
    output->CellMedian = (x[n / 2] + x[n / 2 + 1]) / 2;
  }
  else
  {
    output->CellMedian = x[n / 2 + 1];
  }

  return true;
};

//----------------------------------------------------
bool bmsProcessPacket(byte *packet)
{
  bool isValid = isPacketValid(packet);

  if (isValid != true)
  {
    Serial.println("Invalid packer received");
    return false;
  }

  bmsPacketHeaderStruct *pHeader = (bmsPacketHeaderStruct *)packet;
  byte *data = packet + sizeof(bmsPacketHeaderStruct);                                  // TODO Fix this ugly hack
  unsigned int dataLen = pHeader->dataLen;

  bool result = false;

  // |Decision based on pac ket type (info3 or info4)

  switch (pHeader->type)
  {
    case cBasicInfo3:
      {
        // Process basic info
        result = processBasicInfo(&packBasicInfo, data, dataLen);
        newPacketReceived = true;
        break;
      }

    case cCellInfo4:
      {
        result = processCellInfo(&packCellInfo, data, dataLen);
        newPacketReceived = true;
        break;
      }

    case cVersionsInfo:
      {
        result = processVersionsInfo(&packVersionsInfo, data, dataLen);
        newPacketReceived = true;
        break;
      }

    case cMOS_FET_SW:
      {
        result = true;
        newPacketReceived = true;
        break;
      }

    default:
      result = false;
      Serial.printf("Unsupported packet type detected. Type: %d\n", pHeader->type);
  }
  return result;
}
//----------------------------------------------------
bool bleCollectPacket(char *data, uint32_t dataSize)                                   // reconstruct packet from BLE incomming data, called by notifyCallback function
{
  static uint8_t packetstate = 0;                                                      // 0 - empty, 1 - first half of packet received, 2- second half of packet received
  static uint8_t packetbuff[40] = {0x0};
  static uint32_t previousDataSize = 0;
  bool retVal = false;

  /*
    for (uint8_t i = 0; i < dataSize; i++)
    {
      //Serial.print(data[i], HEX);

      Serial.print(char (data[i]));
      Serial.print("");
    }
    Serial.println("");
  */

  if (data[0] == 0xdd && packetstate == 0)                                               // probably got 1st half of packet
  {
    packetstate = 1;
    previousDataSize = dataSize;
    for (uint8_t i = 0; i < dataSize; i++)
    {
      packetbuff[i] = data[i];
    }
    retVal = false;
  }

  if (data[dataSize - 1] == 0x77 && packetstate == 1)                                     // probably got 2nd half of the packet
  {
    packetstate = 2;
    for (uint8_t i = 0; i < dataSize; i++)
    {
      packetbuff[i + previousDataSize] = data[i];
    }
    retVal = false;
  }

  if (packetstate == 2)                                                                   // got full packet
  {
    uint8_t packet[dataSize + previousDataSize];
    memcpy(packet, packetbuff, dataSize + previousDataSize);

    bmsProcessPacket(packet);                                                               // pass pointer to retrieved packet to processing function
    packetstate = 0;
    retVal = true;
  }
  return retVal;
}

//----------------------------------------------------
void bmsGetInfo3()                                                                           // Grundinfo V, Ah, A, Temp
{
  //  DD  A5  03  00  FF  FD  77
  uint8_t data[7] = {0xdd, 0xa5, 0x3, 0x0, 0xff, 0xfd, 0x77};
  sendCommand(data, sizeof(data));
}

//----------------------------------------------------
void bmsGetInfo4()                                                   // lese Zelleninfo U,
{
  //  DD  A5 04 00  FF  FC  77
  uint8_t data[7] = {0xdd, 0xa5, 0x4, 0x0, 0xff, 0xfc, 0x77};
  sendCommand(data, sizeof(data));
}

//----------------------------------------------------
void get_bms_name()                                                  // lese BMS-Name
{
  //  DD A5 05 00 FF FB 77
  uint8_t data[7] = {0xdd, 0xa5, 0x5, 0x0, 0xff, 0xfb, 0x77};
  sendCommand(data, sizeof(data));
}

//----------------------------------------------------
int16_t two_ints_into16(int highbyte, int lowbyte)                   // turns two bytes into a single long integer
{
  int16_t result = (highbyte);
  result <<= 8;                                                      // Left shift 8 bits,
  result = (result | lowbyte);                                       // OR operation, merge the two
  return result;
}

//----------------------------------------------------
void hexDump(const char *data, uint32_t dataSize) //debug function
{
  Serial.println("HEX data:");
  for (int i = 0; i < dataSize; i++)
  {
    Serial.printf("0x%x, ", data[i]);
  }
  Serial.println("");
}

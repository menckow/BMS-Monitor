void set_mosfet_control(bool charge, bool discharge)         // 0 ist jeweils aus
{
#define BMS_REG_CTL_MOSFET 0xE1
#define BMS_STARTBYTE      0xDD
#define BMS_WRITE          0x5A
#define BMS_READ           0xA5
#define BMS_STOPBYTE       0x77

  uint8_t  length     = 2,                                   // Kommando besteht aus 2 Byte,
           data[2],
           myData[9];                                        // gesamter Befehl sind 9 Byte

  uint16_t checksum   = 0;
  data[0] = 0;                                               // erstes immer 00

  if (charge && discharge)   data[1] = 0b00;                 // alle aus
  if (charge && !discharge)  data[1] = 0b01;                 // Laden aus, entladen ein
  if (!charge && discharge)  data[1] = 0b10;                 // Laden ein, entladen aus
  if (!charge && !discharge) data[1] = 0b11;                 // alle ein

  //                                                         // Daten aufbereiten, Checksum berechnen

  myData[0] = BMS_STARTBYTE;                                 // 0xDD
  myData[1] = BMS_WRITE;                                     // 0x5A es wird etwas gesendet

  myData[2] = BMS_REG_CTL_MOSFET;                            // Write the command code (the register address)
  checksum  = BMS_REG_CTL_MOSFET;

  myData[3] = length;                                        
  checksum += length;

  myData[4] = data[0];                                       // 2 Byte
  checksum += data[0];
  myData[5] = data[1];
  checksum += data[1];

  checksum = (uint16_t)((0x10000UL) - (uint32_t)checksum);   // Berechne checksum

  uint8_t checksum_msb = (uint8_t)((checksum >> 8) & 0xFF);  // most Significant Bit
  myData[6] = checksum_msb;

  uint8_t checksum_lsb = (uint8_t)(checksum & 0xFF);         // least Significant Bit
  myData[7] = checksum_lsb;

  myData[8] = BMS_STOPBYTE;                                  // Stop byte, 0x77

  sendCommand(myData, sizeof(myData));                       // weg damit
}

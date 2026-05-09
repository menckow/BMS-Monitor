fs::FS& getFS() {
  if (sdCardActive) return SD;
  return SPIFFS;
}

String schreibeDatei(String Text, String filename, bool Flag)                        // Flag true ist anhängen.
{
  String sFileName = filename;
  File   sFile;

  if (!sdCardActive && (SPIFFS.totalBytes() - SPIFFS.usedBytes() <= 10000))
  {
    Serial.println("SPIFFS voll");
    return ("voll");
  }

  if (!sFileName.startsWith("/")) sFileName = "/" + filename;

  Serial.print(sdCardActive ? "[SD] Schreibe: " : "[SPIFFS] Schreibe: ");
  Serial.println(sFileName);

  if (Flag)                                                                           // an Datei anhängen
  {
    sFile = getFS().open(sFileName, "a");
    if (sFile) {
        sFile.print(Text);
        sFile.close();
    }
  }
  else
  {
    sFile = getFS().open(sFileName, "w");
    if (sFile) {
        sFile.print(Text);
        sFile.close();
    }
  }
  return "ok";
}

//-------------------------------------------------
bool handleFileRead(String path)
{
  if (path.endsWith("/")) path += "index.html";
  path = server.urlDecode(path);
  String pathWithGz = path + ".gz";
  
  if (getFS().exists(pathWithGz) || getFS().exists(path))
  {
    if (getFS().exists(pathWithGz)) path += ".gz";
    
    Serial.print(sdCardActive ? "[SD] Lese: " : "[SPIFFS] Lese: ");
    Serial.println(path);
    
    File file = getFS().open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return (true);
  }
  return (false);
}

//-------------------------------------------------
void Loeschen()
{
  String sFileName = server.arg(0);
  if (!sFileName.startsWith("/")) sFileName = "/" + sFileName;

  Serial.print(sdCardActive ? "[SD] Loesche : " : "[SPIFFS] Loesche : ");
  Serial.println(sFileName);
  
  getFS().remove(sFileName);                                                       // hier wird gelöscht

  Listen();
}

//-------------------------------------------------
void formatSpeicher() {                                                               // Formatiert den Speicher
  if (sdCardActive) {
      Serial.println("[SD] Formatierung nicht unterstützt.");
  } else {
      Serial.println("[SPIFFS] Formatiere Speicher...");
      SPIFFS.format();
  }
  Listen();
}

size_t getFSTotalBytes() {
  if (sdCardActive) return SD.totalBytes();
  return SPIFFS.totalBytes();
}

size_t getFSUsedBytes() {
  if (sdCardActive) return SD.usedBytes();
  return SPIFFS.usedBytes();
}

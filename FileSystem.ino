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

  Serial.print("Loeschen : " + sFileName);
  getFS().remove(sFileName);                                                       // hier wird gelöscht

  Listen();
}

//-------------------------------------------------
void formatSpeicher() {                                                               // Formatiert den Speicher
  if (sdCardActive) {
      Serial.println("SD Karte kann nicht formatiert werden.");
  } else {
      SPIFFS.format();
  }
  Listen();
}

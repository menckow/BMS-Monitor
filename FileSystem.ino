String schreibeDatei(String Text, String filename, bool Flag)                        // Flag true ist anhängen.
{
  String sFileName = filename;
  File   sFile;

  // Serial.println(SPIFFS.usedBytes());
  // Serial.println(SPIFFS.totalBytes());
  // Serial.println(SPIFFS.totalBytes() - fs_info.usedBytes());

  if (SPIFFS.totalBytes() - SPIFFS.usedBytes() <= 10000)
  {
    Serial.println("voll");
    return ("voll");
  }

  if (!sFileName.startsWith("/")) sFileName = "/" + filename;

  if (Flag)                                                                           // an Datei anhängen
  {
    sFile = SPIFFS.open(sFileName, "a");
    sFile.print(Text);
    sFile.close();
  }
  else
  {
    sFile = SPIFFS.open(sFileName, "w");
    sFile.print(Text);
    sFile.close();
  }
  return "ok";
}
//-------------------------------------------------
bool handleFileRead(String path)
{
  if (path.endsWith("/")) path += "index.html";
  path = server.urlDecode(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {
    if (SPIFFS.exists(pathWithGz)) path += ".gz";
    File file = SPIFFS.open(path, "r");
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
  SPIFFS.remove(sFileName);                                                       // hier wird gelöscht

  Listen();
}
//-------------------------------------------------
void formatSpeicher() {                                                               // Formatiert den Speicher
  SPIFFS.format();
  Listen();
}
//-------------------------------------------------

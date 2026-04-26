f = "TFTAnzeige.ino"
with open(f, "r") as file:
    content = file.read()
content = content.replace("tft.setFreeFont(&FreeSansBold12pt7b)", "tft.loadFont(NotoSansBold15)")
content = content.replace("tft.setFreeFont(&FreeSansBold9pt7b)", "tft.loadFont(NotoSansBold15)")
with open(f, "w") as file:
    file.write(content)

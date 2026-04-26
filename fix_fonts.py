f = "WLAN.ino"
with open(f, "r") as file:
    content = file.read()
content = content.replace("tft.setFreeFont(&FreeSans9pt7b)", "tft.loadFont(NotoSansBold15)")
with open(f, "w") as file:
    file.write(content)

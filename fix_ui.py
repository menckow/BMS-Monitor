f = "TFTAnzeige.ino"
with open(f, "r") as file:
    content = file.read()
    
# 1. Fix battery percent font and centering
content = content.replace('tft.loadFont(NotoSansBold15);', 'tft.setFreeFont(&FreeSansBold9pt7b); // Crisp vector font for transparent rendering')
content = content.replace('tft.setTextColor((fillHeight > 90) ? TFT_BLACK : TFT_WHITE);', 'tft.setTextColor((fillHeight > 90) ? TFT_BLACK : TFT_WHITE);')
content = content.replace('tft.drawString(String(percent) + "%", x + 27, y + 90);', 'tft.drawString(String(percent) + "%", x + 27, y + 95);')
# wait, wait! tft.loadFont(NotoSansBold15) is used EVERYWHERE in TFTAnzeige.ino! I can't just replace it globally!

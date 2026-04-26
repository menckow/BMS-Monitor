f = "TFTAnzeige.ino"
with open(f, "r") as file:
    content = file.read()
content = content.replace("force_battery_redraw = false; // Endgültiger Reset am Ende der UI-Routine!", "force_battery_redraw = false; // Endgültiger Reset am Ende der UI-Routine!\n    tft.unloadFont();")
with open(f, "w") as file:
    file.write(content)

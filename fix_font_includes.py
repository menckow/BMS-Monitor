f = "BMS_Auslesen_TTGO_Version_230123.ino"
with open(f, "r") as file:
    content = file.read()
content = content.replace("#include <Fonts/", "#include <Fonts/GFXFF/")
with open(f, "w") as file:
    file.write(content)

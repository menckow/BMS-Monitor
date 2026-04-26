import glob
for f in glob.glob("*.ino"):
    with open(f, "r") as file:
        content = file.read()
    content = content.replace("ILI9341_", "TFT_")
    with open(f, "w") as file:
        file.write(content)

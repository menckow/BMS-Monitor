import json, glob, os, urllib.parse
history_dir = os.path.expanduser("~/Library/Application Support/Code/User/History/")
recovered = {}
for entries_json in glob.glob(history_dir + "*/entries.json"):
    try:
        with open(entries_json) as f:
            data = json.load(f)
            url = data.get("resource", "")
            if not url.startswith("file://"): continue
            path = urllib.parse.unquote(url[7:])
            if path.startswith("/Users/omenck/Library/CloudStorage/OneDrive-Persönlich/Antigravity/BMS_Auslesen_TTGO_Version_230123/") and path.endswith(".ino"):
                entries = data.get("entries", [])
                if not entries: continue
                # find the most recent entry with actual size > 0
                hash_dir = os.path.dirname(entries_json)
                for entry in reversed(entries): # latest first
                    entry_path = os.path.join(hash_dir, entry["id"])
                    if os.path.getsize(entry_path) > 0:
                        if path not in recovered or entry["timestamp"] > recovered[path]["ts"]:
                            recovered[path] = {"ts": entry["timestamp"], "file": entry_path}
                        break
    except Exception as e:
        pass

for path, info in recovered.items():
    print(f"Recovering {path} from timestamp {info['ts']}")
    with open(info["file"], "rb") as src, open(path, "wb") as dst:
        dst.write(src.read())

print("Done recovering " + str(len(recovered)) + " files.")

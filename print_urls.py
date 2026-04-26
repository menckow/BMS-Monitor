import json, glob, os
history_dir = os.path.expanduser("~/Library/Application Support/Code/User/History/")
for entries_json in glob.glob(history_dir + "*/entries.json")[:50]:
    with open(entries_json) as f:
        print(json.load(f).get("resource", ""))

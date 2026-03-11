import json
import sys

files = sys.argv[1:]
merged = []
for f in files:
    try:
        with open(f, "r") as fd:
            merged.extend(json.load(fd))
    except:
        pass

with open("compile_commands.json", "w") as fd:
    json.dump(merged, fd, indent=2)

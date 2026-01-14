#!/usr/bin/env python3

import sys
import re

map_file = sys.argv[1]
out_file = sys.argv[2]

symbols = []

with open(map_file, "r") as f:
    for line in f:
        # Example lines we want:
        # 0x00101000                kmain
        m = re.match(r"\s*(0x[0-9A-Fa-f]+)\s+(.*?)\s*$", line)
        if m:
            addr = int(m.group(1), 16)
            name = m.group(2)

            # Filter out absolute and non-function symbols
            if name.startswith(".") or name == "":
                continue

            symbols.append((addr, name))

# Sort symbols by address
symbols.sort(key=lambda x: x[0])

with open(out_file, "w") as out:
    out.write("/* Auto-generated symbol table */\n")
    out.write("#include <stdint.h>\n\n")
    out.write("typedef struct { uintptr_t addr; const char* name; } symbol_t;\n\n")
    out.write("symbol_t symbols[] = {\n")

    for addr, name in symbols:
        out.write(f'    {{ 0x{addr:08x}, "{name}" }},\n')

    out.write("};\n\n")
    out.write("unsigned symbol_count = sizeof(symbols)/sizeof(symbols[0]);\n")

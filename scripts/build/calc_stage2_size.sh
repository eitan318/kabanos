#!/bin/bash
# calc_sectors.sh <elf_file> <start_address>
#
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <elf_file> <start_address>" >&2
    exit 1
fi

ELF_FILE=$1
START_ADDR=$2

if [ ! -f "$ELF_FILE" ]; then
    echo "ERROR: File '$ELF_FILE' not found." >&2
    exit 1
fi

# 1. Extract the __end symbol address using nm
END_HEX=$(i686-elf-nm -S "$ELF_FILE" 2>/dev/null | grep ' __end' | awk '{print $1}')

if [ -z "$END_HEX" ]; then
    echo "ERROR: Could not find '__end' symbol in $ELF_FILE" >&2
    exit 1
fi

# 2. Perform math (16# tells bash the input is Hex)
END_VAL=$((16#$END_HEX))
START_VAL=$(($START_ADDR)) # Handles 0x9000 or 36864
ACTUAL_SIZE=$((END_VAL - START_VAL))

# 3. Calculate sectors (round up)
SECTORS=$(( (ACTUAL_SIZE + 511) / 512 ))

# 4. Output ONLY the number so CMake can capture it
echo "$SECTORS"

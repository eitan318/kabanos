#!/bin/bash

# Configuration
SOURCE_DIR="BOOT"
OUTPUT_DIR="BOOT"
CC="gcc"

# Compilation Flags
# -m32: Compile for 32-bit x86
# -ffreestanding: Assert that the standard library may not exist
# -nostdlib / -nostartfiles: Don't link standard startup files or libraries
# -fno-stack-protector: Often necessary for early boot/kernel code
# -no-pie: Disable position-independent executable for static addresses
CFLAGS="-m32 -ffreestanding -nostdlib -nostartfiles -no-pie -fno-stack-protector -O2"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Starting compilation of .c files in $SOURCE_DIR..."

# Loop through all .c files in the BOOT directory
for c_file in "$SOURCE_DIR"/*.c; do
    # Check if files exist to avoid error if directory is empty
    [ -e "$c_file" ] || continue

    # Extract filename without path and extension
    filename=$(basename "$c_file" .c)
    
    echo "Compiling: $filename"

    # Run the compilation
    $CC $CFLAGS "$c_file" -o "$OUTPUT_DIR/$filename.elf"

    if [ $? -eq 0 ]; then
        echo "  [OK] -> $OUTPUT_DIR/$filename.elf"
    else
        echo "  [ERROR] Failed to compile $c_file"
    fi
done

echo "Compilation complete."

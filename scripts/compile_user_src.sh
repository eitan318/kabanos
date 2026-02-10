#!/bin/bash
SRC_DIR="user_src"
DST_DIR="BOOT"

mkdir -p $DST_DIR

for f in "$SRC_DIR"/*.c; do
    fname=$(basename "$f" .c)

    echo "Compiling $fname..."
    
    # -I. adds the current directory to the include path
    gcc -m32 -static -fno-stack-protector -ffreestanding -nostdlib \
        -I"$SRC_DIR/include" "$f" -o "$DST_DIR/$fname.elf"
done

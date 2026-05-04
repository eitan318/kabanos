#!/bin/bash
# i686-myos-ld-user

SYSROOT="${NEWLIB_SYSROOT:-/project/build/sysroot}"

exec i686-myos-gcc -m32 -static -nostdlib \
    -Wl,-Ttext=0x400000 \
    -Wl,--allow-multiple-definition \
    "$SYSROOT/usr/lib/crt0.o" \
    "$@" \
    -L"$SYSROOT/usr/lib" -lc -lnosys

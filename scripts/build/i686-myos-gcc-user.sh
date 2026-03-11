#!/bin/bash

SYSROOT="${NEWLIB_SYSROOT:-/project/build/sysroot}"
GCC_INC=$(i686-myos-gcc -print-file-name=include)
COMMON_FLAGS="-m32 -nostdinc -fno-stack-protector -g -isystem $GCC_INC -I$SYSROOT/usr/include"
LINK_FLAGS="-static -nostdlib -Wl,-Ttext=0x400000 -Wl,--allow-multiple-definition $SYSROOT/usr/lib/crt0.o -L$SYSROOT/usr/lib -lc -lnosys"
if [[ "$*" == *"-c"* ]]; then
    exec i686-myos-gcc $COMMON_FLAGS "$@"
else
    exec i686-myos-gcc $COMMON_FLAGS "$@" $LINK_FLAGS
fi

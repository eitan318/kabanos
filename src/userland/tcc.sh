#!/bin/bash
# bootstrap_tcc_32.sh

MUSL_DIR="$HOME/repos/musl"
TCC_BIN="./tcc.elf"

# 1. Configuration: Driver-neutral config + Arch-specific override
cat <<EOT > config.h
#define TCC_VERSION "0.9.27"
#define TCC_TARGET_LINUX 1
#define TCC_IS_NATIVE 1
#define CONFIG_TCCDIR "/"
EOT

cat <<EOT > fix_arch.h
#ifndef FIX_ARCH_H
#define FIX_ARCH_H
#undef TCC_TARGET_X86_64
#define TCC_TARGET_I386 1
#endif
EOT

cp ./include/tccdefs.h . 2>/dev/null || cp ../../include/tccdefs.h . 2>/dev/null

# 2. Compile Objects (Stealth mode to bypass dispatcher)
$TCC_BIN -c lib/libtcc1.c -o libtcc1.o \
    -include fix_arch.h \
    -nostdinc -I. -I$MUSL_DIR/include -I$MUSL_DIR/obj/include || exit 1

$TCC_BIN -c tcc.c -o tcc_logic.o \
    -DONE_SOURCE=1 \
    -include fix_arch.h \
    -U_WIN32 -U__x86_64__ -UTCC_TARGET_X86_64 \
    -nostdinc -I. \
    -I$MUSL_DIR/arch/i386 -I$MUSL_DIR/arch/generic \
    -I$MUSL_DIR/obj/include -I$MUSL_DIR/include || exit 1

# 3. Final Link (TCC-Only)
# We remove -m32 here. TCC will see 32-bit objects and produce a 32-bit binary
# without realizing it's "cross-compiling," thus avoiding the i386-tcc search.
$TCC_BIN -o tcc_32_musl.elf -static -nostdlib \
    $MUSL_DIR/lib/crt1.o \
    $MUSL_DIR/lib/crti.o \
    tcc_logic.o \
    libtcc1.o \
    $MUSL_DIR/lib/crtn.o \
    -L$MUSL_DIR/lib -lc

# 4. Result and Cleanup
if [ -f tcc_32_musl.elf ]; then
    echo "Successfully built: $(file -b tcc_32_musl.elf)"
    rm -f tcc_logic.o libtcc1.o tccdefs.h fix_arch.h
else
    echo "Link failed."
    exit 1
fi

add file include/fix_arch.h

#ifndef FIX_ARCH_H
#define FIX_ARCH_H
#undef TCC_TARGET_X86_64
#define TCC_TARGET_I386 1
#define TCC_VERSION "0.9.27"
#define CONFIG_TCCDIR ""
#endif

git clone https://repo.or.cz/tinycc.git
cd userland/user_src/tinycc
./configure --cpu=i386 --targetos=linux --with-libgcc=no --disable-static
cd ../../../

add line to config.h include/fix_arch.h
#define CONFIG_TCC_STATIC 1


run:
$(which gcc) -E -P include/tccdefs.h | sed 's/"/\\"/g;s/^/"/;s/$/\\n"/' > tccdefs_.h

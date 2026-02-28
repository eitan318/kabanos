compile tcc with musl
---------------------

~/repos/1001_myos/src/userland/tcc (mob) $ cat <<EOT > config.h
#define TCC_VERSION "0.9.27"
#define TCC_TARGET_X86_64 1
#define TCC_TARGET_LINUX 1
#define TCC_IS_NATIVE 1
EOT
~/repos/1001_myos/src/userland/tcc (mob) $ gcc -o tcc tcc.c \
    -DONE_SOURCE=1 \
    -DTCC_TARGET_X86_64 \
    -DTCC_IS_NATIVE \
    -nostdinc \
    -I. \
    -I$HOME/repos/musl/arch/x86_64 \
    -I$HOME/repos/musl/arch/generic \
    -I$HOME/repos/musl/obj/include \
    -I$HOME/repos/musl/include \
    -I/usr/lib/gcc/x86_64-linux-gnu/11/include \
    -L$HOME/repos/musl/lib \
    -lc -static
In file included from tcc.c:25:
tcc.h:185: warning: "TCC_IS_NATIVE" redefined
  185 | #  define TCC_IS_NATIVE
      |
In file included from tcc.h:26,
                 from tcc.c:25:
config.h:4: note: this is the location of the previous definition
    4 | #define TCC_IS_NATIVE 1
      |
~/repos/1001_myos/src/userland/tcc (mob) $











gcc -m32 -nostdlib -nostdinc -fno-builtin -static \
    -I./lib/include \
    -I./tcc/include \
    lib/*.c lib/stdio/*.c tcc/*.c \
    -o tcc.elf



/home/magshimim/repos/1001_myos/src/userland/user_src/init.c:16: warning: implicit declaration of function 'exit'
[ 92%] TCC Linking init.elf
tcc: error: file '/home/magshimim/repos/1001_myos/src/userland/musl/obj/crt/i386/crt1.o' not found
gmake[2]: *** [src/userland/CMakeFiles/init_target.dir/build.make:75: ../BOOT/init.elf] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:393: src/userland/CMakeFiles/init_target.dir/all] Error 2
gmake: *** [Makefile:91: all] Error 2


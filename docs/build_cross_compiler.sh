#!/bin/bash
set -e 
export ORIGINAL_PATH="$PATH"

# If these are already set by Docker ENV, use them; otherwise, use defaults
export BASE_DIR="${BASE_DIR:-$HOME/myos_freestanding_gcc}"
export PREFIX="${PREFIX:-/opt/cross}"
export DOWNLOADS="$BASE_DIR/downloads"
export TARGET="i686-myos"
export SYSROOT="$BASE_DIR/$TARGET"
export JOBS=$(nproc)

# --- PATCH PATHS ---
# Match the paths we used in the 'COPY' command in Dockerfile
export PROJECT_ROOT="${PROJECT_ROOT:-$HOME/repos/1001_myos/}"
export PATCH_DIR="$PROJECT_ROOT/extern/patches"
export NEWLIB_MYOS_SRC="$PROJECT_ROOT/extern/newlib_myos"


PAIR_OLD="$BASE_DIR/autotools-legacy" # For Newlib
PAIR_NEW="$BASE_DIR/autotools-modern" # For Binutils/GCC

AC_OLD="autoconf-2.65"
AC_NEW="autoconf-2.69"
AM_OLD="automake-1.11"
AM_NEW="automake-1.15.1"

mkdir -p "$BASE_DIR"
mkdir -p "$DOWNLOADS"

build_autotool() {
    local url=$1
    local name=$2
    local inst=$3

    cd "$DOWNLOADS"
    if [ ! -f "$name.tar.gz" ]; then
        wget -nc "$url"
    fi
    tar -xf "$name.tar.gz"
    mkdir -p "build-$name"
    cd "build-$name"
    "$DOWNLOADS/$name/configure" --prefix="$inst"
    make -j$JOBS
    make install
    rm -rf "$DOWNLOADS/build-$name" 
    rm -rf "$DOWNLOADS/$name"
}

patch_other() {
    local url=$1
    local name=$2
    local inst=$3 

    cd "$DOWNLOADS"
    if [ ! -f "$name.tar.gz" ]; then
        wget -nc "$url"
    fi
    rm -rf "$name"
    tar -xf "$name.tar.gz"
    
    cd "$name"
    echo "--- Patching $name ---"
    # Ensure patch exists before applying
    if [ -f "$PATCH_DIR/$name-myos.patch" ]; then
        patch -p1 < "$PATCH_DIR/$name-myos.patch"
    else
        echo "Warning: Patch $PATCH_DIR/$name-myos.patch not found."
    fi

    export PATH="$inst/bin:$ORIGINAL_PATH"
}

prepere_for_configure() {
    local name=$1
    local inst=$2 

    echo "--- Preparing $name for configure ---"
    cd "$DOWNLOADS"
    rm -rf "build-$name"
    mkdir -p "build-$name"
    cd "build-$name"
    export PATH="$inst/bin:$ORIGINAL_PATH"
}

# --- 1. Build Autotools ---

if [ ! -d "$PAIR_OLD" ] || [ -z "$(ls -A "$PAIR_OLD" 2>/dev/null)" ]; then
    mkdir -p "$PAIR_OLD"
    build_autotool "https://ftp.gnu.org/gnu/autoconf/$AC_OLD.tar.gz" "$AC_OLD" "$PAIR_OLD"
    export PATH="$PAIR_OLD/bin:$PATH"
    build_autotool "https://ftp.gnu.org/gnu/automake/$AM_OLD.tar.gz" "$AM_OLD" "$PAIR_OLD"
fi

export PATH="$ORIGINAL_PATH"

if [ ! -d "$PAIR_NEW" ] || [ -z "$(ls -A "$PAIR_NEW" 2>/dev/null)" ]; then
    mkdir -p "$PAIR_NEW"
    build_autotool "https://ftp.gnu.org/gnu/autoconf/$AC_NEW.tar.gz" "$AC_NEW" "$PAIR_NEW"
    export PATH="$PAIR_NEW/bin:$PATH"
    build_autotool "https://ftp.gnu.org/gnu/automake/$AM_NEW.tar.gz" "$AM_NEW" "$PAIR_NEW"
fi

# --- 2. Binutils ---
patch_other "https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz" "binutils-2.39" "$PAIR_NEW"
cd ld
automake
cd "$DOWNLOADS"

# --- 3. GCC ---
patch_other "https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz" "gcc-12.2.0" "$PAIR_NEW"
cd libstdc++-v3
autoconf
cd "$DOWNLOADS"

# --- 4. Newlib & MyOS ---
patch_other "ftp://sourceware.org/pub/newlib/newlib-2.5.0.tar.gz" "newlib-2.5.0" "$PAIR_OLD"


echo "--- Integrating MyOS into Newlib ---"
rm -rf "$DOWNLOADS/newlib-2.5.0/newlib/libc/sys/myos"
cp -r "$NEWLIB_MYOS_SRC" "$DOWNLOADS/newlib-2.5.0/newlib/libc/sys/myos"


# 1. Enter the PHYSICAL copy inside the newlib tree
cd "$DOWNLOADS/newlib-2.5.0/newlib/libc/sys/myos"

# 2. Use the exact tools and include paths from your CMake success
# Use the full path to your legacy autotools to be safe
export PATH="$PAIR_OLD/bin:$ORIGINAL_PATH"

aclocal-1.11 -I ../../.. -I ../../../..
autoconf
automake-1.11 --cygnus --add-missing --copy

# 3. Now the crucial step: tell the parent to update
cd ..
autoconf

echo "Done! Build system regenerated."


prepere_for_configure "binutils-2.39" 
"$DOWNLOADS/binutils-2.39/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-nls --disable-werror
make -j$JOBS
make install

prepere_for_configure "gcc-12.2.0-freestanding" 
"$DOWNLOADS/gcc-12.2.0/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-nls --enable-languages=c,c++ --without-headers --disable-shared --disable-libstdcxx --disable-libmudflap --disable-libssp --disable-libgomp --disable-libquadmath --with-newlib
make -j$JOBS all-gcc all-target-libgcc
make install-gcc install-target-libgcc

prepere_for_configure "newlib-2.5.0" 
"$DOWNLOADS/newlib-2.5.0/configure" --target=$TARGET --prefix="$PREFIX" --disable-newlib-supplied-syscalls --enable-newlib-reent-small --disable-newlib-fvwrite-in-streamio --disable-newlib-fseek-optimization --disable-newlib-wide-orient --enable-newlib-nano-malloc --disable-newlib-unbuf-stream-opt --enable-lite-exit --enable-newlib-global-atexit --disable-nls
make -j$JOBS
make install

prepere_for_configure "gcc-12.2.0-full" 
"$DOWNLOADS/gcc-12.2.0/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --enable-languages=c,c++ --with-newlib --enable-shared --enable-threads=single --disable-nls --disable-libmudflap --disable-libssp --disable-libgomp --disable-libquadmath
make -j$JOBS
make install



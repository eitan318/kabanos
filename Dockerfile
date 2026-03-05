FROM ubuntu:22.04

# Prevent interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# apt-get deps
# -------------
RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    texinfo \
    wget \
    cmake \
    nasm \
    python3 \
    fdisk \
    && rm -rf /var/lib/apt/lists/*

# i686-myos-gcc and friends
# -------------------------
ENV PREFIX="/opt/cross"
ENV TARGET="i686-elf"
ENV PATH="${PREFIX}/bin:${PATH}"
RUN mkdir -p /src

WORKDIR /src

# 1. Build Binutils
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz && \
    tar -xf binutils-2.41.tar.gz && \
    mkdir build-binutils && cd build-binutils && \
    ../binutils-2.41/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror && \
    make -j$(nproc) && \
    make install && \
    cd .. && rm -rf binutils-2.41* build-binutils

# 2. Build GCC (Freestanding/Static)
RUN wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz && \
    tar -xf gcc-13.2.0.tar.gz && \
    mkdir build-gcc && cd build-gcc && \
    ../gcc-13.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers && \
    make -j$(nproc) all-gcc && \
    make -j$(nproc) all-target-libgcc && \
    make install-gcc && \
    make install-target-libgcc && \
    cd .. && rm -rf gcc-13.2.0* build-gcc

# 3. symlink i686-myos-__ ->  i686-elf-__ 
RUN for i in /opt/cross/bin/i686-elf-*; do \
        ln -s "$i" "/opt/cross/bin/$(basename $i | sed 's/i686-elf/i686-myos/')"; \
    done && \
    ln -s /opt/cross/bin/i686-myos-gcc /opt/cross/bin/i686-myos-cc


# Auto tools
# ----------
ENV LEGACY_PATH="/opt/autotools-legacy"
RUN mkdir -p /src/downloads

# 1. Build Autoconf 2.65
WORKDIR /src/downloads
RUN wget -nc https://ftp.gnu.org/gnu/autoconf/autoconf-2.65.tar.gz && \
    tar -xf autoconf-2.65.tar.gz && \
    cd autoconf-2.65 && \
    ./configure --prefix=$LEGACY_PATH && \
    make -j$(nproc) && make install

# 2. Build Automake 1.11
WORKDIR /src/downloads
RUN wget -nc https://ftp.gnu.org/gnu/automake/automake-1.11.tar.gz && \
    tar -xf automake-1.11.tar.gz && \
    cd automake-1.11 && \
    PATH="$LEGACY_PATH/bin:$PATH" ./configure --prefix=$LEGACY_PATH && \
    PATH="$LEGACY_PATH/bin:$PATH" make -j$(nproc) && \
    make install

# 3. Add the legacy autotools to container path
ENV PATH="$LEGACY_PATH/bin:${PATH}"






#---------------------------------------------------------
# Section for quick addings so you dont need to wait alot
# Should be moved up once in a week or so
#---------------------------------------------------------

# Add dosfstools to provide mkfs.fat
RUN apt-get update && apt-get install -y dosfstools mtools patch  && rm -rf /var/lib/apt/lists/*

#---------------------------------------------------------

# FIX: Change directory back to /src before downloading Newlib!
WORKDIR /src

RUN wget https://sourceware.org/pub/newlib/newlib-2.5.0.tar.gz && \
    tar -xf newlib-2.5.0.tar.gz

COPY extern/patches/newlib-2.5.0-myos.patch /tmp/newlib.patch
RUN cd newlib-2.5.0 && patch -p1 < /tmp/newlib.patch

RUN chmod -R 777 /src

WORKDIR /project

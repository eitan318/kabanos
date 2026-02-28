# Define your SYSROOT (Change this to wherever you want your OS files to live)
export SYSROOT=$HOME/myos

mkdir -p build-newlib
cd build-newlib

# The configuration
~/bin/newlib-2.5.0/configure \
    --target=i686-myos \
    --prefix=/usr \
    --with-sysroot=$SYSROOT \
    --enable-newlib-io-long-long \
    --disable-newlib-supplied-syscalls

# Build everything
make all -j$(nproc)

# Install into your SYSROOT
make DESTDIR=$SYSROOT install
#
# cp -af $SYSROOT/usr/i686-myos/* $SYSROOT/usr/
# rm -rf $SYSROOT/usr/i686-myos

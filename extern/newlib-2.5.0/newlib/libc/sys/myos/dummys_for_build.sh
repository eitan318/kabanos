# Store your current location
CURRDIR=$(pwd)

# Go to where your compiler actually lives
cd /usr/local/i386elfgcc/bin/

# Create the symlinks (using sudo if permissions are denied)
sudo ln -s i386-elf-ar i686-myos-ar
sudo ln -s i386-elf-as i686-myos-as
sudo ln -s i386-elf-gcc i686-myos-gcc
sudo ln -s i386-elf-gcc i686-myos-cc
sudo ln -s i386-elf-ranlib i686-myos-ranlib

# Go back
cd $CURRDIR

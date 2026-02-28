cd ~/bin/newlib-2.5.0/newlib/libc/sys/myos

# Run the 1.11 version of aclocal
~/automake-1.11-install/bin/aclocal-1.11 -I ../../.. -I ../../../..

# Run autoconf (the system one is fine)
autoconf

# Run the 1.11 version of automake with the cygnus flag
~/automake-1.11-install/bin/automake-1.11 --cygnus

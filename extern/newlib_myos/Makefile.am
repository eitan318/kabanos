AUTOMAKE_OPTIONS = cygnus
INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)
AM_CCASFLAGS = $(INCLUDES)

# Newlib expects crt0.o to be built but not necessarily part of lib.a
noinst_LIBRARIES = lib.a

if MAY_SUPPLY_SYSCALLS
extra_objs = syscalls.o helpers.o
else
extra_objs =
endif

# 1. Put crt0.c here to satisfy the configure check
lib_a_SOURCES = crt0.c 
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = syscalls.c helpers.c
lib_a_DEPENDENCIES = $(extra_objs)

# 2. Standard Newlib pattern for crt0.o
# We explicitly define the rule to ensure it's built from our source
crt0.o: crt0.c
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c -o $@ $<

all: lib.a crt0.o

ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host

set_header_dir = $(DESTDIR)$(tooldir)/include

install-data-local:
	$(mkinstalldirs) $(set_header_dir)
	$(INSTALL_DATA) $(srcdir)/include/semaphore.h $(set_header_dir)/semaphore.h
	$(INSTALL_DATA) $(srcdir)/include/dirent.h    $(set_header_dir)/dirent.h

uninstall-local:
	rm -f $(set_header_dir)/semaphore.h
	rm -f $(set_header_dir)/dirent.h

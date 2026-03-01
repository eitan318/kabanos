AUTOMAKE_OPTIONS = cygnus
INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)
AM_CCASFLAGS = $(INCLUDES)

noinst_LIBRARIES = lib.a

if MAY_SUPPLY_SYSCALLS
extra_objs = syscalls.o helpers.o
else
extra_objs =
endif

lib_a_SOURCES =
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = syscalls.c crt0.c helpers.c
lib_a_DEPENDENCIES = $(extra_objs)
lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)

# This is what the parent sys/Makefile actually looks for
crt0.o: $(srcdir)/crt0.c
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/crt0.c

all: lib.a crt0.o

ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host


# Define where the headers should go in the final sysroot
# tooldir is usually /usr/i686-myos or similar
set_header_dir = $(DESTDIR)$(tooldir)/include

install-data-local:
	$(mkinstalldirs) $(set_header_dir)
	$(INSTALL_DATA) $(srcdir)/include/semaphore.h $(set_header_dir)/semaphore.h

# Optional: Add a hook to uninstall if needed
uninstall-local:
	rm -f $(set_header_dir)/semaphore.h

AUTOMAKE_OPTIONS = cygnus
INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)
AM_CCASFLAGS = $(INCLUDES)

noinst_LIBRARIES = lib.a

lib_a_SOURCES = syscalls.c
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = crt0.c syscalls.c
lib_a_DEPENDENCIES = $(extra_objs)
lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)

# This is the critical part for crt0.o
extra_objs = crt0.o

all-local: $(extra_objs)

# Explicit rule to ensure crt0.o is built from crt0.c (or crt0.S)
crt0.o: crt0.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@



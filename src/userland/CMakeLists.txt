set(NEWLIB_SRC_DIR "${CMAKE_SOURCE_DIR}/extern/newlib-2.5.0")
set(NEWLIB_SYS_MYOS "${NEWLIB_SRC_DIR}/newlib/libc/sys/myos")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${CMAKE_SOURCE_DIR}/extern/newlib_install")

set(TCC_BIN "${CMAKE_SOURCE_DIR}/extern/tcc/tcc")
set(TCC_DIR "${CMAKE_SOURCE_DIR}/extern/tcc")
set(USER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/BOOT")

# --- 2. Automate Newlib Build & Install ---
include(ExternalProject)

ExternalProject_Add(newlib_project
    SOURCE_DIR "${NEWLIB_SRC_DIR}"
    BINARY_DIR "${NEWLIB_BUILD_DIR}"
    
    CONFIGURE_COMMAND 
        bash -c "cd ${NEWLIB_SYS_MYOS} && \
                 ~/automake-1.11-install/bin/aclocal-1.11 -I ../../.. -I ../../../.. && \
                 autoconf && \
                 ~/automake-1.11-install/bin/automake-1.11 --cygnus"
    COMMAND "${NEWLIB_SRC_DIR}/configure"
                --target=i686-myos
                --prefix=/usr
                --with-sysroot=${NEWLIB_SYSROOT}
                --enable-newlib-io-long-long
    BUILD_COMMAND make all -j${PROCESSOR_COUNT}
    INSTALL_COMMAND make DESTDIR=${NEWLIB_SYSROOT} install
    BUILD_BYPRODUCTS 
        "${NEWLIB_SYSROOT}/usr/lib/libc.a"
        "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
)

ExternalProject_Add_Step(newlib_project flatten
    COMMAND bash -c "cp -af ${NEWLIB_SYSROOT}/usr/i686-myos/* ${NEWLIB_SYSROOT}/usr/ && \
                     rm -rf ${NEWLIB_SYSROOT}/usr/i686-myos"
    DEPENDEES install        # runs after install
    DEPENDERS build          # anything depending on newlib_project waits for this
)

# --- 3. Build User Executables ---
file(GLOB USER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*.c")

foreach(USER_FILE ${USER_SOURCES})
    get_filename_component(USER_NAME ${USER_FILE} NAME_WE)
    set(USER_OBJ "${CMAKE_CURRENT_BINARY_DIR}/user_objs/${USER_NAME}.o")
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    # --- COMPILE STEP ---
    add_custom_command(
        OUTPUT "${USER_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/user_objs"
        COMMAND "${TCC_BIN}" -m32 -c "${USER_FILE}" -o "${USER_OBJ}" 
                -nostdinc 
                "-I${TCC_DIR}/include" 
                "-I${NEWLIB_SYSROOT}/usr/include"
        # IMPORTANT: Depend on the newlib_project install step
        DEPENDS "${USER_FILE}" newlib_project
        COMMENT "TCC compiling ${USER_NAME}.c with Newlib"
        VERBATIM
    )

    # --- LINK STEP ---
    add_custom_command(
        OUTPUT "${USER_ELF}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
        COMMAND "${TCC_BIN}" -m32 -o "${USER_ELF}" -static -nostdlib
                "-Wl,-Ttext=0x400000"
                "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
                "${USER_OBJ}"
                "-L${NEWLIB_SYSROOT}/usr/lib" 
                -lc -lnosys
        DEPENDS "${USER_OBJ}"
        COMMENT "TCC Linking ${USER_NAME}.elf"
        VERBATIM
    ) 
    
    add_custom_target(${USER_NAME}_target ALL DEPENDS ${USER_ELF})
endforeach()

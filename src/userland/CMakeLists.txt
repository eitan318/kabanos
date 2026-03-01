option(REBUILD_NEWLIB "Force rebuild and reinstall newlib" OFF)

set(NEWLIB_SRC_DIR "${CMAKE_SOURCE_DIR}/extern/newlib-2.5.0")
set(NEWLIB_SYS_MYOS "${NEWLIB_SRC_DIR}/newlib/libc/sys/myos")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${CMAKE_SOURCE_DIR}/extern/newlib_install")

set(TCC_BIN "${CMAKE_SOURCE_DIR}/extern/tcc/tcc")
set(TCC_DIR "${CMAKE_SOURCE_DIR}/extern/tcc")
set(USER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/BOOT")

if(REBUILD_NEWLIB)
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
        DEPENDEES install
        DEPENDERS build
    )
    set(NEWLIB_DEP newlib_project)
else()
    # Create a dummy target so DEPENDS newlib_project still works everywhere
    add_custom_target(newlib_project)
    set(NEWLIB_DEP newlib_project)
endif()


file(GLOB USER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*")
foreach(USER_DIR ${USER_DIRS})
    if(NOT IS_DIRECTORY ${USER_DIR})
        continue()
    endif()

    get_filename_component(USER_NAME ${USER_DIR} NAME)
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    if(USER_NAME STREQUAL "tcc")
        file(WRITE "${USER_DIR}/fix_arch.h"
            "#ifndef FIX_ARCH_H\n#define FIX_ARCH_H\n"
            "#undef TCC_TARGET_X86_64\n"
            "#define TCC_TARGET_I386 1\n"
            "#define TCC_VERSION \"0.9.27\"\n"
            "#define CONFIG_TCCDIR \"/\"\n"
            "#endif\n"
        )

        set(TCC_MAIN_OBJ "${CMAKE_CURRENT_BINARY_DIR}/user_objs/tcc/tcc.o")
        set(COMMON_TCC_FLAGS
            -m32 -nostdinc
            -include "${USER_DIR}/fix_arch.h"
            "-I${USER_DIR}"
            "-I${USER_DIR}/include"
            "-I${NEWLIB_SYSROOT}/usr/include"
            -g
        )
        add_custom_command(
            OUTPUT "${TCC_MAIN_OBJ}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/user_objs/tcc"
            COMMAND i686-myos-gcc -c "${USER_DIR}/tcc.c" -o "${TCC_MAIN_OBJ}"
                ${COMMON_TCC_FLAGS}
                -DONE_SOURCE=1
                -DTCC_TARGET_I386
                -DCONFIG_TCC_STATIC
                -DCONFIG_TCCDIR=""
                -DCONFIG_TCC_CRTPREFIX=""
                -DCONFIG_TCC_ELFINTERP=""
                -DCONFIG_TCC_LIBPATHS=""
                -DCONFIG_TCC_SYSINCLUDEPATHS=""
                -U__linux__
                -U__unix__
            DEPENDS "${USER_DIR}/tcc.c" newlib_project
            COMMENT "CC compiling tcc/tcc.c"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${USER_ELF}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
            COMMAND i686-myos-gcc -m32 -g -o "${USER_ELF}" -static -nostdlib
                    "-Wl,-Ttext=0x400000"
                    "-Wl,--allow-multiple-definition"
                    "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
                    "${TCC_MAIN_OBJ}"
                    "-L${NEWLIB_SYSROOT}/usr/lib" -lc -lnosys
            DEPENDS "${TCC_MAIN_OBJ}"
            COMMENT "CC Linking tcc.elf"
            VERBATIM
        )       

    else()
        file(GLOB USER_SOURCES "${USER_DIR}/*.c")
        set(USER_OBJS "")

        foreach(USER_FILE ${USER_SOURCES})
            get_filename_component(SRC_NAME ${USER_FILE} NAME_WE)
            set(USER_OBJ "${CMAKE_CURRENT_BINARY_DIR}/user_objs/${USER_NAME}/${SRC_NAME}.o")
            list(APPEND USER_OBJS "${USER_OBJ}")

            add_custom_command(
                OUTPUT "${USER_OBJ}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/user_objs/${USER_NAME}"
                COMMAND "${TCC_BIN}" -m32 -g -c "${USER_FILE}" -o "${USER_OBJ}"
                        -nostdinc
                        "-I${TCC_DIR}/include"
                        "-I${NEWLIB_SYSROOT}/usr/include"
                DEPENDS "${USER_FILE}" newlib_project
                COMMENT "TCC compiling ${USER_NAME}/${SRC_NAME}.c"
                VERBATIM
            )
        endforeach()

        add_custom_command(
            OUTPUT "${USER_ELF}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
            COMMAND "${TCC_BIN}" -m32 -g -o "${USER_ELF}" -static -nostdlib
                    "-Wl,-Ttext=0x400000"
                    "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
                    ${USER_OBJS}
                    "-L${NEWLIB_SYSROOT}/usr/lib" -lc -lnosys
            DEPENDS ${USER_OBJS}
            COMMENT "TCC Linking ${USER_NAME}.elf"
            VERBATIM
        )
    endif()

    add_custom_target(${USER_NAME}_target ALL DEPENDS "${USER_ELF}")
endforeach()

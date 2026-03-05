include(ExternalProject)

# --- 1. Newlib Setup ---
set(MYOS_STUBS_SRC "${CMAKE_SOURCE_DIR}/extern/newlib_myos")
set(NEWLIB_SRC_DIR "/src/newlib-2.5.0")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${SYSROOT_DIR}")

ExternalProject_Add(newlib_project
    SOURCE_DIR "/src/newlib-2.5.0"
    BINARY_DIR "${NEWLIB_BUILD_DIR}"
    
    # 1. This tells CMake: "I handled the download/source myself, don't verify/touch it"
    DOWNLOAD_COMMAND ""
    
    # 2. Add this to prevent CMake from trying to update/patch the /src dir
    UPDATE_COMMAND ""
    PATCH_COMMAND ""

    CONFIGURE_COMMAND
    bash -c "cmake -E copy_directory ${MYOS_STUBS_SRC} /src/newlib-2.5.0/newlib/libc/sys/myos && \
             cd /src/newlib-2.5.0/newlib/libc/sys && \
             autoconf && \
             cd myos && \
             aclocal-1.11 -I ../../.. -I ../../../.. && \
             autoconf && \
             automake-1.11 --cygnus && \
             cd ${NEWLIB_BUILD_DIR} && \
             /src/newlib-2.5.0/configure \
                --target=i686-myos \
                --prefix=/usr \
                --with-sysroot=${NEWLIB_SYSROOT} \
                --enable-newlib-io-long-long"
    
    BUILD_COMMAND make -C ${NEWLIB_BUILD_DIR} all -j$(nproc)
    INSTALL_COMMAND make -C ${NEWLIB_BUILD_DIR} DESTDIR=${NEWLIB_SYSROOT} install

    STEP_TARGETS flatten
)

ExternalProject_Add_Step(newlib_project flatten
    COMMAND bash -c "cp -af ${NEWLIB_SYSROOT}/usr/i686-myos/* ${NEWLIB_SYSROOT}/usr/ && rm -rf ${NEWLIB_SYSROOT}/usr/i686-myos"
    DEPENDEES install
)

# --- 2. Global Userland Config ---
set(USER_OUTPUT_DIR "${NEWLIB_SYSROOT}/bin")
set(USER_OBJS_BASE "${CMAKE_CURRENT_BINARY_DIR}/user_objs")

execute_process(
    COMMAND i686-myos-gcc -print-file-name=include
    OUTPUT_VARIABLE GCC_INTERNAL_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(COMMON_USER_FLAGS
    -m32 -nostdinc -fno-stack-protector -g
    -isystem "${GCC_INTERNAL_INCLUDE_DIR}"
    "-I${NEWLIB_SYSROOT}/usr/include"
)

# --- 3. User Apps Loop ---
file(GLOB USER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*")

set(USER_TARGET_LIST "")
set(ALL_ELF_FILES "")
set_property(GLOBAL PROPERTY GLOBAL_USER_ELF_FILES "")

foreach(USER_DIR ${USER_DIRS})
    if(NOT IS_DIRECTORY ${USER_DIR})
        continue()
    endif()

    get_filename_component(USER_NAME ${USER_DIR} NAME)
    set(APP_OBJ_DIR "${USER_OBJS_BASE}/${USER_NAME}")
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    # -------------------------------------------------------------------------
    # Per-app source collection and extra flags
    # -------------------------------------------------------------------------

    if(USER_NAME STREQUAL "tcc")
        # TCC uses ONE_SOURCE mode: tcc.c #includes all other .c files itself.
        # We must NOT glob the other .c files or they'll be compiled twice and
        # cause multiply-defined symbol errors.
        set(APP_SOURCES "${USER_DIR}/tcc.c")
        set(EXTRA_APP_FLAGS
            -DONE_SOURCE=1
            -DTCC_TARGET_I386
            -DCONFIG_TCC_STATIC
            -DCONFIG_USE_LIBGCC
            # Disable backtrace/bounds-check which pull in dlfcn.h / pthreads
            -UCONFIG_TCC_BACKTRACE
            -UCONFIG_TCC_BCHECK
            # Disable native-mode check that gates the dlfcn.h include in tcc.h
            -UTCC_IS_NATIVE
            # Runtime paths TCC embeds for finding its own headers/libs on myos
            "-DCONFIG_TCCDIR=\"/usr/lib/tcc\""
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"/usr/include\""
            # Cross-build warnings that would otherwise be fatal
            -Wno-implicit-function-declaration
            -Wno-deprecated-declarations
        )
    else()
        # Generic app: compile all .c/.S files in the top-level app directory.
        # Subdirectories (include/, tests/, examples/ etc.) are excluded.
        file(GLOB APP_SOURCES
            "${USER_DIR}/*.c"
            "${USER_DIR}/*.S"
            "${USER_DIR}/*.s"
        )
        set(EXTRA_APP_FLAGS "")
    endif()

    # -------------------------------------------------------------------------
    # Compile each source to an object file
    # -------------------------------------------------------------------------
    set(APP_OBJECTS "")
    foreach(SRC_FILE ${APP_SOURCES})
        get_filename_component(SRC_NAME ${SRC_FILE} NAME_WE)
        set(OBJ_FILE "${APP_OBJ_DIR}/${SRC_NAME}.o")
        list(APPEND APP_OBJECTS "${OBJ_FILE}")

        add_custom_command(
            OUTPUT "${OBJ_FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_OBJ_DIR}"
            COMMAND i686-myos-gcc
                    ${COMMON_USER_FLAGS}
                    ${EXTRA_APP_FLAGS}
                    "-I${USER_DIR}"
                    "-I${USER_DIR}/include"
                    -c "${SRC_FILE}" -o "${OBJ_FILE}"
            DEPENDS "${SRC_FILE}" newlib_project-flatten
            COMMENT "Compiling ${USER_NAME}: ${SRC_NAME}"
            VERBATIM
        )
    endforeach()

    # -------------------------------------------------------------------------
    # Link
    # -------------------------------------------------------------------------
    add_custom_command(
        OUTPUT "${USER_ELF}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
        COMMAND i686-myos-gcc -m32 -g
                -o "${USER_ELF}"
                -static -nostdlib
                "-Wl,-Ttext=0x400000"
                "-Wl,--allow-multiple-definition"
                "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
                ${APP_OBJECTS}
                "-L${NEWLIB_SYSROOT}/usr/lib" -lc -lnosys
        DEPENDS ${APP_OBJECTS}
        COMMENT "Linking ${USER_NAME}.elf"
        VERBATIM
    )

    add_custom_target(${USER_NAME}_target ALL DEPENDS "${USER_ELF}")
    list(APPEND USER_TARGET_LIST "${USER_NAME}_target")
    list(APPEND ALL_ELF_FILES "${USER_ELF}")
    set_property(GLOBAL APPEND PROPERTY GLOBAL_USER_ELF_FILES "${USER_ELF}")
endforeach()


# --- 4. Userland stamp (cross-scope dependency fix for os_image) ---
# A concrete file that is touched only after every ELF is up to date.
# os_image depends on this file instead of raw ELF paths, which would be
# invisible to CMake across directory scopes causing "no rule to make target".
set(USERLAND_STAMP "${CMAKE_CURRENT_BINARY_DIR}/userland.stamp")

add_custom_command(
    OUTPUT "${USERLAND_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E touch "${USERLAND_STAMP}"
    DEPENDS ${ALL_ELF_FILES}
    COMMENT "All userland ELFs up to date (stamp)"
    VERBATIM
)

add_custom_target(userland_stamp_target ALL
    DEPENDS "${USERLAND_STAMP}"
)
add_dependencies(userland_stamp_target ${USER_TARGET_LIST})

# Export stamp path for the image CMakeLists
set(USERLAND_STAMP_FILE "${USERLAND_STAMP}" CACHE INTERNAL
    "Stamp file touched when all userland ELFs are built")

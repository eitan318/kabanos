include(ExternalProject)

# --- 1. Newlib Setup ---
set(MYOS_STUBS_SRC "${CMAKE_SOURCE_DIR}/extern/newlib_myos")
set(NEWLIB_SRC_DIR "/src/newlib-2.5.0")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${SYSROOT_DIR}")

if(NOT EXISTS "${NEWLIB_SYSROOT}")
    ExternalProject_Add(newlib_project
        SOURCE_DIR "${NEWLIB_SRC_DIR}"
        BINARY_DIR "${NEWLIB_BUILD_DIR}"
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CONFIGURE_COMMAND
        bash -c "cmake -E copy_directory ${MYOS_STUBS_SRC} ${NEWLIB_SRC_DIR}/newlib/libc/sys/myos && \
                 cd ${NEWLIB_SRC_DIR}/newlib/libc/sys && \
                 autoconf && \
                 cd myos && \
                 aclocal-1.11 -I ../../.. -I ../../../.. && \
                 autoconf && \
                 automake-1.11 --cygnus && \
                 cd ${NEWLIB_BUILD_DIR} && \
                 ${NEWLIB_SRC_DIR}/configure \
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
else()
    if(NOT TARGET newlib_project-flatten)
        add_custom_target(newlib_project-flatten)
    endif()
endif()


# --- Helper: add_user_app(USER_DIR) ---
# Compiles and links a single userland app from the given source directory.
function(add_user_app USER_DIR)
    if(NOT IS_DIRECTORY "${USER_DIR}")
        message(WARNING "add_user_app: '${USER_DIR}' is not a directory, skipping.")
        return()
    endif()

    get_filename_component(USER_NAME "${USER_DIR}" NAME)
    set(APP_OBJ_DIR "${USER_OBJS_BASE}/${USER_NAME}")
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    # -------------------------------------------------------------------------
    # Per-app source collection and extra flags
    # -------------------------------------------------------------------------
    if(USER_NAME STREQUAL "tcc")
        set(APP_SOURCES "${USER_DIR}/tcc.c")
        set(EXTRA_APP_FLAGS
            -DONE_SOURCE=1
            -DTCC_TARGET_I386
            -DCONFIG_TCC_STATIC
            -DCONFIG_USE_LIBGCC
            -UCONFIG_TCC_BACKTRACE
            -UCONFIG_TCC_BCHECK
            -UTCC_IS_NATIVE
            "-DCONFIG_TCCDIR=\"/usr/lib/tcc\""
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"/usr/include\""
            -Wno-implicit-function-declaration
            -Wno-deprecated-declarations
        )
    else()
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
        get_filename_component(SRC_NAME "${SRC_FILE}" NAME_WE)
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

    # Propagate results back to the parent scope
    set_property(GLOBAL APPEND PROPERTY GLOBAL_USER_ELF_FILES "${USER_ELF}")
    list(APPEND USER_TARGET_LIST "${USER_NAME}_target")
    set(USER_TARGET_LIST "${USER_TARGET_LIST}" PARENT_SCOPE)
    list(APPEND ALL_ELF_FILES "${USER_ELF}")
    set(ALL_ELF_FILES "${ALL_ELF_FILES}" PARENT_SCOPE)
endfunction()


















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

# --- 3. User Apps ---
set(USER_TARGET_LIST "")
set(ALL_ELF_FILES "")
set_property(GLOBAL PROPERTY GLOBAL_USER_ELF_FILES "")

# Auto-discover all subdirectories...
file(GLOB USER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*")
foreach(USER_DIR ${USER_DIRS})
    add_user_app("${USER_DIR}")
endforeach()


# --- 4. Userland stamp ---
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

add_dependencies(userland_stamp_target newlib_project-flatten)

set(USERLAND_STAMP_FILE "${USERLAND_STAMP}" CACHE INTERNAL
    "Stamp file touched when all userland ELFs are built")

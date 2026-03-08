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




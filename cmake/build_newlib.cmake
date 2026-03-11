include(ExternalProject)

option(FORCE_NEWLIB_BUILD "Force rebuilding Newlib even if sysroot exists" ON)

# --- 1. Newlib Setup ---
if(FORCE_NEWLIB_BUILD OR NOT EXISTS "${NEWLIB_SYSROOT}")
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




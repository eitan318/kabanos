option(REBUILD_NEWLIB "Force rebuild and reinstall newlib" ON)

set(NEWLIB_SRC_DIR "${CMAKE_SOURCE_DIR}/extern/newlib-2.5.0")
set(NEWLIB_SYS_MYOS "${NEWLIB_SRC_DIR}/newlib/libc/sys/myos")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${SYSROOT_DIR}")

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

set(TCC_BIN "${CMAKE_SOURCE_DIR}/extern/i386-tcc")
set(USER_OUTPUT_DIR "${NEWLIB_SYSROOT}/bin")
set(USER_OBJS_BASE "${CMAKE_CURRENT_BINARY_DIR}/user_objs")


# Ask the compiler where its internal headers are
execute_process(
    COMMAND i686-myos-gcc -print-file-name=include
    OUTPUT_VARIABLE GCC_INTERNAL_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Common flags for all userland apps
set(COMMON_USER_FLAGS
    -m32 
    -nostdinc
    -fno-stack-protector
    -g
    -isystem "${GCC_INTERNAL_INCLUDE_DIR}" 
    "-I${NEWLIB_SYSROOT}/usr/include"
)


# 2. Iterate through user apps
file(GLOB USER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*")

foreach(USER_DIR ${USER_DIRS})

    if(NOT IS_DIRECTORY ${USER_DIR})
        continue()
    endif()

    get_filename_component(USER_NAME ${USER_DIR} NAME)

    set(EXTRA_APP_FLAGS "")
    if("${USER_NAME}" STREQUAL "tcc")
        set(EXTRA_APP_FLAGS 
            -DTCC_TARGET_I386 
            -DCONFIG_TCC_STATIC 
            -DONE_SOURCE=1
            -U__linux__
        )
    endif()

    # Define paths for this specific app
    set(APP_OBJ_DIR "${USER_OBJS_BASE}/${USER_NAME}")
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")
    
    # 3. Collect all .c files in the app directory
    file(GLOB_RECURSE APP_SOURCES "${USER_DIR}/*.c")
    set(APP_OBJECTS "")

    if(USER_NAME STREQUAL "tcc")
        # 1. Define the ONE SOURCE file
        set(APP_SOURCES "${USER_DIR}/tcc.c")
        
        # 2. Add the specific TCC flags
        set(EXTRA_APP_FLAGS 
            -DTCC_TARGET_I386 
            -DONE_SOURCE=1 
            -DCONFIG_TCC_STATIC
            -U__linux__
            -U__unix__
        )
    else()
        file(GLOB_RECURSE APP_SOURCES "${USER_DIR}/*.c")
        set(EXTRA_APP_FLAGS "")
    endif()

    # 4. Create a compilation command for each .c file
    foreach(SRC_FILE ${APP_SOURCES})
        get_filename_component(SRC_NAME ${SRC_FILE} NAME_WE)
        set(OBJ_FILE "${APP_OBJ_DIR}/${SRC_NAME}.o")
        list(APPEND APP_OBJECTS "${OBJ_FILE}")

        add_custom_command(
            OUTPUT "${OBJ_FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_OBJ_DIR}"
            COMMAND i686-myos-gcc ${COMMON_USER_FLAGS} ${EXTRA_APP_FLAGS}
                    "-I${USER_DIR}" 
                    "-I${USER_DIR}/include"
                    -c "${SRC_FILE}" -o "${OBJ_FILE}"
            DEPENDS "${SRC_FILE}" newlib_project
            COMMENT "Compiling ${USER_NAME}: ${SRC_NAME}.c"
            VERBATIM
        )
    endforeach()

    # 5. Create a linking command for the final ELF
    add_custom_command(
        OUTPUT "${USER_ELF}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
        COMMAND i686-myos-gcc -m32 -g -o "${USER_ELF}" -static -nostdlib
                "-Wl,-Ttext=0x400000"
                "-Wl,--allow-multiple-definition"
                "${NEWLIB_SYSROOT}/usr/lib/crt0.o"
                ${APP_OBJECTS}
                "-L${NEWLIB_SYSROOT}/usr/lib" -lc -lnosys
        DEPENDS ${APP_OBJECTS}
        COMMENT "Linking ${USER_NAME}.elf"
        VERBATIM
    )       

    # 6. Create the target
    add_custom_target(${USER_NAME}_target ALL DEPENDS "${USER_ELF}")
endforeach()

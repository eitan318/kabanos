include(${CMAKE_SOURCE_DIR}/cmake/build_newlib.cmake)

# --- 1. User App Function ---
function(add_user_app USER_DIR)
    get_filename_component(USER_NAME "${USER_DIR}" NAME)
    
    # Skip TinyCC (Built in Docker)
    if(USER_NAME STREQUAL "tinycc" OR NOT IS_DIRECTORY "${USER_DIR}")
        return()
    endif()

    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    file(GLOB APP_SOURCES "${USER_DIR}/*.c" "${USER_DIR}/*.S" "${USER_DIR}/*.s")

    add_custom_command(
        OUTPUT "${USER_ELF}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E env NEWLIB_SYSROOT=${NEWLIB_SYSROOT} 
                bash "${CMAKE_SOURCE_DIR}/scripts/build/i686-myos-gcc-user.sh" 
                ${APP_SOURCES} -o "${USER_ELF}"
        DEPENDS ${APP_SOURCES} newlib_project-flatten
        COMMENT "Building User App: ${USER_NAME}"
        VERBATIM
    )

    add_custom_target(${USER_NAME}_target ALL DEPENDS "${USER_ELF}")

    # Register for disk image
    list(APPEND ALL_ELF_FILES "${USER_ELF}")
    set(ALL_ELF_FILES "${ALL_ELF_FILES}" PARENT_SCOPE)
endfunction()

# --- 2. Global Config ---
set(USER_OUTPUT_DIR "${NEWLIB_SYSROOT}/bin")

# --- 3. Build TinyCC (from primed Docker source) ---
set(TCC_ELF "${USER_OUTPUT_DIR}/tcc.elf")

add_custom_command(
    OUTPUT "${TCC_ELF}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
    # Use the wrapper script to compile the source prepared in Docker
    COMMAND ${CMAKE_COMMAND} -E env NEWLIB_SYSROOT=${NEWLIB_SYSROOT} 
            bash "${CMAKE_SOURCE_DIR}/scripts/build/i686-myos-gcc-user.sh"
            -DONE_SOURCE=1 
            -DTCC_TARGET_I386 
            -DCONFIG_TCC_STATIC 
            -D_GNU_SOURCE
            -DCONFIG_USE_LIBGCC
            -UCONFIG_TCC_BACKTRACE
            -UTCC_IS_NATIVE
            "-DCONFIG_TCCDIR=\"/usr/lib/tcc\""
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"/usr/include\""
            "-I/opt/tcc-src" 
            "-I/opt/tcc-src/include"
            "/opt/tcc-src/tcc.c" -o "${TCC_ELF}"
    # This must wait for Newlib to be finished so crt0.o and libc.a exist
    DEPENDS newlib_project-flatten
    COMMENT "Compiling TinyCC for MyOS"
    VERBATIM
)

add_custom_target(tcc_target ALL DEPENDS "${TCC_ELF}")

# --- 4. Discovery & Build Apps ---
set(ALL_ELF_FILES "${TCC_ELF}") # Start the list with TCC

file(GLOB USER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*")
foreach(USER_DIR ${USER_DIRS})
    add_user_app("${USER_DIR}")
endforeach()

# --- 5. Userland Stamp ---
set(USERLAND_STAMP "${CMAKE_CURRENT_BINARY_DIR}/userland.stamp")
add_custom_command(
    OUTPUT "${USERLAND_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E touch "${USERLAND_STAMP}"
    DEPENDS ${ALL_ELF_FILES}
    VERBATIM
)

add_custom_target(userland_stamp_target ALL DEPENDS "${USERLAND_STAMP}")
add_dependencies(userland_stamp_target newlib_project-flatten)

cmake_minimum_required(VERSION 3.16)

include(cmake/toolchain.cmake)
project(myos C ASM_NASM)

set(ARCH i686)

enable_language(C ASM_NASM)

include(cmake/paths.cmake)

add_subdirectory("${SRC_DIR}/bootloader")
add_subdirectory("${SRC_DIR}/kernel")
add_subdirectory("${SRC_DIR}/userland")




set(HOST_TOOLS_BINARY_DIR "${CMAKE_BINARY_DIR}/host_tools")
set(MKFS_MYFS_EXE "${HOST_TOOLS_BINARY_DIR}/mkfs_myfs")
set(MKFS_MYFS_SRC_DIR "${CMAKE_SOURCE_DIR}/tools/mkfs_myfs")

include(ExternalProject)

# Use the 'env' command to clear/set variables for the sub-process
ExternalProject_Add(host_tools_project
    SOURCE_DIR "${MKFS_MYFS_SRC_DIR}"
    BINARY_DIR "${HOST_TOOLS_BINARY_DIR}"
    
    # Use the 'env' command to ensure we use the host compiler
    # Note: Added -DSRC_DIR here because CONFIGURE_COMMAND overrides CMAKE_ARGS
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env CC=/usr/bin/gcc 
        ${CMAKE_COMMAND} -S "${MKFS_MYFS_SRC_DIR}" 
                         -B "${HOST_TOOLS_BINARY_DIR}"
                         -DCMAKE_TOOLCHAIN_FILE=
                         -DSRC_DIR=${CMAKE_SOURCE_DIR}  # <--- Added this line
    
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${MKFS_MYFS_EXE}"
)



configure_file(
    "${CMAKE_SOURCE_DIR}/boot.cfg" 
    "${BOOT_DIR}/boot.cfg" 
    COPYONLY
)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    add_custom_target(fix_compile_commands ALL
        COMMAND ln -sf build/compile_commands.json compile_commands.json
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Fixing compile_commands.json for host IDE"
    )
endif()

# A script to merge two JSON files
set(MERGE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build/merge_compile_commands.py")
add_custom_target(combine_db ALL
    COMMAND python3 ${MERGE_SCRIPT} 
            ${CMAKE_BINARY_DIR}/compile_commands.json 
            ${NEWLIB_BUILD_DIR}/compile_commands.json
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS newlib_project # Ensure newlib is built so its JSON exists
    COMMENT "Merging Newlib and Kernel compile commands"
)


include(cmake/initrd.cmake)
include(cmake/disk_image.cmake)


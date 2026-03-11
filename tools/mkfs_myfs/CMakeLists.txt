cmake_minimum_required(VERSION 3.22)
project(MyOS_HostTools C)

if(NOT SRC_DIR)
    message(FATAL_ERROR "SRC_DIR not defined! Pass -DSRC_DIR from the main CMake.")
endif()

add_executable(mkfs_myfs 
    main.c 
    "${SRC_DIR}/kernel/src/fs/myfs/myfs.c"
)

target_include_directories(mkfs_myfs PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SRC_DIR}/kernel/include/fs/myfs"
)

target_compile_definitions(mkfs_myfs PRIVATE MKFS_MYFS _HOST_BUILD_)
target_compile_options(mkfs_myfs PRIVATE -g -O0 -Wall)
target_link_libraries(mkfs_myfs PRIVATE m)
set_target_properties(mkfs_myfs PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

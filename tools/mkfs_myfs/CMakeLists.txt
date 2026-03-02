# tools/mkfs_myfs/CMakeLists.txt

add_executable(mkfs_myfs 
    main.c 
    ${SRC_DIR}/kernel/src/fs/myfs/myfs.c
)

target_include_directories(mkfs_myfs PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SRC_DIR}/kernel/include/fs/myfs    # Specific to the FS
)

target_compile_definitions(mkfs_myfs PRIVATE MKFS_MYFS)

# Ensure we are using host standard libraries
target_link_libraries(mkfs_myfs PRIVATE m c gcc)

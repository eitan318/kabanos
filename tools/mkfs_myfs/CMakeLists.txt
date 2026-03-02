# tools/mkfs_myfs/CMakeLists.txt

add_executable(mkfs_myfs 
    main.c 
    ${SRC_DIR}/kernel/src/fs/myfs/myfs.c
)

# Force the host_compat.h into every source file in this target
target_compile_options(mkfs_myfs PRIVATE 
    -g -O0 
)

target_include_directories(mkfs_myfs PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SRC_DIR}/kernel/include
)

set_target_properties(mkfs_myfs PROPERTIES 
    LINK_FLAGS "" 
    LINKER_LANGUAGE C
)

target_link_libraries(mkfs_myfs PRIVATE m c gcc)

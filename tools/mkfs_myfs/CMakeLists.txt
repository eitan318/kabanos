# tools/mkfs_myfs/CMakeLists.txt

set(CMAKE_C_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")

add_executable(mkfs_myfs 
    main.c 
    ${SRC_DIR}/kernel/src/fs/vfs.c
)

target_include_directories(mkfs_myfs PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SRC_DIR}/kernel/include
)

target_compile_options(mkfs_myfs PRIVATE 
    -g -O0 
)

target_link_libraries(mkfs_myfs PRIVATE m c)

set_target_properties(mkfs_myfs PROPERTIES 
    LINK_FLAGS "" 
    LINKER_LANGUAGE C
)

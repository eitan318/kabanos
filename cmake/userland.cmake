# 1. Collect Library Sources (the "OS Library")
file(GLOB LIB_SOURCES 
    ${SRC_DIR}/userland/lib/*.c
)

# 2. Create the library target
add_library(userlib STATIC ${LIB_SOURCES})
# Apply freestanding flags to the library itself
target_compile_options(userlib PRIVATE -m32 -ffreestanding -fno-stack-protector -nostdlib)
target_include_directories(userlib PUBLIC ${SRC_DIR}/userland/lib/include)

# 3. Collect User Apps
file(GLOB USER_SOURCES 
    ${CMAKE_SOURCE_DIR}/userland/user_src/*.c
)

set(USER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/BOOT)

foreach(USER_FILE ${USER_SOURCES})
    get_filename_component(USER_NAME ${USER_FILE} NAME_WE)
    
    # Create the executable
    add_executable(${USER_NAME}.elf ${USER_FILE})

    # Link against our userlib (this handles headers + compiled lib code)
    target_link_libraries(${USER_NAME}.elf PRIVATE userlib)

    # Executable specific flags
    target_compile_options(${USER_NAME}.elf PRIVATE 
        -m32 -ffreestanding -fno-stack-protector -nostdlib
    )

    target_link_options(${USER_NAME}.elf PRIVATE 
        -m32 -static -nostdlib
        # You may eventually need: -T ${CMAKE_SOURCE_DIR}/userland/linker.ld
    )

    set_target_properties(${USER_NAME}.elf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${USER_OUTPUT_DIR}
    )
endforeach()

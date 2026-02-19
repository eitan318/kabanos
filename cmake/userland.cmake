enable_language(ASM_NASM)

file(GLOB LIB_C_SOURCES ${SRC_DIR}/userland/lib/*.c)
file(GLOB LIB_ASM_SOURCES ${SRC_DIR}/userland/lib/*.asm)

# Manually assemble all .asm files as elf32
set(LIB_ASM_OBJECTS)
foreach(ASM_SOURCE ${LIB_ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_SOURCE} NAME_WE)
    set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/userlib_asm/${ASM_NAME}.o")

    add_custom_command(
        OUTPUT ${ASM_OBJECT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/userlib_asm"
        COMMAND nasm -f elf32 -g -F dwarf ${ASM_SOURCE} -o ${ASM_OBJECT}
        DEPENDS ${ASM_SOURCE}
        COMMENT "Assembling ${ASM_SOURCE} as elf32"
    )
    list(APPEND LIB_ASM_OBJECTS ${ASM_OBJECT})
endforeach()

add_library(userlib STATIC ${LIB_C_SOURCES} ${LIB_ASM_OBJECTS})
target_include_directories(userlib PUBLIC ${SRC_DIR}/userland/lib/include)
target_compile_options(userlib PRIVATE
    -m32 -ffreestanding -fno-stack-protector -nostdlib
)

file(GLOB USER_SOURCES ${SRC_DIR}/userland/user_src/*.c)
set(USER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/BOOT)

foreach(USER_FILE ${USER_SOURCES})
    get_filename_component(USER_NAME ${USER_FILE} NAME_WE)
    
    add_executable(${USER_NAME}.elf ${USER_FILE})
    target_link_libraries(${USER_NAME}.elf PRIVATE userlib)

    target_compile_options(${USER_NAME}.elf PRIVATE 
        -m32 -ffreestanding -fno-stack-protector -nostdlib
    )

    target_link_options(${USER_NAME}.elf PRIVATE 
        -m32 -static -nostdlib
        -Wl,-T,${SRC_DIR}/userland/user_src/linker.ld  
    )

    set_target_properties(${USER_NAME}.elf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${USER_OUTPUT_DIR}
    )
endforeach()

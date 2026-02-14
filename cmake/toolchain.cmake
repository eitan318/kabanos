# Force 32-bit build
set(CMAKE_C_FLAGS "-m32 -ffreestanding -fno-pic")
set(CMAKE_EXE_LINKER_FLAGS "-m32 -nostdlib -nodefaultlibs -nostartfiles -no-pie")

# NASM 32-bit
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-f elf32")

find_program(CMAKE_OBJCOPY NAMES objcopy REQUIRED)
find_program(CMAKE_LINKER NAMES ld REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Symlink compile_commands.json
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
endif()

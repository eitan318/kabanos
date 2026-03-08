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

if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    add_custom_target(fix_compile_commands ALL
        COMMAND ln -sf build/compile_commands.json compile_commands.json
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Fixing compile_commands.json for host IDE"
    )
endif()




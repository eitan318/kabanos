# Kernel CMakeLists.txt
file(MAKE_DIRECTORY ${KERNEL_OUT_DIR})

# Define include directories for kernel
set(KERNEL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}          # kernel/
    ${SRC_DIR}/libs                       # src/libs/
)

# Glob kernel sources
file(GLOB_RECURSE KERNEL_C_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c 
    ${SRC_DIR}/libs/*.c
)

# Remove kernel_symbols.c from the pre-link build
list(FILTER KERNEL_C_SOURCES EXCLUDE REGEX "kernel_symbols\\.c$")
file(GLOB_RECURSE KERNEL_ASM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.asm)

# Set NASM include directories
set(NASM_INCLUDE_DIRS "-I${CMAKE_CURRENT_SOURCE_DIR} -I${SRC_DIR}/libs")

# Process ASM sources
foreach(ASM_SOURCE ${KERNEL_ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_SOURCE} NAME_WE)
    get_filename_component(ASM_DIR ${ASM_SOURCE} DIRECTORY)
    file(RELATIVE_PATH ASM_REL_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${ASM_DIR})
    set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}/${ASM_NAME}.o")
    
    add_custom_command(
        OUTPUT ${ASM_OBJECT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}"
        COMMAND nasm -f elf32 ${NASM_INCLUDE_DIRS} ${ASM_SOURCE} -o ${ASM_OBJECT} -g -F dwarf
        DEPENDS ${ASM_SOURCE}
        COMMENT "Compiling ASM: ${ASM_SOURCE}"
    )
    list(APPEND KERNEL_ASM_OBJECTS ${ASM_OBJECT})
endforeach()

# ============================================================================
# STEP 1: Build kernel WITHOUT symbols (initial link)
# ============================================================================
# Create a stub symbols file for pre-link
set(SYMBOLS_STUB ${CMAKE_CURRENT_BINARY_DIR}/symbols_stub.c)
file(WRITE ${SYMBOLS_STUB} 
"// Stub symbols for pre-link stage
#include <stddef.h>

typedef struct {
    unsigned int addr;
    char name[64];
} symbol_t;

const symbol_t symbols[] = {};
const int symbol_count = 0;

const char* lookup_symbol(unsigned int addr) {
    return NULL;
}
")

add_executable(kernel_pre.elf
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_OBJECTS}
    ${SYMBOLS_STUB}
)

set_target_properties(kernel_pre.elf PROPERTIES
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_options(kernel_pre.elf PRIVATE
    -ffreestanding
    -nostdlib
    -g
    -g3
    -O0
    -fno-omit-frame-pointer
    -fno-inline
)

target_include_directories(kernel_pre.elf PRIVATE ${KERNEL_INCLUDE_DIRS})

target_link_options(kernel_pre.elf PRIVATE
    -nostdlib
    LINKER:-m,elf_i386
    LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    LINKER:-Map=${CMAKE_CURRENT_BINARY_DIR}/kernel_pre.map
)

set_property(TARGET kernel_pre.elf APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

# ============================================================================
# STEP 2: Generate symbols.c from the pre-link map file
# ============================================================================
set(SYMBOLS_C ${CMAKE_CURRENT_BINARY_DIR}/symbols.c)

add_custom_command(
    OUTPUT ${SYMBOLS_C}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/gen_symbols.py
        ${CMAKE_CURRENT_BINARY_DIR}/kernel_pre.map
        ${SYMBOLS_C}
    DEPENDS kernel_pre.elf ${CMAKE_SOURCE_DIR}/scripts/gen_symbols.py
    COMMENT "Generating symbols.c from kernel_pre.map"
)


# ============================================================================
# STEP 3: Build final kernel WITH symbols
# ============================================================================
# For the final build, we need kernel_symbols.c - find it explicitly
file(GLOB KERNEL_SYMBOLS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kernel_symbols.c)

add_executable(kernel.elf
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_OBJECTS}
    ${SYMBOLS_C}  # Include the generated symbols
    ${KERNEL_SYMBOLS_SOURCE}  # Now include kernel_symbols.c
)

set_target_properties(kernel.elf PROPERTIES
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY ${KERNEL_OUT_DIR}
)

target_compile_options(kernel.elf PRIVATE
    -ffreestanding
    -nostdlib
    -g
)

target_include_directories(kernel.elf PRIVATE ${KERNEL_INCLUDE_DIRS})

target_link_options(kernel.elf PRIVATE
    -nostdlib
    LINKER:-m,elf_i386
    LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    LINKER:-Map=${KERNEL_OUT_DIR}/kernel.map
)

set_property(TARGET kernel.elf APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

# ============================================================================
# Export variables and create targets
# ============================================================================
set(KERNEL_ELF ${KERNEL_OUT_DIR}/kernel.elf)
set(KERNEL_OUTPUT ${KERNEL_ELF} PARENT_SCOPE) 
set(KERNEL_BIN_OUTPUT ${KERNEL_BIN} PARENT_SCOPE)

add_custom_target(kernel ALL
    DEPENDS ${KERNEL_ELF}
)

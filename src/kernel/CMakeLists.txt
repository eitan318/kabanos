# Kernel CMakeLists.txt

file(MAKE_DIRECTORY ${KERNEL_OUT_DIR})

# Glob kernel sources
file(GLOB_RECURSE KERNEL_C_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE KERNEL_ASM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.asm)

set(NASM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(NASM_INCLUDE_DIRS "-I${NASM_INCLUDE_DIR}")

# Compile ASM sources manually to ensure 32-bit
set(KERNEL_ASM_OBJECTS "")
foreach(ASM_SOURCE ${KERNEL_ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_SOURCE} NAME_WE)
    get_filename_component(ASM_DIR ${ASM_SOURCE} DIRECTORY)
    file(RELATIVE_PATH ASM_REL_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${ASM_DIR})
    set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}/${ASM_NAME}.o")
    
    add_custom_command(
        OUTPUT ${ASM_OBJECT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}"
        COMMAND nasm -f elf32 ${ASM_SOURCE} -o ${ASM_OBJECT} -g -F dwarf
        DEPENDS ${ASM_SOURCE}
        COMMENT "Compiling ASM: ${ASM_SOURCE}"
    )
    list(APPEND KERNEL_ASM_OBJECTS ${ASM_OBJECT})
endforeach()

# Create ELF executable
add_executable(kernel.elf
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_OBJECTS}
)

set_target_properties(kernel.elf PROPERTIES
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY ${KERNEL_OUT_DIR} 
)

# C compiler flags
target_compile_options(kernel.elf PRIVATE
    -ffreestanding
    -nostdlib

)

# Linker flags
target_link_options(kernel.elf PRIVATE
    -nostdlib
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

target_include_directories(kernel.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define paths
set(KERNEL_ELF ${KERNEL_OUT_DIR}/kernel.elf)
set(KERNEL_BIN_UNPADDED ${KERNEL_OUT_DIR}/kernel_unpadded.bin)
set(KERNEL_BIN ${KERNEL_OUT_DIR}/kernel.bin)

# Convert kernel.elf → kernel_unpadded.bin
add_custom_command(
    OUTPUT ${KERNEL_BIN_UNPADDED}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${KERNEL_ELF} ${KERNEL_BIN_UNPADDED}
    DEPENDS kernel.elf
    COMMENT "Converting kernel.elf → kernel_unpadded.bin"
)

set_property(SOURCE ${KERNEL_SOURCES} PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

# Pad kernel_unpadded.bin → kernel.bin (to next 512-byte sector boundary)
add_custom_command(
    OUTPUT ${KERNEL_BIN}
    COMMAND dd if=${KERNEL_BIN_UNPADDED} of=${KERNEL_BIN} bs=512 conv=sync status=none
    COMMAND rm -f ${KERNEL_BIN_UNPADDED}
    DEPENDS ${KERNEL_BIN_UNPADDED}
    COMMENT "Padding kernel to next sector boundary"
)

# Generate header file with actual kernel sectors count
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(KERNEL_SECTORS_HEADER ${SRC_DIR}/bootloader/stage2/kernel_sectors.h)

# Create a CMake script to generate the header
file(WRITE ${CMAKE_BINARY_DIR}/generate_kernel_sectors.cmake "
file(SIZE \"\${KERNEL_BIN}\" KERNEL_SIZE)
math(EXPR KERNEL_SECTORS \"(\${KERNEL_SIZE} + 511) / 512\")
file(MAKE_DIRECTORY \"\${GENERATED_DIR}\")
file(WRITE \"\${KERNEL_SECTORS_HEADER}\" \"// Auto-generated - do not edit\\n\")
file(APPEND \"\${KERNEL_SECTORS_HEADER}\" \"#define KERNEL_SECTORS_TOTAL \${KERNEL_SECTORS}\\n\")
")

add_custom_command(
    OUTPUT ${KERNEL_SECTORS_HEADER}
    COMMAND ${CMAKE_COMMAND} 
        -DKERNEL_BIN=${KERNEL_BIN}
        -DGENERATED_DIR=${GENERATED_DIR}
        -DKERNEL_SECTORS_HEADER=${KERNEL_SECTORS_HEADER}
        -P ${CMAKE_BINARY_DIR}/generate_kernel_sectors.cmake
    DEPENDS ${KERNEL_BIN}
    COMMENT "Generating kernel_sectors.h with actual sector count"
)


set(${KERNEL_OUTPUT} ${KERNEL_BIN})

# Add target for kernel.bin and header generation
add_custom_target(kernel ALL
    DEPENDS ${KERNEL_OUTPUT} ${KERNEL_SECTORS_HEADER}
)


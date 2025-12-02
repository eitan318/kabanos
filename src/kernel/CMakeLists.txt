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

# CRITICAL: Set include directories
target_include_directories(kernel.elf PRIVATE ${KERNEL_INCLUDE_DIRS})

# Linker flags
target_link_options(kernel.elf PRIVATE
    -nostdlib
    LINKER:-m,elf_i386
    LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

# Make kernel.elf depend on linker script
set_property(TARGET kernel.elf APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

# Define paths
set(KERNEL_ELF ${KERNEL_OUT_DIR}/kernel.elf)
# set(KERNEL_BIN_UNPADDED ${KERNEL_OUT_DIR}/kernel_unpadded.bin)
# set(KERNEL_BIN ${KERNEL_OUT_DIR}/kernel.bin)

# Convert kernel.elf → kernel_unpadded.bin
# add_custom_command(
#     OUTPUT ${KERNEL_BIN_UNPADDED}
#     COMMAND ${CMAKE_OBJCOPY} -O binary ${KERNEL_ELF} ${KERNEL_BIN_UNPADDED}
#     DEPENDS kernel.elf
#     COMMENT "Converting kernel.elf → kernel_unpadded.bin"
# )

# Pad kernel_unpadded.bin → kernel.bin (to next 512-byte sector boundary)
# add_custom_command(
#     OUTPUT ${KERNEL_BIN}
#     COMMAND dd if=${KERNEL_BIN_UNPADDED} of=${KERNEL_BIN} bs=512 conv=sync status=none
#     COMMAND ${CMAKE_COMMAND} -E rm -f ${KERNEL_BIN_UNPADDED}
#     DEPENDS ${KERNEL_BIN_UNPADDED}
#     COMMENT "Padding kernel to next sector boundary"
# )

# CHANGED: Set both kernel.elf and kernel.bin for parent scope
set(KERNEL_OUTPUT ${KERNEL_ELF} PARENT_SCOPE) 
#set(KERNEL_BIN_OUTPUT ${KERNEL_BIN} PARENT_SCOPE)  # if we still need kernel.bin elsewhere

# Add target for kernel.bin and header generation
add_custom_target(kernel ALL
    DEPENDS 
    #    ${KERNEL_BIN}
    ${KERNEL_ELF}
    ${KERNEL_SECTORS_HEADER}
)
# Kernel CMakeLists.txt
file(MAKE_DIRECTORY ${KERNEL_OUT_DIR})

# 1. Architecture Validation
if(${ARCH} STREQUAL "i386" OR ${ARCH} STREQUAL "i686")
    set(NASM_FORMAT elf32)
    set(LINKER_TARGET elf_i386)
else()
    message(FATAL_ERROR "Architecture ${ARCH} not supported!")
endif()

# 2. Source Collection (Same as before)
file(GLOB_RECURSE ALL_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE LIB_SOURCES "${SRC_DIR}/libs/*.c")
file(GLOB_RECURSE ARCH_SPECIFIC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/*.c")


set(KERNEL_GENERIC_SOURCES ${ALL_C_SOURCES})
list(FILTER KERNEL_GENERIC_SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_SOURCE_DIR}/src/arch/.*")

set(KERNEL_C_SOURCES ${KERNEL_GENERIC_SOURCES} ${ARCH_SPECIFIC_SOURCES} ${LIB_SOURCES})

# 3. ASM Processing (Same as before)
file(GLOB_RECURSE KERNEL_ASM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/*.asm)
foreach(ASM_SOURCE ${KERNEL_ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_SOURCE} NAME_WE)
    get_filename_component(ASM_DIR ${ASM_SOURCE} DIRECTORY)
    file(RELATIVE_PATH ASM_REL_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${ASM_DIR})
    set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}/${ASM_NAME}.o")
    
    add_custom_command(
        OUTPUT ${ASM_OBJECT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${ASM_REL_DIR}"
        COMMAND nasm -f ${NASM_FORMAT} ${ASM_SOURCE} -o ${ASM_OBJECT} -g -F dwarf
        DEPENDS ${ASM_SOURCE}
    )
    list(APPEND KERNEL_ASM_OBJECTS ${ASM_OBJECT})
endforeach()

# 4. Build Kernel
add_executable(kernel.elf ${KERNEL_C_SOURCES} ${KERNEL_ASM_OBJECTS})

# --- ADD THIS SECTION ---
set_target_properties(kernel.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${KERNEL_OUT_DIR}"
    OUTPUT_NAME "kernel"
    SUFFIX ".elf"
)

# 5. The "Normal" Include Strategy
# We point to the 'include' folder. 
# Because that folder contains a subfolder 'arch', <arch/types.h> works!
target_include_directories(kernel.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include                    # src/kernel/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/include   # src/kernel/src/arch/i686/include/
    ${SRC_DIR}/libs                                        # src/libs/
)

# 6. Options and Linker (Unchanged)
target_compile_options(kernel.elf PRIVATE -ffreestanding -nostdlib -g -g3 -O0 -fno-omit-frame-pointer -fno-inline)
target_link_options(kernel.elf PRIVATE -nostdlib LINKER:-m,${LINKER_TARGET} LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/linker.ld)


# In kernel/CMakeLists.txt
file(GLOB_RECURSE KERNEL_GENERIC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
list(FILTER KERNEL_GENERIC_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/")

file(GLOB_RECURSE KERNEL_ARCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/*.c")
file(GLOB_RECURSE LIB_SOURCES "${COMMON_DIR}/*.c")

file(GLOB_RECURSE KERNEL_ASM_SOURCES "src/arch/${ARCH}/*.asm")

set(KERNEL_C_SOURCES ${KERNEL_GENERIC_SOURCES} ${KERNEL_ARCH_SOURCES} ${LIB_SOURCES})

add_executable(kernel.elf ${KERNEL_C_SOURCES} ${KERNEL_ASM_SOURCES})

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/${ARCH}/linker.ld")
target_link_options(kernel.elf PRIVATE 
    "-T${LINKER_SCRIPT}"
    "-nostdlib"        
    "-static"         
    "LINKER:-m,elf_i386"
)

# Explicitly use absolute paths for headers
target_include_directories(kernel.elf PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${COMMON_DIR}"
)
target_link_libraries(kernel.elf PRIVATE gcc)

add_custom_command(TARGET kernel.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            "$<TARGET_FILE:kernel.elf>" 
            "${KERNEL_OUTPUT}"
    COMMENT "Deploying kernel.elf to bootroot..."
)

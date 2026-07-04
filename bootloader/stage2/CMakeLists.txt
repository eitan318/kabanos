file(MAKE_DIRECTORY ${STAGE2_OUT_DIR})

# Gather sources
file(GLOB_RECURSE STAGE2_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm" 
    "${COMMON_DIR}/*.c"
)

add_executable(stage2.elf ${STAGE2_SOURCES})

set_target_properties(stage2.elf PROPERTIES
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY ${STAGE2_OUT_DIR}
)

target_include_directories(stage2.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${COMMON_DIR}
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/linker.ld")
target_link_options(stage2.elf PRIVATE
    "-nostdlib"
    "LINKER:-m,elf_i386"
    "-T${LINKER_SCRIPT}"
)

set(STAGE2_ELF ${STAGE2_OUT_DIR}/stage2.elf)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

add_custom_command(
    OUTPUT "${STAGE2_BIN}"
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stage2.elf> "${STAGE2_BIN}"
    DEPENDS stage2.elf
    COMMENT "Generating sector-aligned stage2.bin (Linker handled padding)"
    VERBATIM
)

add_custom_target(bootloader_stage2 ALL 
    DEPENDS "${STAGE2_BIN}"
    DEPENDS stage2.elf
)


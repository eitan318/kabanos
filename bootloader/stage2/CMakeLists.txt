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

# ---- Post-build: strip to flat binary, pad to sector boundary ----
set(STAGE2_ELF ${STAGE2_OUT_DIR}/stage2.elf)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

add_custom_command(
    OUTPUT "${STAGE2_BIN}"
    COMMAND ${CMAKE_OBJCOPY} -O binary "${STAGE2_ELF}" "${STAGE2_BIN}"
    DEPENDS stage2.elf
    COMMENT "Generating sector-aligned stage2.bin (Linker handled padding)"
    VERBATIM
)
# ---- Generate stage2_sectors.inc for stage1.asm ----
set(STAGE2_SECTORS_ASM_HEADER ${SRC_DIR}/bootloader/stage1/stage2_sectors.inc)

file(WRITE ${CMAKE_BINARY_DIR}/generate_stage2_sectors.cmake "
file(SIZE \"\${STAGE2_BIN}\" STAGE2_SIZE)
math(EXPR STAGE2_SECTORS \"(\${STAGE2_SIZE} + 511) / 512\")
file(WRITE \"\${STAGE2_SECTORS_ASM_HEADER}\" \"; Auto-generated - do not edit\\n\")
file(APPEND \"\${STAGE2_SECTORS_ASM_HEADER}\" \"STAGE2_SECTORS_TOTAL equ \${STAGE2_SECTORS}\\n\")
message(STATUS \"Stage2: \${STAGE2_SIZE} bytes = \${STAGE2_SECTORS} sectors\")
")

# 1. The script that calculates sectors
# We define it as a custom command producing the .inc file
add_custom_command(
    OUTPUT "${STAGE2_SECTORS_ASM_HEADER}"
    COMMAND ${CMAKE_COMMAND}
            -DSTAGE2_BIN=${STAGE2_BIN}
            -DSTAGE2_SECTORS_ASM_HEADER=${STAGE2_SECTORS_ASM_HEADER}
            -P ${CMAKE_BINARY_DIR}/generate_stage2_sectors.cmake
    DEPENDS "${STAGE2_BIN}"  # <--- This forces stage2.bin to be created first!
    COMMENT "Calculating Stage2 sector count and writing to .inc"
    VERBATIM
)

# 2. Create the final target that brings it all together
add_custom_target(bootloader_stage2 ALL 
    DEPENDS "${STAGE2_SECTORS_ASM_HEADER}" "${STAGE2_BIN}"
)

# 3. Kernel dependency (if still needed)
add_dependencies(bootloader_stage2 kernel.elf)

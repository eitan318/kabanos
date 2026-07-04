set(STAGE1_ASM "${CMAKE_CURRENT_SOURCE_DIR}/stage1.asm")
set(STAGE1_BIN "${STAGE1_OUT_DIR}/stage1.bin")


add_custom_command(
    OUTPUT "${STAGE1_BIN}"
    # $<TARGET_FILE:stage2.elf> is the "magic" that finds the real path
    COMMAND bash -c " \
        SECTORS=$(${CALC_STAGE2_SECTORS_SH} $<TARGET_FILE:stage2.elf> 0x9000) && \
        echo 'Detected Stage 2 sectors: '\$SECTORS && \
        nasm -f bin ${STAGE1_ASM} \
             -DSTAGE2_SECTORS_TOTAL=\$SECTORS \
             -o ${STAGE1_BIN} \
             -I${CMAKE_CURRENT_SOURCE_DIR}/ \
             -I${CMAKE_CURRENT_SOURCE_DIR}/../../common/"
    # Change STAGE2_BIN to the target name stage2.elf
    DEPENDS "${STAGE1_ASM}" stage2.elf "${CALC_STAGE2_SECTORS_SH}"
    COMMENT "Calculating Stage 2 size and assembling Stage 1"
    VERBATIM
)

add_custom_target(bootloader_stage1 ALL DEPENDS "${STAGE1_BIN}")
add_dependencies(bootloader_stage1 bootloader_stage2)

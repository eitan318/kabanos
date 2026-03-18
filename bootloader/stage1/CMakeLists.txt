set(STAGE1_ASM "${CMAKE_CURRENT_SOURCE_DIR}/stage1.asm")
set(STAGE1_BIN "${STAGE1_OUT_DIR}/stage1.bin")

add_custom_command(
  OUTPUT "${STAGE1_BIN}"
  # This command calculates the size on the fly and passes it to NASM
  COMMAND bash -c "nasm -f bin ${STAGE1_ASM} \
          -DSTAGE2_SECTORS_TOTAL=$(($(stat -c %s ${STAGE2_BIN}) / 512)) \
          -o ${STAGE1_BIN} \
          -I${CMAKE_CURRENT_SOURCE_DIR}/ \
          -I${CMAKE_CURRENT_SOURCE_DIR}/../../common/"
  DEPENDS "${STAGE1_ASM}" "${STAGE2_BIN}"
  COMMENT "Assembling stage1.asm with dynamic sector count"
  VERBATIM
)

add_custom_target(bootloader_stage1 ALL DEPENDS "${STAGE1_BIN}")
add_dependencies(bootloader_stage1 bootloader_stage2)

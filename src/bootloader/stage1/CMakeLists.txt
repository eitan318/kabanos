# ============================================================================
# stage1/CMakeLists.txt
# ============================================================================
# Variables are inherited from parent CMakeLists.txt
file(MAKE_DIRECTORY ${STAGE1_OUT_DIR})

set(STAGE1_ASM ${CMAKE_CURRENT_SOURCE_DIR}/stage1.asm)
set(STAGE2_SECTORS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/stage2_sectors.inc)

# Assemble stage1 directly to binary
# Stage1 needs stage2_sectors.inc to know how many sectors to load
add_custom_command(
  OUTPUT ${STAGE1_BIN}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${STAGE1_OUT_DIR}
  COMMAND nasm -f bin 
          ${STAGE1_ASM} 
          -o ${STAGE1_BIN} 
          -I${CMAKE_CURRENT_SOURCE_DIR}/
          -I${CMAKE_CURRENT_SOURCE_DIR}/../../common/
  DEPENDS ${STAGE1_ASM} ${STAGE2_SECTORS_HEADER}
  COMMENT "Assembling stage1.asm to binary"
  VERBATIM
)

add_custom_target(bootloader_stage1 ALL
  DEPENDS ${STAGE1_BIN}
)

# Stage1 depends on stage2_sectors.inc being generated
add_dependencies(bootloader_stage1 bootloader_stage2)

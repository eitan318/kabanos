# ============================================================================
# bootloader/CMakeLists.txt
# ============================================================================
cmake_minimum_required(VERSION 3.16)

file(MAKE_DIRECTORY ${BOOTLOADER_OUT_DIR})


# Define output directories and paths BEFORE adding subdirectories
# so they're available to child CMakeLists
set(STAGE1_OUT_DIR ${OUT_DIR}/bootloader/stage1)
set(STAGE2_OUT_DIR ${OUT_DIR}/bootloader/stage2)
set(STAGE1_BIN ${STAGE1_OUT_DIR}/stage1.bin)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)


# Add subdirectories for each stage
add_subdirectory(stage1)
add_subdirectory(stage2)

# Combine stage1 and stage2 into one bootloader binary
add_custom_command(
    OUTPUT ${BOOTLOADER_OUTPUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BOOTLOADER_OUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E echo "Combining stage1 + stage2 into bootloader.bin"
  COMMAND cat ${STAGE1_BIN} ${STAGE2_BIN} > ${BOOTLOADER_OUTPUT}
  DEPENDS
    ${STAGE1_BIN}
    ${STAGE2_BIN}
  COMMENT "Creating bootloader.bin (stage1 + stage2)"
  VERBATIM
)


# Create final bootloader target
add_custom_target(bootloader ALL
  DEPENDS ${BOOTLOADER_OUTPUT}
)

# Ensure proper build order
add_dependencies(bootloader bootloader_stage1 bootloader_stage2)


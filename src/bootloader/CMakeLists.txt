# ============================================================================
# bootloader/CMakeLists.txt
# ============================================================================
cmake_minimum_required(VERSION 3.16)

file(MAKE_DIRECTORY ${BOOTLOADER_OUT_DIR})

# Add subdirectories for each stage
add_subdirectory(stage1)
add_subdirectory(stage2)

# Create final bootloader target
add_custom_target(bootloader ALL
    DEPENDS ${STAGE2_BIN} ${STAGE1_BIN}
)

# Ensure proper build order
add_dependencies(bootloader bootloader_stage1 bootloader_stage2)


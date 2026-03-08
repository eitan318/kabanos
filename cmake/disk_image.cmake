# --- OS Image ---
# Retrieve the ELF list (for informational use / python script args if needed)
get_property(ALL_ELF_FILES GLOBAL PROPERTY GLOBAL_USER_ELF_FILES)

add_custom_command(
    OUTPUT "${OS_IMAGE_OUT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
    COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image   ${OS_IMAGE_OUT}
          --boot    ${STAGE1_BIN}
          --stage2  ${STAGE2_BIN}
          --boot_dir   ${BOOT_DIR}
          --sysroot_dir ${SYSROOT_DIR}
          --mkfs_myfs_path $<TARGET_FILE:mkfs_myfs>
    DEPENDS
        # Bootloader / kernel / initrd files (same scope or imported targets — fine)
        "${STAGE1_BIN}"
        "${STAGE2_BIN}"
        "${KERNEL_OUTPUT}"
        "${INITRD_IMG}"
        $<TARGET_FILE:mkfs_myfs>
        # The stamp: one concrete file that tracks ALL userland ELFs
        "${USERLAND_STAMP_FILE}"
    COMMENT "Creating partitioned OS image"
    VERBATIM
)

add_custom_target(os_image ALL
    DEPENDS "${OS_IMAGE_OUT}"
)

# Target-level ordering guards (belt-and-suspenders).
# These alone are NOT enough to fix file deps, but they prevent target
# scheduling races when -j > 1.
# Inside your disk image target definition
add_dependencies(os_image
    bootloader
    kernel.elf
    mkfs_myfs
    userland_stamp_target   # <-- single aggregator target, not the per-app list
)

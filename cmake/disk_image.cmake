add_custom_command(
    OUTPUT "${OS_IMAGE_OUT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
    COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image      ${OS_IMAGE_OUT}
          --boot       ${STAGE1_BIN}
          --stage2     ${STAGE2_BIN}
          --boot_dir   ${BOOT_DIR}
          --sysroot_dir ${SYSROOT_DIR}
          --mkfs_myfs_path ${MKFS_MYFS_EXE}
    DEPENDS
        bootloader
        kernel.elf
        "${KERNEL_BOOT_PATH}"
        "${INITRD_IMG}"
        "${MKFS_MYFS_EXE}"
        "${USERLAND_STAMP_FILE}"   # <-- this must point to the .stamp file
    COMMENT "Creating partitioned OS image using host-built mkfs"
    VERBATIM
)

add_custom_target(os_image ALL
    DEPENDS "${OS_IMAGE_OUT}"
)

add_dependencies(os_image host_tools_project userland_stamp_target)  

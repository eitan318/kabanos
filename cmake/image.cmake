add_custom_command(
  OUTPUT ${OS_IMAGE_OUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
  COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image ${OS_IMAGE_OUT}
          --boot ${STAGE1_BIN}
          --stage2 ${STAGE2_BIN}
          --boot_dir ${SYSROOT_DIR}
          --mkfs_myfs_path $<TARGET_FILE:mkfs_myfs>  

  DEPENDS ${STAGE1_BIN} ${STAGE2_BIN} ${KERNEL_OUTPUT} ${INITRD_IMG} $<TARGET_FILE:mkfs_myfs>
  COMMENT "Creating partitioned OS image"
  VERBATIM
)

add_custom_target(os_image
    DEPENDS ${OS_IMAGE_OUT}
)

add_dependencies(os_image mkfs_myfs bootloader kernel.elf)

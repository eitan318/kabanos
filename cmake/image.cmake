add_custom_command(
  OUTPUT ${OS_IMAGE_OUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
  COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image ${OS_IMAGE_OUT}
          --boot ${STAGE1_BIN}
          --stage2 ${STAGE2_BIN}
          --kernel ${KERNEL_OUTPUT}
          --boot_dir ${BOOT_DIR}
  DEPENDS ${STAGE1_BIN} ${STAGE2_BIN} ${KERNEL_OUTPUT} ${INITRD_IMG}
  COMMENT "Creating partitioned OS image"
  VERBATIM
)

add_custom_target(os_image
    DEPENDS ${OS_IMAGE_OUT}
)

add_dependencies(os_image bootloader kernel.elf)

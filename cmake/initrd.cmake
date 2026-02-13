file(GLOB_RECURSE INITRD_FILES "${INITRD_DIR}/*")

add_custom_command(
  OUTPUT ${INITRD_IMG}
  COMMAND ${Python_EXECUTABLE} ${MAKE_INITRD_PY}
          --output ${INITRD_IMG}
          --directory ${INITRD_DIR}
  DEPENDS ${INITRD_FILES} ${MAKE_INITRD_PY}
  COMMENT "Creating initrd.img from ${INITRD_DIR}"
  VERBATIM
)

add_custom_target(run
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run.py
            --kernel ${KERNEL_OUTPUT}
            --image ${OS_IMAGE_OUT}
    DEPENDS os_image
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running OS in QEMU"
    VERBATIM
)

add_custom_target(debug
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run.py
            --kernel ${KERNEL_OUTPUT}
            --image ${OS_IMAGE_OUT}
            --is_debug
    DEPENDS os_image
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running QEMU with GDB server (port 1234)"
    VERBATIM
)

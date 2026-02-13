file(GLOB USER_SOURCES
    ${CMAKE_SOURCE_DIR}/user_src/*.c
)

set(USER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/BOOT)

foreach(USER_FILE ${USER_SOURCES})
    get_filename_component(USER_NAME ${USER_FILE} NAME_WE)
    message(STATUS "found: ${USER_FILE}")

    add_executable(${USER_NAME}.elf ${USER_FILE})

    target_compile_options(${USER_NAME}.elf PRIVATE
        -m32
        -ffreestanding
        -fno-stack-protector
        -nostdlib
    )

    target_link_options(${USER_NAME}.elf PRIVATE
        -m32
        -static
        -nostdlib
    )

    set_target_properties(${USER_NAME}.elf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${USER_OUTPUT_DIR}
    )
endforeach()

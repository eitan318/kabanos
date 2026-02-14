cmake_minimum_required(VERSION 3.16)
project(myos C ASM_NASM)

set(ARCH i686)

enable_language(C ASM_NASM)

include(cmake/toolchain.cmake)
include(cmake/paths.cmake)

add_subdirectory("${SRC_DIR}/bootloader")
add_subdirectory("${SRC_DIR}/kernel")

include(cmake/userland.cmake)
include(cmake/initrd.cmake)
include(cmake/image.cmake)
include(cmake/run.cmake)

